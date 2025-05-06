#ifndef MPI_DISTRIBUTOR_HPP
#define MPI_DISTRIBUTOR_HPP

#include "graph.hpp"
#include "metis_utils.hpp"
#include <vector>
#include <unordered_map>
#include <mpi.h>

class MPIDistributor {
public:
    MPIDistributor(DynamicGraph& graph);
    ~MPIDistributor();
    
    void partition_and_distribute();
    void synchronize_boundaries();
    DynamicGraph& get_local_partition();
    const std::unordered_map<int, std::vector<int>>& get_boundary_nodes() const;
    
private:
    DynamicGraph& original_graph;
    DynamicGraph local_partition;
    std::vector<int> node_partitions;
    std::unordered_map<int, std::vector<int>> boundary_nodes;
    
    void gather_partition_info();
    void exchange_boundary_data();
};

#endif // MPI_DISTRIBUTOR_HPP