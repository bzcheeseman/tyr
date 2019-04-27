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

#include "Module.hpp"

#include <sstream>

tyr::Parser::Parser(tyr::Module &generator)
    : m_module_(generator), m_current_struct_(nullptr) {}

bool tyr::Parser::parseFile(std::istream &input) {
  uint64_t lineno = 0;
  bool insideComment = false;
  std::string l;
  while (std::getline(input, l)) {
    std::istringstream line{l};
    std::vector<std::string> tokens{std::istream_iterator<std::string>{line},
                                    std::istream_iterator<std::string>{}};

    if (tokens.empty()) { // nothing on that line
      continue;
    }

    if (tokens[0] == "//") { // line comment
      continue;
    }

    if (tokens[0] == "/*") { // block comment
      insideComment = true;
      continue;
    }

    if (tokens[0] == "*/") { // end block comment
      insideComment = false;
      continue;
    }

    if (!insideComment) {
      if (!parseLine(tokens)) {
        llvm::errs() << "Failed to parse line " << lineno << "\n";
        return false;
      }
    }

    ++lineno;
  }
  return true;
}

namespace {
bool addField(tyr::Module &m, const std::string &StructName, bool IsMutable,
              bool IsRepeated, std::string FieldType, std::string FieldName) {
  llvm::Type *FT = m.parseType(std::move(FieldType), IsRepeated);
  if (FT == nullptr) {
    return false;
  }

  // TODO: handle map fields
  if (IsRepeated) {
    m.getOrCreateStruct(StructName)
        ->addRepeatedField(std::move(FieldName), FT, IsMutable);
  } else {
    m.getOrCreateStruct(StructName)
        ->addField(std::move(FieldName), FT, IsMutable);
  }

  return true;
}
} // namespace

bool tyr::Parser::parseLine(const std::vector<std::string> &tokens) {
  if (tokens[0] == "struct") {
    if (m_current_struct_ != nullptr) {
      llvm::errs() << "tyr doesn't support nested struct declarations\n";
      return false;
    }
    const std::string &StructName = tokens[1];
    m_current_struct_ = m_module_.getOrCreateStruct(StructName);
    m_current_struct_->setIsPacked(tokens.size() == 3 && tokens[2] == "packed");
    return true;
  }

  if (tokens[0] == "}") {
    m_current_struct_->finalizeFields(m_module_.getModule());
    m_current_struct_ = nullptr;
    return true;
  }

  bool IsMut = false;
  bool IsRepeated = false;
  std::string FieldTy;

  if (tokens[0] == "mutable" || tokens[1] == "mutable") {
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
  return addField(m_module_, m_current_struct_->getName(), IsMut, IsRepeated,
                  FieldTy, FieldName);
}
