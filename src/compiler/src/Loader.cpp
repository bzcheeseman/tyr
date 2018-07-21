//
// Created by Aman LaChapelle on 7/14/18.
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

#include "Loader.hpp"

#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

namespace tyr {
namespace compiler {
Loader::Loader(llvm::LLVMContext &ctx) : m_ctx_(ctx) {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
}

std::unique_ptr<llvm::Module> Loader::LoadModule(const std::string &filename) {
  llvm::SMDiagnostic smd_err;

  std::unique_ptr<llvm::Module> module =
      llvm::parseIRFile(filename, smd_err, m_ctx_, true);

  if (!module) {
    smd_err.print("LoadModule - from file", llvm::errs(), true);
  }

  this->ResetDataLayout(module.get());

  bool main_found = (module->getFunction("main") != nullptr);
  if (!main_found)
    throw std::runtime_error("no symbol 'main' in module");

  return module;
}

std::unique_ptr<llvm::Module>
Loader::LoadModule(llvm::MemoryBufferRef serialized_module) {
  llvm::SMDiagnostic smd_err;

  std::unique_ptr<llvm::Module> module =
      llvm::parseIR(serialized_module, smd_err, m_ctx_, true);

  if (!module) {
    smd_err.print("LoadModule - from file", llvm::errs(), true);
  }

  this->ResetDataLayout(module.get());

  bool main_found = (module->getFunction("main") != nullptr);
  if (!main_found)
    throw std::runtime_error("no symbol 'main' in module");

  return module;
}

void Loader::ResetDataLayout(llvm::Module *module) {
  const std::string target_triple = llvm::sys::getDefaultTargetTriple();
  const std::string cpu = llvm::sys::getHostCPUName();
  llvm::StringMap<bool> feats_map;
  llvm::sys::getHostCPUFeatures(feats_map);

  std::string features = "";
  for (auto &entry : feats_map) {
    if (entry.second)
      features += "+" + std::string(entry.first()) + ",";
  }

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

  llvm::TargetOptions options;
  auto RM = llvm::Optional<llvm::Reloc::Model>();

  llvm::TargetMachine *target_machine =
      target->createTargetMachine(target_triple, cpu, features, options, RM);

  module->setDataLayout(target_machine->createDataLayout());
  module->setTargetTriple(target_triple);
}
} // namespace compiler
} // namespace tyr
