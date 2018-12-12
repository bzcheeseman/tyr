//
// Created by Aman LaChapelle on 12/1/18.
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

#ifndef TYR_PYTHON_HPP
#define TYR_PYTHON_HPP

#include "../BindGen.hpp"

#include <map>

namespace tyr {
class Python : public Generator {
public:
  void setupOpaqueTypes(llvm::ArrayRef<llvm::StructType *> struct_tys) override;
  std::string getFunctionProto(const llvm::Function &f) override;
  std::string convertType(const llvm::Type *t) override;
  std::string getHeader() override;
  std::string getFooter() override;
  std::string getTypeWrappers() override;

  void setCHeader(std::string c_header);

  std::string getCType(const llvm::Type *t);

private:
  std::map<std::string, std::string> m_struct_wrappers_;
  std::string m_c_header_;
};
} // namespace tyr

#endif // TYR_PYTHON_HPP
