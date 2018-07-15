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

#ifndef TYR_ROUTER_HPP
#define TYR_ROUTER_HPP

#include <memory>
#include <string>

#include <llvm/IR/LLVMContext.h>

namespace llvm {
class Module;
}

namespace tyr {
namespace runtime {
class Router { // router routes requests, does it also join requests?

  void SetRouteModule(std::string serialized);

  // Returns the call's return value, handles connecting to external server
  std::string RouteCall(std::string path, std::string args);

  void ServeRouter(const std::string &base_address, uint16_t port);

private:
  llvm::LLVMContext m_ctx_;
  std::unique_ptr<llvm::Module> m_route_module_;

  std::atomic<bool> m_should_continue_;
};
} // namespace runtime
} // namespace tyr

#endif // TYR_ROUTER_HPP
