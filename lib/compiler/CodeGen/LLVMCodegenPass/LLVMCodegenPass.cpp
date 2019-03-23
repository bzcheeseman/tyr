//
// Created by Aman LaChapelle on 2019-03-22.
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

#include "LLVMCodegenPass.hpp"
#include "Module.hpp"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

tyr::pass::LLVMCodegenPass::LLVMCodegenPass(const std::string &CPU,
                                            const std::string &Features,
                                            const std::string &OutputDir)
    : m_cpu_(CPU), m_features_(Features), m_output_dir_(OutputDir),
      m_target_(nullptr) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeNativeTarget();
}

std::string tyr::pass::LLVMCodegenPass::getName() { return "LLVMCodegenPass"; }

bool tyr::pass::LLVMCodegenPass::runOnModule(tyr::Module &m) {
  llvm::Module *parent = m.getModule();
  const std::string &ModuleName = parent->getName();
  const std::string &TargetTriple = parent->getTargetTriple();

  std::string Error;
  auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

  // Print an error and exit if we couldn't find the requested target.
  // This generally occurs if we've forgotten to initialise the
  // TargetRegistry or we have a bogus target triple.
  if (!Target) {
    llvm::errs() << Error;
    return false;
  }

  // Do the setup for code generation
  llvm::TargetOptions opt;
  auto RM = llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::Model::PIC_);
  m_target_ =
      Target->createTargetMachine(TargetTriple, m_cpu_, m_features_, opt, RM);

  parent->setDataLayout(m_target_->createDataLayout());

  // Now do the module codegen
  llvm::SmallVector<char, 35> path{m_output_dir_.begin(), m_output_dir_.end()};
  llvm::sys::path::append(path, ModuleName + ".bc");

  const std::string Filename{path.begin(), path.end()};

  llvm::legacy::PassManager PM;

  llvm::PassManagerBuilder PMBuilder;
  PMBuilder.OptLevel = 3;

  m_target_->adjustPassManager(PMBuilder);
  PMBuilder.populateModulePassManager(PM);

  PM.run(*parent);

  parent->print(llvm::outs(), nullptr);

  if (llvm::verifyModule(*parent, &llvm::errs())) {
    return false;
  }

  std::error_code EC;
  llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::F_None);

  if (EC) {
    llvm::errs() << "Could not open file to emit LLVM bitcode: " << EC.message()
                 << "\n";
    return false;
  }

  llvm::WriteBitcodeToFile(*parent, dest);

  dest.flush();

  return true;
}

tyr::ir::Pass::Ptr
tyr::pass::createLLVMCodegenPass(const std::string &CPU,
                                 const std::string &Features,
                                 const std::string &OutputDir) {
  return llvm::make_unique<tyr::pass::LLVMCodegenPass>(CPU, Features,
                                                       OutputDir);
}
