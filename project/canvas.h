#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "document.h"
#include "json.h"
#include "transport_catalog.h"

namespace Svg {
class Canvas {
   public:
    Canvas(const std::map<std::string, Json::Node>& settings, const TransportCatalog::TransportCatalog& db);
    std::map<TransportCatalog::StopName, Svg::Point> ConstructStopsPoints();
    std::map<TransportCatalog::BusName, Svg::Color> ConstructBusColors();
    std::string Draw();

   private:
    typedef void (Svg::Canvas::*drawing_func)(Svg::Document&);
    const TransportCatalog::TransportCatalog& db;
    double width, height;
    double padding;
    double stop_radius;
    double line_width;
    uint32_t stop_label_font_size;
    uint32_t bus_label_font_size;
    Svg::Point stop_label_offset;
    Svg::Point bus_label_offset;
    Svg::Color underlayer_color;
    double underlayer_width;
    std::vector<Svg::Color> color_palette;
    double width_zoom_coef, height_zoom_coef;
    double zoom_coef;
    std::map<TransportCatalog::StopName, Svg::Point> stops_points;
    std::map<TransportCatalog::BusName, Svg::Color> bus_colors;
    std::vector<std::string> layers;
    std::map<std::string, void (Svg::Canvas::*)(Svg::Document& svg)> funcs;

    void AddBusesRoutes(Svg::Document& svg) {
        for (const auto& [name, bus] : db.GetBuses()) {
            Polyline line = Svg::Polyline{}
                                .SetStrokeColor(bus_colors.at(name))
                                .SetStrokeWidth(line_width)
                                .SetStrokeLineCap("round")
                                .SetStrokeLineJoin("round");
            for (const auto& stop_name : bus.route) {
                line.AddPoint(stops_points.at(stop_name));
            }
            for (auto it = std::next(bus.route.rbegin()); !bus.is_rounded && it != bus.route.rend(); it = std::next(it)) {
                line.AddPoint(stops_points.at(*it));
            }
            svg.Add(line);
        }
    }

    void AddStopCircles(Svg::Document& svg) {
        for (const auto& [name, stop] : stops_points) {
            svg.Add(Circle{}
                        .SetCenter(stop)
                        .SetRadius(stop_radius)
                        .SetFillColor("white"));
        }
    }

    void AddLayerText(Text base, Svg::Document& svg) {
        svg.Add(base
                    .SetFillColor(underlayer_color)
                    .SetStrokeColor(underlayer_color)
                    .SetStrokeWidth(underlayer_width)
                    .SetStrokeLineCap("round")
                    .SetStrokeLineJoin("round"));
    }

    void AddText(Text base, Svg::Document& svg, Svg::Color color) {
        svg.Add(base.SetFillColor(color));
    }

    void AddStopsNames(Svg::Document& svg) {
        Text base = Text{}
                        .SetOffset(stop_label_offset)
                        .SetFontSize(stop_label_font_size)
                        .SetFontFamily("Verdana");
        for (const auto& [name, point] : stops_points) {
            base.SetData(name).SetPoint(point);
            AddLayerText(base, svg);
            AddText(base, svg, "black");
        }
    }

    void AddBusNames(Svg::Document& svg) {
        Text base = Text{}
                        .SetOffset(bus_label_offset)
                        .SetFontSize(bus_label_font_size)
                        .SetFontFamily("Verdana")
                        .SetFontWeight("bold");
        for (const auto& [name, bus] : db.GetBuses()) {
            base.SetData(name);
            const auto& first_stop = bus.route[0];
            const auto& last_stop = bus.route.back();
            base.SetPoint(stops_points.at(first_stop));
            AddLayerText(base, svg);
            AddText(base, svg, bus_colors.at(name));
            if (!bus.is_rounded && first_stop != last_stop) {
                base.SetPoint(stops_points.at(last_stop));
                AddLayerText(base, svg);
                AddText(base, svg, bus_colors.at(name));
            }
        }
    }
};

};  // namespace Svg