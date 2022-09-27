#include "canvas.h"

#include <limits.h>

#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "sphere.h"

namespace Svg {

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

Canvas::Canvas(const std::map<std::string, Json::Node> &settings, const TransportCatalog::TransportCatalog &db)
    : db(db) {
    funcs.insert(std::make_pair("bus_lines", &Svg::Canvas::AddBusesRoutes));
    funcs.insert(std::make_pair("bus_labels", &Svg::Canvas::AddBusNames));
    funcs.insert(std::make_pair("stop_points", &Svg::Canvas::AddStopCircles));
    funcs.insert(std::make_pair("stop_labels", &Svg::Canvas::AddStopsNames));
    width = settings.at("width").AsDouble();
    height = settings.at("height").AsDouble();
    padding = settings.at("padding").AsDouble();
    stop_radius = settings.at("stop_radius").AsDouble();
    line_width = settings.at("line_width").AsDouble();
    stop_label_font_size = static_cast<uint32_t>(settings.at("stop_label_font_size").AsInt());
    bus_label_font_size = static_cast<uint32_t>(settings.at("bus_label_font_size").AsInt());
    std::vector<Json::Node> offset_point = settings.at("stop_label_offset").AsArray();
    stop_label_offset = {offset_point[0].AsDouble(), offset_point[1].AsDouble()};
    std::vector<Json::Node> bus_offset_point = settings.at("bus_label_offset").AsArray();
    bus_label_offset = {bus_offset_point[0].AsDouble(), bus_offset_point[1].AsDouble()};
    underlayer_width = settings.at("underlayer_width").AsDouble();
    underlayer_color = json_to_color(settings.at("underlayer_color"));
    for (const auto &node : settings.at("layers").AsArray()) {
        layers.push_back(node.AsString());
    }
    for (const auto &node : settings.at("color_palette").AsArray()) {
        color_palette.push_back(json_to_color(node));
    }
    stops_points = ConstructStopsPoints();
    bus_colors = ConstructBusColors();
}

std::map<TransportCatalog::StopName, Svg::Point> Canvas::ConstructStopsPoints() {
    if (db.StopsCount() == 0)
        return {};
    const auto &stops = db.GetStops();
    double e = std::numeric_limits<double>().epsilon();
    double min_lat = stops.begin()->second.geo_pos.latitude;
    double max_lat = stops.begin()->second.geo_pos.latitude;
    double min_lon = stops.begin()->second.geo_pos.longitude;
    double max_lon = stops.begin()->second.geo_pos.longitude;
    for (const auto &[name, stop] : db.GetStops()) {
        if (min_lat - stop.geo_pos.latitude >= e) {
            min_lat = stop.geo_pos.latitude;
        }
        if (stop.geo_pos.latitude - max_lat >= e) {
            max_lat = stop.geo_pos.latitude;
        }
        if (min_lon - stop.geo_pos.longitude >= e) {
            min_lon = stop.geo_pos.longitude;
        }
        if (stop.geo_pos.longitude - max_lon >= e) {
            max_lon = stop.geo_pos.longitude;
        }
    }
    width_zoom_coef = (width - 2 * padding) / (max_lon - min_lon);
    height_zoom_coef = (height - 2 * padding) / (max_lat - min_lat);

    if (std::isinf(width_zoom_coef) && std::isinf(height_zoom_coef))
        zoom_coef = 0;
    else if (std::isinf(width_zoom_coef) || width_zoom_coef - height_zoom_coef > e) {
        zoom_coef = height_zoom_coef;
    } else {
        zoom_coef = width_zoom_coef;
    }
    std::map<TransportCatalog::StopName, Svg::Point> points;
    for (const auto &[name, stop] : stops) {
        double x = (stop.geo_pos.longitude - min_lon) * zoom_coef + padding;
        double y = (max_lat - stop.geo_pos.latitude) * zoom_coef + padding;
        points[name] = {x, y};
    }
    return points;
}

std::map<TransportCatalog::BusName, Svg::Color> Canvas::ConstructBusColors() {
    size_t cnt_color = 0;
    std::map<TransportCatalog::BusName, Svg::Color> colors;
    for (const auto &[name, _] : db.GetBuses()) {
        colors[name] = color_palette[cnt_color];
        cnt_color = (cnt_color + 1) % color_palette.size();
    }
    return colors;
}

std::string Canvas::Draw() {
    Svg::Document svg;
    for (const auto &layer : layers) {
        (this->*funcs.at(layer))(svg);
    }
    std::ostringstream out;
    out << std::setprecision(16);
    svg.Render(out);
    return out.str();
}

};  // namespace Svg