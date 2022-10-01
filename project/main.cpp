#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "canvas.h"
#include "executor.h"
#include "json.h"
#include "transport_catalog.h"
#include "transport_graph.h"

void Execute(std::istream &input, std::ostream &output) {
    using namespace std;
    using namespace Json;
    Document doc = Load(input);
    const auto &data = doc.GetRoot().AsMap();
    const auto &settings = data.at("routing_settings").AsMap();
    const auto &out_requests = data.at("stat_requests").AsArray();
    const auto &in_requests = data.at("base_requests").AsArray();
    const auto &render_settings = data.at("render_settings").AsMap();
    TransportCatalog::Catalog db(in_requests, settings);
    Svg::Canvas canvas(render_settings, db);
    TransportCatalog::TransportGraph graph(db);
    Graph::Router router(graph.GetGraph());
    Executor<TransportCatalog::Time> executor(router, db, graph, canvas);
    Json::PrintValue<std::vector<Json::Node>>(executor.ExecuteRequests(out_requests), output);
}

int main() {
    //std::ifstream input("../.help/input.json");
    //std::ofstream output("../.help/output.json");
    //Execute(input, output);
    Execute(std::cin, std::cout);
    return 0;
}