#pragma once
#include <variant>
#include <vector>

#include "graph.h"
#include "router.h"
#include "transport_catalog.h"

namespace TransportCatalog {

class TransportGraph {
   public:
    TransportGraph(const Catalog& db) : transport_db(db), graph(db.StopsCount() * 2) {
        BuildGraph();
    }

    TransportGraph(const TransportGraph&) = delete;

    struct WaitEdge {
        StopName stop;
        Time time;
    };

    struct BusEdge {
        BusName bus;
        int32_t span_cnt;
        Time time;
    };

    struct Vertex {
        size_t wait;
        size_t ride;
    };

    using Edge = std::variant<WaitEdge, BusEdge>;

    std::unordered_map<StopName, Vertex> vertices;
    std::vector<Edge> edges;

    using TimeGraph = Graph::DirectedWeightedGraph<Time>;

    const TimeGraph& GetGraph() {
        return graph;
    }

   private:
    const Catalog& transport_db;
    TimeGraph graph;

    void BuildGraph() {
        const auto& stops = transport_db.GetStops();
        size_t cnt = 0;
        for (const auto& [name, _] : stops) {
            vertices[name] = {cnt, cnt + 1};
            graph.AddEdge({cnt, cnt + 1, transport_db.wait_time});
            edges.push_back(Edge(WaitEdge{name, transport_db.wait_time}));
            cnt += 2;
        }
        const auto& buses = transport_db.GetBuses();
        for (const auto& [name, bus] : buses) {
            RegisterBusEdges(bus.route.begin(), bus.route.end(), name);
            if (!bus.is_rounded) RegisterBusEdges(bus.route.rbegin(), bus.route.rend(), name);
        }
    }

    template <typename It>
    void RegisterBusEdges(It stop_from, It end, BusName bus) {
        const auto& stops = transport_db.GetStops();
        for (; stop_from != end; stop_from = std::next(stop_from)) {
            int32_t distance = 0, span_cnt = 0;
            It prev = stop_from;
            for (It stop_to = stop_from; stop_to != end; stop_to = std::next(stop_to)) {
                distance += (*prev)->distances.at((*stop_to)->name);
                Time time = (distance / transport_db.bus_velocity) / 60;
                graph.AddEdge({vertices[(*stop_from)->name].ride, vertices[(*stop_to)->name].wait, time});
                edges.push_back(Edge(BusEdge{bus, span_cnt, time}));
                prev = stop_to;
                ++span_cnt;
            }
        }
    }
};
};  // namespace TransportCatalog