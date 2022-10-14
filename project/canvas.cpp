#include "canvas.h"

#include <limits.h>

#include <algorithm>
#include <sstream>
#include <unordered_set>

#include "sphere.h"

namespace Svg {

Svg::Point ProtoPointToSvgPoint(const ProtoCatalog::Point &point) {
    return {point.x(), point.y()};
}

std::map<std::string, const ProtoCatalog::Point*> ConvertStops(const ProtoCatalog::TransportCatalog &db) {
    std::map<std::string, const ProtoCatalog::Point*> stops;
    for (const auto& [name, _] : db.render().stops_points()) {
        stops[name] = &db.render().stops_points().at(name);
    }
    return stops;
}

std::map<std::string, const ProtoCatalog::Bus*> ConvertBuses(const ProtoCatalog::TransportCatalog &db) {
    std::map<std::string, const ProtoCatalog::Bus*> buses;
    for (const auto& [name, _] : db.buses()) {
        buses[name] = &db.buses().at(name);
    }
    return buses;
}

Canvas::Canvas(const ProtoCatalog::TransportCatalog &db_)
    : db(db_), stops_points(ConvertStops(db_)), buses(ConvertBuses(db_)) {
    funcs.insert(std::make_pair("bus_lines", &Svg::Canvas::RenderBusesRoutes));
    funcs.insert(std::make_pair("bus_labels", &Svg::Canvas::RenderBusesLabels));
    funcs.insert(std::make_pair("stop_points", &Svg::Canvas::RenderStopCircles));
    funcs.insert(std::make_pair("stop_labels", &Svg::Canvas::RenderStopLabels));
    ExtractRect();
    ExtractStopPointSettings();
    ExtractBusLayerSettings();
    ExtractStopLayerSettings();
    ExtractBusLabelTextSettings();
    ExtractStopLabelTextSettings();
    ExtractPolylineSettings();
    drawn_map = DrawMap();
    base_map.Add(rect);
}

void Canvas::ExtractRect() {
    double outer_margin = db.render().outer_margin();
    rect.SetXY({-outer_margin, -outer_margin})
        .SetWidth(db.render().width() + 2 * outer_margin)
        .SetHeight(db.render().height() + 2 * outer_margin)
        .SetFillColor(db.render().underlayer_color().color())
        .SetStrokeColor(NoneColor)
        .SetStrokeWidth(1.0);
}

void Canvas::ExtractStopPointSettings() {
    stop_point_base
        .SetRadius(db.render().stop_radius())
        .SetFillColor("white");
}

void Canvas::ExtractBusLayerSettings() {
    bus_layer_text.SetOffset(ProtoPointToSvgPoint(db.render().bus_label_offset()))
        .SetFontSize(db.render().bus_label_font_size())
        .SetFontFamily("Verdana")
        .SetFontWeight("bold")
        .SetFillColor(db.render().underlayer_color().color())
        .SetStrokeColor(db.render().underlayer_color().color())
        .SetStrokeWidth(db.render().underlayer_width())
        .SetStrokeLineCap("round")
        .SetStrokeLineJoin("round");

}  // namespace Svg

void Canvas::ExtractStopLayerSettings() {
    stop_layer_text.SetOffset(ProtoPointToSvgPoint(db.render().stop_label_offset()))
        .SetFontSize(static_cast<uint32_t>(db.render().stop_label_font_size()))
        .SetFontFamily("Verdana")
        .SetFillColor(db.render().underlayer_color().color())
        .SetStrokeColor(db.render().underlayer_color().color())
        .SetStrokeWidth(db.render().underlayer_width())
        .SetStrokeLineCap("round")
        .SetStrokeLineJoin("round");

}  // namespace Svg

void Canvas::ExtractBusLabelTextSettings() {
    bus_label_text.SetOffset(ProtoPointToSvgPoint(db.render().bus_label_offset()))
        .SetFontSize(db.render().bus_label_font_size())
        .SetFontFamily("Verdana")
        .SetFontWeight("bold");
}

void Canvas::ExtractStopLabelTextSettings() {
    stop_label_text.SetOffset(ProtoPointToSvgPoint(db.render().stop_label_offset()))
        .SetFontSize(db.render().stop_label_font_size())
        .SetFontFamily("Verdana")
        .SetFillColor("black");
}

void Canvas::ExtractPolylineSettings() {
    bus_polyline
        .SetStrokeWidth(db.render().line_width())
        .SetStrokeLineCap("round")
        .SetStrokeLineJoin("round");
}

void Canvas::RenderBusesRoutes(Svg::Document &svg) {
    for (const auto &[name, bus] : buses) {
        Svg::Polyline copy_base = bus_polyline;
        copy_base.SetStrokeColor(db.render().buses_colors().at(name).color());
        size_t b = bus->end_points(0);
        size_t len = bus->end_points(1) + 1;
        for (size_t i = b; i < len; ++i) {
            copy_base.AddPoint(ProtoPointToSvgPoint(db.render().stops_points().at(bus->route(i))));
        }
        for (size_t i = len - 2; !bus->is_rouded() && i >= 0 && i < len; --i) {
            copy_base.AddPoint(ProtoPointToSvgPoint(db.render().stops_points().at(bus->route(i))));
        }
        svg.Add(copy_base);
    }
}

void Canvas::RenderStopCircles(Svg::Document &svg) {
    for (const auto &[name, stop] : stops_points) {
        svg.Add(stop_point_base.SetCenter(ProtoPointToSvgPoint(*stop)));
    }
}

void Canvas::RenderBusesLabels(Svg::Document &svg) {
    for (const auto &[name, bus] : buses) {
        const auto &first_stop = bus->route(bus->end_points(0));
        const auto &last_stop = bus->route(bus->end_points(1));
        svg.Add(bus_layer_text
                    .SetData(name)
                    .SetPoint(ProtoPointToSvgPoint(db.render().stops_points().at(first_stop))));
        svg.Add(bus_label_text
                    .SetData(name)
                    .SetPoint(ProtoPointToSvgPoint(db.render().stops_points().at(first_stop)))
                    .SetFillColor(db.render().buses_colors().at(name).color()));
        if (!bus->is_rouded() && first_stop != last_stop) {
            svg.Add(bus_layer_text.SetPoint(ProtoPointToSvgPoint(db.render().stops_points().at(last_stop))));
            svg.Add(bus_label_text.SetPoint(ProtoPointToSvgPoint(db.render().stops_points().at(last_stop))));
        }
    }
}

void Canvas::RenderStopLabels(Svg::Document &svg) {
    for (const auto &[name, point] : stops_points) {
        svg.Add(stop_layer_text.SetData(name).SetPoint(ProtoPointToSvgPoint(*point)));
        svg.Add(stop_label_text.SetData(name).SetPoint(ProtoPointToSvgPoint(*point)));
    }
}

void Canvas::DrawRouteBusesPolylines(Document &svg, const BusRoutes &data) {
    for (const auto &[bus, route] : data) {
        Svg::Polyline copy_base = bus_polyline;
        copy_base.SetStrokeColor(db.render().buses_colors().at(bus).color());
        for (const auto &stop : route) {
            copy_base.AddPoint(ProtoPointToSvgPoint(db.render().stops_points().at(stop->name())));
        }
        svg.Add(copy_base);
    }
}

void Canvas::DrawRouteBusesLabels(Document &svg, const BusRoutes &data) {
    for (const auto &[bus_name, route] : data) {
        bus_layer_text.SetData(bus_name);
        bus_label_text
            .SetData(bus_name)
            .SetFillColor(db.render().buses_colors().at(bus_name).color());
        const auto& bus = db.buses().at(bus_name); 
        const auto& route_first_name = route[0]->name();
        const auto& route_last_name = route.back()->name();
        const auto &first_name = bus.route(bus.end_points(0));
        const auto &last_name = bus.route(bus.end_points(1));
        if (route_first_name == first_name || route_first_name == last_name) {
            svg.Add(bus_layer_text.SetPoint(ProtoPointToSvgPoint(db.render().stops_points().at(route_first_name))));
            svg.Add(bus_label_text.SetPoint(ProtoPointToSvgPoint(db.render().stops_points().at(route_first_name))));
        }
        if (route_last_name != route_first_name && (route_last_name == first_name || route_last_name == last_name)) {
            svg.Add(bus_layer_text.SetPoint(ProtoPointToSvgPoint(db.render().stops_points().at(route_last_name))));
            svg.Add(bus_label_text.SetPoint(ProtoPointToSvgPoint(db.render().stops_points().at(route_last_name))));
        }
    }
}

void Canvas::DrawRouteStopPoints(Document &svg, const Data &data) {
    for (const auto &[stop, _] : data) {
        svg.Add(stop_point_base.SetCenter(ProtoPointToSvgPoint(db.render().stops_points().at(stop->name()))));
    }
}

void Canvas::DrawRouteStopLabels(Document &svg, const Stops &stops) {
    for (const auto &stop : stops) {
        svg.Add(stop_layer_text.SetData(stop).SetPoint(ProtoPointToSvgPoint(db.render().stops_points().at(stop))));
        svg.Add(stop_label_text.SetData(stop).SetPoint(ProtoPointToSvgPoint(db.render().stops_points().at(stop))));
    }
}

std::string Canvas::DrawRoute(const Stops &stops, const BusRoutes &buses_routes, const Data &data) {
    Document route_map = base_map;
    for (const auto &layer : db.render().layers()) {
        if (layer == "bus_lines") {
            DrawRouteBusesPolylines(route_map, buses_routes);
        } else if (layer == "bus_labels") {
            DrawRouteBusesLabels(route_map, buses_routes);
        } else if (layer == "stop_points") {
            DrawRouteStopPoints(route_map, data);
        } else if (layer == "stop_labels") {
            DrawRouteStopLabels(route_map, stops);
        }
    }
    std::stringstream out;
    route_map.Render(out);
    return out.str();
}

std::string Canvas::DrawMap() {
    for (const auto &layer : db.render().layers()) {
        (this->*funcs.at(layer))(base_map);
    }
    std::ostringstream out;
    base_map.Render(out);
    return out.str();
}
};  // namespace Svg