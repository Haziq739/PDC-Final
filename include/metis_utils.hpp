#ifndef METIS_UTILS_HPP
#define METIS_UTILS_HPP

#include "graph.hpp"

class MetisUtils {
public:
    static void partition_graph(DynamicGraph& graph, int nparts);
};

#endif // METIS_UTILS_HPP