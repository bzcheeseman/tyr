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

#include "path.h"

#include "FileHelper.h"

#include <vector>
#include <random>
#include <numeric>
#include <iostream>
#include <fstream>

uint8_t *get_serialized_storage(std::vector<float> &x, std::vector<float> &y) {
  path_ptr data = create_path(5);

  std::random_device rd;
  std::mt19937 e(rd());
  std::uniform_real_distribution<float> dist(0, 3);

  for (int i = 0; i < 125; ++i) {
    x.push_back(dist(e));
    y.push_back(dist(e));
  }

  set_path_x(data, x.data(), x.size());
  set_path_y(data, y.data(), y.size());

  tyr_serialize_to_file("serialized_path.tsf", &serialize_path, data);

  uint8_t *out = serialize_path(data);
  destroy_path(data);
  return out;
}

bool check_serialized(uint8_t *serialized, const std::vector<float> &x, const std::vector<float> &y) {
  path_ptr deserialized = deserialize_path(serialized);
  free(serialized);

  uint32_t idx = 0;
  assert(get_path_idx(deserialized, &idx));
  assert(idx == 5);

  uint64_t x_count;
  assert(get_path_x_count(deserialized, &x_count));
  assert(x_count == x.size());

  uint64_t y_count;
  assert(get_path_y_count(deserialized, &y_count));
  assert(y_count == y.size());

  float *x_, *y_;
  assert(get_path_x(deserialized, &x_));
  assert(get_path_y(deserialized, &y_));
  for (int i = 0; i < 125; ++i) {
    assert(x_[i] == x[i]);
    assert(y_[i] == y[i]);
  }

  destroy_path(deserialized);

  std::cout << "Test succeeded" << std::endl;
  return true;
}

int main() {

  std::vector<float> x, y;
  uint8_t *serialized = get_serialized_storage(x, y);
  // TODO: we should NOT have to know this as a user of the lib...
  uint64_t serialized_len = *((uint64_t *)serialized);

  assert(check_serialized(serialized, x, y));
  
  return 0;

}
