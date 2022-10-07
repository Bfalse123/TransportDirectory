#include "canvas.h"

#include <limits.h>

#include <algorithm>
#include <sstream>
#include <unordered_set>

#include "sphere.h"

namespace Svg {
using namespace TransportCatalog;

Svg::Color ConvertToColor(const Json::Node &node) {
    if (std::holds_alternative<std::string>(node.GetBase())) {
        return node.AsString();
    }
    std::vector<uint8_t> rgb(3);
    for (size_t i = 0; i < 3; ++i) {
        rgb[i] = static_cast<uint8_t>(node.AsArray().at(i).AsInt());
    }
    Rgb color{rgb[0], rgb[1], rgb[2]};
    if (node.AsArray().size() == 4) {
        return Rgba{color, node.AsArray()[3].AsDouble()};
    }
    return color;
}

Svg::Point JsonVectorToSvgPoint(const std::vector<Json::Node> v) {
    return {v[0].AsDouble(), v[1].AsDouble()};
}

Canvas::Canvas(const std::map<std::string, Json::Node> &settings, const Catalog &db)
    : db(db) {
    funcs.insert(std::make_pair("bus_lines", &Svg::Canvas::RenderBusesRoutes));
    funcs.insert(std::make_pair("bus_labels", &Svg::Canvas::RenderBusesLabels));
    funcs.insert(std::make_pair("stop_points", &Svg::Canvas::RenderStopCircles));
    funcs.insert(std::make_pair("stop_labels", &Svg::Canvas::RenderStopLabels));
    width = settings.at("width").AsDouble();
    height = settings.at("height").AsDouble();
    padding = settings.at("padding").AsDouble();
    for (const auto &node : settings.at("layers").AsArray()) {
        layers.push_back(node.AsString());
    }
    for (const auto &node : settings.at("color_palette").AsArray()) {
        color_palette.push_back(ConvertToColor(node));
    }
    ExtractRect(settings);
    ExtractStopPointSettings(settings);
    ExtractBusLayerSettings(settings);
    ExtractStopLayerSettings(settings);
    ExtractBusLabelTextSettings(settings);
    ExtractStopLabelTextSettings(settings);
    ExtractPolylineSettings(settings);
    ConstructStopsPoints();
    ConstructBusesColors();
    drawn_map = DrawMap();
    base_map.Add(rect);
}

void Canvas::ExtractRect(const std::map<std::string, Json::Node> &settings) {
    double outer_margin = settings.at("outer_margin").AsDouble();
    rect.SetXY({-outer_margin, -outer_margin})
        .SetWidth(width + 2 * outer_margin)
        .SetHeight(height + 2 * outer_margin)
        .SetFillColor(ConvertToColor(settings.at("underlayer_color")))
        .SetStrokeColor(NoneColor)
        .SetStrokeWidth(1.0);
}

void Canvas::ExtractStopPointSettings(const std::map<std::string, Json::Node> &settings) {
    stop_point_base
        .SetRadius(settings.at("stop_radius").AsDouble())
        .SetFillColor("white");
}

void Canvas::ExtractBusLayerSettings(const std::map<std::string, Json::Node> &settings) {
    bus_layer_text.SetOffset(JsonVectorToSvgPoint(settings.at("bus_label_offset").AsArray()))
        .SetFontSize(static_cast<uint32_t>(settings.at("bus_label_font_size").AsInt()))
        .SetFontFamily("Verdana")
        .SetFontWeight("bold")
        .SetFillColor(ConvertToColor(settings.at("underlayer_color")))
        .SetStrokeColor(ConvertToColor(settings.at("underlayer_color")))
        .SetStrokeWidth(settings.at("underlayer_width").AsDouble())
        .SetStrokeLineCap("round")
        .SetStrokeLineJoin("round");

}  // namespace Svg

void Canvas::ExtractStopLayerSettings(const std::map<std::string, Json::Node> &settings) {
    stop_layer_text.SetOffset(JsonVectorToSvgPoint(settings.at("stop_label_offset").AsArray()))
        .SetFontSize(static_cast<uint32_t>(settings.at("stop_label_font_size").AsInt()))
        .SetFontFamily("Verdana")
        .SetFillColor(ConvertToColor(settings.at("underlayer_color")))
        .SetStrokeColor(ConvertToColor(settings.at("underlayer_color")))
        .SetStrokeWidth(settings.at("underlayer_width").AsDouble())
        .SetStrokeLineCap("round")
        .SetStrokeLineJoin("round");

}  // namespace Svg

void Canvas::ExtractBusLabelTextSettings(const std::map<std::string, Json::Node> &settings) {
    bus_label_text.SetOffset(JsonVectorToSvgPoint(settings.at("bus_label_offset").AsArray()))
        .SetFontSize(static_cast<uint32_t>(settings.at("bus_label_font_size").AsInt()))
        .SetFontFamily("Verdana")
        .SetFontWeight("bold");
}

void Canvas::ExtractStopLabelTextSettings(const std::map<std::string, Json::Node> &settings) {
    stop_label_text.SetOffset(JsonVectorToSvgPoint(settings.at("stop_label_offset").AsArray()))
        .SetFontSize(static_cast<uint32_t>(settings.at("stop_label_font_size").AsInt()))
        .SetFontFamily("Verdana")
        .SetFillColor("black");
}

void Canvas::ExtractPolylineSettings(const std::map<std::string, Json::Node> &settings) {
    bus_polyline
        .SetStrokeWidth(settings.at("line_width").AsDouble())
        .SetStrokeLineCap("round")
        .SetStrokeLineJoin("round");
}

bool Canvas::IsNeighbours(const Catalog::Stop *stop1, const Catalog::Stop *stop2) {
    for (const auto &[bus, positions] : stop1->pos_in_routes) {
        if (stop2->pos_in_routes.count(bus)) {
            for (const auto &pos1 : positions) {
                for (const auto &pos2 : stop2->pos_in_routes.at(bus)) {
                    if (std::abs(int(pos1) - int(pos2)) == 1) return true;
                }
            }
        }
    }
    return false;
}

std::map<int32_t, std::vector<const Catalog::Stop *>> Canvas::Glue(std::vector<std::pair<double, const Catalog::Stop *>> &sorted) {
    std::map<int32_t, std::vector<const Catalog::Stop *>> glued;
    std::unordered_map<std::string, int32_t> met;
    for (const auto &[_, stop] : sorted) {
        int32_t insert = -1;
        for (const auto &[prev_name, id] : met) {
            if (IsNeighbours(stop, &db.GetStop(prev_name))) {
                insert = std::max(insert, id);
            }
        }
        glued[++insert].push_back(stop);
        met[stop->name] = insert;
    }
    return glued;
}

std::map<std::string, Canvas::StopWithUniformArrangement> Canvas::ComputeUniformArrangements() {
    std::map<std::string, StopWithUniformArrangement> uniform_stops;
    for (const auto &[name, bus] : db.GetBuses()) {
        size_t i = 0;
        const auto &route = bus.route;
        for (size_t j = 1; j < route.size(); ++j) {
            Catalog::Stop *stop = route[j];
            if (j == 0 || j == route.size() - 1 || stop->pos_in_routes.size() > 1 || (stop->pos_in_routes[name].size() * (bus.is_rounded ? 1 : 2)) > 2) {
                double lon_step = (stop->geo_pos.longitude - route[i]->geo_pos.longitude) / (j - i);
                double lat_step = (stop->geo_pos.latitude - route[i]->geo_pos.latitude) / (j - i);
                stop = route[i];
                for (size_t k = i; k < j; ++k) {
                    uniform_stops[route[k]->name] = {
                        .longitude = stop->geo_pos.longitude + lon_step * (k - i),
                        .latitude = stop->geo_pos.latitude + lat_step * (k - i)};
                }
                stop = route[j];
                uniform_stops[stop->name] = {
                    .longitude = stop->geo_pos.longitude,
                    .latitude = stop->geo_pos.latitude};
                i = j;
            }
        }
    }
    return uniform_stops;
}

void Canvas::AddStopsWithNoBuses(std::map<std::string, StopWithUniformArrangement> &uniform_stops) {
    for (const auto &[name, stop] : db.GetStops()) {
        if (stop.pos_in_routes.size() == 0) {
            uniform_stops[name] = {
                stop.geo_pos.longitude,
                stop.geo_pos.latitude};
        }
    }
}

void Canvas::ConstructStopsPoints() {
    if (db.StopsCount() == 0) return;
    auto uniform_stops = ComputeUniformArrangements();
    AddStopsWithNoBuses(uniform_stops);
    std::vector<std::pair<double, const Catalog::Stop *>> lon_sorted, lat_sorted;
    bool first = true;
    for (const auto &[name, stop] : db.GetStops()) {
        lon_sorted.push_back({uniform_stops[name].longitude, &stop});
        lat_sorted.push_back({uniform_stops[name].latitude, &stop});
    }
    auto compare_by_double = [](const auto &lhs, const auto &rhs) {
        return lhs.first < rhs.first;
    };
    std::sort(lon_sorted.begin(), lon_sorted.end(), compare_by_double);
    std::sort(lat_sorted.begin(), lat_sorted.end(), compare_by_double);
    auto glued_by_lon = Glue(lon_sorted);
    auto glued_by_lat = Glue(lat_sorted);
    double x_step = glued_by_lon.size() - 1 ? (width - 2 * padding) / (glued_by_lon.size() - 1) : 0.0;
    double y_step = glued_by_lat.size() - 1 ? (height - 2 * padding) / (glued_by_lat.size() - 1) : 0.0;
    for (const auto &[idx, stops] : glued_by_lon) {
        for (const auto &stop : stops) {
            stops_points[stop->name].x = idx * x_step + padding;
        }
    }
    for (const auto &[idy, stops] : glued_by_lat) {
        for (const auto &stop : stops) {
            stops_points[stop->name].y = height - padding - idy * y_step;
        }
    }
}

void Canvas::RenderBusesRoutes(Svg::Document &svg) {
    for (const auto &[name, bus] : db.GetBuses()) {
        Svg::Polyline copy_base = bus_polyline;
        copy_base.SetStrokeColor(buses_colors[name]);
        for (const auto &stop : bus.route) {
            copy_base.AddPoint(stops_points.at(stop->name));
        }
        for (auto it = std::next(bus.route.rbegin()); !bus.is_rounded && it != bus.route.rend(); it = std::next(it)) {
            copy_base.AddPoint(stops_points.at((*it)->name));
        }
        svg.Add(copy_base);
    }
}

void Canvas::RenderStopCircles(Svg::Document &svg) {
    for (const auto &[name, stop] : stops_points) {
        svg.Add(stop_point_base.SetCenter(stop));
    }
}

void Canvas::RenderBusesLabels(Svg::Document &svg) {
    for (const auto &[name, bus] : db.GetBuses()) {
        const auto &first_stop = bus.route[0];
        const auto &last_stop = bus.route.back();
        svg.Add(bus_layer_text
                    .SetData(name)
                    .SetPoint(stops_points.at(first_stop->name)));
        svg.Add(bus_label_text
                    .SetData(name)
                    .SetPoint(stops_points.at(first_stop->name))
                    .SetFillColor(buses_colors[name]));
        if (!bus.is_rounded && first_stop->name != last_stop->name) {
            svg.Add(bus_layer_text.SetPoint(stops_points.at(last_stop->name)));
            svg.Add(bus_label_text.SetPoint(stops_points.at(last_stop->name)));
        }
    }
}

void Canvas::RenderStopLabels(Svg::Document &svg) {
    for (const auto &[name, point] : stops_points) {
        svg.Add(stop_layer_text.SetData(name).SetPoint(point));
        svg.Add(stop_label_text.SetData(name).SetPoint(point));
    }
}

void Canvas::ConstructBusesColors() {
    size_t cnt_color = 0;
    for (const auto &[name, _] : db.GetBuses()) {
        buses_colors[name] = color_palette[cnt_color];
        cnt_color = (cnt_color + 1) % color_palette.size();
    }
}

void Canvas::DrawRouteBusesPolylines(Document &svg, const BusRoutes &data) {
    for (const auto &[bus, route] : data) {
        Svg::Polyline copy_base = bus_polyline;
        copy_base.SetStrokeColor(buses_colors.at(bus));
        for (const auto &stop : route) {
            copy_base.AddPoint(stops_points.at(stop->name));
        }
        svg.Add(copy_base);
    }
}

void Canvas::DrawRouteBusesLabels(Document &svg, const BusRoutes &data) {
    for (const auto &[bus, route] : data) {
        bus_layer_text.SetData(bus);
        bus_label_text
            .SetData(bus)
            .SetFillColor(buses_colors[bus]);
        const auto &first_stop = db.GetBus(bus).route[0];
        const auto &last_stop = db.GetBus(bus).route.back();
        if (route[0]->name == first_stop->name || route[0]->name == last_stop->name) {
            svg.Add(bus_layer_text.SetPoint(stops_points[route[0]->name]));
            svg.Add(bus_label_text.SetPoint(stops_points[route[0]->name]));
        }
        if (route.back()->name != route[0]->name && (route.back()->name == first_stop->name || route.back()->name == last_stop->name)) {
            svg.Add(bus_layer_text.SetPoint(stops_points[route.back()->name]));
            svg.Add(bus_label_text.SetPoint(stops_points[route.back()->name]));
        }
    }
}

void Canvas::DrawRouteStopPoints(Document &svg, const Data &data) {
    for (const auto &[stop, _] : data) {
        svg.Add(stop_point_base.SetCenter(stops_points[stop->name]));
    }
}

void Canvas::DrawRouteStopLabels(Document &svg, const Stops &stops) {
    for (const auto &stop : stops) {
        svg.Add(stop_layer_text.SetData(stop).SetPoint(stops_points[stop]));
        svg.Add(stop_label_text.SetData(stop).SetPoint(stops_points[stop]));
    }
}

std::string Canvas::DrawRoute(const Stops &stops, const BusRoutes &buses_routes, const Data &data) {
    Document route_map = base_map;
    for (const auto &layer : layers) {
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
    for (const auto &layer : layers) {
        (this->*funcs.at(layer))(base_map);
    }
    std::ostringstream out;
    base_map.Render(out);
    return out.str();
}
};  // namespace Svg