#pragma once

#include <map>
#include <sstream>
#include <vector>

#include "canvas.h"
#include "json.h"
#include "router.h"
#include "transport_catalog.pb.h"

struct Executor {
    // Graph::Router<Time>& router;
    // const TransportCatalog::TransportGraph& graph;
    // Svg::Canvas& canvas;

    // Executor(
    //     , TransportCatalog::Catalog& map,
    //     const TransportCatalog::TransportGraph& graph, Svg::Canvas& canvas)
    //     : router(router), map(map), graph(graph), canvas(canvas) {
    // }

    ProtoCatalog::TransportCatalog& data;
    Graph::Router& router;
    Svg::Canvas& canvas;

    Executor(ProtoCatalog::TransportCatalog& data, Graph::Router& router, Svg::Canvas& canvas) : data(data), router(router), canvas(canvas) {
    }

    Json::Dict ExecuteBusRequest(const std::string& name) {
        if (data.buses().count(name) == 0) return {{"error_message", Json::Node("not found")}};
        const ProtoCatalog::Bus& bus = data.buses().at(name);
        Json::Dict res;
        res["route_length"] = Json::Node(bus.route_length());
        res["curvature"] = Json::Node(bus.curvature());
        res["stop_count"] = Json::Node(bus.stops_cnt());
        res["unique_stop_count"] = Json::Node(bus.unique_stops_cnt());
        return res;
    }

    Json::Dict ExecuteStopRequest(const std::string& name) {
        if (data.stops().count(name) == 0) return {{"error_message", Json::Node("not found")}};
        const ProtoCatalog::Stop& stop = data.stops().at(name);
        Json::Dict res;
        std::vector<Json::Node> buses;
        buses.reserve(stop.buses_size());
        for (size_t i = 0; i < stop.buses_size(); ++i) {
            buses.emplace_back(stop.buses(i));
        }
        res["buses"] = Json::Node(buses);
        return res;
    }

    Json::Dict LoadWaitEdge(const ProtoCatalog::WaitOrBus edge) {
        Json::Dict res;
        res["type"] = Json::Node("Wait");
        res["time"] = Json::Node(edge.time());
        res["stop_name"] = Json::Node(edge.wait().stop());
        return res;
    }

    Json::Dict LoadBusEdge(const ProtoCatalog::WaitOrBus edge) {
        Json::Dict res;
        res["type"] = Json::Node("Bus");
        res["bus"] = Json::Node(edge.bus().bus());
        res["time"] = Json::Node(edge.time());
        res["span_count"] = Json::Node(edge.bus().span_cnt());
        return res;
    }

    Json::Node LoadEdge(const ProtoCatalog::WaitOrBus edge) {
        if (edge.is_wait_edge()) {
            return LoadWaitEdge(edge);
        }
        return LoadBusEdge(edge);
    }

    Json::Dict ExecuteRouteRequest(const std::string& from, const std::string& to) {
        using namespace TransportCatalog;
        std::optional<Graph::Router::RouteInfo> info = router.BuildRoute(data.graph().vertices().at(from).wait(), data.graph().vertices().at(to).wait());
        if (info == std::nullopt) return {{"error_message", Json::Node("not found")}};
        Json::Dict res;
        res["total_time"] = info->weight;
        std::vector<Json::Node> items;
        Svg::Canvas::Data stops_buses;
        Svg::Canvas::Stops stops;
        Svg::Canvas::BusRoutes buses_routes;
        for (size_t i = 0; i < info->edge_count; ++i) {
            Graph::EdgeId edge_id = router.GetRouteEdge(info->id, i);
            const ProtoCatalog::WaitOrBus& edge = data.graph().edges(edge_id);
            items.push_back(LoadEdge(edge));
            if (edge.is_wait_edge()) {
                stops.push_back(edge.wait().stop());
            } else {
                std::vector<const ProtoCatalog::Stop*> route_stops;
                const auto& bus = data.buses().at(edge.bus().bus());
                for (size_t i = edge.bus().end_points(0); i < edge.bus().end_points(1) + 1; ++i) {
                    route_stops.push_back(&data.stops().at(bus.route(i)));
                    stops_buses.push_back({&data.stops().at(bus.route(i)), &data.buses().at(bus.name())});
                }
                buses_routes.push_back({bus.name(), std::move(route_stops)});
            }
        }
        if (from != to) stops.push_back(to);
        res["map"] = canvas.DrawRoute(stops, buses_routes, stops_buses);
        res["items"] = Json::Node(items);
        router.ReleaseRoute(info->id);
        return res;
    }

    Json::Dict ExecuteMapRequest() {
        Json::Dict res;
        res["map"] = Json::Node(canvas.GetDrawnMap());
        return res;
    }

    std::vector<Json::Node> ExecuteRequests(const std::vector<Json::Node>& requests) {
        std::vector<Json::Node> result;
        result.reserve(requests.size());
        for (const auto& node : requests) {
            const auto& request = node.AsMap();
            Json::Dict dict;
            const auto& type = request.at("type").AsString();
            if (type == "Bus") {
                dict = ExecuteBusRequest(request.at("name").AsString());
            } else if (type == "Stop") {
                dict = ExecuteStopRequest(request.at("name").AsString());
            }
            else if (type == "Route") {
                dict = ExecuteRouteRequest(request.at("from").AsString(), request.at("to").AsString());
            } 
            else if (type == "Map") {
                dict = ExecuteMapRequest();
            }
            dict["request_id"] = Json::Node(request.at("id").AsInt());
            result.push_back(Json::Node(dict));
        }
        return result;
    }
};