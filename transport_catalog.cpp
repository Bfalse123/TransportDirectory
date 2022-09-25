#include "transport_catalog.hpp"

#include "json.hpp"
#include <iostream>

using namespace std;

namespace TransportCatalog {

    using Stop = TransportCatalog::Stop;
    using Bus = TransportCatalog::Bus;

    StopId TransportCatalog::AddStop(const string& name) {
        if (stops_ids.count(name) == 0) {
            stops.emplace_back();
            stops.back().id = stops.size() - 1;
            stops_ids[name] = stops.size() - 1;
        }
        return stops_ids[name];
    }

    BusId TransportCatalog::AddBus(const string& name) {
        if (buses_ids.count(name) == 0) {
            buses.emplace_back();
            buses.back().id = buses.size() - 1;
            buses_ids[name] = buses.size() - 1;
        }
        return buses_ids[name];
    }

    void TransportCatalog::LoadStop(const Json::Dict& data) {
        const auto& name = data.at("name").AsString();
        StopId stop = AddStop(name);
        stops[stop].name = name;
        stops[stop].geo_pos.latitude = data.at("latitude").AsDouble(); 
        stops[stop].geo_pos.longitude = data.at("longitude").AsDouble();
        stops[stop].distances[stop] = 0;
        for (const auto& [name, distance] : data.at("road_distances").AsMap()) {
            StopId to_stop = AddStop(name);
            stops[stop].distances[to_stop] = distance.AsInt();
            if (stops[to_stop].distances.count(stop) == 0) {
                stops[to_stop].distances[stop] = distance.AsInt();
            }
        }
    }

    template <typename It>
    std::pair<int32_t, double> TransportCatalog::ComputeDistances(It begin, It end) {
        int32_t route_length = 0;
        double geo_route_length = 0.0;
        for (; std::next(begin) != end; begin = std::next(begin)) {
            Stop& curr = stops[*begin];
            StopId nextid = *std::next(begin);
            route_length += curr.distances[nextid];
            geo_route_length += Sphere::Distance(curr.geo_pos, stops[nextid].geo_pos);
        }
        return {route_length, geo_route_length};
    }

    void TransportCatalog::LoadBus(const Json::Dict& data) {
        const auto& bus_name = data.at("name").AsString();
        Bus& bus = buses[AddBus(bus_name)];
        bus.name = bus_name;
        bus.is_rounded = data.at("is_roundtrip").AsBool();
        for (const auto& stop_name : data.at("stops").AsArray()) {
            Stop& stop = stops[stops_ids[stop_name.AsString()]];
            if (bus.unique_stops.count(stop.id) == 0) {
                stop.buses.insert(bus_name);
                bus.unique_stops.insert(stop.id);
            }
            bus.route.push_back(stop.id);
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