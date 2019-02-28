//
// Created by Aman LaChapelle on 2019-02-13.
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


#include "PassManager.hpp"

#include "IR.hpp"

void tyr::ir::PassManager::registerPass(tyr::ir::Pass::Ptr pass) {
  m_passes_.push_back(std::move(pass));
}

bool tyr::ir::PassManager::runOnStruct(const tyr::ir::Struct &s) {
  bool retval = true;
  for (auto &p : m_passes_) {
    bool visitSuccess = s.visit(*p);
    if (!visitSuccess) {
      llvm::errs() << "Pass " << p->getName() << " failed";
    }
    retval &= visitSuccess;
  }

  return retval;
}
