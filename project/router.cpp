#include "router.h"

namespace Graph {
Router::Router(const ProtoCatalog::TransportCatalog& data) : data(data) {
}

std::optional<typename Router::RouteInfo> Router::BuildRoute(VertexId from, VertexId to) const {
    const auto& route_internal_data = data.route_internal_data(from).element(to);
    // for (size_t i = 0; i < data.route_internal_data_size(); ++i) {
    //     for (size_t j = 0; j < data.route_internal_data(i).element_size(); ++j) {
    //         if (data.route_internal_data(i).element(j).has_prev()) {
    //             std::cout << "bingo";
    //         }
    //     }
    // }
    if (!route_internal_data.has_value()) {
        return std::nullopt;
    }
    const double weight = route_internal_data.weight();
    std::vector<size_t> edges;
    if (route_internal_data.has_prev()) {
        for (auto edge_id = route_internal_data.prev_edge();
             data.route_internal_data(from).element(data.graph().edges(edge_id).from()).has_value();
             edge_id = data.route_internal_data(from).element(data.graph().edges(edge_id).from()).prev_edge()) {
            edges.push_back(edge_id);
            if (!data.route_internal_data(from).element(data.graph().edges(edge_id).from()).has_prev()) {
                break;
            }
        }
    }
    std::reverse(std::begin(edges), std::end(edges));

    const RouteId route_id = next_route_id_++;
    const size_t route_edge_count = edges.size();
    expanded_routes_cache_[route_id] = std::move(edges);
    return RouteInfo{route_id, weight, route_edge_count};
}

EdgeId Router::GetRouteEdge(RouteId route_id, size_t edge_idx) const {
    return expanded_routes_cache_.at(route_id)[edge_idx];
}

void Router::ReleaseRoute(RouteId route_id) {
    expanded_routes_cache_.erase(route_id);
}
}  // namespace Graph