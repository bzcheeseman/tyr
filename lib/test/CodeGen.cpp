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
#include "CodeGen.hpp"

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

TEST(CodeGen, verif_correct) {
  tyr::CodeGen cg{"test", llvm::sys::getDefaultTargetTriple(),
                  llvm::sys::getHostCPUName(), ""};

  cg.newStruct("testStruct", false);

  EXPECT_TRUE(cg.addField("testStruct", true, false, "float", "floatField"));
  EXPECT_TRUE(cg.addField("testStruct", true, false, "int16", "intField"));
  EXPECT_TRUE(cg.addField("testStruct", true, true, "int8", "arrayField"));

  cg.finalizeStruct("testStruct");

  EXPECT_FALSE(cg.verifyModule(llvm::errs()));
}

TEST(CodeGen, verif_correct_be) {
  tyr::CodeGen cg{"test", "aarch64_be-unknown-unknown", "", ""};

  cg.newStruct("testStruct", false);

  EXPECT_TRUE(cg.addField("testStruct", true, false, "float", "floatField"));
  EXPECT_TRUE(cg.addField("testStruct", true, false, "int16", "intField"));
  EXPECT_TRUE(cg.addField("testStruct", true, true, "int8", "arrayField"));

  cg.finalizeStruct("testStruct");

  EXPECT_FALSE(cg.verifyModule(llvm::errs()));
}

TEST(CodeGen, code_correct) {
  tyr::CodeGen cg{"test_module", llvm::sys::getDefaultTargetTriple(),
                  llvm::sys::getHostCPUName(), ""};

  cg.newStruct("test", true);

  EXPECT_TRUE(cg.addField("test", true, false, "float", "float"));
  EXPECT_TRUE(cg.addField("test", false, false, "int16", "int16"));
  EXPECT_TRUE(cg.addField("test", true, true, "int32", "ptr"));

  cg.finalizeStruct("test");
  EXPECT_FALSE(cg.verifyModule(llvm::errs()));

  llvm::ExecutionEngine *engine = cg.getExecutionEngine();
  EXPECT_TRUE(engine != nullptr);

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
  auto item_getter =
      (bool (*)(void *, uint64_t, uint32_t *))engine->getFunctionAddress(
          "get_test_ptr_item");
  auto count_getter = (bool (*)(void *, uint64_t *))engine->getFunctionAddress(
      "get_test_ptr_count");
  auto count_setter = (bool (*)(void *, uint64_t))engine->getFunctionAddress(
          "set_test_ptr_count");
  auto setter = (bool (*)(void *, uint32_t *,
                          uint64_t))engine->getFunctionAddress("set_test_ptr");
  auto item_setter =
      (bool (*)(void *, uint64_t, uint32_t))engine->getFunctionAddress(
          "set_test_ptr_item");
  auto destructor =
      (void (*)(void *))engine->getFunctionAddress("destroy_test");

  auto serializer =
      (uint8_t * (*)(void *)) engine->getFunctionAddress("serialize_test");
  auto deserializer =
      (void *(*)(uint8_t *))engine->getFunctionAddress("deserialize_test");

  uint32_t *test_data = (uint32_t *)calloc(35, sizeof(uint32_t));
  for (int i = 0; i < 35; ++i) {
    test_data[i] = (uint32_t)rand();
  }
  uint32_t *test_out_data = nullptr;
  uint16_t test_out_int16 = 0;
  float test_out_float = 0;
  uint64_t test_out_data_count = 0;

  void *test_struct = constructor(5);

  EXPECT_TRUE(count_setter(test_struct, 35));
  for (int i = 0; i < 35; ++i) {
    EXPECT_TRUE(item_setter(test_struct, i, test_data[i]));
  }
  EXPECT_TRUE(set_float(test_struct, 3.14159265));
  EXPECT_TRUE(getter(test_struct, &test_out_data));
  EXPECT_TRUE(count_getter(test_struct, &test_out_data_count));
  EXPECT_TRUE(get_int(test_struct, &test_out_int16));
  EXPECT_TRUE(get_float(test_struct, &test_out_float));

  EXPECT_EQ(test_out_int16, 5);
  EXPECT_FLOAT_EQ(test_out_float, 3.14159265);

  uint32_t got_item = 0;
  for (int i = 0; i < test_out_data_count; ++i) {
    EXPECT_EQ(test_data[i], test_out_data[i]);
    EXPECT_TRUE(item_getter(test_struct, i, &got_item));
    EXPECT_EQ(got_item, test_data[i]);
  }

  uint32_t rand_to_set = rand();
  EXPECT_TRUE(item_setter(test_struct, 3, rand_to_set));
  EXPECT_TRUE(item_getter(test_struct, 3, &got_item));
  EXPECT_EQ(got_item, rand_to_set);

  uint8_t *serialized = serializer(test_struct);

  void *deserialized_struct = deserializer(serialized);
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
    if (i == 3) {
      EXPECT_EQ(deserialized_data[i], rand_to_set);
    }
    EXPECT_EQ(deserialized_data[i], test_out_data[i]);
  }

  destructor(deserialized_struct);
  destructor(test_struct);
  free(test_data);
  free(serialized);
}

} // namespace
