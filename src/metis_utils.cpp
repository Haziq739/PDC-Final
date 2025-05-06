#include "../include/metis_utils.hpp"
#include "../include/graph.hpp"
#include <metis.h>

void MetisUtils::partition_graph(DynamicGraph& graph, int nparts) {
    idx_t n = graph.node_count();
    idx_t ncon = 1;
    idx_t objval;
    std::vector<idx_t> part(n);

    idx_t* xadj = graph.get_metis_xadj();
    idx_t* adjncy = graph.get_metis_adjncy();
    
    METIS_PartGraphKway(&n, &ncon, xadj, adjncy,
                       NULL, NULL, NULL, &nparts, 
                       NULL, NULL, NULL, &objval, part.data());

    for (idx_t i = 0; i < n; ++i) {
        graph.set_partition(i, part[i]);
    }
}