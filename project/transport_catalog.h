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
    struct Bus {
        bool is_rounded;
        int32_t route_length;
        double geo_route_length;
        std::vector<StopName> route;
        std::unordered_set<StopName> unique_stops;

        int32_t GetStopCount() const {
            return is_rounded ? route.size() : route.size() * 2 - 1;
        }

        int32_t GetUniqueStopCount() const {
            return unique_stops.size();
        }
    };

    struct Stop {
        std::string name;
        Sphere::Point geo_pos;
        std::unordered_map<StopName, int32_t> distances;
        std::map<BusName, std::vector<size_t>> buses_positions;
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

    template <typename It>
    std::pair<int32_t, double> ComputeDistances(It begin, It end);

    Catalog(const std::vector<Json::Node>& data, const Json::Dict& settings);
    Catalog(const Catalog&) = delete;

   private:
    std::map<std::string, Stop> stops;
    std::map<std::string, Bus> buses;

    void LoadStop(const Json::Dict& data);

    void LoadBus(const Json::Dict& data);
};

};  // namespace TransportCatalog
