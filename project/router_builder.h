#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "graph.h"

namespace Graph {

struct RouterBuilder {
    using Graph = DirectedWeightedGraph<double>;
    
    RouterBuilder(const Graph& graph) : graph_(graph),
                                        routes_internal_data_(graph.GetVertexCount(), std::vector<std::optional<RouteInternalData>>(graph.GetVertexCount())) {
        InitializeRoutesInternalData(graph);

        const size_t vertex_count = graph.GetVertexCount();
        for (VertexId vertex_through = 0; vertex_through < vertex_count; ++vertex_through) {
            RelaxRoutesInternalDataThroughVertex(vertex_count, vertex_through);
        }
    }

    using RouteId = uint64_t;

    const Graph& graph_;

    struct RouteInternalData {
        double weight;
        std::optional<EdgeId> prev_edge;
    };
    using RoutesInternalData = std::vector<std::vector<std::optional<RouteInternalData>>>;

    //using ExpandedRoute = std::vector<EdgeId>;
    //mutable RouteId next_route_id_ = 0;

    void InitializeRoutesInternalData(const Graph& graph) {
        const size_t vertex_count = graph.GetVertexCount();
        for (VertexId vertex = 0; vertex < vertex_count; ++vertex) {
            routes_internal_data_[vertex][vertex] = RouteInternalData{0, std::nullopt};
            for (const EdgeId edge_id : graph.GetIncidentEdges(vertex)) {
                const auto& edge = graph.GetEdge(edge_id);
                assert(edge.weight >= 0);
                auto& route_internal_data = routes_internal_data_[vertex][edge.to];
                if (!route_internal_data || route_internal_data->weight > edge.weight) {
                    route_internal_data = RouteInternalData{edge.weight, edge_id};
                }
            }
        }
    }

    void RelaxRoute(VertexId vertex_from, VertexId vertex_to,
                    const RouteInternalData& route_from, const RouteInternalData& route_to) {
        auto& route_relaxing = routes_internal_data_[vertex_from][vertex_to];
        const double candidate_weight = route_from.weight + route_to.weight;
        if (!route_relaxing || candidate_weight < route_relaxing->weight) {
            route_relaxing = {
                candidate_weight,
                route_to.prev_edge
                    ? route_to.prev_edge
                    : route_from.prev_edge};
        }
    }

    void RelaxRoutesInternalDataThroughVertex(size_t vertex_count, VertexId vertex_through) {
        for (VertexId vertex_from = 0; vertex_from < vertex_count; ++vertex_from) {
            if (const auto& route_from = routes_internal_data_[vertex_from][vertex_through]) {
                for (VertexId vertex_to = 0; vertex_to < vertex_count; ++vertex_to) {
                    if (const auto& route_to = routes_internal_data_[vertex_through][vertex_to]) {
                        RelaxRoute(vertex_from, vertex_to, *route_from, *route_to);
                    }
                }
            }
        }
    }

    RoutesInternalData routes_internal_data_;
};

}  // namespace Graph
