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

#include "StructGen.hpp"

namespace { // Utilities
            // Returns the non-null block
llvm::BasicBlock *insertNullCheck(llvm::ArrayRef<llvm::Value *> Ptrs,
                                  llvm::Value *ReturnFail,
                                  llvm::IRBuilder<> &builder,
                                  llvm::Function *Parent,
                                  llvm::BasicBlock *OnSuccess = nullptr) {
  llvm::LLVMContext &ctx = builder.getContext();

  llvm::Value *NotNull = builder.CreateIsNotNull(Ptrs[0]);
  for (auto ptrIter = Ptrs.begin() + 1, end = Ptrs.end(); ptrIter != end;
       ++ptrIter) {
    NotNull = builder.CreateOr(NotNull, builder.CreateIsNotNull(*ptrIter));
  }

  llvm::BasicBlock *IsNull, *IsNotNull;
  IsNull = llvm::BasicBlock::Create(ctx, "", Parent);
  if (OnSuccess == nullptr) {
    IsNotNull = llvm::BasicBlock::Create(ctx, "", Parent);
  } else {
    IsNotNull = OnSuccess;
  }
  builder.CreateCondBr(NotNull, IsNotNull, IsNull);

  // Handle it if it was null
  builder.SetInsertPoint(IsNull);
  builder.CreateRet(ReturnFail);

  return IsNotNull;
}

} // namespace

tyr::Field::Field(std::string Name, llvm::Type *FieldType, bool IsMutable,
                  bool IsStruct, bool IsCount)
    : m_name_(std::move(Name)), m_type_(FieldType), m_is_mut_(IsMutable),
      m_is_struct_(IsStruct), m_is_count_(IsCount), m_offset_(0) {}

void tyr::Field::setParentType(llvm::StructType *ParentType) {
  m_parent_type_ = ParentType;
}

void tyr::Field::setOffsetIntoParent(uint32_t Offset) { m_offset_ = Offset; }

void tyr::Field::setCountField(Field *CountField) {
  m_count_field_ = CountField;
}

llvm::Type *tyr::Field::getType() const { return m_type_; }

bool tyr::Field::isMut() const { return m_is_mut_; }

bool tyr::Field::isCount() const { return m_is_count_; }

llvm::Value *tyr::Field::getFieldAllocSize(llvm::Value *Struct,
                                           llvm::IRBuilder<> &builder) const {
  llvm::Module *Parent = builder.GetInsertBlock()->getParent()->getParent();

  const llvm::DataLayout &DL = Parent->getDataLayout();

  if (m_type_->isPointerTy()) { // array
    uint64_t FieldAllocSize = DL.getTypeAllocSize(m_type_->getPointerElementType());
    llvm::Value *CountGEP =
        builder.CreateStructGEP(Struct, m_count_field_->m_offset_);
    return builder.CreateMul(builder.getInt64(FieldAllocSize),
                             builder.CreateLoad(CountGEP));
  }

  return builder.getInt64(DL.getTypeAllocSize(m_type_));
}

bool tyr::Field::initField(llvm::Value *Struct, llvm::Argument *Arg,
                           llvm::IRBuilder<> &builder) const {
  if (m_is_mut_) {
    // Initialize to zero (still works even if it's a pointer)
    builder.CreateStore(
        builder.CreateTruncOrBitCast(builder.getInt64(0), m_type_),
        builder.CreateStructGEP(Struct, m_offset_));
  } else {
    if (Arg == nullptr) {
      llvm::errs() << "Arg was null on a field that is immutable (and "
                      "therefore needs an initializer), aborting\n";
      return false;
    }
    // Initialize to the value in the constructor
    builder.CreateStore(Arg, builder.CreateStructGEP(Struct, m_offset_));
  }

  return true;
}

bool tyr::Field::destroyField(llvm::Value *Struct,
                              llvm::IRBuilder<> &builder) const {
  llvm::Module *Parent = builder.GetInsertBlock()->getParent()->getParent();
  llvm::Value *FieldGEP = builder.CreateStructGEP(Struct, m_offset_);

  if (m_type_->isPointerTy()) {
    builder.CreateCall(Parent->getFunction("free"),
                       builder.CreateBitCast(builder.CreateLoad(FieldGEP),
                                             builder.getInt8PtrTy(0)));
  }

  return true;
}

uint64_t tyr::Field::getFieldSize(llvm::Module *Parent) const {
  const llvm::DataLayout &DL = Parent->getDataLayout();
  uint64_t FieldSize = DL.getTypeAllocSize(m_type_);
  return FieldSize;
}

bool tyr::Field::getGetter(llvm::Module *Parent) const {
  llvm::Type *StructPtrType = m_parent_type_->getPointerTo(0);

  // Get an alias to the context
  llvm::LLVMContext &ctx = Parent->getContext();

  std::string GetterName =
      "get_" + std::string(m_parent_type_->getName()) + "_" + m_name_;
  // Getter returns bool, returns the thing by reference
  llvm::FunctionType *GetterType =
      llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx),
                              {StructPtrType, m_type_->getPointerTo(0)}, false);

  // Create the function
  llvm::Function *Getter = llvm::cast<llvm::Function>(
      Parent->getOrInsertFunction(GetterName, GetterType));
  llvm::BasicBlock *GetterBlock = llvm::BasicBlock::Create(ctx, "", Getter);
  llvm::IRBuilder<> builder(GetterBlock);

  // Get the first input to the function, the struct we're operating on
  auto arg_iter = Getter->arg_begin();
  llvm::Value *Self = &*arg_iter;
  ++arg_iter;

  // Make sure it's not NULL
  llvm::BasicBlock *SelfIsNotNull = insertNullCheck(
      {Self, &*arg_iter}, builder.getInt1(false), builder, Getter);

  // Handle if it's not null
  builder.SetInsertPoint(SelfIsNotNull);
  llvm::Value *FieldGEP = builder.CreateStructGEP(Self, m_offset_);
  llvm::Value *FieldLoad = builder.CreateLoad(FieldGEP);

  // Get the place where we're storing the result
  llvm::Value *OutVal = &*arg_iter;

  // If it's not mutable alloc a new array and copy it over
  if (m_type_->isPointerTy() && !m_is_mut_) {
    const size_t &PtrFieldCountOffset = m_count_field_->m_offset_;

    llvm::Value *FieldAllocSize = getFieldAllocSize(Self, builder);

    llvm::Value *AllocdMem =
        builder.CreateCall(Parent->getFunction("malloc"), FieldAllocSize);
    llvm::BasicBlock *AllocSucceeded =
        insertNullCheck({AllocdMem}, builder.getInt1(false), builder, Getter);

    builder.SetInsertPoint(AllocSucceeded);

    builder.CreateMemCpy(AllocdMem, 0, FieldLoad, 0, FieldAllocSize, false);
    builder.CreateStore(builder.CreateBitCast(AllocdMem, m_type_), OutVal);
    builder.CreateRet(builder.getInt1(true));
    return true;
  }

  // It's mutable so we return by reference and that's it (if it's a pointer we
  // return the memory directly)
  builder.CreateStore(FieldLoad, OutVal);
  builder.CreateRet(builder.getInt1(true));

  return true;
}

bool tyr::Field::getSetter(llvm::Module *Parent) const {
  if (!m_is_mut_ || m_is_count_) {
    return true;
  }

  llvm::Type *StructPtrType = m_parent_type_->getPointerTo(0);

  // Get an alias to the context
  llvm::LLVMContext &ctx = Parent->getContext();

  std::string SetterName =
      "set_" + std::string(m_parent_type_->getName()) + "_" + m_name_;
  // Setter returns a bool to indicate if it was successfully set (will return
  // false always if field is non-mutable)
  llvm::FunctionType *SetterType;
  if (m_type_->isPointerTy()) {
    // If it's a pointer type then you have to pass in the size of the array
    // you're copying
    SetterType = llvm::FunctionType::get(
        llvm::Type::getInt1Ty(ctx),
        {StructPtrType, m_type_, llvm::Type::getInt64Ty(ctx)}, false);
  } else {
    SetterType = llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx),
                                         {StructPtrType, m_type_}, false);
  }

  // Create the function
  llvm::Function *Setter = llvm::cast<llvm::Function>(
      Parent->getOrInsertFunction(SetterName, SetterType));
  llvm::BasicBlock *SetterBlock = llvm::BasicBlock::Create(ctx, "", Setter);
  llvm::IRBuilder<> builder(SetterBlock);

  // Get the first input to the function, which is the struct we're
  // operating on
  auto arg_iterator = Setter->arg_begin();
  llvm::Value *Self = &*arg_iterator;
  llvm::cast<llvm::Argument>(Self)->addAttr(
      llvm::Attribute::AttrKind::NoCapture);
  // Make sure it's not NULL
  llvm::BasicBlock *SelfIsNotNull =
      insertNullCheck({Self}, builder.getInt1(false), builder, Setter);

  // It's not NULL so we check what we're inserting
  builder.SetInsertPoint(SelfIsNotNull);
  ++arg_iterator;
  llvm::Value *ToInsert = &*arg_iterator;

  // GEP the field
  llvm::Value *FieldGEP = builder.CreateStructGEP(Self, m_offset_);
  if (m_type_->isPointerTy()) {
    llvm::Value *PtrFieldCount =
        builder.CreateStructGEP(Self, m_count_field_->m_offset_);

    // Need the number of elements to copy
    ++arg_iterator;
    llvm::Value *NumElts = &*arg_iterator;

    llvm::Value *LoadedField = builder.CreateLoad(FieldGEP);

    // Check if the field is NULL
    llvm::Value *FieldNotNull = builder.CreateIsNotNull(LoadedField);
    llvm::BasicBlock *FieldIsNull = llvm::BasicBlock::Create(ctx, "", Setter);
    llvm::BasicBlock *CheckSize = llvm::BasicBlock::Create(ctx, "", Setter);
    builder.CreateCondBr(FieldNotNull, CheckSize, FieldIsNull);

    // If it is NULL, allocate memory and branch to the next block
    builder.SetInsertPoint(FieldIsNull);
    // Gotta store the correct value for the field count
    builder.CreateStore(NumElts, PtrFieldCount);
    // Get the field alloc size
    llvm::Value *FieldAllocSize = getFieldAllocSize(Self, builder);
    // Do the allocation
    llvm::Value *AllocdMem =
        builder.CreateCall(Parent->getFunction("malloc"), FieldAllocSize);
    AllocdMem = builder.CreateBitCast(AllocdMem, m_type_);
    // Check that it succeeded, if it's not NULL go to SizeIsRight
    llvm::BasicBlock *SizeIsRight =
        insertNullCheck({AllocdMem}, builder.getInt1(false), builder, Setter);

    // The field is not NULL so check the size of the field
    builder.SetInsertPoint(CheckSize);
    // Get the current count and check to make sure the sizes match
    llvm::Value *FieldCount = builder.CreateLoad(PtrFieldCount);
    llvm::Value *SizeMatches = builder.CreateICmpEQ(FieldCount, NumElts);

    // Set up in case size is wrong
    llvm::BasicBlock *SizeIsWrong = llvm::BasicBlock::Create(ctx, "", Setter);
    builder.CreateCondBr(SizeMatches, SizeIsRight, SizeIsWrong);

    // Size was wrong so reallocate memory
    builder.SetInsertPoint(SizeIsWrong);
    // Store the correct size
    builder.CreateStore(NumElts, PtrFieldCount);
    // Get the field alloc size now in case it was recalculated
    FieldAllocSize = getFieldAllocSize(Self, builder);
    // And realloc
    llvm::Value *ReallocFieldMem = builder.CreateCall(
        Parent->getFunction("realloc"),
        {builder.CreateBitCast(LoadedField, builder.getInt8PtrTy(0)),
         FieldAllocSize});
    ReallocFieldMem = builder.CreateBitCast(ReallocFieldMem, m_type_);
    // Make sure realloc succeeded
    (void)insertNullCheck({ReallocFieldMem}, builder.getInt1(false), builder,
                          Setter, SizeIsRight);

    // Size is correct and everything seems fine
    builder.SetInsertPoint(SizeIsRight);
    llvm::PHINode *FinalFieldMem = builder.CreatePHI(m_type_, 3);
    FinalFieldMem->addIncoming(AllocdMem, FieldIsNull);
    FinalFieldMem->addIncoming(LoadedField, CheckSize);
    FinalFieldMem->addIncoming(ReallocFieldMem, SizeIsWrong);

    // Store the correct count in the correct field
    builder.CreateStore(NumElts, PtrFieldCount);

    // Get the field alloc size now in case it was recalculated
    FieldAllocSize = getFieldAllocSize(Self, builder);
    builder.CreateMemCpy(FinalFieldMem, 0, ToInsert, 0, FieldAllocSize, false);

    // Store the memory in the field
    builder.CreateStore(FinalFieldMem, FieldGEP);

    builder.CreateRet(builder.getInt1(true));
  } else {
    builder.CreateStore(ToInsert, FieldGEP);
    builder.CreateRet(builder.getInt1(true));
  }

  return true;
}

bool tyr::Field::getSerializer(llvm::Module *Parent) const {
  if (m_is_count_) { // Don't serialize count fields
    return true;
  }

  llvm::LLVMContext &ctx = Parent->getContext();

  std::string Name = getSerializerName();

  // This function is meant for internal use only, it serializes the field into
  // the second arg It returns the number of bytes written
  llvm::FunctionType *SerializerType = llvm::FunctionType::get(
      llvm::Type::getInt64Ty(ctx),
      {m_parent_type_->getPointerTo(0), llvm::Type::getInt8PtrTy(ctx)}, false);

  // Create the function
  llvm::Function *Serializer = llvm::cast<llvm::Function>(
      Parent->getOrInsertFunction(Name, SerializerType));
  llvm::BasicBlock *EntryBlock = llvm::BasicBlock::Create(ctx, "", Serializer);
  llvm::IRBuilder<> builder(EntryBlock);

  // Get the args
  auto arg_iter = Serializer->arg_begin();
  llvm::Value *Self = &*arg_iter;
  ++arg_iter;
  llvm::Value *OutBuf = &*arg_iter;

  // Check if the args are null
  llvm::BasicBlock *IsNotNull =
      insertNullCheck({Self, OutBuf}, builder.getInt64(0), builder, Serializer);

  // Not null, we can continue
  builder.SetInsertPoint(IsNotNull);

  llvm::Value *CurrentPtr = builder.CreateGEP(OutBuf, builder.getInt64(0));
  llvm::Value *OutSize;
  if (m_type_->isPointerTy()) {
    // Get the count and store it first
    llvm::Value *Count = builder.CreateLoad(
        builder.CreateStructGEP(Self, m_count_field_->m_offset_));
    llvm::Value *CastedCurrentPtr = builder.CreateBitCast(
        CurrentPtr, m_count_field_->m_type_->getPointerTo(0));
    builder.CreateStore(Count, CastedCurrentPtr);
    // Set the output size to the size of the count field
    OutSize = m_count_field_->getFieldAllocSize(Self, builder);

    // Increment CurrentPtr so we can store the data
    CurrentPtr = builder.CreateGEP(OutBuf, OutSize);

    // Now store the data
    llvm::Value *FieldData =
        builder.CreateLoad(builder.CreateStructGEP(Self, m_offset_));
    llvm::Value *PtrFieldAllocSize = getFieldAllocSize(Self, builder);
    builder.CreateMemCpy(CurrentPtr, 0, FieldData, 0, PtrFieldAllocSize);
    // Increment the OutSize by the size of the pointer field
    OutSize = builder.CreateAdd(OutSize, PtrFieldAllocSize);
  } else {
    // Just store the data
    llvm::Value *FieldData =
        builder.CreateLoad(builder.CreateStructGEP(Self, m_offset_));
    OutSize = getFieldAllocSize(Self, builder);
    llvm::Value *CastedCurrentPtr =
        builder.CreateBitCast(CurrentPtr, m_type_->getPointerTo(0));
    builder.CreateStore(FieldData, CastedCurrentPtr);
  }

  builder.CreateRet(OutSize);

  return true;
}

bool tyr::Field::getDeserializer(llvm::Module *Parent) const {
  if (m_is_count_) { // Don't serialize count fields
    return true;
  }

  llvm::LLVMContext &ctx = Parent->getContext();

  std::string Name = getDeserializerName();

  // This function is meant for internal use only, it deserializes the field
  // from the second arg It returns the number of bytes read
  llvm::FunctionType *DeserializerType = llvm::FunctionType::get(
      llvm::Type::getInt64Ty(ctx),
      {m_parent_type_->getPointerTo(0), llvm::Type::getInt8PtrTy(ctx)}, false);

  // Create the function
  llvm::Function *Deserializer = llvm::cast<llvm::Function>(
      Parent->getOrInsertFunction(Name, DeserializerType));
  llvm::BasicBlock *EntryBlock =
      llvm::BasicBlock::Create(ctx, "", Deserializer);
  llvm::IRBuilder<> builder(EntryBlock);

  // Get the args
  auto arg_iter = Deserializer->arg_begin();
  llvm::Value *Self = &*arg_iter;
  ++arg_iter;
  llvm::Value *InBuf = &*arg_iter;

  // Check if the args are null
  llvm::BasicBlock *IsNotNull = insertNullCheck(
      {Self, InBuf}, builder.getInt64(0), builder, Deserializer);

  // Not null, we can continue
  builder.SetInsertPoint(IsNotNull);

  llvm::Value *CurrentPtr = builder.CreateGEP(InBuf, builder.getInt64(0));
  llvm::Value *OutSize;
  if (m_type_->isPointerTy()) {
    // Get the count first
    llvm::Value *CastedCurrentPtr = builder.CreateBitCast(
        CurrentPtr, m_count_field_->m_type_->getPointerTo(0));
    llvm::Value *Count = builder.CreateLoad(CastedCurrentPtr);
    // Store the count into Self
    builder.CreateStore(
        Count, builder.CreateStructGEP(Self, m_count_field_->m_offset_));
    OutSize = m_count_field_->getFieldAllocSize(Self, builder);

    // Increment CurrentPtr so we can read the data
    CurrentPtr = builder.CreateGEP(InBuf, OutSize);

    // Now get the data (alloc space)
    CastedCurrentPtr =
        builder.CreateBitCast(CurrentPtr, m_type_->getPointerTo(0));
    llvm::Value *PtrFieldAllocSize = getFieldAllocSize(Self, builder);
    llvm::Value *FieldMem =
        builder.CreateCall(Parent->getFunction("malloc"), PtrFieldAllocSize);
    llvm::BasicBlock *MallocSucceeded =
        insertNullCheck({FieldMem}, builder.getInt64(0), builder, Deserializer);

    // Malloc succeeded, so now handle it
    builder.SetInsertPoint(MallocSucceeded);
    // Do the copy
    builder.CreateMemCpy(FieldMem, 0, CastedCurrentPtr, 0, PtrFieldAllocSize);
    // And store it into Self
    builder.CreateStore(builder.CreateBitCast(FieldMem, m_type_),
                        builder.CreateStructGEP(Self, m_offset_));
    OutSize = builder.CreateAdd(OutSize, PtrFieldAllocSize);
  } else {
    // Just store the data
    llvm::Value *CastedCurrentPtr =
        builder.CreateBitCast(CurrentPtr, m_type_->getPointerTo(0));
    llvm::Value *FieldData = builder.CreateLoad(CastedCurrentPtr);
    OutSize = getFieldAllocSize(Self, builder);
    builder.CreateStore(FieldData, builder.CreateStructGEP(Self, m_offset_));
  }

  builder.CreateRet(OutSize);

  return true;
}

std::string tyr::Field::getSerializerName() const {
  return "__serialize_" + std::string(m_parent_type_->getName()) + "_" +
         m_name_;
}

std::string tyr::Field::getDeserializerName() const {
  return "__deserialize_" + std::string(m_parent_type_->getName()) + "_" +
         m_name_;
}

tyr::StructGen::StructGen(std::string Name, bool IsPacked)
    : m_name_(std::move(Name)), m_packed_(IsPacked) {}

bool tyr::StructGen::addField(std::string Name, llvm::Type *FieldType,
                              bool IsMutable) {
  llvm::LLVMContext &ctx = FieldType->getContext();

  if (FieldType->isPointerTy()) {
    // Insert the count for the field first
    m_elements_.emplace_back(llvm::make_unique<Field>(
        Name + "_count", llvm::Type::getInt64Ty(ctx), IsMutable, false, true));
    // snag a ref to the count field now
    Field *CountFieldPtr = m_elements_.rbegin()->get();

    // Insert the new field
    m_elements_.emplace_back(llvm::make_unique<Field>(
        Name, FieldType, IsMutable, FieldType->isStructTy(), false));
    // And set the count field
    (*m_elements_.rbegin())->setCountField(CountFieldPtr);
  } else {
    // Just insert the new field
    m_elements_.emplace_back(llvm::make_unique<Field>(
        Name, FieldType, IsMutable, FieldType->isStructTy(), false));
  }

  return true;
}

void tyr::StructGen::finalizeFields(llvm::Module *Parent) {
  // Order the entries by size of field
  std::sort(m_elements_.begin(), m_elements_.end(),
            [Parent](const std::unique_ptr<Field> &lhs,
                     const std::unique_ptr<Field> &rhs) {
              return lhs->getFieldSize(Parent) < rhs->getFieldSize(Parent);
            });

  std::vector<llvm::Type *> element_types;
  for (auto &entry : m_elements_) {
    element_types.push_back(entry->getType());
  }

  m_type_ = llvm::StructType::create(element_types, m_name_, m_packed_);

  // Set the parent type in each of the fields
  uint32_t offset = 0;
  for (auto &entry : m_elements_) {
    entry->setParentType(m_type_);
    entry->setOffsetIntoParent(offset);
    ++offset;
  }
}

llvm::Type *tyr::StructGen::getStructType() const { return m_type_; }

bool tyr::StructGen::getConstructor(llvm::Module *Parent) {
  std::vector<llvm::Type *> NonMutFields;
  for (auto &entry : m_elements_) {
    if (!entry->isMut()) {
      NonMutFields.push_back(entry->getType());
    }
  }

  llvm::LLVMContext &ctx = Parent->getContext();

  std::string ConstrName = "create_" + m_name_;

  llvm::StructType *GenStructType = m_type_;
  llvm::Type *StructPtrType = GenStructType->getPointerTo(0);

  // constructor has parameters for all of the non-mutable fields
  llvm::FunctionType *ConstructorType =
      llvm::FunctionType::get(StructPtrType, NonMutFields, false);
  // Create the function
  llvm::Function *Constructor = llvm::cast<llvm::Function>(
      Parent->getOrInsertFunction(ConstrName, ConstructorType));
  llvm::BasicBlock *EntryBlock = llvm::BasicBlock::Create(ctx, "", Constructor);
  llvm::IRBuilder<> builder(EntryBlock);

  // Allocate space for the output
  const llvm::DataLayout &DL = Parent->getDataLayout();
  uint64_t StructAllocSize = DL.getTypeAllocSize(GenStructType);
  llvm::Value *StructOutRaw = builder.CreateCall(
      Parent->getFunction("malloc"), builder.getInt64(StructAllocSize));
  // Make sure malloc succeeds
  llvm::BasicBlock *MallocSuccess = insertNullCheck(
      {StructOutRaw},
      builder.CreateIntToPtr(builder.getInt64(0), StructPtrType), builder,
      Constructor);

  // Malloc succeeded
  builder.SetInsertPoint(MallocSuccess);
  llvm::Value *StructOut =
      builder.CreatePointerCast(StructOutRaw, StructPtrType);

  // Initialize all the fields
  auto ArgIter = Constructor->arg_begin();
  for (auto &entry : m_elements_) {
    if (!entry->isMut()) {
      entry->initField(StructOut, &*ArgIter, builder);
      // Only increment the argument iterator if it's not a mutable field
      // otherwise, the field is initialized to zero
      ++ArgIter;
    } else {
      entry->initField(StructOut, nullptr, builder);
    }
  }

  // Return the created object
  builder.CreateRet(StructOut);

  return true;
}

bool tyr::StructGen::getDestructor(llvm::Module *Parent) {
  llvm::LLVMContext &ctx = Parent->getContext();

  std::string DestrName = "destroy_" + m_name_;

  llvm::StructType *GenStructType = m_type_;
  llvm::Type *StructPtrType = GenStructType->getPointerTo(0);

  // constructor has parameters for all of the non-mutable fields
  llvm::FunctionType *DestructorType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(ctx), {StructPtrType}, false);

  llvm::Function *Destructor = llvm::cast<llvm::Function>(
      Parent->getOrInsertFunction(DestrName, DestructorType));
  llvm::BasicBlock *EntryBlock = llvm::BasicBlock::Create(ctx, "", Destructor);
  llvm::IRBuilder<> builder(EntryBlock);

  // Destroy all the fields
  llvm::Value *Struct = &*Destructor->arg_begin();
  for (auto &entry : m_elements_) {
    entry->destroyField(Struct, builder);
  }

  builder.CreateCall(Parent->getFunction("free"),
                     builder.CreateBitCast(Struct, builder.getInt8PtrTy(0)));
  builder.CreateRetVoid();

  return true;
}

bool tyr::StructGen::getSerializer(llvm::Module *Parent) {
  llvm::LLVMContext &ctx = Parent->getContext();

  std::string Name = "serialize_" + m_name_;

  llvm::StructType *GenStructType = m_type_;
  llvm::Type *StructPtrType = GenStructType->getPointerTo(0);

  // constructor has parameters for all of the non-mutable fields
  llvm::FunctionType *SerializerType = llvm::FunctionType::get(
      llvm::Type::getInt8PtrTy(ctx),
      {StructPtrType, llvm::Type::getInt64PtrTy(ctx)}, false);

  // Create the function
  llvm::Function *Serializer = llvm::cast<llvm::Function>(
      Parent->getOrInsertFunction(Name, SerializerType));
  llvm::BasicBlock *EntryBlock = llvm::BasicBlock::Create(ctx, "", Serializer);
  llvm::IRBuilder<> builder(EntryBlock);

  auto arg_iter = Serializer->arg_begin();
  llvm::Value *Self = &*arg_iter;
  ++arg_iter;
  llvm::Value *OutSizePtr = &*arg_iter;

  // Check if the args are null
  llvm::BasicBlock *IsNotNull =
      insertNullCheck({Self, OutSizePtr},
                      llvm::ConstantPointerNull::get(builder.getInt8PtrTy(0)),
                      builder, Serializer);

  // Not null, we can continue
  builder.SetInsertPoint(IsNotNull);

  llvm::Value *AllocSize = builder.getInt64(0);
  // add up the output memory
  for (auto &entry : m_elements_) {
    AllocSize =
        builder.CreateAdd(AllocSize, entry->getFieldAllocSize(Self, builder));
  }
  builder.CreateStore(AllocSize, OutSizePtr);

  llvm::Value *AllocdMem =
      builder.CreateCall(Parent->getFunction("malloc"), AllocSize);

  llvm::BasicBlock *MallocSucceeded = insertNullCheck(
      {AllocdMem}, llvm::ConstantPointerNull::get(builder.getInt8PtrTy(0)),
      builder, Serializer);
  builder.SetInsertPoint(MallocSucceeded);
  builder.CreateStore(AllocSize, OutSizePtr);

  llvm::Value *CurrentIDX = builder.getInt64(0);
  for (auto &entry : m_elements_) {
    if (entry->isCount()) { // count fields are handled already
      continue;
    }
    llvm::Function *EntrySerializer =
        Parent->getFunction(entry->getSerializerName());
    llvm::Value *CurrentPtr = builder.CreateGEP(AllocdMem, CurrentIDX);
    llvm::Value *OutSize = builder.CreateCall(
        EntrySerializer, {Self, CurrentPtr});
    CurrentIDX = builder.CreateAdd(CurrentIDX, OutSize);
  }

  builder.CreateRet(AllocdMem);

  return true;
}

bool tyr::StructGen::getDeserializer(llvm::Module *Parent) {
  llvm::LLVMContext &ctx = Parent->getContext();

  std::string Name = "deserialize_" + m_name_;

  llvm::StructType *GenStructType = m_type_;
  llvm::PointerType *StructPtrType = GenStructType->getPointerTo(0);

  // constructor has parameters for all of the non-mutable fields
  llvm::FunctionType *SerializerType = llvm::FunctionType::get(
      StructPtrType,
      {llvm::Type::getInt8PtrTy(ctx), llvm::Type::getInt64Ty(ctx)}, false);
  llvm::Function *Deserializer = llvm::cast<llvm::Function>(
      Parent->getOrInsertFunction(Name, SerializerType));
  llvm::BasicBlock *EntryBlock =
      llvm::BasicBlock::Create(ctx, "", Deserializer);
  llvm::IRBuilder<> builder(EntryBlock);

  // read in the byte array and its length
  auto arg_iter = Deserializer->arg_begin();
  llvm::Value *SerializedSelf = &*arg_iter;
  ++arg_iter;
  llvm::Value *SerializedSize = &*arg_iter;

  // Check if the args are invalid
  llvm::BasicBlock *IsNotNull = insertNullCheck(
      {SerializedSelf, SerializedSize},
      llvm::ConstantPointerNull::get(StructPtrType), builder, Deserializer);

  builder.SetInsertPoint(IsNotNull);
  // allocate a new thing
  const llvm::DataLayout &DL = Parent->getDataLayout();
  uint64_t StructAllocSize = DL.getTypeAllocSize(GenStructType);
  llvm::Value *StructOutRaw = builder.CreateCall(
      Parent->getFunction("malloc"), builder.getInt64(StructAllocSize));

  llvm::BasicBlock *MallocSucceeded = insertNullCheck(
      {StructOutRaw}, llvm::ConstantPointerNull::get(StructPtrType), builder,
      Deserializer);

  builder.SetInsertPoint(MallocSucceeded);
  llvm::Value *StructOut =
      builder.CreatePointerCast(StructOutRaw, StructPtrType);

  // Now set all the fields
  llvm::Value *CurrentIDX = builder.getInt64(0);
  for (auto &entry : m_elements_) {
    if (entry->isCount()) { // count fields are handled already
      continue;
    }
    llvm::Function *EntryDeserializer =
        Parent->getFunction(entry->getDeserializerName());
    llvm::Value *OutSize = builder.CreateCall(
        EntryDeserializer,
        {StructOut, builder.CreateGEP(SerializedSelf, CurrentIDX)});
    CurrentIDX = builder.CreateAdd(CurrentIDX, OutSize);
  }

  builder.CreateRet(StructOut);

  return true;
}

bool tyr::StructGen::populateModule(llvm::Module *Parent) {
  if (!getConstructor(Parent)) {
    llvm::errs() << "Get constructor failed for struct " << m_type_->getName() << " aborting\n";
    return false;
  }
  for (auto &entry : m_elements_) {
    if (!entry->getGetter(Parent)) {
      llvm::errs() << "Get getter failed for struct " << m_type_->getName() << " aborting\n";
      return false;
    }
    if (!entry->getSetter(Parent)) {
      llvm::errs() << "Get setter failed for struct " << m_type_->getName() << " aborting\n";
      return false;
    }
    if (!entry->getSerializer(Parent)) {
      llvm::errs() << "Get field serializer failed for struct " << m_type_->getName() << " aborting\n";
      return false;
    }
    if (!entry->getDeserializer(Parent)) {
      llvm::errs() << "Get field deserializer failed for struct " << m_type_->getName() << " aborting\n";
      return false;
    }
  }
  if (!getSerializer(Parent)) {
    llvm::errs() << "Get serializer failed for struct " << m_type_->getName() << " aborting\n";
    return false;
  }
  if (!getDeserializer(Parent)) {
    llvm::errs() << "Get deserializer failed for struct " << m_type_->getName() << " aborting\n";
    return false;
  }
  if (!getDestructor(Parent)) {
    llvm::errs() << "Get destructor failed for struct " << m_type_->getName() << " aborting\n";
    return false;
  }
  return true;
}
