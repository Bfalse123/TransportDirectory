#include "transport_catalog.h"

#include "json.h"
#include <iostream>

using namespace std;

namespace TransportCatalog {

    using Stop = TransportCatalog::Stop;
    using Bus = TransportCatalog::Bus;

    void TransportCatalog::LoadStop(const Json::Dict& data) {
        const auto& name = data.at("name").AsString();
        stops[name].geo_pos.latitude = data.at("latitude").AsDouble(); 
        stops[name].geo_pos.longitude = data.at("longitude").AsDouble();
        stops[name].distances[name] = 0;
        for (const auto& [to_stop, distance] : data.at("road_distances").AsMap()) {
            stops[name].distances[to_stop] = distance.AsInt();
            if (stops[to_stop].distances.count(name) == 0) {
                stops[to_stop].distances[name] = distance.AsInt();
            }
        }
    }

    template <typename It>
    std::pair<int32_t, double> TransportCatalog::ComputeDistances(It begin, It end) {
        int32_t route_length = 0;
        double geo_route_length = 0.0;
        for (; std::next(begin) != end; begin = std::next(begin)) {
            Stop& curr = stops[*begin];
            const StopName& next = *std::next(begin);
            route_length += curr.distances[next];
            geo_route_length += Sphere::Distance(curr.geo_pos, stops[next].geo_pos);
        }
        return {route_length, geo_route_length};
    }

    void TransportCatalog::LoadBus(const Json::Dict& data) {
        const auto& bus_name = data.at("name").AsString();
        Bus& bus = buses[bus_name];
        bus.is_rounded = data.at("is_roundtrip").AsBool();
        for (const auto& node : data.at("stops").AsArray()) {
            StopName stop_name = node.AsString();
            Stop& stop = stops[stop_name];
            if (bus.unique_stops.count(stop_name) == 0) {
                stop.buses.insert(bus_name);
                bus.unique_stops.insert(stop_name);
            }
            bus.route.push_back(stop_name);
        }
        auto lengths = ComputeDistances(bus.route.begin(), bus.route.end());
        bus.route_length = lengths.first;
        bus.geo_route_length = lengths.second;
        if (!bus.is_rounded) {
            lengths = ComputeDistances(bus.route.rbegin(), bus.route.rend());
            bus.route_length += lengths.first;
            bus.geo_route_length += lengths.second;
        }
    }

    TransportCatalog::TransportCatalog(const vector<Json::Node>& data, const Json::Dict& settings)
    : bus_velocity(settings.at("bus_velocity").AsDouble() / 3.6), wait_time(settings.at("bus_wait_time").AsInt()){
        using namespace Json;
        for (const Node& node : data) {
            const auto& mapnode = node.AsMap();
            if (mapnode.at("type").AsString() == "Stop") {
                LoadStop(mapnode);
            }
        }

        for (const Node& node : data) {
            const auto& mapnode = node.AsMap();
            if (mapnode.at("type").AsString() == "Bus") {
                LoadBus(mapnode);
            }
        }

    }
};