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

#include "ObjectCodegenPass.hpp"
#include "Module.hpp"
#include "Passes.hpp"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

tyr::pass::ObjectCodegenPass::ObjectCodegenPass(const llvm::StringRef CPU,
                                                const llvm::StringRef Features,
                                                const llvm::StringRef OutputDir)
    : m_cpu_(CPU), m_features_(Features), m_output_dir_(OutputDir),
      m_target_(nullptr) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeNativeTarget();
}

std::string tyr::pass::ObjectCodegenPass::getName() {
  return "ObjectCodegenPass";
}

bool tyr::pass::ObjectCodegenPass::runOnModule(tyr::Module &m) {
  llvm::Module *parent = m.getModule();
  const llvm::StringRef ModuleName = parent->getName();
  const llvm::StringRef TargetTriple = parent->getTargetTriple();

  std::string Error;
  auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

  // Print an error and exit if we couldn't find the requested target.
  // This generally occurs if we've forgotten to initialise the
  // TargetRegistry or we have a bogus target triple.
  if (!Target) {
    llvm::errs() << Error << "\n";
    return false;
  }

  // Do the setup for code generation
  llvm::TargetOptions opt;
  auto RM = llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::Model::PIC_);
  m_target_ =
      Target->createTargetMachine(TargetTriple, m_cpu_, m_features_, opt, RM);

  parent->setDataLayout(m_target_->createDataLayout());

  // Now do the module codegen
  llvm::SmallVector<char, 100> path{m_output_dir_.begin(), m_output_dir_.end()};
  llvm::sys::path::append(path, ModuleName + ".o");
  llvm::sys::fs::make_absolute(path);

  const std::string Filename{path.begin(), path.end()};

  llvm::legacy::PassManager PM;
  auto FileType = llvm::TargetMachine::CGFT_ObjectFile;

  std::error_code EC;
  llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::F_None);

  if (EC) {
    llvm::errs() << "Could not open file to emit object code: " << EC.message()
                 << "\n";
    return false;
  }

  PM.add(createUnifyFnAttrsPass(m_cpu_, m_features_));

  llvm::PassManagerBuilder PMBuilder;
  PMBuilder.OptLevel = 2;
  PMBuilder.SizeLevel = 2;

  m_target_->adjustPassManager(PMBuilder);
  PMBuilder.populateModulePassManager(PM);

  if (m_target_->addPassesToEmitFile(PM, dest, nullptr, FileType)) {
    llvm::errs() << "TargetMachine can't emit a file of this type";
    return false;
  }

  PM.run(*parent); // here

  if (llvm::verifyModule(*parent, &llvm::errs())) {
    return false;
  }

  dest.flush();
  dest.close();
  return true;
}

tyr::ir::Pass::Ptr
tyr::pass::createObjectCodegenPass(const llvm::StringRef CPU,
                                   const llvm::StringRef Features,
                                   const llvm::StringRef OutputDir) {
  return llvm::make_unique<tyr::pass::ObjectCodegenPass>(CPU, Features,
                                                         OutputDir);
}
