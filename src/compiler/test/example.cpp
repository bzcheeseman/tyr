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

int main(int argc, char *argv[]) {
  do_some_stuff("hello", "world");
  switch (std::atoi(argv[1])) {
    case 1:
      func1(std::atoi(argv[2])); break;
    case 2:
      func2(std::atoi(argv[2])); break;
    case 3:
      func3(std::atoi(argv[2])); break;
  }
}

