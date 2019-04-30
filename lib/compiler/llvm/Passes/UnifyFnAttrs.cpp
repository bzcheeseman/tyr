//
// Created by Aman LaChapelle on 2019-04-26.
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

#include "Passes.hpp"

#include <string>

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

namespace {
class UnifyFnAttrs : public llvm::ModulePass {
public:
  UnifyFnAttrs(llvm::StringRef cpu, llvm::StringRef features)
      : llvm::ModulePass(ID), m_cpu_(cpu), m_features_(features) {}

  bool runOnModule(llvm::Module &M) override {
    for (auto &F : M) {
      F.removeFnAttr("target-cpu");
      F.addFnAttr("target-cpu", m_cpu_);
      F.removeFnAttr("target-features");
      F.addFnAttr("target-features", m_features_);
    }

    return true;
  }

public:
  static char ID;

private:
  std::string m_cpu_, m_features_;
};
} // namespace

char UnifyFnAttrs::ID = 0;
llvm::Pass *tyr::createUnifyFnAttrsPass(llvm::StringRef cpu,
                                        llvm::StringRef features) {
  return new UnifyFnAttrs(cpu, features);
}
