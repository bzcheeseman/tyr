//
// Created by Aman LaChapelle on 7/16/18.
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


#include "PureFinder.hpp"

#include <llvm/IR/Module.h>
#include <llvm/IR/Attributes.h>
#include <llvm/Support/raw_ostream.h>

void tyr::compile::PureFinder::AddFunctionIfPure(llvm::Function *f) {
  if (!f->hasFnAttribute(llvm::Attribute::AttrKind::ReadOnly) && !f->hasFnAttribute(llvm::Attribute::AttrKind::ReadNone)) {
    return;
  }

  // find the call context
  llvm::Use *call_site = &(*f->uses().end());
  call_site->getUser()->getName();
}
