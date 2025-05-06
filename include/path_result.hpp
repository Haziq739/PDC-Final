#pragma once
#include <vector>
#include <iostream>

struct PathResult {
    std::vector<int> nodes;
    std::vector<double> objectives;
    
    void print() const {
        std::cout << "Path: ";
        for (int n : nodes) std::cout << n << "→";
        std::cout << "\b\b  "; // Remove last arrow
        std::cout << " | Time=" << objectives[0] 
                  << " mins, Cost=$" << objectives[1] << "\n";
    }
};