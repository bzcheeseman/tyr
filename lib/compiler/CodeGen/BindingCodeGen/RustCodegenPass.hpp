//
// Created by Aman LaChapelle on 2019-03-23.
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

#ifndef TYR_RUSTCODEGENPASS_HPP
#define TYR_RUSTCODEGENPASS_HPP

#include "Pass.hpp"

#include <llvm/ADT/StringRef.h>

namespace tyr {
class Module;

namespace pass {
class RustCodegenPass : public ir::Pass {
public:
  explicit RustCodegenPass(const llvm::StringRef OutputDir, uint32_t RTOptions);

  std::string getName() override;
  bool runOnModule(Module &m) override;

private:
  const std::string m_output_dir_;
  const uint32_t m_rt_options_;
};

ir::Pass::Ptr createRustCodegenPass(const llvm::StringRef OutputDir,
                                    uint32_t RTOptions);
} // namespace pass
} // namespace tyr

#endif // TYR_RUSTCODEGENPASS_HPP
