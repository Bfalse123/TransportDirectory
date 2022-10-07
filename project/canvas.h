#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "json.h"
#include "svg.h"
#include "transport_catalog.h"

namespace Svg {
class Canvas {
   public:
    struct StopBusPair {
        TransportCatalog::Catalog::Stop* stop;
        TransportCatalog::Catalog::Bus* bus;
    };

    struct BusRoute {
        TransportCatalog::BusName busname;
        std::vector<TransportCatalog::Catalog::Stop*> stops;
    };

    using Stops = std::vector<TransportCatalog::StopName>;
    using BusRoutes = std::vector<BusRoute>;
    using Data = std::vector<StopBusPair>;

    Canvas(const std::map<std::string, Json::Node>& settings, const TransportCatalog::Catalog& db);
    std::string GetDrawnMap() {
        return drawn_map;
    }

    std::string DrawRoute(const Stops& stops, const BusRoutes& buses_routes, const Data& data);

    Document GetBaseMap() {
        return base_map;
    }

   private:
    const TransportCatalog::Catalog& db;
    std::string drawn_map;
    double width, height, padding;
    Circle stop_point_base;
    Text bus_layer_text;
    Text stop_layer_text;
    Text bus_label_text;
    Text stop_label_text;
    Polyline bus_polyline;
    Rect rect;
    Document base_map;
    std::vector<Color> color_palette;
    std::vector<std::string> layers;
    std::map<TransportCatalog::StopName, Point> stops_points;
    std::map<TransportCatalog::BusName, Color> buses_colors;
    std::map<std::string, void (Svg::Canvas::*)(Document& svg)> funcs;

    void ExtractRect(const std::map<std::string, Json::Node>& settings);
    void ExtractStopPointSettings(const std::map<std::string, Json::Node>& settings);
    void ExtractBusLayerSettings(const std::map<std::string, Json::Node>& settings);
    void ExtractStopLayerSettings(const std::map<std::string, Json::Node>& settings);
    void ExtractBusLabelTextSettings(const std::map<std::string, Json::Node>& settings);
    void ExtractStopLabelTextSettings(const std::map<std::string, Json::Node>& settings);
    void ExtractPolylineSettings(const std::map<std::string, Json::Node>& settings);
    void ConstructStopsPoints();
    void ConstructBusesColors();
    std::string DrawMap();

    struct StopWithUniformArrangement {
        double longitude;
        double latitude;
    };

    bool IsNeighbours(const TransportCatalog::Catalog::Stop* stop1, const TransportCatalog::Catalog::Stop* stop2);
    std::map<int32_t, std::vector<const TransportCatalog::Catalog::Stop*>> Glue(std::vector<std::pair<double, const TransportCatalog::Catalog::Stop*>>& sorted);
    std::map<std::string, StopWithUniformArrangement> ComputeUniformArrangements();
    void AddStopsWithNoBuses(std::map<std::string, StopWithUniformArrangement>& uniform_stops);

    void RenderBusesRoutes(Document& svg);
    void RenderStopCircles(Document& svg);
    void RenderStopLabels(Document& svg);
    void RenderBusesLabels(Document& svg);

    void DrawRouteBusesPolylines(Document& svg, const BusRoutes& data);
    void DrawRouteBusesLabels(Document& svg, const BusRoutes& data);
    void DrawRouteStopPoints(Document& svg, const Data& data);
    void DrawRouteStopLabels(Document& svg, const Stops& stops);
};

};  // namespace Svg