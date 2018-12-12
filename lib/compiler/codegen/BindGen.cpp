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

#include "BindGen.hpp"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

void tyr::Generator::setModuleName(std::string module_name) {
  m_module_name_ = std::move(module_name);
}

void tyr::Binding::setGenerator(tyr::Generator *gen) { m_generator_ = gen; }

void tyr::Binding::setModule(llvm::Module *m) { m_module_ = m; }

std::string tyr::Binding::assembleBinding(bool funcsOnly) const {
  std::string out{};

  std::vector<llvm::StructType *> StructTys =
      m_module_->getIdentifiedStructTypes();
  m_generator_->setupOpaqueTypes(StructTys);
  m_generator_->setModuleName(m_module_->getName());

  if (!funcsOnly) {
    out += m_generator_->getHeader();
  }

  out += m_generator_->getTypeWrappers();

  for (const auto &f : m_module_->functions()) {
    // Don't export anything internal (prefixed by "__")
    if (f.isIntrinsic() || f.getName() == "malloc" ||
        f.getName() == "realloc" || f.getName() == "free" ||
        (f.getName()[0] == '_' && f.getName()[1] == '_')) {
      continue;
    }
    out += m_generator_->getFunctionProto(f);
  }

  if (!funcsOnly) {
    out += m_generator_->getFooter();
  }

  return out;
}

llvm::raw_ostream &tyr::operator<<(llvm::raw_ostream &os,
                                   const tyr::Binding &b) {
  os << b.assembleBinding();
  return os;
}

std::ostream &tyr::operator<<(std::ostream &os, const tyr::Binding &b) {
  os << b.assembleBinding();
  return os;
}
