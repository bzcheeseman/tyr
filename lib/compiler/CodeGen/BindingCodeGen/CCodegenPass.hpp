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

#include <string>

#include "Pass.hpp"

namespace tyr {
namespace pass {
class CCodegenPass : public ir::Pass {
public:
  explicit CCodegenPass(const std::string &OutputDir);

  std::string getName() override;
  bool runOnModule(Module &m) override;

private:
  const std::string &m_output_dir_;
};

ir::Pass::Ptr createCCodegenPass(const std::string &Filename);
} // namespace pass
} // namespace tyr

#endif // TYR_CBINDINGVISITOR_HPP
