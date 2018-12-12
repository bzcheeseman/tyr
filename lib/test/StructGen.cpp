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

#include <gtest/gtest.h>

#include <llvm/IR/Verifier.h>

// For JIT
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

namespace {

TEST(StructGen, verif_correct) {
  tyr::StructGen sg{"test", false};

  llvm::LLVMContext ctx;

  EXPECT_TRUE(sg.addField("float", llvm::Type::getFloatTy(ctx), true));
  EXPECT_TRUE(sg.addField(
      "int16", llvm::cast<llvm::Type>(llvm::Type::getInt16Ty(ctx)), false));
  EXPECT_TRUE(sg.addField(
      "ptr", llvm::cast<llvm::Type>(llvm::Type::getInt8PtrTy(ctx)), true));
  auto Parent = llvm::make_unique<llvm::Module>("test_module", ctx);

  sg.finalizeFields(Parent.get());

  Parent->getOrInsertFunction("malloc", llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt64Ty(ctx));
  Parent->getOrInsertFunction("realloc", llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt64Ty(ctx));
  Parent->getOrInsertFunction("free", llvm::Type::getVoidTy(ctx),
                              llvm::Type::getInt8PtrTy(ctx));

  sg.populateModule(Parent.get());
  //  Parent->print(llvm::outs(), nullptr);
  EXPECT_FALSE(llvm::verifyModule(*Parent, &llvm::errs()));
}

TEST(StructGen, code_correct) {
  tyr::StructGen sg{"test", false};

  llvm::LLVMContext ctx;

  sg.addField("float", llvm::Type::getFloatTy(ctx), true);
  sg.addField("int16", llvm::cast<llvm::Type>(llvm::Type::getInt16Ty(ctx)),
              false);
  sg.addField("ptr", llvm::cast<llvm::Type>(llvm::Type::getInt32PtrTy(ctx)),
              true);
  auto Parent = llvm::make_unique<llvm::Module>("test_module", ctx);

  sg.finalizeFields(Parent.get());

  Parent->getOrInsertFunction("malloc", llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt64Ty(ctx));
  Parent->getOrInsertFunction("realloc", llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt8PtrTy(ctx),
                              llvm::Type::getInt64Ty(ctx));
  Parent->getOrInsertFunction("free", llvm::Type::getVoidTy(ctx),
                              llvm::Type::getInt8PtrTy(ctx));
  Parent->getOrInsertFunction(
      "memcpy", llvm::Type::getInt8PtrTy(ctx), llvm::Type::getInt8PtrTy(ctx),
      llvm::Type::getInt8PtrTy(ctx), llvm::Type::getInt64Ty(ctx));

  sg.populateModule(Parent.get());

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  Parent->setTargetTriple(llvm::sys::getDefaultTargetTriple());

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(
      llvm::sys::getDefaultTargetTriple(), error);

  llvm::TargetOptions options;
  auto RM = llvm::Optional<llvm::Reloc::Model>();

  llvm::TargetMachine *target_machine =
      target->createTargetMachine(llvm::sys::getDefaultTargetTriple(),
                                  llvm::sys::getHostCPUName(), "", options, RM);

  Parent->setDataLayout(target_machine->createDataLayout());
  Parent->setTargetTriple(llvm::sys::getDefaultTargetTriple());

  llvm::verifyModule(*Parent, &llvm::errs());

  llvm::legacy::PassManager pm;
  llvm::PassManagerBuilder PMBuilder;

  PMBuilder.OptLevel = 3;

  target_machine->adjustPassManager(PMBuilder);
  PMBuilder.populateModulePassManager(pm);

  pm.run(*Parent);

  EXPECT_FALSE(llvm::verifyModule(*Parent, &llvm::errs()));

  Parent->print(llvm::outs(), nullptr);

  std::string error_str;

  llvm::EngineBuilder engineBuilder(std::move(Parent));
  engineBuilder.setErrorStr(&error_str);
  engineBuilder.setEngineKind(llvm::EngineKind::JIT);
  llvm::ExecutionEngine *engine = engineBuilder.create();

  auto constructor =
      (void *(*)(uint16_t))engine->getFunctionAddress("create_test");
  auto get_int = (bool (*)(void *, uint16_t *))engine->getFunctionAddress(
      "get_test_int16");
  auto get_float =
      (bool (*)(void *, float *))engine->getFunctionAddress("get_test_float");
  auto set_float =
      (bool (*)(void *, float))engine->getFunctionAddress("set_test_float");
  auto getter =
      (bool (*)(void *, uint32_t **))engine->getFunctionAddress("get_test_ptr");
  auto count_getter = (bool (*)(void *, uint64_t *))engine->getFunctionAddress(
      "get_test_ptr_count");
  auto setter = (bool (*)(void *, uint32_t *,
                          uint64_t))engine->getFunctionAddress("set_test_ptr");
  auto destructor =
      (void (*)(void *))engine->getFunctionAddress("destroy_test");

  auto serializer = (uint8_t * (*)(void *, uint64_t *))
                        engine->getFunctionAddress("serialize_test");
  auto deserializer =
      (void *(*)(uint8_t *, uint64_t))engine->getFunctionAddress(
          "deserialize_test");

  uint32_t *test_data = (uint32_t *)calloc(35, sizeof(uint32_t));
  for (int i = 0; i < 35; ++i) {
    test_data[i] = (uint32_t)rand();
  }
  uint32_t *test_out_data = nullptr;
  uint16_t test_out_int16 = 0;
  float test_out_float = 0;
  uint64_t test_out_data_count = 0;

  void *test_struct = constructor(5);

  EXPECT_TRUE(setter(test_struct, test_data, 35));
  EXPECT_TRUE(set_float(test_struct, 3.14159265));
  EXPECT_TRUE(getter(test_struct, &test_out_data));
  EXPECT_TRUE(count_getter(test_struct, &test_out_data_count));
  EXPECT_TRUE(get_int(test_struct, &test_out_int16));
  EXPECT_TRUE(get_float(test_struct, &test_out_float));

  EXPECT_EQ(test_out_int16, 5);
  EXPECT_FLOAT_EQ(test_out_float, 3.14159265);

  for (int i = 0; i < test_out_data_count; ++i) {
    EXPECT_EQ(test_data[i], test_out_data[i]);
  }

  uint64_t serialized_size = 0;
  uint8_t *serialized = serializer(test_struct, &serialized_size);

  void *deserialized_struct = deserializer(serialized, serialized_size);
  EXPECT_TRUE(deserialized_struct != nullptr);

  uint32_t *deserialized_data = nullptr;
  uint16_t deserialized_int16 = 0;
  float deserialized_float = 0;
  uint64_t deserialized_data_count = 0;

  EXPECT_TRUE(getter(deserialized_struct, &deserialized_data));
  EXPECT_TRUE(count_getter(deserialized_struct, &deserialized_data_count));
  EXPECT_TRUE(get_int(deserialized_struct, &deserialized_int16));
  EXPECT_TRUE(get_float(deserialized_struct, &deserialized_float));

  EXPECT_EQ(deserialized_int16, 5);
  EXPECT_FLOAT_EQ(deserialized_float, 3.14159265);
  EXPECT_EQ(deserialized_data_count, 35);

  for (int i = 0; i < deserialized_data_count; ++i) {
    EXPECT_EQ(deserialized_data[i], test_out_data[i]);
  }

  destructor(deserialized_struct);
  destructor(test_struct);
  free(test_data);
  free(serialized);
}

} // namespace
