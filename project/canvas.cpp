#include "canvas.h"

#include <limits.h>

#include <sstream>
#include <unordered_set>

#include "sphere.h"

namespace Svg {
using namespace TransportCatalog;

Svg::Color json_to_color(const Json::Node &node) {
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

Canvas::Canvas(const std::map<std::string, Json::Node> &settings, const Catalog &db)
    : db(db) {
    funcs.insert(std::make_pair("bus_lines", &Svg::Canvas::RenderBusesRoutes));
    funcs.insert(std::make_pair("bus_labels", &Svg::Canvas::RenderBusesLabels));
    funcs.insert(std::make_pair("stop_points", &Svg::Canvas::RenderStopCircles));
    funcs.insert(std::make_pair("stop_labels", &Svg::Canvas::RenderStopLabels));

    width = settings.at("width").AsDouble();
    height = settings.at("height").AsDouble();
    padding = settings.at("padding").AsDouble();
    stop_radius = settings.at("stop_radius").AsDouble();
    line_width = settings.at("line_width").AsDouble();
    stop_label_font_size = static_cast<uint32_t>(settings.at("stop_label_font_size").AsInt());
    bus_label_font_size = static_cast<uint32_t>(settings.at("bus_label_font_size").AsInt());
    auto offset_point = settings.at("stop_label_offset").AsArray();
    stop_label_offset = {offset_point[0].AsDouble(), offset_point[1].AsDouble()};
    auto bus_offset_point = settings.at("bus_label_offset").AsArray();
    bus_label_offset = {bus_offset_point[0].AsDouble(), bus_offset_point[1].AsDouble()};
    underlayer_width = settings.at("underlayer_width").AsDouble();
    underlayer_color = json_to_color(settings.at("underlayer_color"));
    for (const auto &node : settings.at("layers").AsArray()) {
        layers.push_back(node.AsString());
    }
    for (const auto &node : settings.at("color_palette").AsArray()) {
        color_palette.push_back(json_to_color(node));
    }
    ConstructStopsPoints();
    ConstructBusesColors();
}

bool IsNeighbours(const Catalog::Stop *stop1, const Catalog::Stop *stop2) {
    for (const auto &[bus, positions] : stop1->buses_positions) {
        if (stop2->buses_positions.count(bus)) {
            for (const auto &pos1 : positions) {
                for (const auto &pos2 : stop2->buses_positions.at(bus)) {
                    if (std::abs(int(pos1) - int(pos2)) == 1) return true;
                }
            }
        }
    }
    return false;
}

auto Glue(const std::vector<std::pair<double, const Catalog::Stop *>> &sorted) {
    std::vector<std::pair<double, std::vector<const Catalog::Stop *>>> glued;
    bool first = true;
    for (const auto &[val, stop] : sorted) {
        bool glue = true;
        if (!first) {
            for (const auto &prev : glued.back().second) {
                if(IsNeighbours(stop, prev)) {
                    glue = false;
                    break;
                }
            }
        }
        if (glue && !first) {
            glued.back().second.push_back(stop);
        } else {
            glued.push_back({val, {stop}});
        }
        first = false;
    }
    return glued;
}

void Canvas::ConstructStopsPoints() {
    if (db.StopsCount() == 0) return;
    const auto &stops = db.GetStops();
    double e = std::numeric_limits<double>().epsilon();
    std::vector<std::pair<double, const Catalog::Stop *>> lon_sorted, lat_sorted;
    lon_sorted.reserve(stops.size());
    lat_sorted.reserve(stops.size());
    bool first = true;
    for (const auto &[name, stop] : db.GetStops()) {
        lon_sorted.emplace_back(stop.geo_pos.longitude, &stop);
        lat_sorted.emplace_back(stop.geo_pos.latitude, &stop);
    }
    sort(lon_sorted.begin(), lon_sorted.end());
    sort(lat_sorted.begin(), lat_sorted.end());
    auto glued_by_lon = Glue(lon_sorted);
    auto glued_by_lat = Glue(lat_sorted);
    double x_step = glued_by_lon.size() - 1 ? (width - 2 * padding) / (glued_by_lon.size() - 1) : 0.0;
    double y_step = glued_by_lat.size() - 1 ? (height - 2 * padding) / (glued_by_lat.size() - 1) : 0.0;
    for (size_t i = 0; i < glued_by_lon.size(); ++i) {
        for (const auto &stop : glued_by_lon[i].second) {
            stops_points[stop->name].x = i * x_step + padding;
        }
    }
    for (size_t i = 0; i < glued_by_lat.size(); ++i) {
        for (const auto &stop : glued_by_lat[i].second) {
            stops_points[stop->name].y = height - padding - i * y_step;
        }
    }
}

void Canvas::RenderBusesRoutes(Svg::Document &svg) {
    for (const auto &[name, bus] : db.GetBuses()) {
        Polyline line = Svg::Polyline{}
                            .SetStrokeColor(buses_colors.at(name))
                            .SetStrokeWidth(line_width)
                            .SetStrokeLineCap("round")
                            .SetStrokeLineJoin("round");
        for (const auto &stop_name : bus.route) {
            line.AddPoint(stops_points.at(stop_name));
        }
        for (auto it = std::next(bus.route.rbegin()); !bus.is_rounded && it != bus.route.rend(); it = std::next(it)) {
            line.AddPoint(stops_points.at(*it));
        }
        svg.Add(line);
    }
}

void Canvas::RenderStopCircles(Svg::Document &svg) {
    for (const auto &[name, stop] : stops_points) {
        svg.Add(Circle{}
                    .SetCenter(stop)
                    .SetRadius(stop_radius)
                    .SetFillColor("white"));
    }
}

void Canvas::RenderLayerText(Text base, Svg::Document &svg) {
    svg.Add(base
                .SetFillColor(underlayer_color)
                .SetStrokeColor(underlayer_color)
                .SetStrokeWidth(underlayer_width)
                .SetStrokeLineCap("round")
                .SetStrokeLineJoin("round"));
}

void Canvas::RenderBusesLabels(Svg::Document &svg) {
    Text base = Text{}
                    .SetOffset(bus_label_offset)
                    .SetFontSize(bus_label_font_size)
                    .SetFontFamily("Verdana")
                    .SetFontWeight("bold");
    for (const auto &[name, bus] : db.GetBuses()) {
        base.SetData(name);
        const auto &first_stop = bus.route[0];
        const auto &last_stop = bus.route.back();
        base.SetPoint(stops_points.at(first_stop));
        RenderLayerText(base, svg);
        RenderText(base, svg, buses_colors.at(name));
        if (!bus.is_rounded && first_stop != last_stop) {
            base.SetPoint(stops_points.at(last_stop));
            RenderLayerText(base, svg);
            RenderText(base, svg, buses_colors.at(name));
        }
    }
}

void Canvas::RenderStopLabels(Svg::Document &svg) {
    Text base = Text{}
                    .SetOffset(stop_label_offset)
                    .SetFontSize(stop_label_font_size)
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