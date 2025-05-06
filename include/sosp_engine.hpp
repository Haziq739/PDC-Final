#pragma once
#include "graph.hpp"
#include <omp.h>
#include <vector>
#include <queue>
#include <limits>
#include <atomic>
#include <mutex>

class SOSPEngine {
    DynamicGraph& graph;
    std::vector<double> distances;
    std::vector<int> predecessors;
    std::vector<std::atomic<bool>> in_queue;

public:
    explicit SOSPEngine(DynamicGraph& g) : graph(g), in_queue(g.node_count()) {}

    void compute(int source) {
        const double INF = std::numeric_limits<double>::max();
        distances.assign(graph.node_count(), INF);
        predecessors.assign(graph.node_count(), -1);

        for (auto& flag : in_queue) {
            flag.store(false);
        }

        distances[source] = 0.0;

        // Shared priority queue protected by a mutex
        std::priority_queue<std::pair<double, int>, 
                            std::vector<std::pair<double, int>>, 
                            std::greater<>> global_queue;
        std::mutex queue_mutex;

        global_queue.emplace(0.0, source);
        in_queue[source].store(true);

        int active_threads = omp_get_max_threads(); // ← Plain int now
        bool global_active = true;

        #pragma omp parallel
        {
            while (global_active) {
                std::pair<double, int> current;
                bool has_work = false;

                // Try to get work from the queue
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    if (!global_queue.empty()) {
                        current = global_queue.top();
                        global_queue.pop();
                        has_work = true;
                    }
                }

                if (!has_work) {
                    #pragma omp atomic
                    --active_threads;

                    #pragma omp barrier

                    #pragma omp single
                    {
                        if (active_threads == 0) {
                            global_active = false;
                        }
                    }

                    #pragma omp barrier

                    if (!global_active) break;

                    #pragma omp atomic
                    ++active_threads;

                    continue;
                }

                int u = current.second;
                in_queue[u].store(false);

                if (current.first > distances[u]) continue;

                // Process neighbors
                for (const auto& edge : graph.get_edges(u)) {
                    double new_dist = current.first + edge.weights[0];

                    if (new_dist < distances[edge.target]) {
                        #pragma omp critical(distance_update)
                        {
                            if (new_dist < distances[edge.target]) {
                                distances[edge.target] = new_dist;
                                predecessors[edge.target] = u;
                            }
                        }

                        bool expected = false;
                        if (in_queue[edge.target].compare_exchange_strong(expected, true)) {
                            std::lock_guard<std::mutex> lock(queue_mutex);
                            global_queue.emplace(new_dist, edge.target);
                        }
                    }
                }
            }
        }
    }

    void update(const std::vector<int>& changed_edges) {
        std::vector<bool> affected(graph.node_count(), false);
        for (int e : changed_edges) {
            affected[e] = true;
        }

        #pragma omp parallel for schedule(dynamic, 32)
        for (int u = 0; u < graph.node_count(); ++u) {
            if (affected[u]) {
                for (const auto& edge : graph.get_edges(u)) {
                    double new_dist = distances[u] + edge.weights[0];
                    if (new_dist < distances[edge.target]) {
                        #pragma omp critical
                        {
                            if (new_dist < distances[edge.target]) {
                                distances[edge.target] = new_dist;
                                predecessors[edge.target] = u;
                            }
                        }
                    }
                }
            }
        }
    }

    double get_distance(int node) const {
        if (node < 0 || node >= static_cast<int>(distances.size())) {
            throw std::out_of_range("Node ID out of range");
        }
        return distances[node];
    }

    const std::vector<double>& get_all_distances() const { return distances; }
};
