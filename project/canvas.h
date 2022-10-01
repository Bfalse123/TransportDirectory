#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.h"
#include "svg.h"
#include "transport_catalog.h"

namespace Svg {
class Canvas {
   public:
    Canvas(const std::map<std::string, Json::Node>& settings, const TransportCatalog::Catalog& db);
    std::string Draw();

   private:
    typedef void (Svg::Canvas::*drawing_func)(Svg::Document&);
    const TransportCatalog::Catalog& db;
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
    std::map<TransportCatalog::BusName, Svg::Color> buses_colors;
    std::vector<std::string> layers;
    std::map<std::string, void (Svg::Canvas::*)(Svg::Document& svg)> funcs;

    void ConstructStopsPoints();
    void ConstructBusesColors();

    void RenderBusesRoutes(Svg::Document& svg);
    void RenderStopCircles(Svg::Document& svg);
    void RenderLayerText(Text base, Svg::Document& svg);
    void RenderText(Text base, Svg::Document& svg, Svg::Color color);
    void RenderStopLabels(Svg::Document& svg);
    void RenderBusesLabels(Svg::Document& svg);
};

};  // namespace Svg