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

#ifndef TYR_PARSER_HPP
#define TYR_PARSER_HPP

#include <istream>
#include <llvm/ADT/ArrayRef.h>

namespace tyr {

class Module;

namespace ir {
class Struct;
}

/*
 * This class is set up to handle things that look like
 * struct thing <packed> {
 *   mutable repeated int8 bytes
 *   int16 someint
 *   mutable float myfloat
 * }
 */

class Parser {
public:
  explicit Parser(Module &generator);

  bool parseFile(std::istream &input);

private:
  bool parseLine(const llvm::ArrayRef<std::string> tokens);

private:
  ir::Struct *m_current_struct_;
  Module &m_module_;
};
} // namespace tyr

#endif // TYR_PARSER_HPP
