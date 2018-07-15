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

#ifndef TYR_LOADER_HPP
#define TYR_LOADER_HPP

#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>

namespace llvm {
class LLVMContext;
}

namespace tyr {
namespace compiler {
class Loader {
  explicit Loader(llvm::LLVMContext &ctx);

  std::unique_ptr<llvm::Module> LoadModule(const std::string &filename);

  std::unique_ptr<llvm::Module>
  LoadModule(llvm::MemoryBufferRef serialized_module);

private:
  void ResetDataLayout(llvm::Module *module);

private:
  llvm::LLVMContext &m_ctx_;
};
} // namespace compiler
} // namespace tyr

#endif // TYR_LOADER_HPP
