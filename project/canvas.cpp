#include "canvas.h"
#include "sphere.h"

#include <limits.h>
#include <numeric>
#include <sstream>
#include <cmath>
#include <iomanip>

namespace Svg {

    Svg::Color json_to_color(const Json::Node& node) {
        if (std::holds_alternative<std::string>(node.GetBase())) {
            return node.AsString();
        }
        std::vector<uint8_t> rgb(3);
        for (size_t i = 0; i < 3; ++i) {
            rgb[i] = static_cast<uint8_t>(node.AsArray().at(i).AsInt());
        }
        Rgb color { rgb[0], rgb[1], rgb[2] };
        if (node.AsArray().size() == 4) {
            return Rgba{color, node.AsArray()[3].AsDouble()};
        }
        return color;
    }

    Canvas::Canvas(const std::map<std::string, Json::Node>& settings, const TransportCatalog::TransportCatalog& db) : db(db) {
        width = settings.at("width").AsDouble();
        height = settings.at("height").AsDouble();
        padding = settings.at("padding").AsDouble();
        stop_radius = settings.at("stop_radius").AsDouble();
        line_width = settings.at("line_width").AsDouble();
        stop_label_font_size = static_cast<uint32_t>(settings.at("stop_label_font_size").AsInt());
        std::vector<Json::Node> offset_point = settings.at("stop_label_offset").AsArray();
        stop_label_offset = {offset_point[0].AsDouble(), offset_point[1].AsDouble()};
        underlayer_width = settings.at("underlayer_width").AsDouble();
        underlayer_color = json_to_color(settings.at("underlayer_color"));
        for (const auto& node : settings.at("color_palette").AsArray()) {
            color_palette.push_back(json_to_color(node));
        }
        stops_points = ConstructStopsPoints();
    }

    std::map<TransportCatalog::StopName, Svg::Point> Canvas::ConstructStopsPoints() {
        if (db.StopsCount() == 0) return {};
        const auto& stops = db.GetStops();
        double e = std::numeric_limits<double>().epsilon();
        double min_lat = stops.begin()->second.geo_pos.latitude;
        double max_lat = stops.begin()->second.geo_pos.latitude;
        double min_lon = stops.begin()->second.geo_pos.longitude;
        double max_lon = stops.begin()->second.geo_pos.longitude;
        for (const auto& [name, stop] : db.GetStops()) {
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

        if (std::isinf(width_zoom_coef) && std::isinf(height_zoom_coef)) zoom_coef = 0;
        else if (std::isinf(width_zoom_coef) || width_zoom_coef - height_zoom_coef > e) {
            zoom_coef = height_zoom_coef;
        }
        else {
            zoom_coef = width_zoom_coef;
        }
        std::map<TransportCatalog::StopName, Svg::Point> points;
        for (const auto& [name, stop] : stops) {
            double x = (stop.geo_pos.longitude - min_lon) * zoom_coef + padding;
            double y = (max_lat - stop.geo_pos.latitude) * zoom_coef + padding;
            points[name] = {x, y};
        }
        return points;
    }

    std::string Canvas::Draw() const {
        Svg::Document svg;
        AddBusesRoutes(svg);
        AddStopCircles(svg);
        AddStopNames(svg);
        std::ostringstream out;
        out << std::setprecision(16);
        svg.Render(out);
        return out.str();
    }

};