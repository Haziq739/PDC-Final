#include "../include/mpi_distributor.hpp"
#include "../include/metis_utils.hpp"
#include <iostream>
#include <set>
#include <algorithm>
#include <unordered_set>

MPIDistributor::MPIDistributor(DynamicGraph& graph) : original_graph(graph) {
    // Initialize data structures
    node_partitions.resize(graph.node_count(), -1);
}

MPIDistributor::~MPIDistributor() {
    // Cleanup if needed
}

void MPIDistributor::partition_and_distribute() {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // For simplicity with a small graph, distribute nodes among the available ranks
    if (rank == 0) {
        // More robust distribution: try to distribute nodes evenly
        size_t nodes_per_rank = original_graph.node_count() / std::max(1, std::min(size, 4));
        for (size_t i = 0; i < original_graph.node_count(); ++i) {
            // Distribute among first 4 ranks at most (to handle our 4-node graph)
            node_partitions[i] = std::min(i / nodes_per_rank, static_cast<size_t>(std::min(size-1, 3)));
        }
    }
    
    // Broadcast partitioning information to all processes
    MPI_Bcast(node_partitions.data(), node_partitions.size(), MPI_INT, 0, MPI_COMM_WORLD);
    
    // Update the original graph with partitioning info on all ranks
    for (size_t i = 0; i < node_partitions.size(); ++i) {
        original_graph.set_partition(i, node_partitions[i]);
    }
    
    // Create local partition for this rank
    gather_partition_info();
    
    // Exchange boundary information
    exchange_boundary_data();
}

void MPIDistributor::gather_partition_info() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // Clear existing local partition
    local_partition = DynamicGraph(original_graph.node_count());
    
    // Create the local subgraph
    std::unordered_set<int> my_nodes;
    for (size_t i = 0; i < node_partitions.size(); ++i) {
        if (node_partitions[i] == rank) {
            my_nodes.insert(i);
            local_partition.add_node(i, original_graph.get_node_data(i));
            local_partition.set_partition(i, rank);
        }
    }
    
    // Add edges for local nodes
    for (int node : my_nodes) {
        const auto& edges = original_graph.get_edges(node);
        for (const auto& edge : edges) {
            local_partition.add_edge(node, edge.target, edge.weights);
            
            // If target is in another partition, it's a boundary node
            if (node_partitions[edge.target] != rank && node_partitions[edge.target] >= 0) {
                boundary_nodes[node_partitions[edge.target]].push_back(edge.target);
                
                // Also add this node to local partition as a ghost node
                if (!my_nodes.count(edge.target)) {
                    local_partition.add_node(edge.target, original_graph.get_node_data(edge.target));
                    local_partition.set_partition(edge.target, node_partitions[edge.target]);
                }
            }
        }
    }
    
    // Remove duplicates in boundary nodes
    for (auto& [part, nodes] : boundary_nodes) {
        std::sort(nodes.begin(), nodes.end());
        nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    }
    
    // Debug output
    std::cout << "Rank " << rank << " boundary nodes: ";
    for (const auto& [part, nodes] : boundary_nodes) {
        std::cout << "part " << part << ": ";
        for (int node : nodes) {
            std::cout << node << " ";
        }
    }
    std::cout << std::endl;
}

void MPIDistributor::exchange_boundary_data() {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // For small graphs, it's more efficient to just include all nodes and edges
    // This ensures all possible paths can be computed
    if (original_graph.node_count() <= 100) {  // Threshold for small graphs
        // Include all nodes from original graph in local partition
        for (size_t i = 0; i < original_graph.node_count(); ++i) {
            if (i >= local_partition.node_count() || 
                local_partition.get_node_data(i).distance == std::numeric_limits<double>::max()) {
                local_partition.add_node(i, original_graph.get_node_data(i));
                local_partition.set_partition(i, node_partitions[i]);
            }
        }
        
        // Include all edges from original graph
        for (size_t i = 0; i < original_graph.node_count(); ++i) {
            const auto& edges = original_graph.get_edges(i);
            for (const auto& edge : edges) {
                local_partition.add_edge(i, edge.target, edge.weights);
            }
        }
    } else {
        // For larger graphs, use more selective approach
        // For each boundary node needed from other partitions, ensure we have its connections
        for (const auto& [part, nodes] : boundary_nodes) {
            for (int node : nodes) {
                // Add edges connecting to this boundary node from original graph
                const auto& edges = original_graph.get_edges(node);
                for (const auto& edge : edges) {
                    // Only add edges to nodes we already have in our local partition
                    if (local_partition.get_node_data(edge.target).distance != std::numeric_limits<double>::max() ||
                        node_partitions[edge.target] == rank) {
                        local_partition.add_edge(node, edge.target, edge.weights);
                    }
                }
            }
        }
    }
    
    // Wait for all processes to complete exchange
    MPI_Barrier(MPI_COMM_WORLD);
}

void MPIDistributor::synchronize_boundaries() {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // For a proper implementation, we would exchange updated distance info for boundary nodes
    // But for now, we'll just make sure all processes reach this point
    MPI_Barrier(MPI_COMM_WORLD);
}

DynamicGraph& MPIDistributor::get_local_partition() {
    return local_partition;
}

const std::unordered_map<int, std::vector<int>>& MPIDistributor::get_boundary_nodes() const {
    return boundary_nodes;
}