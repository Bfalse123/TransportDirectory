#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "json.h"
#include "transport_catalog.h"
#include "transport_graph.h"
#include "serialize.h"
#include "render_builder.h"

void MakeBase(std::istream &input) {
    using namespace std;
    using namespace Json;
    Document doc = Load(input);
    const auto &data = doc.GetRoot().AsMap();
    const auto &in_requests = data.at("base_requests").AsArray();
    const auto &settings = data.at("routing_settings").AsMap();
    const auto &render_settings = data.at("render_settings").AsMap();
    const auto &serialization_settings = data.at("serialization_settings").AsMap();
    TransportCatalog::Catalog db(in_requests, settings);
    TransportCatalog::TransportGraph graph(db);
    Svg::RenderBuilder render_builder(db, render_settings);
    Serialize::Serializator serializator(db, graph, render_builder);
    serializator.SerializeTo(serialization_settings.at("file").AsString());
}