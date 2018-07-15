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

#ifndef TYR_CLIENT_HPP
#define TYR_CLIENT_HPP

#include <memory>
#include <string>

#include <runtime.grpc.pb.h>

#include <llvm/IR/LLVMContext.h>

namespace llvm {
class Module;
}

namespace tyr {
namespace runtime {
class Processor : ProcessorContext::Service {

  void SetExecModule(std::string serialized);

  // Returns the call's return value
  std::string ExecCall(std::string args);

private:
  llvm::LLVMContext m_ctx_;
  std::unique_ptr<llvm::Module> m_exec_module_;
};
} // namespace runtime
} // namespace tyr

#endif // TYR_CLIENT_HPP
