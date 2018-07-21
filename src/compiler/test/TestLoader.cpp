//
// Created by Aman LaChapelle on 7/20/18.
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

#include <gtest/gtest.h>
#include <Loader.hpp>

namespace {
  using namespace tyr::compiler;

  TEST(Loader, from_file) {
    llvm::LLVMContext ctx;
    Loader loader {ctx};

    EXPECT_NO_THROW(std::unique_ptr<llvm::Module> module = loader.LoadModule("example.ll"));
  }
}

