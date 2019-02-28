//
// Created by Aman LaChapelle on 2019-02-12.
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


#ifndef TYR_CBINDINGVISITOR_HPP
#define TYR_CBINDINGVISITOR_HPP

#include <Pass.hpp>
#include <llvm/Support/raw_ostream.h>

namespace tyr {
  namespace pass {
  class BindingCodegenPass : public ir::Pass {
  public:
    explicit BindingCodegenPass(llvm::raw_ostream &out) : m_out_(out) {}

    bool runOnField(const ir::Field &f) override {
      return true;
    }

  protected:
    llvm::raw_ostream &m_out_;
  };

  class CCodegenPass : public BindingCodegenPass {
  public:
    explicit CCodegenPass(llvm::raw_ostream &out);

    std::string getName() override;
    bool runOnStruct(const ir::Struct &s) override;
  };

  class PythonCodegenPass : public BindingCodegenPass {
  public:
    explicit PythonCodegenPass(llvm::raw_ostream &out, std::string soName);

    std::string getName() override;
    bool runOnStruct(const ir::Struct &s) override;

  private:
    const std::string m_so_name_;
  };
  }
}


#endif //TYR_CBINDINGVISITOR_HPP
