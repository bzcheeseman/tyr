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
  bool checkFunctionMarkCall(Function *f) {
    return f->getName() == "mark_tyr_function";
  }

  struct FindFunctionsMarkedForRouting : public FunctionPass {
    static char ID;
    FindFunctionsMarkedForRouting() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      bool should_run = false;
      for (auto &BB : F) {
        for (auto &I : BB) {
          CallInst *call = dyn_cast<CallInst>(&I);
          if (call && checkFunctionMarkCall(call->getCalledFunction())) {
            should_run = true;
            break;
          }
        }
      }

      if (!should_run) return false;

      for (auto &BB : F) {
        for (auto &I : BB) {
          CallInst *call = dyn_cast<CallInst>(&I);
          if (call && (call->getNumOperands() > 1)) {
            bool mark_call_exists = false; CallInst *mark_call;
            for (auto &bb : *call->getCalledFunction()) {
              for (auto &i : bb) {
                if ((mark_call = dyn_cast<CallInst>(&i)) && checkFunctionMarkCall(mark_call->getCalledFunction())) {
                  mark_call_exists = true;
                }
              }
            }
            if (!mark_call_exists) continue;

            Value *route_value = call->getOperand(0);
            Function *foi = dyn_cast<Function>(call->getOperand(1));
            if (foi) {
              errs() << *route_value << " " << *foi << "\n"; // prints the function corresponding to the arg we want
            }
          }
        }
      }

      return false;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesCFG();
      AU.addRequiredID(createPostOrderFunctionAttrsLegacyPass()->getPassID()); // lol wtf
    }
  }; // end of struct PrintFnName
}  // end of anonymous namespace

char FindFunctionsMarkedForRouting::ID = 0;
static RegisterPass<FindFunctionsMarkedForRouting> X("find-marked",
                                                     "Finds functions that are marked with the mark_tyr_function call",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
