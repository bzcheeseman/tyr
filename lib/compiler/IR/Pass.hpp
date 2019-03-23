//
// Created by Aman LaChapelle on 2019-01-27.
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

#ifndef TYR_VISITOR_HPP
#define TYR_VISITOR_HPP

#include <memory>

namespace tyr {
class Module;
namespace ir {
class Struct;
struct Field;

class Pass {
public:
  virtual ~Pass() = default;

  using Ptr = std::unique_ptr<Pass>;

  virtual std::string getName() = 0;

  /**
   * Pass members are run in the following order for each module:
   * 1. runOnField for each field of each struct in the module
   * 2. runOnStruct for each struct in the module
   * 3. runOnModule on the module itself.
   *
   * Each pass should override the method(s) it needs to perform its
   * work.
   */

  virtual bool runOnModule(Module &m) { return true; }

  virtual bool runOnStruct(const Struct &s) { return true; }

  virtual bool runOnField(const Field &f) { return true; }
};
} // namespace ir
} // namespace tyr

#endif // TYR_VISITOR_HPP
