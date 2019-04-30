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

#ifndef TYR_STRUCT_HPP
#define TYR_STRUCT_HPP

#include <llvm/IR/IRBuilder.h>
#include <string>
#include <vector>

#include "Pass.hpp"

namespace llvm {
class Type;
class StructType;
class Module;
class Value;
} // namespace llvm

namespace tyr {
namespace ir {
using FieldPtr = std::unique_ptr<Field>;

class Struct {
public:
  explicit Struct(llvm::StringRef name);
  virtual ~Struct() = default;

  void setIsPacked(bool isPacked);
  void addField(llvm::StringRef name, llvm::Type *type, bool isMutable);
  void addRepeatedField(llvm::StringRef name, llvm::Type *type, bool isMutable);
  void finalizeFields(llvm::Module *Parent);

  llvm::ArrayRef<FieldPtr> getFields() const;
  const llvm::StringRef getName() const;
  llvm::StructType *getType() const;

private:
  const std::string m_name_;
  bool m_packed_ = false;
  llvm::StructType *m_type_ = nullptr;

  llvm::SmallVector<FieldPtr, 0> m_fields_;
};

struct Field {
  // Basic information
  std::string name;
  llvm::Type *type;
  bool isMutable;
  // If it's a struct we have special handling
  bool isStruct;
  // Repeated fields
  bool isRepeated;
  bool isCount;
  Field *countField = nullptr;
  Field *countsFor = nullptr;
  // LLVM information
  llvm::StructType *parentType;
  uint32_t offset;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const tyr::ir::Field &f);
llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const tyr::ir::Struct &s);

} // namespace ir
} // namespace tyr

#endif // TYR_STRUCT_HPP
