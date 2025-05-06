#include "../include/graph.hpp"
#include "../include/sosp2.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>

int main() {
    const int num_nodes = 4;

    // Graph Construction - using only first weight for distance
    DynamicGraph graph;
    const std::vector<std::tuple<int, int, std::vector<double>>> edges = {
        {0, 1, {4.0}}, {1, 0, {4.0}},
        {0, 2, {2.0}}, {2, 0, {2.0}},
        {1, 3, {5.0}}, {3, 1, {5.0}},
        {2, 3, {1.0}}, {3, 2, {1.0}}
    };

    for (const auto& [src, tgt, weights] : edges) {
        graph.add_edge(src, tgt, weights);
    }

    // Each node computes paths from itself
    std::vector<std::vector<double>> all_distances(num_nodes);

    for (int source_node = 0; source_node < num_nodes; ++source_node) {
        SOSPEngine engine(graph);
        all_distances[source_node] = engine.compute_shortest_paths(source_node);

        // Output distances for this source node
        std::cout << "Distances from node " << source_node << ":\n";
        for (int i = 0; i < num_nodes; ++i) {
            std::cout << "  to " << i << ": " << all_distances[source_node][i] << "\n";
        }
        std::cout << "\n";
    }

    // Process results
    std::cout << "\n=== Complete Distance Matrix ===\n";
    for (int src = 0; src < num_nodes; ++src) {
        std::cout << "From " << src << ": ";
        for (int dst = 0; dst < num_nodes; ++dst) {
            std::cout << all_distances[src][dst] << " ";
        }
        std::cout << "\n";
    }

    // Find shortest paths (minimum of each column, excluding diagonal)
    std::cout << "\n=== Shortest Paths Between Different Nodes ===\n";
    for (int dst = 0; dst < num_nodes; ++dst) {
        double min_dist = std::numeric_limits<double>::max();
        for (int src = 0; src < num_nodes; ++src) {
            if (src != dst) {  // Exclude distance to self
                min_dist = std::min(min_dist, all_distances[src][dst]);
            }
        }
        std::cout << "Minimum distance to node " << dst << ": " << min_dist << "\n";
    }

    return 0;
}
