#define CATCH_CONFIG_MAIN
#include "../include/catch.hpp"
#include "../include/mpi_distributor.hpp"
#include "../include/graph.hpp"

TEST_CASE("MPI Distributor basic functionality", "[mpi]") {
    // Initialize MPI for test environment
    MPI_Init(nullptr, nullptr);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Create and initialize graph on rank 0
    DynamicGraph graph;
    if (rank == 0) {
        // Add nodes 0-4
        for (int i = 0; i < 5; i++) {
            graph.add_node(i);
        }
        // Add edges
        graph.add_edge(0, 1, {1, 2});
        graph.add_edge(1, 2, {2, 3});
        graph.add_edge(2, 3, {3, 4});
        graph.add_edge(3, 4, {4, 5});
        graph.add_edge(4, 0, {5, 6});
    }

    // Test MPI distribution
    MPIDistributor distributor(graph);
    distributor.partition_and_distribute();
    
    auto& local_part = distributor.get_local_partition();
    REQUIRE(local_part.node_count() > 0);
    
    distributor.synchronize_boundaries();
    
    // Add verification for boundary sync
    if (rank == 0) {
        auto& boundary_nodes = distributor.get_boundary_nodes();
        REQUIRE_FALSE(boundary_nodes.empty());
    }

    MPI_Finalize();
}

TEST_CASE("MPI Distributor handles empty graph", "[mpi]") {
    MPI_Init(nullptr, nullptr);
    
    DynamicGraph empty_graph;
    MPIDistributor distributor(empty_graph);
    
    REQUIRE_NOTHROW(distributor.partition_and_distribute());
    
    auto& local_part = distributor.get_local_partition();
    REQUIRE(local_part.node_count() == 0);
    
    MPI_Finalize();
}