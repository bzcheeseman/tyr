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

#include "BindGen.hpp"
#include "Generators/C.hpp"
#include "Generators/Python.hpp"

#include <gtest/gtest.h>

#include <llvm/IR/Verifier.h>

namespace {

// TODO: fix up these tests
TEST(BindGen, C) {
  tyr::StructGen sg1{"test1", false};
  tyr::StructGen sg2{"test2", false};

  llvm::LLVMContext ctx;

  EXPECT_TRUE(sg1.addField("float", llvm::Type::getFloatTy(ctx), true));
  EXPECT_TRUE(sg2.addField(
      "int16", llvm::cast<llvm::Type>(llvm::Type::getInt16Ty(ctx)), false));
  EXPECT_TRUE(sg1.addField(
      "ptr", llvm::cast<llvm::Type>(llvm::Type::getInt8PtrTy(ctx)), true));

  auto Parent = llvm::make_unique<llvm::Module>("test_module", ctx);

  sg1.finalizeFields(Parent.get());
  sg2.finalizeFields(Parent.get());

  Parent->getOrInsertFunction("malloc", llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt64Ty(ctx));
  Parent->getOrInsertFunction("realloc", llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt64Ty(ctx));
  Parent->getOrInsertFunction("free", llvm::Type::getVoidTy(ctx),
                              llvm::Type::getInt8PtrTy(ctx));

  sg1.populateModule(Parent.get());
  sg2.populateModule(Parent.get());
  EXPECT_FALSE(llvm::verifyModule(*Parent, &llvm::errs()));

  tyr::Binding b;
  tyr::C generator{};
  b.setGenerator(&generator);
  b.setModule(Parent.get());

  llvm::errs() << b;
}

TEST(BindGen, Python) {
  tyr::StructGen sg1{"test1", false};
  tyr::StructGen sg2{"test2", false};

  llvm::LLVMContext ctx;

  EXPECT_TRUE(sg1.addField("float", llvm::Type::getFloatTy(ctx), true));
  EXPECT_TRUE(sg2.addField(
      "int16", llvm::cast<llvm::Type>(llvm::Type::getInt16Ty(ctx)), false));
  EXPECT_TRUE(sg1.addField(
      "ptr", llvm::cast<llvm::Type>(llvm::Type::getInt8PtrTy(ctx)), true));

  auto Parent = llvm::make_unique<llvm::Module>("test_module", ctx);

  sg1.finalizeFields(Parent.get());
  sg2.finalizeFields(Parent.get());

  Parent->getOrInsertFunction("malloc", llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt64Ty(ctx));
  Parent->getOrInsertFunction("realloc", llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt64Ty(ctx));
  Parent->getOrInsertFunction("free", llvm::Type::getVoidTy(ctx),
                              llvm::Type::getInt8PtrTy(ctx));

  sg1.populateModule(Parent.get());
  sg2.populateModule(Parent.get());
  //    Parent->print(llvm::outs(), nullptr);
  EXPECT_FALSE(llvm::verifyModule(*Parent, &llvm::errs()));

  tyr::Binding b;
  tyr::Python generator{};
  b.setGenerator(&generator);
  b.setModule(Parent.get());

  llvm::errs() << b;
}

TEST(BindGen, Go) {
  tyr::StructGen sg1{"test1", false};
  tyr::StructGen sg2{"test2", false};

  llvm::LLVMContext ctx;

  EXPECT_TRUE(sg1.addField("float", llvm::Type::getFloatTy(ctx), true));
  EXPECT_TRUE(sg2.addField(
      "int16", llvm::cast<llvm::Type>(llvm::Type::getInt16Ty(ctx)), false));
  EXPECT_TRUE(sg1.addField(
      "ptr", llvm::cast<llvm::Type>(llvm::Type::getInt8PtrTy(ctx)), true));

  auto Parent = llvm::make_unique<llvm::Module>("test_module", ctx);

  sg1.finalizeFields(Parent.get());
  sg2.finalizeFields(Parent.get());

  Parent->getOrInsertFunction("malloc", llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt64Ty(ctx));
  Parent->getOrInsertFunction("realloc", llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt64Ty(ctx));
  Parent->getOrInsertFunction("free", llvm::Type::getVoidTy(ctx),
                              llvm::Type::getInt8PtrTy(ctx));

  sg1.populateModule(Parent.get());
  sg2.populateModule(Parent.get());
  //    Parent->print(llvm::outs(), nullptr);
  EXPECT_FALSE(llvm::verifyModule(*Parent, &llvm::errs()));

  tyr::Binding b;
  tyr::Go generator{};
  b.setGenerator(&generator);
  b.setModule(Parent.get());

  llvm::errs() << b;
}

} // namespace
