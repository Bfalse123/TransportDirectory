#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "json.h"
#include "svg.h"
#include "transport_catalog.pb.h"

namespace Svg {
class Canvas {
   public:
    struct StopBusPair {
        const ProtoCatalog::Stop* stop;
        const ProtoCatalog::Bus* bus;
    };

    struct BusRoute {
        std::string busname;
        std::vector<const ProtoCatalog::Stop*> stops;
    };

    using Stops = std::vector<std::string>;
    using BusRoutes = std::vector<BusRoute>;
    using Data = std::vector<StopBusPair>;

    Canvas(const ProtoCatalog::TransportCatalog& db);
    std::string GetDrawnMap() {
        return drawn_map;
    }

    std::string DrawRoute(const Stops& stops, const BusRoutes& buses_routes, const Data& data);

    Document GetBaseMap() {
        return base_map;
    }

   private:
    const ProtoCatalog::TransportCatalog& db;
    std::map<std::string, const ProtoCatalog::Bus*> buses;
    std::map<std::string, const ProtoCatalog::Point*> stops_points;
    std::string drawn_map;
    Circle stop_point_base;
    Text bus_layer_text;
    Text stop_layer_text;
    Text bus_label_text;
    Text stop_label_text;
    Polyline bus_polyline;
    Rect rect;
    Document base_map;
    std::map<std::string, void (Svg::Canvas::*)(Document& svg)> funcs;

    void ExtractRect();
    void ExtractStopPointSettings();
    void ExtractBusLayerSettings();
    void ExtractStopLayerSettings();
    void ExtractBusLabelTextSettings();
    void ExtractStopLabelTextSettings();
    void ExtractPolylineSettings();
    std::string DrawMap();

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