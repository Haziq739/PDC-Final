#pragma once
#include "graph.hpp"
#include "sosp_engine.hpp"
#include "pareto_utils.hpp"
#include "path_result.hpp"
#include <omp.h>

class MOSPEngine {
    DynamicGraph& graph;
    std::vector<SOSPEngine> sosp_engines;
    std::vector<size_t> weight_indices;

public:
    explicit MOSPEngine(DynamicGraph& g, 
                       const std::vector<size_t>& indices = {0})
        : graph(g), weight_indices(indices) {
        for (auto idx : indices) {
            sosp_engines.emplace_back(g);
        }
    }

    std::vector<PathResult> compute_pareto(int source, int target) {
        #pragma omp parallel for
        for (size_t i = 0; i < sosp_engines.size(); ++i) {
            sosp_engines[i].compute(source);
        }

        std::vector<PathResult> all_paths;
        extract_paths(source, target, {source}, std::vector<double>(weight_indices.size(), 0.0), all_paths);

        Pareto::filter_dominated(all_paths);
        return all_paths;
    }

    void update(const std::vector<int>& changed_edges) {
        #pragma omp parallel for
        for (size_t i = 0; i < sosp_engines.size(); ++i) {
            sosp_engines[i].update(changed_edges);
        }
    }

private:
    void extract_paths(int u, int target, 
                      std::vector<int> current_nodes,
                      std::vector<double> current_objs,
                      std::vector<PathResult>& paths) {
        if (u == target) {
            paths.push_back({current_nodes, current_objs});
            return;
        }

        for (const auto& edge : graph.get_edges(u)) {
            auto new_nodes = current_nodes;
            new_nodes.push_back(edge.target);
            
            auto new_objs = current_objs;
            for (size_t i = 0; i < weight_indices.size(); ++i) {
                new_objs[i] += edge.weights[weight_indices[i]];
            }
            
            extract_paths(edge.target, target, new_nodes, new_objs, paths);
        }
    }
};