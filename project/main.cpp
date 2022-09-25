#include "json.hpp"
#include "transport_catalog.hpp"
#include "executor.hpp"
#include "transport_graph.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <string>

void Execute(std::istream& input, std::ostream& output) {
    using namespace std;
    using namespace Json;
    Document doc = Load(input);
    const auto& data = doc.GetRoot().AsMap();
    const auto& settings = data.at("routing_settings").AsMap();
    const auto& out_requests  = data.at("stat_requests").AsArray();
    const auto& in_requests = data.at("base_requests").AsArray();
    TransportCatalog::TransportCatalog TransportCatalog(in_requests, settings);
    TransportCatalog::TransportGraph graph(TransportCatalog);
    Graph::Router router(graph.GetGraph());
    Executor<TransportCatalog::Time> executor(router, TransportCatalog, graph);
    Json::PrintValue<std::vector<Json::Node>>(executor.ExecuteRequests(out_requests), output);
}

void Test2() {
}

int main() {
    std::ifstream input(".help/input.json");
    std::ofstream output(".help/output.json");
    Execute(input, output);
    return 0;
}