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

#include "IR.hpp"

#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>

tyr::ir::Struct::Struct(std::string name, bool isPacked)
    : m_name_(std::move(name)), m_packed_(isPacked) {}

void tyr::ir::Struct::addField(std::string name, llvm::Type *type,
                               bool isMutable) {
  llvm::LLVMContext &ctx = type->getContext();

  if (type->isPointerTy() && !type->getPointerElementType()->isStructTy()) {
    // Insert the count for the field first
    Field count = {};
    count.name = name + "_count";
    count.type = llvm::Type::getInt64Ty(ctx);
    count.mut = isMutable;
    count.isStruct = false;
    count.isCount = true;
    count.countField = nullptr;
    count.parentType = nullptr;
    count.offset = 0;

    m_fields_.push_back(llvm::make_unique<Field>(count));
    // snag a ref to the count field now
    Field *CountFieldPtr = m_fields_.rbegin()->get();

    // Insert the new field (and set the count field)
    Field f = {};
    f.name = name;
    f.type = type;
    f.mut = isMutable;
    f.isStruct = false;
    f.isCount = false;
    f.countField = CountFieldPtr;
    f.parentType = nullptr;
    f.offset = 0;

    m_fields_.push_back(llvm::make_unique<Field>(f));
  } else {
    // Insert the new field (and set the count field)
    Field f = {};
    f.name = name;
    f.type = type;
    f.mut = isMutable;
    f.isStruct =
        type->isPointerTy() && type->getPointerElementType()->isStructTy();
    f.isCount = false;
    f.countField = nullptr;
    f.parentType = nullptr;
    f.offset = 0;

    m_fields_.push_back(llvm::make_unique<Field>(f));
  }
}

namespace {
uint64_t getFieldSize(const tyr::ir::Field *f, llvm::Module *Parent) {
  const llvm::DataLayout &DL = Parent->getDataLayout();
  return DL.getTypeAllocSize(f->type);
}
} // namespace

void tyr::ir::Struct::finalizeFields(llvm::Module *Parent) {
  // Order the entries by size of field
  std::sort(m_fields_.begin(), m_fields_.end(),
            [Parent](const std::unique_ptr<Field> &lhs,
                     const std::unique_ptr<Field> &rhs) {
              return getFieldSize(lhs.get(), Parent) <
                     getFieldSize(rhs.get(), Parent);
            });

  std::vector<llvm::Type *> element_types;
  for (auto &entry : m_fields_) {
    element_types.push_back(entry->type);
  }

  m_type_ = llvm::StructType::create(element_types, m_name_, m_packed_);

  // Set the parent type in each of the fields
  uint32_t offset = 0;
  for (auto &entry : m_fields_) {
    entry->parentType = m_type_;
    entry->offset = offset;
    ++offset;
  }
}

llvm::ArrayRef<tyr::ir::FieldPtr> tyr::ir::Struct::getFields() const {
  return m_fields_;
}

const std::string &tyr::ir::Struct::getName() const { return m_name_; }

llvm::StructType *tyr::ir::Struct::getType() const { return m_type_; }

bool tyr::ir::Struct::visit(tyr::ir::Visitor &visitor) {
  bool success = true;
  for (auto &field : m_fields_) {
    bool visitSuccess = visitor.visitField(*field);
    if (!visitSuccess) {
      llvm::errs() << "Visiting field " << field->name << " failed";
    }
    success &= visitSuccess;
  }

  bool visitSuccess = visitor.visitStruct(*this);
  if (!visitSuccess) {
    llvm::errs() << "Visiting struct " << this->m_name_ << " failed";
  }
  success &= visitSuccess;

  return success;
}
