#pragma once

#include <map>
#include <sstream>
#include <vector>

#include "canvas.h"
#include "json.h"
#include "router.h"
#include "transport_catalog.h"
#include "transport_graph.h"

template <typename Weight>
struct Executor {
    using Time = TransportCatalog::Time;
    using WaitEdge = TransportCatalog::TransportGraph::WaitEdge;
    using BusEdge = TransportCatalog::TransportGraph::BusEdge;

    Graph::Router<Time>& router;
    const TransportCatalog::Catalog& map;
    const TransportCatalog::TransportGraph& graph;
    Svg::Canvas& canvas;

    Executor(
        Graph::Router<Time>& router, const TransportCatalog::Catalog& map,
        const TransportCatalog::TransportGraph& graph, Svg::Canvas& canvas)
        : router(router), map(map), graph(graph), canvas(canvas) {
    }

    Json::Dict ExecuteBusRequest(const std::string& name) {
        if (!map.IsBusExists(name)) return {{"error_message", Json::Node("not found")}};
        const TransportCatalog::Catalog::Bus& bus = map.GetBus(name);
        Json::Dict res;
        res["route_length"] = Json::Node(bus.route_length);
        res["curvature"] = Json::Node(bus.route_length / bus.geo_route_length);
        res["stop_count"] = Json::Node(bus.GetStopCount());
        res["unique_stop_count"] = Json::Node(bus.GetUniqueStopCount());
        return res;
    }

    Json::Dict ExecuteStopRequest(const std::string& name) {
        if (!map.IsStopExists(name)) return {{"error_message", Json::Node("not found")}};
        const TransportCatalog::Catalog::Stop& stop = map.GetStop(name);
        Json::Dict res;
        std::vector<Json::Node> buses;
        buses.reserve(stop.buses_positions.size());
        for (const auto& [name, _] : stop.buses_positions) {
            buses.emplace_back(name);
        }
        res["buses"] = Json::Node(buses);
        return res;
    }

    Json::Dict LoadWaitEdge(const TransportCatalog::TransportGraph::WaitEdge& edge) {
        Json::Dict res;
        res["type"] = Json::Node("Wait");
        res["time"] = Json::Node(edge.time);
        res["stop_name"] = Json::Node(edge.stop);
        return res;
    }

    Json::Dict LoadBusEdge(const TransportCatalog::TransportGraph::BusEdge& edge) {
        Json::Dict res;
        res["type"] = Json::Node("Bus");
        res["bus"] = Json::Node(edge.bus);
        res["time"] = Json::Node(edge.time);
        res["span_count"] = Json::Node(edge.span_cnt);
        return res;
    }

    Json::Dict ExecuteRouteRequest(const std::string& from, const std::string& to) {
        using namespace TransportCatalog;
        std::optional<Graph::Router<Time>::RouteInfo> info = router.BuildRoute(graph.vertices.at(from).wait, graph.vertices.at(to).wait);
        if (info == std::nullopt) return {{"error_message", Json::Node("not found")}};
        Json::Dict res;
        res["total_time"] = info->weight;
        std::vector<Json::Node> items;
        for (size_t i = 0; i < info->edge_count; ++i) {
            Graph::EdgeId edge = router.GetRouteEdge(info->id, i);
            if (std::holds_alternative<WaitEdge>(graph.edges[edge])) {
                items.push_back(Json::Node(LoadWaitEdge(std::get<WaitEdge>(graph.edges[edge]))));
            } else if (std::holds_alternative<BusEdge>(graph.edges[edge])) {
                items.push_back(Json::Node(LoadBusEdge(std::get<BusEdge>(graph.edges[edge]))));
            }
        }
        res["items"] = Json::Node(items);
        router.ReleaseRoute(info->id);
        return res;
    }

    Json::Dict ExecuteMapRequest() {
        Json::Dict res;
        res["map"] = Json::Node(canvas.Draw());
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
            } else if (type == "Route") {
                dict = ExecuteRouteRequest(request.at("from").AsString(), request.at("to").AsString());
            } else if (type == "Map") {
                dict = ExecuteMapRequest();
            }
            dict["request_id"] = Json::Node(request.at("id").AsInt());
            result.push_back(Json::Node(dict));
        }
        return result;
    }
};