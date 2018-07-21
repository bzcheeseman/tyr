//
// Created by Aman LaChapelle on 7/20/18.
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

#ifndef TYR_ROUTEFINDER_HPP
#define TYR_ROUTEFINDER_HPP

#include <string>
#include <map>

namespace llvm {
class Module;
class Instruction;
class Function;
} // namespace llvm

namespace tyr {
namespace compiler {

class RouteFinder {
public:
  explicit RouteFinder(llvm::Module *module);

private:
  // generate routing table from main (what is the route? is it a string? do we assert that it's a string?)
  std::map<std::string, llvm::Function *> GenerateRouteTable_();

private:
  llvm::Module *m_module_;

};
} // namespace compiler
} // namespace tyr

#endif // TYR_ROUTEFINDER_HPP
