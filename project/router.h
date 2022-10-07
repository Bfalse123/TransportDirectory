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
#include "transport_catalog.pb.h"

namespace Graph {

class Router {
   private:
    using Graph = DirectedWeightedGraph<double>;

   public:
    Router(const ProtoCatalog::TransportCatalog& data);

    using RouteId = uint64_t;

    struct RouteInfo {
        RouteId id;
        double weight;
        size_t edge_count;
    };

    std::optional<RouteInfo> BuildRoute(VertexId from, VertexId to) const;
    EdgeId GetRouteEdge(RouteId route_id, size_t edge_idx) const;
    void ReleaseRoute(RouteId route_id);

   private:
    const ProtoCatalog::TransportCatalog& data;

    using ExpandedRoute = std::vector<EdgeId>;
    mutable RouteId next_route_id_ = 0;
    mutable std::unordered_map<RouteId, ExpandedRoute> expanded_routes_cache_;
};

}  // namespace Graph
