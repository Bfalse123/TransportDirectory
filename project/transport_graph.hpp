#pragma once
#include "transport_catalog.hpp"
#include "graph.hpp"
#include "router.hpp"

#include <vector>
#include <variant>

namespace TransportCatalog {

    class TransportGraph {
        public:
            TransportGraph(const TransportCatalog& db) :
            transport_db(db), vertices(db.StopsCount()), graph(db.StopsCount() * 2) {
                BuildGraph();
            }

            TransportGraph(const TransportGraph&) = delete;

            struct WaitEdge {
                StopId stop;
                Time time;
            };

            struct BusEdge {
                BusId bus;
                int32_t span_cnt;
                Time time;
            };

            struct Vertex {
                size_t wait;
                size_t ride;
            };

            using Edge = std::variant<WaitEdge, BusEdge>;
            
            std::vector<Vertex> vertices;
            std::vector<Edge> edges;

            using TimeGraph = Graph::DirectedWeightedGraph<Time>;

            const TimeGraph& GetGraph() {
                return graph;
            }
            

        private:
            const TransportCatalog& transport_db;
            TimeGraph graph;

            void BuildGraph() {
                const auto& stops = transport_db.GetStops();
                size_t cnt = 0;
                for (StopId stop = 0; stop < stops.size(); ++stop) {
                    vertices[stop] = {cnt, cnt + 1};
                    graph.AddEdge({cnt, cnt + 1, transport_db.wait_time});
                    edges.push_back(Edge(WaitEdge{stop, transport_db.wait_time}));
                    cnt += 2;
                }
                const auto& buses = transport_db.GetBuses();
                for (const auto& bus : buses) {
                    RegisterBusEdges(bus.route.begin(), bus.route.end(), bus.id);
                    if (!bus.is_rounded) RegisterBusEdges(bus.route.rbegin(), bus.route.rend(), bus.id);
                }
            }

            template <typename It>
            void RegisterBusEdges(It stop_from, It end, BusId bus) {
                const auto& stops = transport_db.GetStops();
                for (; stop_from != end; stop_from = std::next(stop_from)) {
                    int32_t distance = 0, span_cnt = 0;
                    It prev = stop_from;
                    for (It stop_to = stop_from; stop_to != end; stop_to = std::next(stop_to)) {
                        distance += stops[*prev].distances.at(*stop_to);
                        Time time = (distance / transport_db.bus_velocity) / 60;
                        graph.AddEdge({vertices[*stop_from].ride, vertices[*stop_to].wait, time});
                        edges.push_back(Edge(BusEdge{bus, span_cnt, time}));
                        prev = stop_to;
                        ++span_cnt;
                    }
                }
            }
    };
};