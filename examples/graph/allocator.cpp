//
// Created by Aman LaChapelle on 2019-04-30.
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

extern "C" {

#include <printf.h>
#include <stdint.h>
#include <stdlib.h>

void *custom_alloc(uint64_t size) {
  return malloc(size);
}

void *custom_realloc(void *ptr, uint64_t new_size) {
  return realloc(ptr, new_size);
}

void custom_free(void *ptr) {
  free(ptr);
}
}
