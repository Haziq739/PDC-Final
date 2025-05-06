#include "../include/hybrid_engine.hpp"
#include <limits>
#include <algorithm>
#include <iostream>

HybridEngine::HybridEngine(DynamicGraph& g) :
    graph(g),
    atomic_distances(g.node_count()),
    predecessors(g.node_count(), -1)
{
    // Initialize distances to infinity
    #pragma omp parallel for
    for (size_t i = 0; i < atomic_distances.size(); ++i) {
        atomic_distances[i].store(std::numeric_limits<double>::max(), std::memory_order_relaxed);
    }
}

void HybridEngine::compute_parallel(int source) {
    // Check if source is valid
    if (source < 0 || source >= static_cast<int>(graph.node_count())) {
        std::cerr << "Invalid source node: " << source << std::endl;
        return;
    }

    // Reset all distances to infinity first
    #pragma omp parallel for
    for (size_t i = 0; i < atomic_distances.size(); ++i) {
        atomic_distances[i].store(std::numeric_limits<double>::max(), std::memory_order_relaxed);
    }

    // Initialize source distance
    atomic_distances[source].store(0.0, std::memory_order_relaxed);

    // Get number of available threads
    int num_threads = omp_get_max_threads();
    
    // Debug output
    std::cout << "Computing paths from source " << source << " with " << num_threads << " threads" << std::endl;

    // Use a fixed number of iterations to ensure we process longer paths
    // For a graph with n nodes, we need at most n-1 iterations to find all shortest paths
    int node_count = graph.node_count();
    int max_needed_iterations = node_count - 1;
    bool changed;
    int iterations = 0;
    const int MAX_ITERATIONS = std::max(100, max_needed_iterations);  // Safety limit
    
    do {
        changed = false;
        iterations++;
        
        #pragma omp parallel for reduction(||:changed)
        for (size_t u = 0; u < graph.node_count(); ++u) {
            double dist_u = atomic_distances[u].load(std::memory_order_acquire);
            if (dist_u == std::numeric_limits<double>::max()) continue;
            
            try {
                for (const auto& edge : graph.get_edges(u)) {
                    if (edge.target >= atomic_distances.size()) {
                        std::cerr << "Edge target out of bounds: " << edge.target 
                                << " (atomic_distances size: " << atomic_distances.size() << ")" << std::endl;
                        continue;
                    }
                    
                    if (edge.weights.empty()) {
                        std::cerr << "Edge has no weights" << std::endl;
                        continue;
                    }
                    
                    double new_dist = dist_u + edge.weights[0];
                    double old_dist = atomic_distances[edge.target].load(std::memory_order_acquire);
                    
                    // Use compare and exchange for atomic update
                    while (new_dist < old_dist) {
                        if (atomic_distances[edge.target].compare_exchange_weak(
                                old_dist, new_dist, std::memory_order_acq_rel)) {
                            #pragma omp critical
                            {
                                predecessors[edge.target] = u;
                            }
                            changed = true;
                            break;
                        }
                        // If CAS failed, old_dist has been updated with the current value
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception in compute_parallel: " << e.what() << std::endl;
            }
        }
    } while (changed && iterations < MAX_ITERATIONS);  // Repeat until no more updates or safety limit
    
    std::cout << "Completed in " << iterations << " iterations" << std::endl;
}

std::vector<double> HybridEngine::get_distances() const {
    std::vector<double> distances(atomic_distances.size());
    for (size_t i = 0; i < atomic_distances.size(); ++i) {
        distances[i] = atomic_distances[i].load(std::memory_order_relaxed);
    }
    return distances;
}

const std::vector<int>& HybridEngine::get_predecessors() const {
    return predecessors;
}