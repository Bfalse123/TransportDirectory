#pragma once

#include "json.h"
#include "svg.h"
#include "transport_catalog.h"

namespace Svg {
struct RenderSettings {
    double max_width;
    double max_height;
    double padding;
    double outer_margin;
    std::vector<Svg::Color> palette;
    double line_width;
    Svg::Color underlayer_color;
    double underlayer_width;
    double stop_radius;
    Svg::Point bus_label_offset;
    int bus_label_font_size;
    Svg::Point stop_label_offset;
    int stop_label_font_size;
    std::vector<std::string> layers;
};

struct RenderBuilder {
    RenderBuilder(const TransportCatalog::Catalog& db, const Json::Dict& settings);
    RenderSettings settings_;
    const TransportCatalog::Catalog& db;
    std::map<TransportCatalog::StopName, Svg::Point> stops_points;
    std::map<TransportCatalog::BusName, Svg::Color> buses_colors;
};
}  // namespace Svg