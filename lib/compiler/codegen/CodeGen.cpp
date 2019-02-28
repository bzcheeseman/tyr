//
// Created by Aman LaChapelle on 11/20/18.
//
// tyr
// Copyright (c) 2018 Aman LaChapelle
// Full license at tyr/LICENSE.txt
//

/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#include "CodeGen.hpp"

#include <sstream>

//#include "BindGen.hpp"
//#include "Generators/C.hpp"
//#include "Generators/Python.hpp"
#include "LLVMCodegenPass.hpp"
#include "BindingCodegenPass.hpp"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

tyr::CodeGen::CodeGen(const std::string &ModuleName,
                      const std::string &TargetTriple, const std::string &CPU,
                      const std::string &Features) : m_binding_(), m_binding_out_(m_binding_) {
  m_parent_ = llvm::make_unique<llvm::Module>(ModuleName, m_ctx_);

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeNativeTarget();

  std::string Error;
  auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

  // Print an error and exit if we couldn't find the requested target.
  // This generally occurs if we've forgotten to initialise the
  // TargetRegistry or we have a bogus target triple.
  if (!Target) {
    llvm::errs() << Error;
    return;
  }

  llvm::TargetOptions opt;
  auto RM = llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::Model::PIC_);
  m_target_machine_ =
      Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

  m_parent_->setDataLayout(m_target_machine_->createDataLayout());
  m_parent_->setTargetTriple(TargetTriple);
  m_parent_->setPICLevel(llvm::PICLevel::BigPIC);

  m_parent_->getOrInsertFunction("malloc", llvm::Type::getInt8PtrTy(m_ctx_),
                                 llvm::Type::getInt64Ty(m_ctx_));
  m_parent_->getOrInsertFunction("realloc", llvm::Type::getInt8PtrTy(m_ctx_),
                                 llvm::Type::getInt8PtrTy(m_ctx_),
                                 llvm::Type::getInt64Ty(m_ctx_));
  m_parent_->getOrInsertFunction("free", llvm::Type::getVoidTy(m_ctx_),
                                 llvm::Type::getInt8PtrTy(m_ctx_));

  // Register the codegen pass
  m_manager_.registerPass(llvm::make_unique<pass::LLVMCodegenPass>(m_parent_.get()));
}

bool tyr::CodeGen::newStruct(const std::string &Name, bool IsPacked) {
  auto inserted = m_module_structs_.emplace(
      std::make_pair(Name, ir::Struct{Name, IsPacked}));
  if (!inserted.second) {
    llvm::errs() << "Attempting to overwrite an existing struct " << Name;
    return false;
  }
  return true;
}

bool tyr::CodeGen::addBindLangPass(tyr::UseLang lang) {
  m_bind_lang_ = lang;
  switch (lang) {
    case kUseLangC: {
      m_manager_.registerPass(llvm::make_unique<pass::CCodegenPass>(m_binding_out_));
      break;
    }
//    case kUseLangPython: {
//      m_manager_.registerPass(llvm::make_unique<pass::PythonCodegenPass>(m_binding_out_, m_parent_->getName()));
//      break;
//    }
    default:
      llvm::errs() << "Unsupported binding lang " << lang;
      return false;
  }

  return true;
}

bool tyr::CodeGen::addField(const std::string &StructName, bool IsMutable,
                            bool IsRepeated, std::string FieldType,
                            std::string FieldName) {

  llvm::Type *FT = parseType(std::move(FieldType), IsRepeated);
  if (FT == nullptr) {
    return false;
  }

  // TODO: handle map fields
  if (IsRepeated) {
    m_module_structs_.at(StructName)
        .addRepeatedField(std::move(FieldName), FT, IsMutable);
  } else {
    m_module_structs_.at(StructName)
        .addField(std::move(FieldName), FT, IsMutable);
  }

  return true;
}

bool tyr::CodeGen::finalizeStruct(const std::string &StructName) {
  ir::Struct &s = m_module_structs_.at(StructName);
  s.finalizeFields(m_parent_.get());
  return m_manager_.runOnStruct(s);
}

bool tyr::CodeGen::emit(bool EmitLLVM, bool EmitText, const std::string &OutputDir) {
  bool retval = true;

  std::string moduleName = m_parent_->getName();
  m_parent_->setSourceFileName(m_parent_->getSourceFileName() + ".tyr");

  llvm::SmallVector<char, 35> path{OutputDir.begin(), OutputDir.end()};

  if (OutputDir.empty()) {
    retval &= emitLLVM("", true);
  } else {
    if (EmitLLVM) {
      if (EmitText) {
        llvm::sys::path::append(path, moduleName + ".ll");
      } else {
        llvm::sys::path::append(path, moduleName + ".bc");
      }

      retval &= emitLLVM(std::string(path.begin(), path.end()), EmitText);
    } else {
      llvm::sys::path::append(path, moduleName + ".o");
      retval &= emitObjectCode(std::string(path.begin(), path.end()));
    }
  }

  if (!retval) {
    llvm::errs() << "Code generation failed, aborting\n";
    return false;
  }

  if (OutputDir.empty()) {
    retval &= emitBindings("");
  } else {
    std::string extension{};
    switch (m_bind_lang_) {
      case kUseLangC:
        extension = ".h";
        break;
//      case kUseLangPython:
//        extension = ".py";
//        break;
      default:
        llvm::errs() << "Unknown binding language: " << m_bind_lang_;
        return false;
    }

    llvm::sys::path::replace_extension(path, extension);

    retval &= emitBindings(std::string(path.begin(), path.end()));
  }

  if (!retval) {
    llvm::errs() << "Binding generation failed, aborting\n";
    return false;
  }

  return retval;
}

bool tyr::CodeGen::emitLLVM(const std::string &filename, bool EmitText) {
  llvm::legacy::PassManager PM;

  llvm::PassManagerBuilder PMBuilder;
  PMBuilder.OptLevel = 3;

  m_target_machine_->adjustPassManager(PMBuilder);
  PMBuilder.populateModulePassManager(PM);

  PM.run(*m_parent_);

  if (llvm::verifyModule(*m_parent_, &llvm::errs())) {
    return false;
  }

  if (filename.empty()) {
//    m_parent_->print(llvm::outs(), nullptr);
    return true;
  }

  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::F_None);

  if (EC) {
    llvm::errs() << "Could not open file to emit LLVM bitcode: " << EC.message()
                 << "\n";
    return false;
  }

  if (EmitText) {
    m_parent_->print(dest, nullptr);
  } else {
    llvm::WriteBitcodeToFile(*m_parent_, dest);
  }

  dest.flush();

  return true;
}

bool tyr::CodeGen::emitObjectCode(const std::string &filename) {
  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::F_None);

  if (EC) {
    llvm::errs() << "Could not open file to emit object code: " << EC.message()
                 << "\n";
    return false;
  }

  llvm::legacy::PassManager PM;
  auto FileType = llvm::TargetMachine::CGFT_ObjectFile;

  llvm::PassManagerBuilder PMBuilder;
  PMBuilder.OptLevel = 3;

  m_target_machine_->adjustPassManager(PMBuilder);
  PMBuilder.populateModulePassManager(PM);

  if (m_target_machine_->addPassesToEmitFile(PM, dest, nullptr, FileType)) {
    llvm::errs() << "TargetMachine can't emit a file of this type";
    return false;
  }

  PM.run(*m_parent_);

  if (llvm::verifyModule(*m_parent_, &llvm::errs())) {
    return false;
  }

  dest.flush();
  return true;
}

bool tyr::CodeGen::emitBindings(const std::string &filename) {
  if (filename.empty()) {
//    llvm::outs() << m_binding_out_.str();
    return true;
  }

  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::F_None);

  if (EC) {
    llvm::errs() << "Could not open file to emit bindings: " << EC.message()
                 << "\n";
    return false;
  }

  dest << m_binding_out_.str();

  dest.flush();

  return true;
}

llvm::Type *tyr::CodeGen::parseType(std::string FieldType, bool IsRepeated) {
  llvm::Type *OutTy;
  if (FieldType == "bool") {
    OutTy = llvm::Type::getInt1Ty(m_ctx_);
  } else if (FieldType == "int8" || FieldType == "uint8" ||
             FieldType == "byte") {
    OutTy = llvm::Type::getInt8Ty(m_ctx_);
  } else if (FieldType == "int16" || FieldType == "uint16") {
    OutTy = llvm::Type::getInt16Ty(m_ctx_);
  } else if (FieldType == "int32" || FieldType == "uint32") {
    OutTy = llvm::Type::getInt32Ty(m_ctx_);
  } else if (FieldType == "int64" || FieldType == "uint64") {
    OutTy = llvm::Type::getInt64Ty(m_ctx_);
  } else if (FieldType == "float") {
    OutTy = llvm::Type::getFloatTy(m_ctx_);
  } else if (FieldType == "double") {
    OutTy = llvm::Type::getDoubleTy(m_ctx_);
  } else if (m_module_structs_.find(FieldType) != m_module_structs_.end()) {
    OutTy = m_module_structs_.at(FieldType).getType()->getPointerTo(0);
  } else {
    llvm::errs() << "Unknown field type: " << FieldType << "\n";
    return nullptr;
  }

  if (IsRepeated) {
    OutTy = OutTy->getPointerTo(0);
  }

  return OutTy;
}

bool tyr::CodeGen::linkOutsideModule(const std::string &filename) {
  auto FileBuf = llvm::MemoryBuffer::getFile(filename);
  if (std::error_code ec = FileBuf.getError()) {
    llvm::errs() << "Error getting file " << filename << ": " << ec.message()
                 << "\n";
    return false;
  }

  auto ExpOutsideModule = llvm::parseBitcodeFile(*FileBuf.get(), m_ctx_);
  if (!ExpOutsideModule) {
    llvm::errs() << "Error parsing bitcode file " << filename << "\n";
    return false;
  }

  std::unique_ptr<llvm::Module> OutsideModule =
      std::move(ExpOutsideModule.get());

  OutsideModule->setTargetTriple(m_parent_->getTargetTriple());
  OutsideModule->setDataLayout(m_parent_->getDataLayout());
  OutsideModule->setPICLevel(m_parent_->getPICLevel());

  bool LinkFailed =
      llvm::Linker::linkModules(*m_parent_, std::move(OutsideModule));
  if (LinkFailed) {
    llvm::errs() << "Linking modules failed for input file " << filename
                 << " and this module may be unstable\n";
    return false;
  }

  return true;
}

bool tyr::CodeGen::verifyModule(llvm::raw_ostream &out) {
  return llvm::verifyModule(*m_parent_, &out);
}

llvm::Module *tyr::CodeGen::getModule() { return m_parent_.get(); }

llvm::ExecutionEngine *tyr::CodeGen::getExecutionEngine() {
  std::string error_str;

  llvm::EngineBuilder engineBuilder(std::move(m_parent_));
  engineBuilder.setErrorStr(&error_str);
  engineBuilder.setEngineKind(llvm::EngineKind::JIT);
  llvm::ExecutionEngine *engine = engineBuilder.create();

  if (!error_str.empty()) {
    llvm::errs() << error_str;
    return nullptr;
  }

  return engine;
}
