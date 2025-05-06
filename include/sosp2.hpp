#pragma once
#include "graph.hpp"
#include <vector>
#include <queue>
#include <limits>
#include <iostream>

class SOSPEngine {
    DynamicGraph& graph;
    
public:
    explicit SOSPEngine(DynamicGraph& g) : graph(g) {}

    std::vector<double> compute_shortest_paths(int source) {
        const double INF = std::numeric_limits<double>::max();
        std::vector<double> distances(graph.node_count(), INF);
        distances[source] = 0.0;

        // Min-heap: pairs of (distance, node)
        std::priority_queue<std::pair<double, int>,
                          std::vector<std::pair<double, int>>,
                          std::greater<>> pq;
        pq.push({0.0, source});

        while (!pq.empty()) {
            auto [current_dist, u] = pq.top();
            pq.pop();

            // Crucial optimization: Skip if we already found a better path
            if (current_dist > distances[u]) {
                continue;
            }

            // Explore all neighbors
            for (const auto& edge : graph.get_edges(u)) {
                double new_dist = current_dist + edge.weights[0];
                
                // Only update and push to queue if we found a better path
                if (new_dist < distances[edge.target]) {
                    distances[edge.target] = new_dist;
                    pq.push({new_dist, edge.target});
                    
                    // Debug output to verify relaxation
                    std::cout << "Relaxed " << u << "->" << edge.target 
                              << " with new distance: " << new_dist << std::endl;
                }
            }
        }

        return distances;
    }
};