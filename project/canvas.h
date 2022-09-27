#pragma once

#include "json.h"
#include "document.h"
#include "transport_catalog.h"

#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace Svg {

    class Canvas {
    public:
        Canvas(const std::map<std::string, Json::Node>& settings, const TransportCatalog::TransportCatalog& db);
        std::map<TransportCatalog::StopName, Svg::Point> ConstructStopsPoints();
        std::string Draw() const;
    private:
        const TransportCatalog::TransportCatalog& db;
        double width, height;
        double padding;
        double stop_radius;
        double line_width;
        uint32_t stop_label_font_size;
        Svg::Point stop_label_offset;
        Svg::Color underlayer_color;
        double underlayer_width;
        std::vector<Svg::Color> color_palette;
        double width_zoom_coef, height_zoom_coef;
        double zoom_coef;
        std::map<TransportCatalog::StopName, Svg::Point> stops_points;

        void AddBusesRoutes(Svg::Document& svg) const {
            size_t cnt_color = 0;
            for (const auto& [name, bus] : db.GetBuses()) {
                Polyline line = Svg::Polyline{}
                    .SetStrokeColor(color_palette[cnt_color])
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
                cnt_color = (cnt_color + 1) % color_palette.size();
            }
        }

        void AddStopCircles(Svg::Document& svg) const {
            for (const auto& [name, stop] : stops_points) {
                svg.Add(Circle{}
                        .SetCenter(stop)
                        .SetRadius(stop_radius)
                        .SetFillColor("white")
                );
            }
        }

        void AddLayerText(Text base, Svg::Document& svg) const {
            svg.Add(base
                    .SetFillColor(underlayer_color)
                    .SetStrokeColor(underlayer_color)
                    .SetStrokeWidth(underlayer_width)
                    .SetStrokeLineCap("round")
                    .SetStrokeLineJoin("round")
            );
        }

        void AddText(Text base, Svg::Document& svg) const {
            svg.Add(base.SetFillColor("black"));
        }

        void AddStopNames(Svg::Document& svg) const {
            for (const auto& [name, _] : stops_points) {
                Text base = Text{}
                            .SetPoint(stops_points.at(name))
                            .SetOffset(stop_label_offset)
                            .SetFontSize(stop_label_font_size)
                            .SetFontFamily("Verdana")
                            .SetData(name);
                AddLayerText(base, svg);
                AddText(base, svg);
            }
        }
    };
};