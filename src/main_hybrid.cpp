#include "../include/hybrid_engine.hpp"
#include "../include/mpi_distributor.hpp"
#include <mpi.h>
#include <iostream>
#include <chrono>
#include <limits>
#include <vector>
#include <map>

int main(int argc, char** argv) {
    // Initialize MPI with thread support
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    if (provided < MPI_THREAD_FUNNELED) {
        std::cerr << "ERROR: Insufficient MPI thread support\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        std::cout << "Initializing graph with " << size << " MPI processes\n";
    }

    // Add MPI barrier to ensure all processes are ready before proceeding
    MPI_Barrier(MPI_COMM_WORLD);

    // Initialize a simple 4-node graph
    DynamicGraph graph(4);
    const std::vector<std::tuple<int, int, std::vector<double>>> edges = {
        {0, 1, {4.0}}, {1, 0, {4.0}},
        {0, 2, {2.0}}, {2, 0, {2.0}},
        {1, 3, {5.0}}, {3, 1, {5.0}},
        {2, 3, {1.0}}, {3, 2, {1.0}}
    };

    for (const auto& [src, tgt, weights] : edges) {
        graph.add_edge(src, tgt, weights);
    }

    // Debug output
    if (rank == 0) {
        std::cout << "Graph initialized with " << graph.node_count() << " nodes and " 
                  << graph.edge_count() << " edges\n";
    }

    // Add barrier to ensure graph is fully initialized on all processes
    MPI_Barrier(MPI_COMM_WORLD);

    try {
        // Partition the graph
        MPIDistributor distributor(graph);
        
        if (rank == 0) {
            std::cout << "Starting partition_and_distribute...\n";
        }
        distributor.partition_and_distribute();
        
        if (rank == 0) {
            std::cout << "Partition complete.\n";
        }
        
        // Add barrier after partition to ensure all processes are ready
        MPI_Barrier(MPI_COMM_WORLD);

        DynamicGraph& local_graph = distributor.get_local_partition();
        std::cout << "Rank " << rank << " got " << local_graph.node_count() 
                << " local nodes\n";
        std::cout.flush();
        
        // Add barrier after reporting local graph info
        MPI_Barrier(MPI_COMM_WORLD);

        // Hybrid computation
        HybridEngine engine(local_graph);

        // Map to store all local node distances
        std::map<int, std::vector<double>> node_distances;
        
        // Get global node mapping (original node IDs)
        std::vector<int> local_nodes;
        for (size_t i = 0; i < local_graph.node_count(); ++i) {
            // Only compute for nodes that actually belong to this partition
            if (local_graph.get_partition(i) == rank) {
                local_nodes.push_back(i);
            }
        }

        // Each rank computes paths for one specific node in the graph
        // For small graphs like this, it's more efficient to have each rank focus on a single source node
        int local_node_count = local_nodes.size();
        if (local_node_count > 0) {
            // For this particular example where we have a small graph:
            // Have each rank compute shortest paths for the node matching its rank (if it exists)
            // This ensures we get comprehensive results across all nodes
            for (int i = 0; i < local_node_count; ++i) {
                int node_id = local_nodes[i];
                
                // For small graphs, have each rank compute paths for all nodes
                // This ensures we get complete solutions even with limited node distribution
                if (graph.node_count() <= 8) {  // Fixed: replaced original_graph with graph
                    for (size_t src = 0; src < graph.node_count(); ++src) {  // Fixed: replaced original_graph with graph
                        // Only process this source if it's assigned to this rank
                        // or if this rank needs to handle more sources
                        if (local_graph.get_partition(src) == rank ||  // Fixed: replaced node_partitions with local_graph.get_partition
                            (i == 0 && src % size == rank)) {
                            engine.compute_parallel(src);
                            node_distances[src] = engine.get_distances();
                            std::cout << "Rank " << rank << " computed paths for node " << src << "\n";
                            std::cout.flush();
                        }
                    }
                } else {
                    // For larger graphs, use the original approach
                    engine.compute_parallel(node_id);
                    node_distances[node_id] = engine.get_distances();
                    
                    // Add some debug output
                    if (i % 5 == 0 || i == local_node_count - 1) {
                        std::cout << "Rank " << rank << " computed paths for node " << node_id << "\n";
                        std::cout.flush();
                    }
                }
            }
        } else {
            std::cout << "Rank " << rank << " has no local nodes to process.\n";
            std::cout.flush();
        }

        // Add barrier before synchronization
        MPI_Barrier(MPI_COMM_WORLD);
        
        if (rank == 0) {
            std::cout << "Starting boundary synchronization...\n";
        }
        
        // Synchronize boundaries
        distributor.synchronize_boundaries();
        
        if (rank == 0) {
            std::cout << "Boundary synchronization complete.\n";
        }

        // Add barrier after synchronization
        MPI_Barrier(MPI_COMM_WORLD);

        // Initialize with maximum distance values
        int graph_size = graph.node_count();
        std::vector<double> result_matrix(graph_size * graph_size, std::numeric_limits<double>::max());
        std::vector<double> global_matrix(graph_size * graph_size, std::numeric_limits<double>::max());
        
        // Fill the local distance matrix with computed results
        for (const auto& [source, distances] : node_distances) {
            for (size_t target = 0; target < distances.size(); ++target) {
                if (distances[target] != std::numeric_limits<double>::max()) {
                    // Use matrix indexing: source*size + target
                    result_matrix[source * graph_size + target] = distances[target];
                }
            }
        }
        
        std::cout << "Rank " << rank << " before Allreduce, local matrix size: " << result_matrix.size() 
                << ", global matrix size: " << global_matrix.size() << std::endl;
        
        // Debug output of local distance matrix
        std::cout << "Rank " << rank << " local distances: ";
        for (size_t i = 0; i < local_nodes.size(); ++i) {
            int source = local_nodes[i];
            std::cout << "\nFrom node " << source << ": ";
            for (size_t j = 0; j < graph_size; ++j) {
                double dist = result_matrix[source * graph_size + j];
                if (dist == std::numeric_limits<double>::max())
                    std::cout << "INF ";
                else
                    std::cout << dist << " ";
            }
        }
        std::cout << std::endl;
        std::cout.flush();
        
        // Make sure all processes have the same size arrays for reduction
        MPI_Barrier(MPI_COMM_WORLD);

        // Use MPI_Allreduce to combine results
        MPI_Allreduce(result_matrix.data(), global_matrix.data(), graph_size * graph_size,
                   MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);

        // Final output
        if (rank == 0) {
            std::cout << "=== Final Results ===\n";
            for (int i = 0; i < graph_size; ++i) {
                std::cout << "Distances from Node " << i << ":\n";
                for (int j = 0; j < graph_size; ++j) {
                    double dist = global_matrix[i * graph_size + j];
                    if (dist == std::numeric_limits<double>::max()) {
                        std::cout << "  Node " << j << ": INFINITY\n";
                    } else {
                        std::cout << "  Node " << j << ": " << dist << "\n";
                    }
                }
                std::cout << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Rank " << rank << " caught exception: " << e.what() << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Finalize();
    return 0;
}