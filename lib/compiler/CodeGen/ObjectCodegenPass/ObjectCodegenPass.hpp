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

#ifndef TYR_OBJECTCODEGENPASS_HPP
#define TYR_OBJECTCODEGENPASS_HPP

#include <string>

#include "Pass.hpp"

namespace llvm {
class TargetMachine;
}

namespace tyr {
class Module;
namespace pass {
class ObjectCodegenPass : public ir::Pass {
public:
  ObjectCodegenPass(const std::string &CPU, const std::string &Features,
                    const std::string &OutputDir);

  std::string getName() override;

  bool runOnModule(Module &m) override;

private:
  const std::string m_cpu_;
  const std::string m_features_;
  const std::string m_output_dir_;
  llvm::TargetMachine *m_target_;
};

ir::Pass::Ptr createObjectCodegenPass(const std::string &CPU,
                                      const std::string &Features,
                                      const std::string &OutputDir);
} // namespace pass
} // namespace tyr

#endif // TYR_OBJECTCODEGENPASS_HPP
