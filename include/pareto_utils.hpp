#pragma once
#include <vector>
#include "path_result.hpp"  // Include the new header

namespace Pareto {
    // Check if path A dominates path B
    inline bool dominates(const std::vector<double>& a, 
                         const std::vector<double>& b) {
        bool at_least_one_better = false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (a[i] > b[i]) return false;
            if (a[i] < b[i]) at_least_one_better = true;
        }
        return at_least_one_better;
    }

    // Primary template
    template <typename T, typename Enable = void>
    struct DominanceFilter;

    // Specialization for vector<double>
    template <>
    struct DominanceFilter<std::vector<double>> {
        static void filter(std::vector<std::vector<double>>& paths) {
            auto it = paths.begin();
            while (it != paths.end()) {
                bool dominated = false;
                for (const auto& other : paths) {
                    if (&(*it) != &other && dominates(other, *it)) {
                        dominated = true;
                        break;
                    }
                }
                it = dominated ? paths.erase(it) : ++it;
            }
        }
    };

    // Specialization for PathResult
    template <>
    struct DominanceFilter<PathResult> {
        static void filter(std::vector<PathResult>& paths) {
            auto it = paths.begin();
            while (it != paths.end()) {
                bool dominated = false;
                for (const auto& other : paths) {
                    if (&(*it) != &other && dominates(other.objectives, it->objectives)) {
                        dominated = true;
                        break;
                    }
                }
                it = dominated ? paths.erase(it) : ++it;
            }
        }
    };

    // Convenience wrapper
    template <typename T>
    void filter_dominated(std::vector<T>& paths) {
        DominanceFilter<T>::filter(paths);
    }
}