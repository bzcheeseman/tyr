#include <utility>

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

#include "BindGen.hpp"
#include "Generators/C.hpp"
#include "Generators/Python.hpp"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

tyr::CodeGen::CodeGen(const std::string &ModuleName,
                      const std::string &TargetTriple, const std::string &CPU,
                      const std::string &Features) {
  m_parent_ = llvm::make_unique<llvm::Module>(ModuleName, m_ctx_);

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

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
}

bool tyr::CodeGen::newStruct(const std::string &Name, bool IsPacked) {
  auto inserted = m_module_structs_.emplace(
      std::make_pair(Name, StructGen{Name, IsPacked}));
  if (!inserted.second) {
    llvm::errs() << "Attempting to overwrite an existing struct " << Name;
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

  return m_module_structs_.at(StructName)
      .addField(std::move(FieldName), FT, IsMutable);
}

void tyr::CodeGen::finalizeStruct(const std::string &StructName) {
  m_module_structs_.at(StructName).finalizeFields(m_parent_.get());
  m_module_structs_.at(StructName).populateModule(m_parent_.get());
}

bool tyr::CodeGen::emitStructForUse(UseLang bind, bool EmitLLVM,
                                    const std::string &OutputDir) {
  bool retval = true;

  std::string moduleName = m_parent_->getName();

  llvm::SmallVector<char, 35> path{OutputDir.begin(), OutputDir.end()};

  if (EmitLLVM) {
    llvm::sys::path::append(path, moduleName + ".bc");
    retval &= emitLLVM(path.data());
  } else {
    llvm::sys::path::append(path, moduleName + ".o");
    retval &= emitObjectCode(path.data());
  }

  std::string extension{};
  switch (bind) {
  case kUseLangC:
    extension = ".h";
    break;
  case kUseLangPython:
    extension = ".py";
    break;
  default:
    llvm::errs() << "Unknown binding language: " << bind;
    return false;
  }

  llvm::sys::path::replace_extension(path, extension);

  retval &= emitBindings(std::string(path.begin(), path.end()), bind);

  return retval;
}

bool tyr::CodeGen::emitLLVM(const std::string &filename) {
  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::F_None);

  if (EC) {
    llvm::errs() << "Could not open file: " << EC.message();
    return false;
  }

  llvm::legacy::PassManager PM;

  llvm::PassManagerBuilder PMBuilder;
  PMBuilder.OptLevel = 3;

  m_target_machine_->adjustPassManager(PMBuilder);
  PMBuilder.populateModulePassManager(PM);

  PM.run(*m_parent_);

  if (llvm::verifyModule(*m_parent_, &llvm::errs())) {
    return false;
  }

//  m_parent_->print(dest, nullptr);
  llvm::WriteBitcodeToFile(*m_parent_, dest);
  dest.flush();
  return true;
}

bool tyr::CodeGen::emitObjectCode(const std::string &filename) {
  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::F_None);

  if (EC) {
    llvm::errs() << "Could not open file: " << EC.message();
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

bool tyr::CodeGen::emitBindings(const std::string &filename,
                                tyr::UseLang bind) {
  Binding bindings{};

  std::string bound;
  switch (bind) {
  case kUseLangC: {
    C gen{};
    bindings.setGenerator(&gen);
    bindings.setModule(m_parent_.get());
    bound = bindings.assembleBinding();
    break;
  }
  case kUseLangPython: {
    // Python needs the C header
    C cgen{};
    bindings.setGenerator(&cgen);
    bindings.setModule(m_parent_.get());

    // Create the python generator and get the C header bindings
    Python gen{};
    gen.setCHeader(bindings.assembleBinding(true));

    bindings.setGenerator(&gen);
    bound = bindings.assembleBinding();
    break;
  }
  default: {
    llvm::errs() << "Unknown binding language\n";
    return false;
  }
  }

  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::F_None);

  if (EC) {
    llvm::errs() << "Could not open file: " << EC.message();
    return false;
  }

  dest << bound;
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
    OutTy = m_module_structs_.at(FieldType).getStructType()->getPointerTo(0);
  } else {
    llvm::errs() << "Unknown field type: " << FieldType << "\n";
    return nullptr;
  }

  if (IsRepeated) {
    OutTy = OutTy->getPointerTo(0);
  }

  return OutTy;
}
