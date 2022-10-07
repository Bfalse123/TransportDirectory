#include "serialize.h"

#include "router_builder.h"

namespace Serialize {
Serializator::Serializator(const TransportCatalog::Catalog& db,
                           const TransportCatalog::TransportGraph& graph) : db(db), graph(graph) {}

void Serializator::LoadBuses(ProtoCatalog::TransportCatalog& data) {
    for (const auto& [name, body] : db.GetBuses()) {
        ProtoCatalog::Bus& response_bus = (*data.mutable_buses())[name];
        response_bus.set_name(body.name);
        response_bus.set_route_length(body.route_length);
        response_bus.set_curvature(body.route_length / body.geo_route_length);
        response_bus.set_stops_cnt(body.stops_cnt);
        response_bus.set_unique_stops_cnt(body.unique_stops_cnt);
    }
}

void Serializator::BuildAndLoadRouter(ProtoCatalog::TransportCatalog& data) {
    Graph::RouterBuilder router(graph.GetGraph());
    size_t i = 0;
    for (const auto& row : router.routes_internal_data_) {
        ProtoCatalog::Row* new_row = data.add_route_internal_data();
        size_t j = 0;
        for (const auto& element : row) {
            ProtoCatalog::RouteInternalData* new_element = new_row->add_element();
            if (element) {
                new_element->set_has_value(true);
                if (element->prev_edge) {
                    new_element->set_has_prev(true);
                    new_element->set_prev_edge(*element->prev_edge);
                    new_element->set_weight(element->weight);
                }
                else {
                    new_element->set_has_prev(false);
                }
            }
            else {
                new_element->set_has_value(false);
            }
            ++j;
        }
        ++i;
    }
}

void Serializator::LoadGraphInfo(ProtoCatalog::TransportCatalog& data) {
    using namespace TransportCatalog;
    ProtoCatalog::Graph* serializing_graph = data.mutable_graph();
    for (const auto& [name, vertex] : graph.vertices) {
        ProtoCatalog::Vertex& v = (*serializing_graph->mutable_vertices())[name];
        v.set_name(name);
        v.set_wait(vertex.wait);
        v.set_ride(vertex.ride);
    }
    for (const auto& edge : graph.edges) {
        ProtoCatalog::WaitOrBus* e = serializing_graph->add_edges();
        if (std::holds_alternative<TransportGraph::BusEdge>(edge)) {
            TransportGraph::BusEdge bus_edge = std::get<TransportGraph::BusEdge>(edge);
            e->set_is_wait_edge(false);
            e->set_from(bus_edge.from);
            e->set_to(bus_edge.to);
            e->set_time(bus_edge.time);
            ProtoCatalog::BusEdge* bus = e->mutable_bus();
            bus->set_bus(bus_edge.bus);
            bus->set_span_cnt(bus_edge.span_cnt);
        } else if (std::holds_alternative<TransportGraph::WaitEdge>(edge)) {
            TransportGraph::WaitEdge wait_edge = std::get<TransportGraph::WaitEdge>(edge);
            e->set_is_wait_edge(true);
            e->set_from(wait_edge.from);
            e->set_to(wait_edge.to);
            e->set_time(wait_edge.time);
            ProtoCatalog::WaitEdge* wait = e->mutable_wait();
            wait->set_stop(wait_edge.stop);
        }
    }
    BuildAndLoadRouter(data);
}

void Serializator::LoadStops(ProtoCatalog::TransportCatalog& data) {
    for (const auto& [name, body] : db.GetStops()) {
        ProtoCatalog::Stop& response_stop = (*data.mutable_stops())[name];
        response_stop.set_name(body.name);
        for (const auto& [bus_name, _] : body.pos_in_routes) {
            response_stop.add_buses(bus_name);
        }
    }
}

void Serializator::SerializeTo(const std::string& path) {
    ProtoCatalog::TransportCatalog serializing_db;
    LoadBuses(serializing_db);
    LoadStops(serializing_db);
    LoadGraphInfo(serializing_db);
    std::ofstream file(path);
    serializing_db.SerializePartialToOstream(&file);
    file.close();
}

}  // namespace Serialize