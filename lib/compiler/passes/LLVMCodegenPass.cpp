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

#include "LLVMCodegenPass.hpp"

#include "IR.hpp"

#include "llvm/Support/Endian.h"

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

// Swap bytes to or from little endian (if it is little endian, then it's a
// no-op)
llvm::Value *swapBytes(llvm::Value *val, llvm::IRBuilder<> &builder) {
  llvm::Module *m = builder.GetInsertBlock()->getParent()->getParent();
  if (m->getDataLayout().isLittleEndian()) {
    return val;
  }

  llvm::Type *ValueType = val->getType();
  llvm::Type *PrevValueType = val->getType();

  if (ValueType->isIntOrIntVectorTy() &&
      ((ValueType->getIntegerBitWidth() % 8) != 0)) {
    return val;
  }

  if (!ValueType->isIntOrIntVectorTy()) {
    uint32_t TypeSize = m->getDataLayout().getTypeSizeInBits(ValueType);
    ValueType = builder.getIntNTy(TypeSize);
  }

  llvm::Function *bswap =
      llvm::Intrinsic::getDeclaration(m, llvm::Intrinsic::bswap, {ValueType});
  return builder.CreateBitCast(
      builder.CreateCall(bswap, {builder.CreateBitCast(val, ValueType)}),
      PrevValueType);
}

// Modifies the array in place, swaps all values to little endian
void swapArrayBytes(llvm::Value *ArrayPtr, llvm::Value *Count,
                    llvm::IRBuilder<> &builder) {
  llvm::Module *m = builder.GetInsertBlock()->getParent()->getParent();
  if (m->getDataLayout().isLittleEndian()) {
    return;
  }

  llvm::Type *ArrayType = ArrayPtr->getType()->getPointerElementType();
  llvm::Type *PrevArrayType = ArrayPtr->getType()->getPointerElementType();

  if (ArrayType->isIntOrIntVectorTy() &&
      ((ArrayType->getIntegerBitWidth() % 8) != 0)) {
    return;
  }

  if (!ArrayType->isIntOrIntVectorTy()) {
    uint32_t TypeSize = m->getDataLayout().getTypeSizeInBits(ArrayType);
    ArrayType = builder.getIntNTy(TypeSize);
  }

  llvm::Function *bswap =
      llvm::Intrinsic::getDeclaration(m, llvm::Intrinsic::bswap, {ArrayType});

  llvm::Function *ParentFunc = builder.GetInsertBlock()->getParent();
  llvm::BasicBlock *PrevBlock = builder.GetInsertBlock();

  llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(
      m->getContext(), "byteswap_loop_" + ParentFunc->getName(), ParentFunc);
  llvm::BasicBlock *nextBlock =
      llvm::BasicBlock::Create(m->getContext(), "", ParentFunc);
  builder.CreateBr(loopBlock);
  builder.SetInsertPoint(loopBlock);

  llvm::PHINode *loopIter = builder.CreatePHI(Count->getType(), 2);
  loopIter->addIncoming(
      builder.CreateBitCast(builder.getInt64(0), Count->getType()), PrevBlock);

  llvm::Value *ElementGEP = builder.CreateGEP(ArrayPtr, loopIter);
  llvm::Value *ArrayElement =
      builder.CreateBitCast(builder.CreateLoad(ElementGEP), ArrayType);
  llvm::Value *SwappedElement = builder.CreateBitCast(
      builder.CreateCall(bswap, {ArrayElement}), PrevArrayType);
  builder.CreateStore(SwappedElement, ElementGEP);

  llvm::Value *loopNextIter = builder.CreateAdd(
      builder.CreateBitCast(builder.getInt64(1), Count->getType()), loopIter);
  llvm::Value *loopEnd = builder.CreateICmpEQ(loopNextIter, Count);
  // If the next iteration is equal to the loop end then break out
  builder.CreateCondBr(loopEnd, nextBlock, loopBlock);
  loopIter->addIncoming(loopNextIter, loopBlock);

  builder.SetInsertPoint(nextBlock);
}
} // namespace

tyr::pass::LLVMCodegenPass::LLVMCodegenPass(llvm::Module *Parent)
    : m_parent_(Parent) {}

std::string tyr::pass::LLVMCodegenPass::getName() {
  return "LLVMCodegenPass";
}

bool tyr::pass::LLVMCodegenPass::runOnStruct(const tyr::ir::Struct &s) {
  if (!getConstructor(&s)) {
    llvm::errs() << "Get constructor failed for struct "
                 << s.getType()->getName() << " aborting\n";
    return false;
  }
  if (!getSerializer(&s)) {
    llvm::errs() << "Get serializer failed for struct "
                 << s.getType()->getName() << " aborting\n";
    return false;
  }
  if (!getDeserializer(&s)) {
    llvm::errs() << "Get deserializer failed for struct "
                 << s.getType()->getName() << " aborting\n";
    return false;
  }
  if (!getDestructor(&s)) {
    llvm::errs() << "Get destructor failed for struct "
                 << s.getType()->getName() << " aborting\n";
    return false;
  }
  return true;
}

bool tyr::pass::LLVMCodegenPass::runOnField(const tyr::ir::Field &f) {
  if (!getGetter(&f)) {
    llvm::errs() << "Get getter failed for field " << f.name << " aborting\n";
    return false;
  }
  if (!getItemGetter(&f)) {
    llvm::errs() << "Get item getter failed for field " << f.name
                 << " aborting\n";
    return false;
  }
  if (!getSetter(&f)) {
    llvm::errs() << "Get setter failed for field " << f.name << " aborting\n";
    return false;
  }
  if (!getItemSetter(&f)) {
    llvm::errs() << "Get setter failed for field " << f.name << " aborting\n";
    return false;
  }
  if (!getSerializer(&f)) {
    llvm::errs() << "Get field serializer failed for field " << f.name
                 << " aborting\n";
    return false;
  }
  if (!getDeserializer(&f)) {
    llvm::errs() << "Get field deserializer failed for field " << f.name
                 << " aborting\n";
    return false;
  }
  return true;
}

llvm::Value *
tyr::pass::LLVMCodegenPass::getFieldAllocSize(const tyr::ir::Field *f,
                                             llvm::Value *Struct,
                                             llvm::IRBuilder<> &builder) const {
  const llvm::DataLayout &DL = m_parent_->getDataLayout();

  if (f->type->isPointerTy() && !f->isStruct) { // array
    uint64_t FieldAllocSize =
        DL.getTypeAllocSize(f->type->getPointerElementType());
    llvm::Value *CountGEP =
        builder.CreateStructGEP(Struct, f->countField->offset);
    return builder.CreateMul(builder.getInt64(FieldAllocSize),
                             builder.CreateLoad(CountGEP));
  }

  return builder.getInt64(DL.getTypeAllocSize(f->type));
}

bool tyr::pass::LLVMCodegenPass::initField(const tyr::ir::Field *f,
                                          llvm::Value *Struct,
                                          llvm::Argument *Arg,
                                          llvm::IRBuilder<> &builder) {
  if (f->mut) {
    // Initialize to zero (still works even if it's a pointer)
    builder.CreateStore(
        builder.CreateTruncOrBitCast(builder.getInt64(0), f->type),
        builder.CreateStructGEP(Struct, f->offset));
  } else {
    if (Arg == nullptr) {
      llvm::errs() << "Arg was null on a field that is immutable (and "
                      "therefore needs an initializer), aborting\n";
      return false;
    }
    // Initialize to the value in the constructor
    if (f->type->isPointerTy()) {
      // Immutable repeated field, so need to alloc memory for it
      // Get the field alloc size
      llvm::Value *FieldAllocSize = getFieldAllocSize(f, Struct, builder);
      // Do the allocation
      llvm::Value *AllocdMem =
          builder.CreateCall(m_parent_->getFunction("malloc"), FieldAllocSize);
      llvm::Function *Constructor = builder.GetInsertBlock()->getParent();
      // Check that it succeeded
      llvm::BasicBlock *AllocSucceeded = insertNullCheck(
          {AllocdMem},
          builder.CreateIntToPtr(builder.getInt64(0), Struct->getType()),
          builder, Constructor);

      // Now we can do the memcpy
      builder.SetInsertPoint(AllocSucceeded);
      unsigned int FieldAlignment =
          m_parent_->getDataLayout().getABITypeAlignment(f->type);
      builder.CreateMemCpy(AllocdMem, FieldAlignment, Arg, FieldAlignment,
                           FieldAllocSize, false);
      builder.CreateStore(builder.CreateBitCast(AllocdMem, f->type),
                          builder.CreateStructGEP(Struct, f->offset));
    } else {
      builder.CreateStore(Arg, builder.CreateStructGEP(Struct, f->offset));
    }
  }

  return true;
}

bool tyr::pass::LLVMCodegenPass::destroyField(const tyr::ir::Field *f,
                                             llvm::Value *Struct,
                                             llvm::IRBuilder<> &builder) {
  llvm::Value *FieldGEP = builder.CreateStructGEP(Struct, f->offset);

  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  if (f->type->isPointerTy()) {
    builder.CreateCall(m_parent_->getFunction("free"),
                       builder.CreateBitCast(builder.CreateLoad(FieldGEP),
                                             builder.getInt8PtrTy(AddrSpace)));
  }

  return true;
}

bool tyr::pass::LLVMCodegenPass::getGetter(const ir::Field *f) const {
  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  llvm::Type *StructPtrType = f->parentType->getPointerTo(AddrSpace);

  // Get an alias to the context
  llvm::LLVMContext &ctx = m_parent_->getContext();

  std::string GetterName =
      "get_" + std::string(f->parentType->getName()) + "_" + f->name;
  // Getter returns bool, returns the thing by reference
  llvm::FunctionType *GetterType = llvm::FunctionType::get(
      llvm::Type::getInt1Ty(ctx),
      {StructPtrType, f->type->getPointerTo(AddrSpace)}, false);

  // Create the function
  llvm::Function *Getter = llvm::cast<llvm::Function>(
      m_parent_->getOrInsertFunction(GetterName, GetterType));
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
  llvm::Value *FieldGEP = builder.CreateStructGEP(Self, f->offset);
  llvm::Value *FieldLoad = builder.CreateLoad(FieldGEP);

  // Get the place where we're storing the result
  llvm::Value *OutVal = &*arg_iter;

  // If it's not mutable alloc a new thing and copy it over
  if (f->isStruct && !f->mut) {
    // The serializer is just "serialize_<name>"
    std::string FieldSerializerName =
        "serialize_" + std::string(f->type->getStructName());
    // Call it
    llvm::Value *SerializedField = builder.CreateCall(
        m_parent_->getFunction(FieldSerializerName), {FieldLoad});

    // Now we deserialize into the output pointer
    // The serializer is just "serialize_<name>"
    std::string FieldDeserializerName =
        "deserialize_" + std::string(f->type->getStructName());

    llvm::Value *DeserializedField = builder.CreateCall(
        m_parent_->getFunction(FieldDeserializerName), {SerializedField});
    builder.CreateStore(DeserializedField, OutVal);
    builder.CreateRet(builder.getInt1(true));
    return true;
  } else if (f->isRepeated && !f->mut) {
    llvm::Value *FieldAllocSize = getFieldAllocSize(f, Self, builder);

    llvm::Value *AllocdMem =
        builder.CreateCall(m_parent_->getFunction("malloc"), FieldAllocSize);
    llvm::BasicBlock *AllocSucceeded =
        insertNullCheck({AllocdMem}, builder.getInt1(false), builder, Getter);

    builder.SetInsertPoint(AllocSucceeded);

    unsigned int FieldAlignment =
        m_parent_->getDataLayout().getABITypeAlignment(f->type);
    builder.CreateMemCpy(AllocdMem, FieldAlignment, FieldLoad, FieldAlignment,
                         FieldAllocSize, false);
    builder.CreateStore(builder.CreateBitCast(AllocdMem, f->type), OutVal);
    builder.CreateRet(builder.getInt1(true));
    return true;
  }

  // It's mutable so we return by reference and that's it (if it's a pointer we
  // return the memory directly)
  builder.CreateStore(FieldLoad, OutVal);
  builder.CreateRet(builder.getInt1(true));

  return true;
}

bool tyr::pass::LLVMCodegenPass::getItemGetter(const tyr::ir::Field *f) const {
  if (!f->isRepeated) { // Not an array
    return true;
  }

  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  llvm::Type *StructPtrType = f->parentType->getPointerTo(AddrSpace);

  // Get an alias to the context
  llvm::LLVMContext &ctx = m_parent_->getContext();

  std::string GetterName =
      "get_" + std::string(f->parentType->getName()) + "_" + f->name + "_item";

  // Getter returns bool, takes an index, and returns the element by reference
  llvm::FunctionType *GetterType = llvm::FunctionType::get(
      llvm::Type::getInt1Ty(ctx),
      {StructPtrType, llvm::Type::getInt64Ty(ctx), f->type}, false);

  // Create the function
  llvm::Function *Getter = llvm::cast<llvm::Function>(
      m_parent_->getOrInsertFunction(GetterName, GetterType));
  llvm::BasicBlock *GetterBlock = llvm::BasicBlock::Create(ctx, "", Getter);
  llvm::IRBuilder<> builder(GetterBlock);

  // Get the first input to the function, the struct we're operating on
  auto arg_iter = Getter->arg_begin();
  llvm::Value *Self = &*arg_iter;
  ++arg_iter;
  llvm::Value *IDX = &*arg_iter;
  ++arg_iter;

  // Make sure it's not NULL
  llvm::BasicBlock *SelfIsNotNull = insertNullCheck(
      {Self, &*arg_iter}, builder.getInt1(false), builder, Getter);

  // Handle if it's not null
  builder.SetInsertPoint(SelfIsNotNull);
  llvm::Value *FieldGEP = builder.CreateStructGEP(Self, f->offset);
  llvm::Value *FieldLoad = builder.CreateLoad(FieldGEP);

  llvm::Value *CountGEP = builder.CreateStructGEP(Self, f->countField->offset);
  llvm::Value *Count = builder.CreateLoad(CountGEP);

  // Get the place where we're storing the result
  llvm::Value *OutVal = &*arg_iter;

  llvm::BasicBlock *OutOfBounds = llvm::BasicBlock::Create(ctx, "", Getter);
  llvm::BasicBlock *InBounds = llvm::BasicBlock::Create(ctx, "", Getter);

  // Check if IDX < Count
  llvm::Value *IsInBounds = builder.CreateICmpULT(IDX, Count);
  builder.CreateCondBr(IsInBounds, InBounds, OutOfBounds);

  // Handle out of bounds
  builder.SetInsertPoint(OutOfBounds);
  builder.CreateRet(builder.getInt1(false));

  // Handle in bounds
  builder.SetInsertPoint(InBounds);
  llvm::Value *ItemGEP = builder.CreateGEP(FieldLoad, IDX);
  llvm::Value *Item = builder.CreateLoad(ItemGEP);
  builder.CreateStore(Item, OutVal);
  builder.CreateRet(builder.getInt1(true));

  return true;
}

bool tyr::pass::LLVMCodegenPass::getSetter(const tyr::ir::Field *f) const {
  if (!f->mut) {
    return true;
  }

  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  llvm::Type *StructPtrType = f->parentType->getPointerTo(AddrSpace);

  // Get an alias to the context
  llvm::LLVMContext &ctx = m_parent_->getContext();

  std::string SetterName =
      "set_" + std::string(f->parentType->getName()) + "_" + f->name;
  // Setter returns a bool to indicate if it was successfully set (will return
  // false always if field is non-mutable)
  llvm::FunctionType *SetterType;
  if (f->isRepeated) {
    // If it's a pointer type then you have to pass in the size of the array
    // you're copying
    SetterType = llvm::FunctionType::get(
        llvm::Type::getInt1Ty(ctx),
        {StructPtrType, f->type, llvm::Type::getInt64Ty(ctx)}, false);
  } else {
    SetterType = llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx),
                                         {StructPtrType, f->type}, false);
  }

  // Create the function
  llvm::Function *Setter = llvm::cast<llvm::Function>(
      m_parent_->getOrInsertFunction(SetterName, SetterType));
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
  llvm::Value *FieldGEP = builder.CreateStructGEP(Self, f->offset);
  if (f->isStruct) {
    // If it's a struct we can just serialize the thing and then deserialize
    // into our field gep The serializer is just "serialize_<name>"
    std::string FieldSerializerName =
        "serialize_" + std::string(f->type->getStructName());
    // Call it
    llvm::Value *SerializedField = builder.CreateCall(
        m_parent_->getFunction(FieldSerializerName), {ToInsert});

    // Now we deserialize into the output pointer
    // The serializer is just "serialize_<name>"
    std::string FieldDeserializerName =
        "deserialize_" + std::string(f->type->getStructName());

    llvm::Value *DeserializedField = builder.CreateCall(
        m_parent_->getFunction(FieldDeserializerName), {SerializedField});
    builder.CreateStore(DeserializedField, FieldGEP);
    builder.CreateRet(builder.getInt1(true));
  } else if (f->isRepeated) {
    llvm::Value *PtrFieldCount =
        builder.CreateStructGEP(Self, f->countField->offset);

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
    llvm::Value *FieldAllocSize = getFieldAllocSize(f, Self, builder);
    // Do the allocation
    llvm::Value *AllocdMem =
        builder.CreateCall(m_parent_->getFunction("malloc"), FieldAllocSize);
    AllocdMem = builder.CreateBitCast(AllocdMem, f->type);
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
    FieldAllocSize = getFieldAllocSize(f, Self, builder);
    // And realloc
    llvm::Value *ReallocFieldMem = builder.CreateCall(
        m_parent_->getFunction("realloc"),
        {builder.CreateBitCast(LoadedField, builder.getInt8PtrTy(AddrSpace)),
         FieldAllocSize});
    ReallocFieldMem = builder.CreateBitCast(ReallocFieldMem, f->type);
    // Make sure realloc succeeded
    (void)insertNullCheck({ReallocFieldMem}, builder.getInt1(false), builder,
                          Setter, SizeIsRight);

    // Size is correct and everything seems fine
    builder.SetInsertPoint(SizeIsRight);
    llvm::PHINode *FinalFieldMem = builder.CreatePHI(f->type, 3);
    FinalFieldMem->addIncoming(AllocdMem, FieldIsNull);
    FinalFieldMem->addIncoming(LoadedField, CheckSize);
    FinalFieldMem->addIncoming(ReallocFieldMem, SizeIsWrong);

    // Store the correct count in the correct field
    builder.CreateStore(NumElts, PtrFieldCount);

    // Get the field alloc size now in case it was recalculated
    FieldAllocSize = getFieldAllocSize(f, Self, builder);
    // Have to align the memcpy to the original types
    unsigned int FieldAlignment =
        m_parent_->getDataLayout().getABITypeAlignment(f->type);
    builder.CreateMemCpy(FinalFieldMem, FieldAlignment, ToInsert,
                         FieldAlignment, FieldAllocSize, false);

    // Store the memory in the field
    builder.CreateStore(FinalFieldMem, FieldGEP);

    builder.CreateRet(builder.getInt1(true));
  } else if (f->isCount) {
    // Check if the input size is 0
    llvm::BasicBlock *NewSizeIsZero = llvm::BasicBlock::Create(ctx, "", Setter);
    llvm::BasicBlock *NewSizeIsNotZero = llvm::BasicBlock::Create(ctx, "", Setter);
    llvm::Value *InputSizeIsZero = builder.CreateICmpEQ(ToInsert, builder.CreateBitCast(builder.getInt64(0), f->type));
    builder.CreateCondBr(InputSizeIsZero, NewSizeIsZero, NewSizeIsNotZero);

    // And return false if it is
    builder.SetInsertPoint(NewSizeIsZero);
    builder.CreateRet(builder.getInt1(false));

    // Otherwise, continue
    builder.SetInsertPoint(NewSizeIsNotZero);

    // Store the new size
    llvm::Value *LoadedCount = builder.CreateLoad(FieldGEP);
    builder.CreateStore(ToInsert, FieldGEP);

    // Load the repeated field
    llvm::Value *RepeatedFieldGEP = builder.CreateStructGEP(Self, f->countsFor->offset);
    llvm::Value *LoadedRepeatedField = builder.CreateLoad(RepeatedFieldGEP);
    // Realloc to set the size
    llvm::Value *ReallocFieldMem = builder.CreateCall(
            m_parent_->getFunction("realloc"),
            {builder.CreateBitCast(LoadedRepeatedField, builder.getInt8PtrTy(AddrSpace)),
             getFieldAllocSize(f->countsFor, Self, builder)});
    // Replace the old size just in case
    builder.CreateStore(LoadedCount, FieldGEP);

    // Exit block
    llvm::BasicBlock *SizeIsRight = llvm::BasicBlock::Create(ctx, "", Setter);

    // Cast to the right type
    ReallocFieldMem = builder.CreateBitCast(ReallocFieldMem, f->countsFor->type);
    // Make sure realloc succeeded
    (void)insertNullCheck({ReallocFieldMem}, builder.getInt1(false), builder,
                          Setter, SizeIsRight);

    builder.SetInsertPoint(SizeIsRight);
    builder.CreateStore(ReallocFieldMem, RepeatedFieldGEP);
    builder.CreateStore(ToInsert, FieldGEP);
    builder.CreateRet(builder.getInt1(true));
  } else {
      builder.CreateStore(ToInsert, FieldGEP);
      builder.CreateRet(builder.getInt1(true));
  }

  return true;
}

bool tyr::pass::LLVMCodegenPass::getItemSetter(const tyr::ir::Field *f) const {
  if (!f->isRepeated) { // not an array
    return true;
  }

  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  llvm::Type *StructPtrType = f->parentType->getPointerTo(AddrSpace);

  // Get an alias to the context
  llvm::LLVMContext &ctx = m_parent_->getContext();

  std::string SetterName =
      "set_" + std::string(f->parentType->getName()) + "_" + f->name + "_item";

  // Setter returns bool, takes an index and the item to store
  llvm::FunctionType *SetterType =
      llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx),
                              {StructPtrType, llvm::Type::getInt64Ty(ctx),
                               f->type->getPointerElementType()},
                              false);

  // Create the function
  llvm::Function *Setter = llvm::cast<llvm::Function>(
      m_parent_->getOrInsertFunction(SetterName, SetterType));
  llvm::BasicBlock *SetterBlock = llvm::BasicBlock::Create(ctx, "", Setter);
  llvm::IRBuilder<> builder(SetterBlock);

  // Get the first input to the function, the struct we're operating on
  auto arg_iter = Setter->arg_begin();
  llvm::Value *Self = &*arg_iter;
  ++arg_iter;
  llvm::Value *IDX = &*arg_iter;
  ++arg_iter;

  // Make sure Self not NULL
  llvm::BasicBlock *SelfIsNotNull =
      insertNullCheck({Self}, builder.getInt1(false), builder, Setter);

  // Handle if it's not null
  builder.SetInsertPoint(SelfIsNotNull);
  llvm::Value *FieldGEP = builder.CreateStructGEP(Self, f->offset);
  llvm::Value *FieldLoad = builder.CreateLoad(FieldGEP);

  llvm::Value *CountGEP = builder.CreateStructGEP(Self, f->countField->offset);
  llvm::Value *Count = builder.CreateLoad(CountGEP);

  llvm::BasicBlock *OutOfBounds = llvm::BasicBlock::Create(ctx, "", Setter);
  llvm::BasicBlock *InBounds = llvm::BasicBlock::Create(ctx, "", Setter);

  // Check if IDX < Count
  llvm::Value *IsInBounds = builder.CreateICmpULT(IDX, Count);
  builder.CreateCondBr(IsInBounds, InBounds, OutOfBounds);

  // Handle out of bounds
  builder.SetInsertPoint(OutOfBounds);
  builder.CreateRet(builder.getInt1(false));

  // Handle in bounds
  // Get the thing we want to set
  llvm::Value *ValToSet = &*arg_iter;

  builder.SetInsertPoint(InBounds);
  llvm::Value *ItemGEP = builder.CreateGEP(FieldLoad, IDX);
  builder.CreateStore(ValToSet, ItemGEP);
  builder.CreateRet(builder.getInt1(true));

  return true;
}

std::string
tyr::pass::LLVMCodegenPass::getSerializerName(const tyr::ir::Field *f) const {
  return "__serialize_" + std::string(f->parentType->getName()) + "_" + f->name;
}

std::string
tyr::pass::LLVMCodegenPass::getDeserializerName(const tyr::ir::Field *f) const {
  return "__deserialize_" + std::string(f->parentType->getName()) + "_" +
         f->name;
}

bool tyr::pass::LLVMCodegenPass::getSerializer(const tyr::ir::Field *f) const {
  if (f->isCount) { // Don't serialize count fields
    return true;
  }

  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  llvm::LLVMContext &ctx = m_parent_->getContext();

  std::string Name = getSerializerName(f);

  // This function is meant for internal use only, it serializes the field into
  // the second arg It returns the number of bytes written
  llvm::FunctionType *SerializerType =
      llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx),
                              {f->parentType->getPointerTo(AddrSpace),
                               llvm::Type::getInt8PtrTy(ctx, AddrSpace)},
                              false);

  // Create the function
  llvm::Function *Serializer = llvm::cast<llvm::Function>(
      m_parent_->getOrInsertFunction(Name, SerializerType));
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

  // Get the field data
  llvm::Value *FieldData =
      builder.CreateLoad(builder.CreateStructGEP(Self, f->offset));

  llvm::Value *CurrentPtr = builder.CreateGEP(OutBuf, builder.getInt64(0));
  llvm::Value *OutSize;
  if (f->isStruct) {
    // The serializer is just "serialize_<name>"
    std::string FieldSerializerName =
        "serialize_" + std::string(f->type->getStructName());
    // Call it
    llvm::Value *SerializedField = builder.CreateCall(
        m_parent_->getFunction(FieldSerializerName), {FieldData});
    // Set the size of the thing (includes the size for the int header)
    OutSize = builder.CreateLoad(builder.CreateBitCast(
        SerializedField, builder.getInt64Ty()->getPointerTo(AddrSpace)));
    // Copy the memory over - alignment is 1 because it's already uint8
    builder.CreateMemCpy(CurrentPtr, 0, SerializedField, 0, OutSize);
    // Free the allocated buffer
    builder.CreateCall(m_parent_->getFunction("free"), {SerializedField});
  } else if (f->type->isPointerTy()) {
    // Get the count and store it first
    llvm::Value *Count = builder.CreateLoad(
        builder.CreateStructGEP(Self, f->countField->offset));

    // Swap the bytes in count if necessary
    llvm::Value *SwappedCount = swapBytes(Count, builder);
    llvm::Value *CastedCurrentPtr = builder.CreateBitCast(
        CurrentPtr, f->countField->type->getPointerTo(AddrSpace));
    builder.CreateStore(SwappedCount, CastedCurrentPtr);
    // Set the output size to the size of the count field
    OutSize = getFieldAllocSize(f->countField, Self, builder);

    // Increment CurrentPtr so we can store the data
    CurrentPtr = builder.CreateGEP(OutBuf, OutSize);

    llvm::Value *PtrFieldAllocSize = getFieldAllocSize(f, Self, builder);
    unsigned int FieldAlignment =
        m_parent_->getDataLayout().getABITypeAlignment(f->type);
    builder.CreateMemCpy(CurrentPtr, 0, FieldData, FieldAlignment,
                         PtrFieldAllocSize);

    // Swap bytes in the CurrentPtr if necessary
    CastedCurrentPtr = builder.CreateBitCast(CurrentPtr, FieldData->getType());
    swapArrayBytes(CastedCurrentPtr, Count, builder);
    // Increment the OutSize by the size of the pointer field
    OutSize = builder.CreateAdd(OutSize, PtrFieldAllocSize);
  } else {
    // Just store the data
    llvm::Value *SwappedData = swapBytes(FieldData, builder);
    OutSize = getFieldAllocSize(f, Self, builder);
    llvm::Value *CastedCurrentPtr =
        builder.CreateBitCast(CurrentPtr, f->type->getPointerTo(AddrSpace));
    builder.CreateStore(SwappedData, CastedCurrentPtr);
  }

  builder.CreateRet(OutSize);

  return true;
}

bool tyr::pass::LLVMCodegenPass::getDeserializer(const tyr::ir::Field *f) const {
  if (f->isCount) { // Don't serialize count fields
    return true;
  }

  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  llvm::LLVMContext &ctx = m_parent_->getContext();

  std::string Name = getDeserializerName(f);

  // This function is meant for internal use only, it deserializes the field
  // from the second arg It returns the number of bytes read
  llvm::FunctionType *DeserializerType =
      llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx),
                              {f->parentType->getPointerTo(AddrSpace),
                               llvm::Type::getInt8PtrTy(ctx, AddrSpace)},
                              false);

  // Create the function
  llvm::Function *Deserializer = llvm::cast<llvm::Function>(
      m_parent_->getOrInsertFunction(Name, DeserializerType));
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
  if (f->isStruct) {
    // The serializer is just "deserialize_<name>"
    std::string FieldDeserializerName =
        "deserialize_" + std::string(f->type->getStructName());
    // Call it
    llvm::Value *Field = builder.CreateCall(
        m_parent_->getFunction(FieldDeserializerName), {CurrentPtr});
    // Now we have it, so we can just store it
    builder.CreateStore(Field, builder.CreateStructGEP(Self, f->offset));
    // Set the size of the thing
    OutSize = builder.CreateLoad(builder.CreateBitCast(
        CurrentPtr, builder.getInt64Ty()->getPointerTo(AddrSpace)));
  } else if (f->type->isPointerTy()) {
    // Get the count first
    llvm::Value *CastedCurrentPtr = builder.CreateBitCast(
        CurrentPtr, f->countField->type->getPointerTo(AddrSpace));
    llvm::Value *Count =
        swapBytes(builder.CreateLoad(CastedCurrentPtr), builder);
    // Store the count into Self
    builder.CreateStore(Count,
                        builder.CreateStructGEP(Self, f->countField->offset));
    OutSize = getFieldAllocSize(f->countField, Self, builder);

    // Increment CurrentPtr so we can read the data
    CurrentPtr = builder.CreateGEP(InBuf, OutSize);

    // Now get the data (alloc space)
    CastedCurrentPtr =
        builder.CreateBitCast(CurrentPtr, f->type->getPointerTo(AddrSpace));
    llvm::Value *PtrFieldAllocSize = getFieldAllocSize(f, Self, builder);
    llvm::Value *FieldMem =
        builder.CreateCall(m_parent_->getFunction("malloc"), PtrFieldAllocSize);
    llvm::BasicBlock *MallocSucceeded =
        insertNullCheck({FieldMem}, builder.getInt64(0), builder, Deserializer);

    // Malloc succeeded, so now handle it
    builder.SetInsertPoint(MallocSucceeded);
    // Do the copy
    unsigned int FieldAlignment =
        m_parent_->getDataLayout().getABITypeAlignment(f->type);
    builder.CreateMemCpy(FieldMem, FieldAlignment, CastedCurrentPtr, 0,
                         PtrFieldAllocSize);

    llvm::Value *CastedFieldMem = builder.CreateBitCast(FieldMem, f->type);

    // Swap the bytes in the array if necessary
    swapArrayBytes(CastedFieldMem, Count, builder);

    // And store it into Self
    builder.CreateStore(CastedFieldMem,
                        builder.CreateStructGEP(Self, f->offset));

    OutSize = builder.CreateAdd(OutSize, PtrFieldAllocSize);
  } else {
    // Just store the data
    llvm::Value *CastedCurrentPtr =
        builder.CreateBitCast(CurrentPtr, f->type->getPointerTo(AddrSpace));
    llvm::Value *FieldData =
        swapBytes(builder.CreateLoad(CastedCurrentPtr), builder);
    OutSize = getFieldAllocSize(f, Self, builder);
    builder.CreateStore(FieldData, builder.CreateStructGEP(Self, f->offset));
  }

  builder.CreateRet(OutSize);

  return true;
}

uint64_t
tyr::pass::LLVMCodegenPass::getStructAllocSize(const tyr::ir::Struct *s) {
  const llvm::DataLayout &DL = m_parent_->getDataLayout();
  return DL.getTypeAllocSize(s->getType());
}

bool tyr::pass::LLVMCodegenPass::getConstructor(const tyr::ir::Struct *s) {
  llvm::ArrayRef<ir::FieldPtr> structFields = s->getFields();

  std::vector<llvm::Type *> NonMutFields;
  for (auto &entry : structFields) {
    if (!entry->mut) {
      NonMutFields.push_back(entry->type);
    }
  }

  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  llvm::LLVMContext &ctx = m_parent_->getContext();

  std::string ConstrName = "create_" + s->getName();

  llvm::StructType *GenStructType = s->getType();
  llvm::Type *StructPtrType = GenStructType->getPointerTo(AddrSpace);

  // constructor has parameters for all of the non-mutable fields
  llvm::FunctionType *ConstructorType =
      llvm::FunctionType::get(StructPtrType, NonMutFields, false);
  // Create the function
  llvm::Function *Constructor = llvm::cast<llvm::Function>(
      m_parent_->getOrInsertFunction(ConstrName, ConstructorType));
  llvm::BasicBlock *EntryBlock = llvm::BasicBlock::Create(ctx, "", Constructor);
  llvm::IRBuilder<> builder(EntryBlock);

  // Allocate space for the output
  llvm::Value *StructOutRaw =
      builder.CreateCall(m_parent_->getFunction("malloc"),
                         builder.getInt64(getStructAllocSize(s)));
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
  for (auto &entry : structFields) {
    if (!entry->mut) {
      initField(entry.get(), StructOut, &*ArgIter, builder);
      // Only increment the argument iterator if it's not a mutable field
      // otherwise, the field is initialized to zero
      ++ArgIter;
    } else {
      initField(entry.get(), StructOut, nullptr, builder);
    }
  }

  // Return the created object
  builder.CreateRet(StructOut);

  return true;
}

bool tyr::pass::LLVMCodegenPass::getDestructor(const tyr::ir::Struct *s) {
  llvm::ArrayRef<ir::FieldPtr> structFields = s->getFields();
  llvm::LLVMContext &ctx = m_parent_->getContext();

  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  std::string DestrName = "destroy_" + s->getName();

  llvm::StructType *GenStructType = s->getType();
  llvm::Type *StructPtrType = GenStructType->getPointerTo(AddrSpace);

  // constructor has parameters for all of the non-mutable fields
  llvm::FunctionType *DestructorType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(ctx), {StructPtrType}, false);

  llvm::Function *Destructor = llvm::cast<llvm::Function>(
      m_parent_->getOrInsertFunction(DestrName, DestructorType));
  llvm::BasicBlock *EntryBlock = llvm::BasicBlock::Create(ctx, "", Destructor);
  llvm::IRBuilder<> builder(EntryBlock);

  // Destroy all the fields
  llvm::Value *Struct = &*Destructor->arg_begin();
  for (auto &entry : structFields) {
    destroyField(entry.get(), Struct, builder);
  }

  builder.CreateCall(
      m_parent_->getFunction("free"),
      builder.CreateBitCast(Struct, builder.getInt8PtrTy(AddrSpace)));
  builder.CreateRetVoid();

  return true;
}

bool tyr::pass::LLVMCodegenPass::getSerializer(const tyr::ir::Struct *s) {
  llvm::ArrayRef<ir::FieldPtr> structFields = s->getFields();
  llvm::LLVMContext &ctx = m_parent_->getContext();

  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  std::string Name = "serialize_" + s->getName();

  llvm::StructType *GenStructType = s->getType();
  llvm::Type *StructPtrType = GenStructType->getPointerTo(AddrSpace);

  // constructor has parameters for all of the non-mutable fields
  llvm::FunctionType *SerializerType = llvm::FunctionType::get(
      llvm::Type::getInt8PtrTy(ctx, AddrSpace), {StructPtrType}, false);

  // Create the function
  llvm::Function *Serializer = llvm::cast<llvm::Function>(
      m_parent_->getOrInsertFunction(Name, SerializerType));
  llvm::BasicBlock *EntryBlock = llvm::BasicBlock::Create(ctx, "", Serializer);
  llvm::IRBuilder<> builder(EntryBlock);

  auto arg_iter = Serializer->arg_begin();
  llvm::Value *Self = &*arg_iter;
  llvm::cast<llvm::Argument>(Self)->addAttr(
      llvm::Attribute::AttrKind::ReadOnly);

  // Check if the args are null
  llvm::BasicBlock *IsNotNull = insertNullCheck(
      {Self}, llvm::ConstantPointerNull::get(builder.getInt8PtrTy(AddrSpace)),
      builder, Serializer);

  // Not null, we can continue
  builder.SetInsertPoint(IsNotNull);

  // Start out with enough space for an int64
  llvm::Value *AllocSize = builder.getInt64(8);
  // add up the output memory
  for (auto &entry : structFields) {
    AllocSize = builder.CreateAdd(
        AllocSize, getFieldAllocSize(entry.get(), Self, builder));
  }

  llvm::Value *AllocdMem =
      builder.CreateCall(m_parent_->getFunction("malloc"), AllocSize);

  llvm::BasicBlock *MallocSucceeded = insertNullCheck(
      {AllocdMem},
      llvm::ConstantPointerNull::get(builder.getInt8PtrTy(AddrSpace)), builder,
      Serializer);
  builder.SetInsertPoint(MallocSucceeded);

  // Store the total size of the struct, swap the bytes in the total size if
  // necessary
  builder.CreateStore(
      swapBytes(AllocSize, builder),
      builder.CreateBitCast(AllocdMem,
                            builder.getInt64Ty()->getPointerTo(AddrSpace)));

  llvm::Value *CurrentIDX = builder.getInt64(8);
  for (auto &entry : structFields) {
    if (entry->isCount) { // count fields are handled already
      continue;
    }
    llvm::Function *EntrySerializer =
        m_parent_->getFunction(getSerializerName(entry.get()));
    llvm::Value *CurrentPtr = builder.CreateGEP(AllocdMem, CurrentIDX);
    llvm::Value *OutSize =
        builder.CreateCall(EntrySerializer, {Self, CurrentPtr});
    CurrentIDX = builder.CreateAdd(CurrentIDX, OutSize);
  }

  builder.CreateRet(AllocdMem);

  return true;
}

bool tyr::pass::LLVMCodegenPass::getDeserializer(const tyr::ir::Struct *s) {
  llvm::ArrayRef<ir::FieldPtr> structFields = s->getFields();
  llvm::LLVMContext &ctx = m_parent_->getContext();

  const uint32_t AddrSpace =
      m_parent_->getDataLayout().getProgramAddressSpace();

  std::string Name = "deserialize_" + s->getName();

  llvm::StructType *GenStructType = s->getType();
  llvm::PointerType *StructPtrType = GenStructType->getPointerTo(AddrSpace);

  // constructor has parameters for all of the non-mutable fields
  llvm::FunctionType *DeserializerType = llvm::FunctionType::get(
      StructPtrType, {llvm::Type::getInt8PtrTy(ctx, AddrSpace)}, false);

  llvm::Function *Deserializer = llvm::cast<llvm::Function>(
      m_parent_->getOrInsertFunction(Name, DeserializerType));
  llvm::BasicBlock *EntryBlock =
      llvm::BasicBlock::Create(ctx, "", Deserializer);
  llvm::IRBuilder<> builder(EntryBlock);

  // read in the byte array and its length
  auto arg_iter = Deserializer->arg_begin();
  llvm::Value *SerializedSelf = &*arg_iter;
  llvm::cast<llvm::Argument>(SerializedSelf)
      ->addAttr(llvm::Attribute::AttrKind::ReadOnly);

  // Swap the bytes in the total size if necessary
  llvm::Value *SerializedSize = swapBytes(
      builder.CreateLoad(builder.CreateBitCast(
          SerializedSelf, builder.getInt64Ty()->getPointerTo(AddrSpace))),
      builder);

  // Check if the args are invalid
  llvm::BasicBlock *IsNotNull = insertNullCheck(
      {SerializedSelf}, llvm::ConstantPointerNull::get(StructPtrType), builder,
      Deserializer);

  builder.SetInsertPoint(IsNotNull);

  // allocate a new thing
  const llvm::DataLayout &DL = m_parent_->getDataLayout();
  llvm::Value *StructAllocSize =
      builder.getInt64(DL.getTypeAllocSize(GenStructType));

  llvm::Value *StructOutRaw =
      builder.CreateCall(m_parent_->getFunction("malloc"), StructAllocSize);

  // Make sure malloc succeeded
  llvm::BasicBlock *MallocSucceeded = insertNullCheck(
      {StructOutRaw}, llvm::ConstantPointerNull::get(StructPtrType), builder,
      Deserializer);

  builder.SetInsertPoint(MallocSucceeded);
  llvm::Value *StructOut =
      builder.CreatePointerCast(StructOutRaw, StructPtrType);

  // Now set all the fields
  // We start at 8 because we already loaded the serialized size
  llvm::Value *CurrentIDX = builder.getInt64(8);
  for (auto &entry : structFields) {
    if (entry->isCount) { // count fields are handled already
      continue;
    }
    llvm::Function *EntryDeserializer =
        m_parent_->getFunction(getDeserializerName(entry.get()));
    llvm::Value *OutSize = builder.CreateCall(
        EntryDeserializer,
        {StructOut, builder.CreateGEP(SerializedSelf, CurrentIDX)});
    CurrentIDX = builder.CreateAdd(CurrentIDX, OutSize);
  }

  // Check that the size of everything is OK
  // Start out with enough space for an int64
  llvm::Value *AllocSize = builder.getInt64(8);
  // add up the output memory
  for (auto &entry : structFields) {
    AllocSize = builder.CreateAdd(
        AllocSize, getFieldAllocSize(entry.get(), StructOut, builder));
  }

  // Make sure the serialized size matches (we have to throw it all away
  // otherwise)
  llvm::Value *CheckSize = builder.CreateICmpEQ(SerializedSize, AllocSize);
  llvm::BasicBlock *SizeIsRight =
      llvm::BasicBlock::Create(ctx, "", Deserializer);
  llvm::BasicBlock *SizeIsWrong =
      llvm::BasicBlock::Create(ctx, "", Deserializer);
  builder.CreateCondBr(CheckSize, SizeIsRight, SizeIsWrong);

  // If it's wrong, return NULL
  builder.SetInsertPoint(SizeIsWrong);
  builder.CreateRet(llvm::ConstantPointerNull::get(StructPtrType));

  // We're OK, so return the thing
  builder.SetInsertPoint(SizeIsRight);
  builder.CreateRet(StructOut);

  return true;
}
