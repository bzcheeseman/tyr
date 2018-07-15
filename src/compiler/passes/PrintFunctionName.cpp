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

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/FunctionAttrs.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

namespace {
  // Simple pass to check memory read attributes
  struct PrintFnNameIfReadOnly : public FunctionPass {
    static char ID;
    PrintFnNameIfReadOnly() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      if (F.hasFnAttribute(Attribute::AttrKind::ReadOnly)) {
        errs() << "ReadOnly: ";
        errs().write_escaped(F.getName()) << '\n';
      }
      if (F.hasFnAttribute(Attribute::AttrKind::ReadNone)) {
        errs() << "ReadNone: ";
        errs().write_escaped(F.getName()) << '\n';
      }
      return false;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesCFG();
      AU.addRequiredID(createPostOrderFunctionAttrsLegacyPass()->getPassID()); // lol wtf
    }
  }; // end of struct PrintFnName
}  // end of anonymous namespace

char PrintFnNameIfReadOnly::ID = 0;
static RegisterPass<PrintFnNameIfReadOnly> X("printfn", "Prints function names if they have the readonly attribute",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
