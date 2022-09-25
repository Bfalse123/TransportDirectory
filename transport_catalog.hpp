#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>

#include "json.hpp"
#include "graph.hpp"
#include "sphere.hpp"

namespace TransportCatalog {

    using Time = double;
    using BusId = size_t;
    using StopId = size_t;
    
    class TransportCatalog {
    public:
        struct Bus {
            BusId id;
            std::string name;
            bool is_rounded;
            int32_t route_length;
            double geo_route_length;
            std::vector<StopId> route;
            std::unordered_set<StopId> unique_stops;

            int32_t GetStopCount() const {
                return is_rounded ? route.size() : route.size() * 2 - 1;
            }

            int32_t GetUniqueStopCount() const {
                return unique_stops.size();
            }
        };

        struct Stop {
            StopId id;
            std::string name;
            Sphere::Point geo_pos;
            std::unordered_map<StopId, int32_t> distances;
            std::set<std::string> buses;
        };

        Time wait_time;
        double bus_velocity;

        size_t StopsCount() const {
            return stops.size();
        }

        const std::vector<Stop> GetStops() const {
            return stops;
        }

        bool IsStopExists(const std::string& name) const {
            return stops_ids.count(name);
        }

        StopId GetStopId(const std::string& name) const {
            return stops_ids.at(name);
        }

        StopId AddStop(const std::string& name);

        const Stop& GetStop(const StopId stopid) const {
            return stops.at(stopid);
        }

        const Stop& GetStop(const std::string& name) const {
            return GetStop(stops_ids.at(name));
        }

        size_t BusesCount() const {
            return buses.size();
        }

        BusId GetBusId(const std::string& name) const {
            return buses_ids.at(name);
        }

        const std::vector<Bus> GetBuses() const {
            return buses;
        }

        bool IsBusExists(const std::string& name) const {
            return buses_ids.count(name);
        }

        BusId AddBus(const std::string& name);
        
        const Bus& GetBus(const BusId busid) const {
            return buses.at(busid);
        }

        const Bus& GetBus(const std::string& name) const {
            return GetBus(buses_ids.at(name));
        }

        template <typename It>
        std::pair<int32_t, double> ComputeDistances(It begin, It end);

        TransportCatalog(const std::vector<Json::Node>& data, const Json::Dict& settings);
        TransportCatalog(const TransportCatalog&) = delete;

    private:
        std::unordered_map<std::string, StopId> stops_ids;
        std::vector<Stop> stops;
        std::unordered_map<std::string, BusId> buses_ids;
        std::vector<Bus> buses;

        void LoadStop(const Json::Dict& data);

        void LoadBus(const Json::Dict& data);

    };

};
