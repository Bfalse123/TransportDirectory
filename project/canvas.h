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
    struct MapRenderSettings {
        double width, height;
        double padding;
        Svg::Color underlayer_color;
        double underlayer_width;
    };

    struct StopRenderSettings {
        double stop_radius;
        uint32_t stop_label_font_size;
        Svg::Point stop_label_offset;
        Svg::Color underlayer_color;
        double underlayer_width;
    };

    struct BusRenderSettings {
        double line_width;
        uint32_t bus_label_font_size;
        Svg::Point bus_label_offset;
    };

    Canvas(const std::map<std::string, Json::Node>& settings, const TransportCatalog::Catalog& db);
    std::string Draw();

   private:
    const TransportCatalog::Catalog& db;
    MapRenderSettings map_render_settings;
    StopRenderSettings stop_render_settings;
    BusRenderSettings bus_render_settings;
    std::vector<Svg::Color> color_palette;
    std::map<TransportCatalog::StopName, Svg::Point> stops_points;
    std::map<TransportCatalog::BusName, Svg::Color> buses_colors;
    std::vector<std::string> layers;
    std::map<std::string, void (Svg::Canvas::*)(Svg::Document& svg)> funcs;


    void ExtractMapRenderSettings(const std::map<std::string, Json::Node> &settings);
    void ExtractStopRenderSettings(const std::map<std::string, Json::Node> &settings);
    void ExtractBusRenderSettings(const std::map<std::string, Json::Node> &settings);
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