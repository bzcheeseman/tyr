//
// Created by Aman LaChapelle on 7/14/18.
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

#include <functional>
#include <map>
#include <string>

#include "../src/compiler/include/Register.hpp"

template<typename T>
struct tyr_retval {
  tyr_retval(T value): value(value) {}

  T value;
};

void do_some_stuff(const char *string, std::string string2) {
  string2 += std::string(string);
  // do some init
  return;
}

int func1(int x) {
  return x + 1;
}

int func2(int y) {
  return y + 2;
}

int func3(int z) {
  return z + 3;
}

void route() {
  mark_tyr_function();
  do_some_stuff("hello", "world");
  set_tyr_route("one", func1);
  set_tyr_route("two", func2);
  set_tyr_route("three", func3);
}

