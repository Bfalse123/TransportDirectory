#include "canvas.h"

#include <limits.h>

#include <sstream>
#include <algorithm>
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

void Canvas::ExtractMapRenderSettings(const std::map<std::string, Json::Node> &settings) {
    map_render_settings = {
        .width = settings.at("width").AsDouble(),
        .height = settings.at("height").AsDouble(),
        .padding = settings.at("padding").AsDouble(),
        .underlayer_color = ConvertToColor(settings.at("underlayer_color")),
        .underlayer_width = settings.at("underlayer_width").AsDouble()};
}  // namespace Svg

Svg::Point JsonVectorToSvgPoint(const std::vector<Json::Node> v) {
    return {v[0].AsDouble(), v[1].AsDouble()};
}

void Canvas::ExtractStopRenderSettings(const std::map<std::string, Json::Node> &settings) {
    stop_render_settings = {
        .stop_radius = settings.at("stop_radius").AsDouble(),
        .stop_label_font_size = static_cast<uint32_t>(settings.at("stop_label_font_size").AsInt()),
        .stop_label_offset = JsonVectorToSvgPoint(settings.at("stop_label_offset").AsArray())};
}  // namespace Svg

void Canvas::ExtractBusRenderSettings(const std::map<std::string, Json::Node> &settings) {
    bus_render_settings = {
        .line_width = settings.at("line_width").AsDouble(),
        .bus_label_font_size = static_cast<uint32_t>(settings.at("bus_label_font_size").AsInt()),
        .bus_label_offset = JsonVectorToSvgPoint(settings.at("bus_label_offset").AsArray())};
}

Canvas::Canvas(const std::map<std::string, Json::Node> &settings, const Catalog &db)
    : db(db) {
    funcs.insert(std::make_pair("bus_lines", &Svg::Canvas::RenderBusesRoutes));
    funcs.insert(std::make_pair("bus_labels", &Svg::Canvas::RenderBusesLabels));
    funcs.insert(std::make_pair("stop_points", &Svg::Canvas::RenderStopCircles));
    funcs.insert(std::make_pair("stop_labels", &Svg::Canvas::RenderStopLabels));
    ExtractMapRenderSettings(settings);
    ExtractStopRenderSettings(settings);
    ExtractBusRenderSettings(settings);
    for (const auto &node : settings.at("layers").AsArray()) {
        layers.push_back(node.AsString());
    }
    for (const auto &node : settings.at("color_palette").AsArray()) {
        color_palette.push_back(ConvertToColor(node));
    }
    ConstructStopsPoints();
    ConstructBusesColors();
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
    auto compare_by_double = [](const auto& lhs, const auto& rhs){
        return lhs.first < rhs.first;
    };
    std::sort(lon_sorted.begin(), lon_sorted.end(), compare_by_double);
    std::sort(lat_sorted.begin(), lat_sorted.end(), compare_by_double);
    auto glued_by_lon = Glue(lon_sorted);
    auto glued_by_lat = Glue(lat_sorted);
    double padding = map_render_settings.padding;
    double width = map_render_settings.width;
    double height = map_render_settings.height;
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
        Polyline line = Svg::Polyline{}
                            .SetStrokeColor(buses_colors.at(name))
                            .SetStrokeWidth(bus_render_settings.line_width)
                            .SetStrokeLineCap("round")
                            .SetStrokeLineJoin("round");
        for (const auto &stop : bus.route) {
            line.AddPoint(stops_points.at(stop->name));
        }
        for (auto it = std::next(bus.route.rbegin()); !bus.is_rounded && it != bus.route.rend(); it = std::next(it)) {
            line.AddPoint(stops_points.at((*it)->name));
        }
        svg.Add(line);
    }
}

void Canvas::RenderStopCircles(Svg::Document &svg) {
    for (const auto &[name, stop] : stops_points) {
        svg.Add(Circle{}
                    .SetCenter(stop)
                    .SetRadius(stop_render_settings.stop_radius)
                    .SetFillColor("white"));
    }
}

void Canvas::RenderLayerText(Text base, Svg::Document &svg) {
    svg.Add(base
                .SetFillColor(map_render_settings.underlayer_color)
                .SetStrokeColor(map_render_settings.underlayer_color)
                .SetStrokeWidth(map_render_settings.underlayer_width)
                .SetStrokeLineCap("round")
                .SetStrokeLineJoin("round"));
}

void Canvas::RenderBusesLabels(Svg::Document &svg) {
    Text base = Text{}
                    .SetOffset(bus_render_settings.bus_label_offset)
                    .SetFontSize(bus_render_settings.bus_label_font_size)
                    .SetFontFamily("Verdana")
                    .SetFontWeight("bold");
    for (const auto &[name, bus] : db.GetBuses()) {
        base.SetData(name);
        const auto &first_stop = bus.route[0];
        const auto &last_stop = bus.route.back();
        base.SetPoint(stops_points.at(first_stop->name));
        RenderLayerText(base, svg);
        RenderText(base, svg, buses_colors.at(name));
        if (!bus.is_rounded && first_stop != last_stop) {
            base.SetPoint(stops_points.at(last_stop->name));
            RenderLayerText(base, svg);
            RenderText(base, svg, buses_colors.at(name));
        }
    }
}

void Canvas::RenderStopLabels(Svg::Document &svg) {
    Text base = Text{}
                    .SetOffset(stop_render_settings.stop_label_offset)
                    .SetFontSize(stop_render_settings.stop_label_font_size)
                    .SetFontFamily("Verdana");
    for (const auto &[name, point] : stops_points) {
        base.SetData(name).SetPoint(point);
        RenderLayerText(base, svg);
        RenderText(base, svg, "black");
    }
}

void Canvas::RenderText(Text base, Svg::Document &svg, Svg::Color color) {
    svg.Add(base.SetFillColor(color));
}

void Canvas::ConstructBusesColors() {
    size_t cnt_color = 0;
    std::map<BusName, Svg::Color> colors;
    for (const auto &[name, _] : db.GetBuses()) {
        buses_colors[name] = color_palette[cnt_color];
        cnt_color = (cnt_color + 1) % color_palette.size();
    }
}

std::string Canvas::Draw() {
    Svg::Document svg;
    for (const auto &layer : layers) {
        (this->*funcs.at(layer))(svg);
    }
    std::ostringstream out;
    svg.Render(out);
    return out.str();
}
};  // namespace Svg