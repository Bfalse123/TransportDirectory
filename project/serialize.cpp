#include "serialize.h"

#include <sstream>

#include "router_builder.h"

namespace Serialize {
Serializator::Serializator(const TransportCatalog::Catalog& db,
                           const TransportCatalog::TransportGraph& graph,
                           const Svg::RenderBuilder& render)
    : db(db), graph(graph), render(render) {}

void Serializator::SerializeBuses(ProtoCatalog::TransportCatalog& data) {
    for (const auto& [name, body] : db.GetBuses()) {
        ProtoCatalog::Bus& response_bus = (*data.mutable_buses())[name];
        response_bus.set_name(body.name);
        response_bus.set_route_length(body.route_length);
        response_bus.set_curvature(body.route_length / body.geo_route_length);
        response_bus.set_stops_cnt(body.stops_cnt);
        response_bus.set_unique_stops_cnt(body.unique_stops_cnt);
        response_bus.set_is_rouded(body.is_rounded);
        response_bus.add_end_points(body.end_points.first);
        response_bus.add_end_points(body.end_points.second);
        for (const auto& stop : body.route) {
            (*response_bus.add_route()) = stop->name;
        }
    }
}

void Serializator::BuildAndSerializeRouter(ProtoCatalog::TransportCatalog& data) {
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
                } else {
                    new_element->set_has_prev(false);
                }
            } else {
                new_element->set_has_value(false);
            }
            ++j;
        }
        ++i;
    }
}

void Serializator::SerializeGraphInfo(ProtoCatalog::TransportCatalog& data) {
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
            bus->add_end_points(bus_edge.end_points.first);
            bus->add_end_points(bus_edge.end_points.second);
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
    BuildAndSerializeRouter(data);
}

void Serializator::SerializeStops(ProtoCatalog::TransportCatalog& data) {
    for (const auto& [name, body] : db.GetStops()) {
        ProtoCatalog::Stop& response_stop = (*data.mutable_stops())[name];
        response_stop.set_name(body.name);
        for (const auto& [bus_name, _] : body.pos_in_routes) {
            response_stop.add_buses(bus_name);
        }
    }
}

std::string ColorToStr(const Svg::Color& color) {
    std::ostringstream color_to_string;
    Svg::Render::RenderColor(color_to_string, color);
    return color_to_string.str();
}

void Serializator::SerializeRender(ProtoCatalog::TransportCatalog& data) {
    ProtoCatalog::RenderSettings* serializing_render = data.mutable_render();
    const Svg::RenderSettings& settings = render.settings_;
    serializing_render->set_width(settings.max_width);
    serializing_render->set_height(settings.max_height);
    serializing_render->set_padding(settings.padding);
    serializing_render->set_outer_margin(settings.outer_margin);
    for (const auto& color : settings.palette) {
        serializing_render->add_color_palette()->set_color(ColorToStr(color));
    }
    serializing_render->set_line_width(settings.line_width);
    serializing_render->mutable_underlayer_color()->set_color(ColorToStr(settings.underlayer_color));
    serializing_render->set_underlayer_width(settings.underlayer_width);
    serializing_render->set_stop_radius(settings.stop_radius);
    serializing_render->mutable_bus_label_offset()->set_x(settings.bus_label_offset.x);
    serializing_render->mutable_bus_label_offset()->set_y(settings.bus_label_offset.y);
    serializing_render->set_bus_label_font_size(settings.bus_label_font_size);
    serializing_render->mutable_stop_label_offset()->set_x(settings.stop_label_offset.x);
    serializing_render->mutable_stop_label_offset()->set_y(settings.stop_label_offset.y);
    serializing_render->set_stop_label_font_size(settings.stop_label_font_size);
    for (const auto& layer : settings.layers) {
        *serializing_render->add_layers() = layer;
    }
    for (const auto& [name, point] : render.stops_points) {
        (*serializing_render->mutable_stops_points())[name].set_x(point.x);
        (*serializing_render->mutable_stops_points())[name].set_y(point.y);
    }
    for (const auto& [name, color] : render.buses_colors) {
        (*serializing_render->mutable_buses_colors())[name].set_color(ColorToStr(color));
    }
}

void Serializator::SerializeTo(const std::string& path) {
    ProtoCatalog::TransportCatalog serializing_db;
    SerializeBuses(serializing_db);
    SerializeStops(serializing_db);
    SerializeGraphInfo(serializing_db);
    SerializeRender(serializing_db);
    std::ofstream file(path);
    serializing_db.SerializePartialToOstream(&file);
    file.close();
}

}  // namespace Serialize