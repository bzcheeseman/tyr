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

#include <vector>
#include <random>
#include <numeric>
#include <iostream>

uint8_t *get_serialized_storage(std::vector<float> &x, std::vector<float> &y, uint64_t *len) {
  path_ptr data = create_path(5);

  std::random_device rd;
  std::mt19937 e(rd());
  std::uniform_real_distribution<float> dist(0, 3);

  for (int i = 0; i < 125; ++i) {
    x.push_back(dist(e));
    y.push_back(dist(e));
  }
  x.shrink_to_fit();
  y.shrink_to_fit();

  set_path_x(data, x.data(), x.size());
  set_path_y(data, y.data(), y.size());
  uint8_t *out = serialize_path(data, len);
  destroy_path(data);
  return out;
}

bool check_serialized(uint8_t *serialized, uint64_t len, const std::vector<float> &x, const std::vector<float> &y) {
  path_ptr deserialized = deserialize_path(serialized, len);
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

bool check_graph() {
  std::vector<node_ptr> nodes;
  for (int i = 0; i < 15; ++i) {
    nodes.push_back(create_node(i, rand()));
  }

  std::vector<edge_ptr> edges;
  for (int i = 0; i < 14; ++i) {
    edges.push_back(create_edge(i, i+1));
  }

  graph_ptr g = create_graph();
  set_graph_node(g, nodes.data(), nodes.size());
  set_graph_edge(g, edges.data(), edges.size());

  edge_ptr *out_edges = nullptr;
  get_graph_edge(g, &out_edges);
  for (int i = 0; i < 14; ++i) {
    uint16_t src, sink;
    get_edge_src(out_edges[i], &src);
    get_edge_sink(out_edges[i], &sink);
    assert(src == i && sink == i+1);
  }

  node_ptr *out_nodes = nullptr;
  get_graph_node(g, &out_nodes);
  for (int i = 0; i < 15; ++i) {
    uint16_t id;
    uint64_t data;

    get_node_id(out_nodes[i], &id);
    get_node_data(out_nodes[i], &data);
    assert(id == i);
  }

  uint64_t serialized_size = 0;
  uint8_t *serialized = serialize_graph(g, &serialized_size);
  destroy_graph(g);

  // Now deserialize the thing
  graph_ptr deserialized = deserialize_graph(serialized, serialized_size);
  free(serialized);

  out_edges = nullptr;
  get_graph_edge(deserialized, &out_edges);
  for (int i = 0; i < 14; ++i) {
    uint16_t src, sink;
    get_edge_src(out_edges[i], &src);
    get_edge_sink(out_edges[i], &sink);
    assert(src == i && sink == i+1);
  }

  out_nodes = nullptr;
  get_graph_node(deserialized, &out_nodes);
  for (int i = 0; i < 15; ++i) {
    uint16_t id;
    uint64_t data;

    get_node_id(out_nodes[i], &id);
    get_node_data(out_nodes[i], &data);
    assert(id == i);
  }

  destroy_graph(deserialized);

  return true;
}

int main() {

  uint64_t serialized_len = 0;
  std::vector<float> x, y;
  uint8_t *serialized = get_serialized_storage(x, y, &serialized_len);
  assert(check_serialized(serialized, serialized_len, x, y));

  assert(check_graph());
  
  return 0;

}
