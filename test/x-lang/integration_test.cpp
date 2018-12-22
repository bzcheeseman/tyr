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

#include <array>
#include <vector>
#include <cassert>
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

  tyr_serialize_to_file("serialized_path.tsf", true, &serialize_path, data);

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

  return true;
}

uint8_t *get_serialized_graph() {
  std::vector<node_ptr> nodes;
  for (int i = 0; i < 15; ++i) {
    std::array<uint64_t, 3> data_vec {1, 1, 1};
    nodes.push_back(create_node(i, data_vec.size(), data_vec.data()));
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
  uint64_t edge_count;
  get_graph_edge_count(g, &edge_count);
  assert(edge_count == edges.size());
  for (int i = 0; i < 14; ++i) {
    uint16_t src, sink;
    get_edge_src(out_edges[i], &src);
    get_edge_sink(out_edges[i], &sink);
    assert(src == i && sink == i+1);
  }

  node_ptr *out_nodes = nullptr;
  get_graph_node(g, &out_nodes);
  uint64_t node_count;
  get_graph_node_count(g, &node_count);
  assert(node_count == nodes.size());
  for (int i = 0; i < 15; ++i) {
    uint16_t id;
    uint64_t data_count;
    uint64_t *data;

    get_node_id(out_nodes[i], &id);
    assert(id == i);

    get_node_data_count(out_nodes[i], &data_count);
    assert(data_count == 3);

    get_node_data(out_nodes[i], &data);
    for (int j = 0; j < 3; ++j) {
      assert(data[j] == 1);
    }
  }
  
  tyr_serialize_to_file("serialized_graph.tsf", true, &serialize_graph, g);

  uint8_t *serialized = serialize_graph(g);
  destroy_graph(g);
  return serialized;
}

bool check_graph(uint8_t *serialized) {
  graph_ptr deserialized = deserialize_graph(serialized);
  free(serialized);

  edge_ptr *out_edges = nullptr;
  get_graph_edge(deserialized, &out_edges);
  for (int i = 0; i < 14; ++i) {
    uint16_t src, sink;
    get_edge_src(out_edges[i], &src);
    get_edge_sink(out_edges[i], &sink);
    assert(src == i && sink == i+1);
  }

  node_ptr *out_nodes = nullptr;
  get_graph_node(deserialized, &out_nodes);
  for (int i = 0; i < 15; ++i) {
    uint16_t id;
    uint64_t data_count;
    uint64_t *data;

    get_node_id(out_nodes[i], &id);
    assert(id == i);
    get_node_data_count(out_nodes[i], &data_count);
    assert(data_count == 3);
    get_node_data(out_nodes[i], &data);
    for (int j = 0; j < 3; ++j) {
      assert(data[j] == 1);
    }
  }

  destroy_graph(deserialized);

  return true;
}

bool check_graph_file() {
  graph_ptr deserialized = tyr_deserialize_from_file("serialized_graph.tsf", &deserialize_graph);

  edge_ptr *out_edges = nullptr;
  get_graph_edge(deserialized, &out_edges);
  for (int i = 0; i < 14; ++i) {
    uint16_t src, sink;
    get_edge_src(out_edges[i], &src);
    get_edge_sink(out_edges[i], &sink);
    assert(src == i && sink == i+1);
  }

  node_ptr *out_nodes = nullptr;
  get_graph_node(deserialized, &out_nodes);
  for (int i = 0; i < 15; ++i) {
    uint16_t id;
    uint64_t data_count;
    uint64_t *data;

    get_node_id(out_nodes[i], &id);
    assert(id == i);
    get_node_data_count(out_nodes[i], &data_count);
    assert(data_count == 3);
    get_node_data(out_nodes[i], &data);
    for (int j = 0; j < 3; ++j) {
      assert(data[j] == 1);
    }
  }

  destroy_graph(deserialized);

  return true;
}

int main() {

  std::vector<float> x, y;
  uint8_t *serialized = get_serialized_storage(x, y);
  assert(check_serialized(serialized, x, y));

  uint8_t *serialized_graph = get_serialized_graph();
  assert(check_graph(serialized_graph));
  assert(check_graph_file());

  std::cout << "Test succeeded" << std::endl;
  
  return 0;

}
