//
// Created by Aman LaChapelle on 11/25/18.
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

#ifndef TYR_STRUCTGEN_HPP
#define TYR_STRUCTGEN_HPP

#include <string>
#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>

namespace llvm {
class Type;
class StructType;
class Function;
class Argument;
} // namespace llvm

namespace tyr {
class Field {
public:
  Field(std::string Name, llvm::Type *FieldType, bool IsMutable, bool IsStruct,
        bool IsCount);

  Field &operator=(const Field &other) = default;

  void setParentType(llvm::StructType *ParentType);
  void setOffsetIntoParent(uint32_t Offset);
  void setCountField(Field *CountField);

  llvm::Type *getType() const;
  bool isMut() const;
  bool isCount() const;

  // Initializes a field to zero in struct \p Struct with \p builder
  bool initField(llvm::Value *Struct, llvm::Argument *Arg,
                 llvm::IRBuilder<> &builder, llvm::Module *Parent) const;
  // Destroys a field in \p Struct with \p builder. Calls free if necessary
  bool destroyField(llvm::Value *Struct, llvm::IRBuilder<> &builder) const;

  // Get the size of the field in memory
  llvm::Value *getFieldAllocSize(llvm::Value *Struct,
                                 llvm::IRBuilder<> &builder) const;

  // Get the size of the thing sitting in memory in the struct
  uint64_t getFieldSize(llvm::Module *Parent) const;

  bool getGetter(llvm::Module *Parent) const;
  bool getSetter(llvm::Module *Parent) const;

  bool getSerializer(llvm::Module *Parent) const;
  bool getDeserializer(llvm::Module *Parent) const;

  std::string getSerializerName() const;
  std::string getDeserializerName() const;

protected:
  std::string m_name_;
  llvm::Type *m_type_;
  bool m_is_mut_;
  bool m_is_struct_;
  // Helpers for pointer fields
  bool m_is_count_;
  Field *m_count_field_;
  // Properties of the field within the parent
  llvm::StructType *m_parent_type_;
  uint32_t m_offset_;
};

class StructGen {
public:
  StructGen(std::string Name, bool IsPacked);

  bool addField(std::string Name, llvm::Type *FieldType, bool IsMutable);
  bool populateModule(llvm::Module *Parent);
  void finalizeFields(llvm::Module *Parent);
  llvm::Type *getStructType() const;

private:
  bool getConstructor(llvm::Module *Parent);
  bool getDestructor(llvm::Module *Parent);

  bool getSerializer(llvm::Module *Parent);
  bool getDeserializer(llvm::Module *Parent);

private:
  const std::string m_name_;
  const bool m_packed_;
  llvm::StructType *m_type_;
  std::vector<std::unique_ptr<Field>> m_elements_;
};
} // namespace tyr

#endif // TYR_STRUCTGEN_HPP
