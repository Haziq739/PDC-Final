#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <metis.h>
#include <limits>

class DynamicGraph {
public:
    struct NodeData {
        std::vector<double> weights;
        int predecessor;
        double distance;
        bool visited;
        
        NodeData() : 
            predecessor(-1),
            distance(std::numeric_limits<double>::max()),
            visited(false) {}
    };

    struct Edge {
        int target;
        std::vector<double> weights;
    };

    // Added constructor with initial size
    explicit DynamicGraph(size_t initial_size = 0) {
        if (initial_size > 0) {
            resize_if_needed(initial_size - 1);
        }
    }

    // Add node with optional data
    void add_node(int node_id, const NodeData& data = NodeData()) {
        if (node_id < 0) throw std::invalid_argument("Node IDs must be non-negative");
        resize_if_needed(node_id);
        node_data[node_id] = data;
    }

    // Add edge with multiple weights
    void add_edge(int src, int tgt, const std::vector<double>& weights) {
        if (src < 0 || tgt < 0) throw std::invalid_argument("Node IDs must be non-negative");
        if (weights.empty()) throw std::invalid_argument("Edge must have at least one weight");
        
        resize_if_needed(std::max(src, tgt));
        adj[src].push_back({tgt, weights});
    }

    // Remove edge
    void remove_edge(int src, int tgt) {
        if (src >= adj.size() || tgt >= adj.size()) return;
        
        auto& edges = adj[src];
        edges.erase(std::remove_if(edges.begin(), edges.end(),
            [tgt](const Edge& e) { return e.target == tgt; }), edges.end());
    }

    // Get edges from a node
    const std::vector<Edge>& get_edges(int node) const {
        if (node >= adj.size()) throw std::out_of_range("Node ID out of range");
        return adj[node];
    }

    // Get neighbors (target nodes)
    std::vector<int> get_neighbors(int node) const {
        std::vector<int> neighbors;
        if (node < adj.size()) {
            for (const auto& edge : adj[node]) {
                neighbors.push_back(edge.target);
            }
        }
        return neighbors;
    }

    // Node count
    size_t node_count() const { 
        return node_data.size();
    }

    // Edge count
    size_t edge_count() const {
        size_t count = 0;
        for (const auto& edges : adj) {
            count += edges.size();
        }
        return count;
    }

    // Backward compatibility
    size_t size() const { return node_count(); }

    // Node data access
    NodeData& get_node_data(int node) {
        if (node >= node_data.size()) throw std::out_of_range("Node ID out of range");
        return node_data[node];
    }

    const NodeData& get_node_data(int node) const {
        if (node >= node_data.size()) throw std::out_of_range("Node ID out of range");
        return node_data[node];
    }

    void update_node(int node, const NodeData& data) {
        if (node >= node_data.size()) throw std::out_of_range("Node ID out of range");
        node_data[node] = data;
    }

    // METIS partitioning support
    void set_partition(int node_id, int partition_id) {
        if (node_id < 0) throw std::invalid_argument("Node ID must be non-negative");
        resize_if_needed(node_id);
        partitions[node_id] = partition_id;
    }

    int get_partition(int node_id) const {
        if (node_id < 0 || node_id >= partitions.size()) {
            throw std::out_of_range("Invalid node ID");
        }
        return partitions[node_id];
    }

    const std::vector<int>& get_partitions() const { return partitions; }

    // METIS CSR format conversion
    idx_t* get_metis_xadj() {
        xadj_metis.resize(adj.size() + 1);
        adjncy_metis.clear();
        weights_metis.clear();
        
        xadj_metis[0] = 0;
        for (size_t i = 0; i < adj.size(); i++) {
            xadj_metis[i+1] = xadj_metis[i] + adj[i].size();
            for (const auto& edge : adj[i]) {
                adjncy_metis.push_back(edge.target);
                if (!edge.weights.empty()) {
                    weights_metis.push_back(edge.weights[0]);
                }
            }
        }
        return xadj_metis.data();
    }

    idx_t* get_metis_adjncy() {
        return adjncy_metis.data();
    }

    idx_t* get_metis_weights() {
        return weights_metis.empty() ? nullptr : weights_metis.data();
    }

    // Extract a subgraph for a partition
    DynamicGraph extract_partition(const std::vector<int>& partitions, int my_partition) const {
        DynamicGraph subgraph;
        
        for (size_t i = 0; i < partitions.size(); i++) {
            if (partitions[i] == my_partition) {
                subgraph.add_node(i, node_data[i]);
            }
        }
        
        for (size_t i = 0; i < adj.size(); i++) {
            if (partitions[i] == my_partition) {
                for (const auto& edge : adj[i]) {
                    if (partitions[i] == my_partition || partitions[edge.target] == my_partition) {
                        subgraph.add_edge(i, edge.target, edge.weights);
                    }
                }
            }
        }
        
        return subgraph;
    }

    void clear() {
        adj.clear();
        node_data.clear();
        partitions.clear();
        xadj_metis.clear();
        adjncy_metis.clear();
        weights_metis.clear();
    }

private:
    std::vector<std::vector<Edge>> adj;
    std::vector<NodeData> node_data;
    std::vector<int> partitions;
    
    std::vector<idx_t> xadj_metis;
    std::vector<idx_t> adjncy_metis;
    std::vector<idx_t> weights_metis;

    void resize_if_needed(int max_node) {
        if (max_node >= adj.size()) {
            adj.resize(max_node + 1);
        }
        if (max_node >= node_data.size()) {
            node_data.resize(max_node + 1);
        }
        if (max_node >= partitions.size()) {
            partitions.resize(max_node + 1, -1);
        }
    }
};