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

#include <gtest/gtest.h>

namespace {
using namespace tyr;

TEST(Parser, basic_C) {
  CodeGen generator{"basic_test_c", llvm::sys::getDefaultTargetTriple(),
                    llvm::sys::getHostCPUName(), ""};

  std::string struct_def = "struct test_struct {\n"
                           "  mutable int16 int_field\n"
                           "  mutable repeated float float_array\n"
                           "  repeated bool boolean_array\n"
                           "}";

  std::istringstream is(struct_def);
  Parser p{generator};
  EXPECT_TRUE(p.parseFile(is));

  EXPECT_TRUE(generator.emitStructForUse(kUseLangC, true, false, false, ""));
};

TEST(Parser, basic_Python) {
  CodeGen generator{"basic_test_py", llvm::sys::getDefaultTargetTriple(),
                    llvm::sys::getHostCPUName(), ""};

  std::string struct_def = "struct test_struct {\n"
                           "  mutable repeated int16 int_field\n"
                           "  mutable float float_array\n"
                           "  repeated bool boolean_array\n"
                           "}";

  std::istringstream is(struct_def);
  Parser p{generator};
  EXPECT_TRUE(p.parseFile(is));

  EXPECT_TRUE(generator.emitStructForUse(kUseLangPython, false, false, false, ""));
};
} // namespace
