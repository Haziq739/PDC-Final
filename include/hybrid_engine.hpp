#pragma once
#include "graph.hpp"
#include <atomic>
#include <vector>
#include <omp.h>

class HybridEngine {
    DynamicGraph& graph;
    std::vector<std::atomic<double>> atomic_distances;
    std::vector<int> predecessors;

public:
    explicit HybridEngine(DynamicGraph& g);
    
    // Compute shortest paths from source using hybrid parallelism
    void compute_parallel(int source);
    
 // Get computed distances (thread-safe)
    std::vector<double> get_distances() const;

    // Get path predecessors
    const std::vector<int>& get_predecessors() const;
};