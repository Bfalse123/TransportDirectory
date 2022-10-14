#pragma once
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "graph.h"
#include "json.h"
#include "sphere.h"

namespace TransportCatalog {

using Time = double;
using StopName = std::string;
using BusName = std::string;

class Catalog {
   public:
    struct Stop {
        std::string name;
        Sphere::Point geo_pos;
        std::unordered_map<StopName, int32_t> distances;
        std::map<BusName, std::set<size_t>> pos_in_routes;
    };

    struct Bus {
        std::string name;
        int32_t route_length;
        int32_t unique_stops_cnt;
        int32_t stops_cnt;
        double geo_route_length;
        bool is_rounded;
        std::pair<size_t, size_t> end_points;
        std::vector<Stop*> route;
    };

    Time wait_time;
    double bus_velocity;

    size_t StopsCount() const {
        return stops.size();
    }

    const std::map<StopName, Stop>& GetStops() const {
        return stops;
    }

    bool IsStopExists(const StopName& name) const {
        return stops.count(name);
    }

    const Stop& GetStop(const StopName& name) const {
        return stops.at(name);
    }

    size_t BusesCount() const {
        return buses.size();
    }

    const std::map<BusName, Bus>& GetBuses() const {
        return buses;
    }

    bool IsBusExists(const BusName& name) const {
        return buses.count(name);
    }

    const Bus& GetBus(const BusName& name) const {
        return buses.at(name);
    }

    Bus& GetBus(const BusName& name) {
        return buses[name];
    }

    Catalog(const std::vector<Json::Node>& data, const Json::Dict& settings);
    Catalog(const Catalog&) = delete;

   private:
    std::map<std::string, Stop> stops;
    std::map<std::string, Bus> buses;

    void LoadStop(const Json::Dict& data);

    void LoadBus(const Json::Dict& data);
};

};  // namespace TransportCatalog
