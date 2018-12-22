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

#include "Parser.hpp"

tyr::Parser::Parser(tyr::CodeGen &generator) : m_generator_(generator) {}

bool tyr::Parser::parseFile(std::istream &input) {
  std::string line;
  uint64_t lineno = 0;
  while (std::getline(input, line)) {
    if (!line.empty()) {
      std::istringstream iss(line);
      if (!parseLine(iss)) {
        llvm::errs() << "Failed to parse line " << lineno << "\n";
        return false;
      }
      ++lineno;
    }
  }
  return true;
}

bool tyr::Parser::parseLine(std::istringstream &input) {
  std::vector<std::string> tokens{std::istream_iterator<std::string>{input},
                                  std::istream_iterator<std::string>{}};

  if (tokens[0] == "struct") {
    if (!m_current_struct_.empty()) {
      llvm::errs() << "tyr doesn't support nested struct declarations\n";
      return false;
    }
    std::string &StructName = tokens[1];
    m_current_struct_ = StructName;
    return m_generator_.newStruct(StructName,
                                  tokens.size() == 3 && tokens[2] == "packed");
  }

  if (tokens[0] == "}") {
    if (!m_generator_.finalizeStruct(m_current_struct_)) {
      llvm::errs() << "Finalization for the struct failed\n";
      return false;
    }
    m_current_struct_.clear();
    return true;
  }

  bool IsMut = false;
  bool IsRepeated = false;
  std::string FieldTy;

  if (tokens[0] == "mutable") {
    IsMut = true;
  }

  if (tokens[0] == "repeated" || tokens[1] == "repeated") {
    IsRepeated = true;
  }

  if (IsMut && IsRepeated) {
    // mutable repeated <type>
    FieldTy = tokens[2];
  } else if (IsMut || IsRepeated) {
    // mutable <type> or repeated <type>
    FieldTy = tokens[1];
  } else {
    // <type>
    FieldTy = tokens[0];
  }

  std::string FieldName = *tokens.rbegin();
  return m_generator_.addField(m_current_struct_, IsMut, IsRepeated, FieldTy,
                               FieldName);
}
