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

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/Path.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Utils/Cloning.h>

tyr::Module::Module(const llvm::StringRef ModuleName, llvm::LLVMContext &Ctx)
    : m_ctx_(Ctx) {
  m_parent_ = llvm::make_unique<llvm::Module>(ModuleName, m_ctx_);
}

tyr::Module::~Module() {
  for (auto &s : m_module_structs_) {
    delete s.second;
  }
}

void tyr::Module::setTargetTriple(const llvm::StringRef TargetTriple) {
  m_parent_->setTargetTriple(TargetTriple);
}

void tyr::Module::setSourceFileName(const llvm::StringRef SourceFilename) {
  m_parent_->setSourceFileName(SourceFilename);
}

void tyr::Module::setBuiltinName(const llvm::StringRef Which,
                                 const llvm::StringRef New) {
  // Try to insert a new key
  auto inserted = m_builtin_names_.insert({Which, New});
  // If one already exists, then update it
  if (!inserted.second) {
    inserted.first->setValue(New);
  }
}

void tyr::Module::finalizeBuiltins() {
  m_parent_->getOrInsertFunction(m_builtin_names_.lookup("malloc"), llvm::Type::getInt8PtrTy(m_ctx_),
                                 llvm::Type::getInt64Ty(m_ctx_));
  m_parent_->getOrInsertFunction(m_builtin_names_.lookup("realloc"), llvm::Type::getInt8PtrTy(m_ctx_),
                                 llvm::Type::getInt8PtrTy(m_ctx_),
                                 llvm::Type::getInt64Ty(m_ctx_));
  m_parent_->getOrInsertFunction(m_builtin_names_.lookup("free"), llvm::Type::getVoidTy(m_ctx_),
                                 llvm::Type::getInt8PtrTy(m_ctx_));

  m_builtins_finalized_ = true;
}

tyr::ir::Struct *tyr::Module::getOrCreateStruct(const llvm::StringRef name) {
  auto got_struct = m_module_structs_.find(name);
  if (got_struct != m_module_structs_.end()) {
    return got_struct->second;
  }

  m_module_structs_[name] = new ir::Struct{name};
  return m_module_structs_[name];
}

const llvm::StringMap<tyr::ir::Struct *> &tyr::Module::getStructs() const {
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

llvm::Module *tyr::Module::getModule() {
  if (!m_builtins_finalized_) {
    llvm::errs() << "Must finalize builtins before getting module pointer\n";
    return nullptr;
  }

  return m_parent_.get();
}

llvm::StringMap<std::string> tyr::Module::getBuiltins() const {
  return m_builtin_names_;
}

llvm::Type *tyr::Module::parseType(llvm::StringRef FieldType, bool IsRepeated) {
  llvm::Type *OutTy;
  if (FieldType == "bool") {
    OutTy = llvm::Type::getInt1Ty(m_ctx_);
  } else if (FieldType.startswith_lower("int") ||
             FieldType.startswith_lower("uint")) {
    // Find the integer bit width
    const size_t tIdx = FieldType.rfind_lower("t") + 1;
    if (tIdx == llvm::StringRef::npos) {
      llvm::errs() << "Unable to validate type string: " << FieldType << "\n";
      return nullptr;
    }

    uint64_t intBits = 0;
    const uint32_t radix = 10;
    if (llvm::getAsUnsignedInteger(FieldType.substr(tIdx), radix, intBits)) {
      llvm::errs() << "Unable to parse integer bit width for field type: "
                   << FieldType << "\n";
      return nullptr;
    }

    OutTy = llvm::Type::getIntNTy(m_ctx_, (uint32_t)intBits);
  } else if (FieldType == "float") {
    OutTy = llvm::Type::getFloatTy(m_ctx_);
  } else if (FieldType == "double") {
    OutTy = llvm::Type::getDoubleTy(m_ctx_);
  } else if (m_module_structs_.find(FieldType) != m_module_structs_.end()) {
    OutTy = m_module_structs_[FieldType]->getType()->getPointerTo(0);
  } else {
    llvm::errs() << "Unknown field type: " << FieldType << "\n";
    return nullptr;
  }

  if (IsRepeated) {
    OutTy = OutTy->getPointerTo(0);
  }

  return OutTy;
}

namespace {
const std::string TYR_FILE_HELPER_FILE = "tyr-rt-file.bc";
const std::string TYR_BASE64_FILE = "tyr-rt-base64.bc";

std::unique_ptr<llvm::Module>
getModuleFromFile(llvm::LLVMContext &ctx, const llvm::StringRef filename,
                  const llvm::StringRef TargetTriple) {
  auto FileBuf = llvm::MemoryBuffer::getFile(filename);
  if (std::error_code ec = FileBuf.getError()) {
    llvm::errs() << "Error getting file " << filename << ": " << ec.message()
                 << "\n";
    return nullptr;
  }

  auto ExpOutsideModule = llvm::parseBitcodeFile(*FileBuf.get(), ctx);
  if (!ExpOutsideModule) {
    llvm::errs() << "Error parsing bitcode file " << filename << "\n";
    return nullptr;
  }

  std::unique_ptr<llvm::Module> OutsideModule =
      std::move(ExpOutsideModule.get());
  OutsideModule->setTargetTriple(TargetTriple);

  return OutsideModule;
}
} // namespace

bool tyr::Module::linkRuntimeModules(const llvm::StringRef Directory,
                                     uint32_t options) {
  llvm::Linker Linker{*m_parent_};

  llvm::SmallVector<std::unique_ptr<llvm::Module>, 3> OutsideModules = {};

  if (rt::isFileEnabled(options)) { // link in the file helpers
    llvm::SmallVector<char, 0> path{Directory.begin(), Directory.end()};
    llvm::sys::path::append(path, TYR_FILE_HELPER_FILE);
    llvm::sys::fs::make_absolute(path);
    const std::string Filename{path.begin(), path.end()};

    OutsideModules.push_back(
        getModuleFromFile(m_ctx_, Filename, m_parent_->getTargetTriple()));
  }

  if (rt::isB64Enabled(options)) { // link in the file helpers
    llvm::SmallVector<char, 0> path{Directory.begin(), Directory.end()};
    llvm::sys::path::append(path, TYR_BASE64_FILE);
    llvm::sys::fs::make_absolute(path);
    const std::string Filename{path.begin(), path.end()};

    OutsideModules.push_back(
        getModuleFromFile(m_ctx_, Filename, m_parent_->getTargetTriple()));
  }

  if (!OutsideModules.empty()) {
    for (auto &OM : OutsideModules) {
      bool LinkFailed = Linker.linkInModule(std::move(OM));
      if (LinkFailed) {
        llvm::errs() << "Linking modules failed for options ";
        llvm::errs().write_hex(options);
        llvm::errs() << " and this module may be unstable\n";
        return false;
      }
    }
  }

  return true;
}

void tyr::Module::setDefaultBuiltins() {
  setBuiltinName("malloc", "malloc");
  setBuiltinName("realloc", "realloc");
  setBuiltinName("free", "free");
  finalizeBuiltins();
}

llvm::ExecutionEngine *tyr::getExecutionEngine(llvm::Module *Parent) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeNativeTarget();

  std::string error_str;

  llvm::EngineBuilder engineBuilder{llvm::CloneModule(*Parent)};
  engineBuilder.setErrorStr(&error_str);
  engineBuilder.setEngineKind(llvm::EngineKind::Either);
  llvm::ExecutionEngine *engine = engineBuilder.create();

  if (!error_str.empty()) {
    llvm::errs() << error_str;
    return nullptr;
  }

  return engine;
}

bool tyr::rt::isFileEnabled(uint32_t options) {
  return (options & (0b1u << 0u)) == 1u;
}

bool tyr::rt::isB64Enabled(uint32_t options) {
  return (options & (0b1u << 1u)) >> 1u == 1u;
}
