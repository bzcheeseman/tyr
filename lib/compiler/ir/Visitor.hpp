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

namespace tyr {
namespace ir {
class Struct;
struct Field;

class Visitor {
public:
  virtual bool visitStruct(const Struct &s) = 0;
  virtual bool visitField(const Field &f) = 0;
};
} // namespace ir
} // namespace tyr

#endif // TYR_VISITOR_HPP
