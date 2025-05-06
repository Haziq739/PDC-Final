#include "../include/graph.hpp"
#include "../include/metis_utils.hpp"
#include <cassert>
#include <iostream>

void test_add_remove_edges() {
    DynamicGraph graph;
    graph.add_edge(0, 1, {4.0, 10.0});
    graph.add_edge(0, 2, {2.0, 15.0});
    
    assert(graph.node_count() == 3);
    assert(graph.get_edges(0).size() == 2);
    
    graph.remove_edge(0, 1);
    assert(graph.get_edges(0).size() == 1);
    
    std::cout << "✅ Passed add/remove edge tests\n";
}

void test_metis_partitioning() {
    DynamicGraph graph;
    graph.add_edge(0, 1, {1.0, 1.0});
    graph.add_edge(1, 2, {1.0, 1.0});
    graph.add_edge(2, 0, {1.0, 1.0});
    
    MetisUtils::partition_graph(graph, 2);
    auto parts = graph.get_partitions();
    
    assert(parts.size() == 3);
    std::cout << "✅ Passed METIS partitioning test\n";
}

int main() {
    test_add_remove_edges();
    test_metis_partitioning();
    return 0;
}