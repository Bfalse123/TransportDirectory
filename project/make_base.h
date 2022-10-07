#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "json.h"
#include "transport_catalog.h"
#include "transport_graph.h"
#include "serialize.h"

void MakeBase(std::istream &input) {
    using namespace std;
    using namespace Json;
    Document doc = Load(input);
    const auto &data = doc.GetRoot().AsMap();
    const auto &in_requests = data.at("base_requests").AsArray();
    const auto &settings = data.at("routing_settings").AsMap();
    //const auto &out_requests = data.at("stat_requests").AsArray();
    const auto &render_settings = data.at("render_settings").AsMap();
    const auto &serialization_settings = data.at("serialization_settings").AsMap();
    TransportCatalog::Catalog db(in_requests, settings);
    TransportCatalog::TransportGraph graph(db);
    Serialize::Serializator serializator(db, graph);
    serializator.SerializeTo(serialization_settings.at("file").AsString());
    //Svg::Canvas canvas(render_settings, db);
    //TransportCatalog::TransportGraph graph(db);
    //Graph::Router router(graph.GetGraph());
    //Json::PrintValue<std::vector<Json::Node>>(executor.ExecuteRequests(out_requests), output);
}