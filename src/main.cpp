#include "../include/graph.hpp"
#include "../include/sosp2.hpp"
#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int num_nodes = 4;
    if (size != num_nodes && rank == 0) {
        std::cerr << "Error: Need exactly 4 MPI processes\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

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

    // Each rank computes paths from its own node
    int source_node = rank;
    SOSPEngine engine(graph);
    std::vector<double> distances = engine.compute_shortest_paths(source_node);

    // Synchronize output to prevent interleaving
    for (int r = 0; r < size; r++) {
        if (rank == r) {
            std::cout << "Rank " << rank << " distances from node " 
                      << source_node << ":\n";
            for (int i = 0; i < num_nodes; ++i) {
                std::cout << "  to " << i << ": " << distances[i] << "\n";
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Gather ALL distances at root (rank 0)
    std::vector<double> all_distances(num_nodes * num_nodes);
    MPI_Gather(distances.data(), num_nodes, MPI_DOUBLE,
              all_distances.data(), num_nodes, MPI_DOUBLE,
              0, MPI_COMM_WORLD);

    // Process results at root
    if (rank == 0) {
        std::cout << "\n=== Complete Distance Matrix ===\n";
        for (int src = 0; src < num_nodes; ++src) {
            std::cout << "From " << src << ": ";
            for (int dst = 0; dst < num_nodes; ++dst) {
                std::cout << all_distances[src * num_nodes + dst] << " ";
            }
            std::cout << "\n";
        }

        // Find shortest paths (minimum of each column, excluding diagonal)
        std::cout << "\n=== Shortest Paths Between Different Nodes ===\n";
        for (int dst = 0; dst < num_nodes; ++dst) {
            double min_dist = std::numeric_limits<double>::max();
            for (int src = 0; src < num_nodes; ++src) {
                if (src != dst) {  // Exclude distance to self
                    min_dist = std::min(min_dist, all_distances[src * num_nodes + dst]);
                }
            }
            std::cout << "Minimum distance to node " << dst << ": " << min_dist << "\n";
        }
    }

    MPI_Finalize();
    return 0;
}