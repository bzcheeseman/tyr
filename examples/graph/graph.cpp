//
// Created by Aman LaChapelle on 2019-05-01.
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

#include "graph.h"

#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

uint8_t *get_serialized_graph() {
  std::vector<node_t *> nodes;
  std::cout << "Creating Nodes" << std::endl;
  for (int i = 0; i < 15; ++i) {
    std::array<uint64_t, 3> data_vec{1, 1, 1};
    nodes.push_back(create_node(i, data_vec.size(), data_vec.data()));
  }

  std::cout << "Creating Edges" << std::endl;
  std::vector<edge_t *> edges;
  for (int i = 0; i < 14; ++i) {
    edges.push_back(create_edge(i, i + 1));
  }

  std::cout << "Creating Graph" << std::endl;
  graph_t *g = create_graph();
  set_graph_node(g, nodes.data(), nodes.size());
  set_graph_edge(g, edges.data(), edges.size());

  edge_t **out_edges = nullptr;
  get_graph_edge(g, &out_edges);
  uint64_t edge_count;
  get_graph_edge_count(g, &edge_count);
  assert(edge_count == edges.size());
  for (int i = 0; i < 14; ++i) {
    uint16_t src, sink;
    get_edge_src(out_edges[i], &src);
    get_edge_sink(out_edges[i], &sink);
    assert(src == i && sink == i + 1);
  }

  node_t **out_nodes = nullptr;
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

    free(data);
  }

  std::cout << "Serializing Graph" << std::endl;
  uint8_t *serialized = serialize_graph(g);
  destroy_graph(g);
  return serialized;
}

bool check_graph(uint8_t *serialized) {
  graph_t *deserialized = (graph_t *)deserialize_graph(serialized);
  free(serialized);

  edge_t **out_edges = nullptr;
  get_graph_edge(deserialized, &out_edges);
  for (int i = 0; i < 14; ++i) {
    uint16_t src, sink;
    get_edge_src(out_edges[i], &src);
    get_edge_sink(out_edges[i], &sink);
    std::cout << "Checking Edge " << i << std::endl;
    assert(src == i && sink == i + 1);
  }

  node_t **out_nodes = nullptr;
  get_graph_node(deserialized, &out_nodes);
  for (int i = 0; i < 15; ++i) {
    uint16_t id;
    uint64_t data_count;
    uint64_t *data;

    get_node_id(out_nodes[i], &id);
    assert(id == i);
    get_node_data_count(out_nodes[i], &data_count);
    assert(data_count == 3);
    std::cout << "Checking Node " << i << std::endl;
    get_node_data(out_nodes[i], &data);
    for (int j = 0; j < 3; ++j) {
      assert(data[j] == 1);
    }

    free(data);
  }

  destroy_graph(deserialized);

  return true;
}

int main() {

  std::vector<float> x, y;

  uint8_t *serialized_graph = get_serialized_graph();
  assert(check_graph(serialized_graph));

  std::cout << "Test succeeded" << std::endl;

  return 0;
}
