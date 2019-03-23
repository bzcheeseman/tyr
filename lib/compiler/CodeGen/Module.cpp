//
// Created by Aman LaChapelle on 2019-03-21.
//
// tyr
// Copyright (c) 2019 Aman LaChapelle
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

#include "Module.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/Path.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Utils/Cloning.h>

tyr::Module::Module(const std::string &ModuleName, llvm::LLVMContext &Ctx)
    : m_ctx_(Ctx) {
  m_parent_ = llvm::make_unique<llvm::Module>(ModuleName, m_ctx_);

  m_parent_->getOrInsertFunction("malloc", llvm::Type::getInt8PtrTy(m_ctx_),
                                 llvm::Type::getInt64Ty(m_ctx_));
  m_parent_->getOrInsertFunction("realloc", llvm::Type::getInt8PtrTy(m_ctx_),
                                 llvm::Type::getInt8PtrTy(m_ctx_),
                                 llvm::Type::getInt64Ty(m_ctx_));
  m_parent_->getOrInsertFunction("free", llvm::Type::getVoidTy(m_ctx_),
                                 llvm::Type::getInt8PtrTy(m_ctx_));
}

tyr::Module::~Module() {
  for (auto &s : m_module_structs_) {
    delete s.second;
  }
}

void tyr::Module::setTargetTriple(const std::string &TargetTriple) {
  m_parent_->setTargetTriple(TargetTriple);
}

void tyr::Module::setSourceFileName(const std::string &SourceFilename) {
  m_parent_->setSourceFileName(SourceFilename);
}

tyr::ir::Struct *tyr::Module::getOrCreateStruct(const std::string &name) {
  auto got_struct = m_module_structs_.find(name);
  if (got_struct != m_module_structs_.end()) {
    return got_struct->second;
  }

  m_module_structs_[name] = new ir::Struct{name};
  return m_module_structs_[name];
}

const std::unordered_map<std::string, tyr::ir::Struct *> &
tyr::Module::getStructs() const {
  return m_module_structs_;
}

llvm::raw_ostream &tyr::operator<<(llvm::raw_ostream &os, const Module &m) {
  for (auto &s : m.m_module_structs_) {
    os << *s.second << "\n";
  }

  return os;
}

bool tyr::Module::visit(tyr::ir::Pass &p) {
  bool retval = true;
  for (auto &s : m_module_structs_) {
    for (auto &field : s.second->getFields()) {
      bool visitSuccess = p.runOnField(*field);
      if (!visitSuccess) {
        llvm::errs() << "Visiting field " << field->name << " failed";
      }
      retval &= visitSuccess;
    }

    bool visitSuccess = p.runOnStruct(*s.second);
    if (!visitSuccess) {
      llvm::errs() << "Visiting struct " << s.second->getName() << " failed";
    }
    retval &= visitSuccess;
  }

  retval &= p.runOnModule(*this);

  return retval;
}

llvm::Module *tyr::Module::getModule() { return m_parent_.get(); }

llvm::Type *tyr::Module::parseType(std::string FieldType, bool IsRepeated) {
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
    OutTy = m_module_structs_.at(FieldType)->getType()->getPointerTo(0);
  } else {
    llvm::errs() << "Unknown field type: " << FieldType << "\n";
    return nullptr;
  }

  if (IsRepeated) {
    OutTy = OutTy->getPointerTo(0);
  }

  return OutTy;
}

bool tyr::Module::linkOutsideModule(const std::string &filename) {
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

  bool LinkFailed =
      llvm::Linker::linkModules(*m_parent_, std::move(OutsideModule));
  if (LinkFailed) {
    llvm::errs() << "Linking modules failed for input file " << filename
                 << " and this module may be unstable\n";
    return false;
  }

  return true;
}

llvm::ExecutionEngine *tyr::getExecutionEngine(llvm::Module *Parent) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeNativeTarget();

  std::string error_str;

  llvm::EngineBuilder engineBuilder {llvm::CloneModule(*Parent)};
  engineBuilder.setErrorStr(&error_str);
  engineBuilder.setEngineKind(llvm::EngineKind::Either);
  llvm::ExecutionEngine *engine = engineBuilder.create();

  if (!error_str.empty()) {
    llvm::errs() << error_str;
    return nullptr;
  }

  return engine;
}
