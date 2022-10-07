#include <fstream>
#include <string>
#include <unordered_map>

#include "executor.h"
#include "graph.h"
#include "json.h"
#include "transport_catalog.pb.h"

ProtoCatalog::TransportCatalog Deserialize(const std::string &path) {
    ProtoCatalog::TransportCatalog data;
    std::ifstream file(path);
    data.ParseFromIstream(&file);
    file.close();
    return data;
}

void ProcessRequests(std::istream &input, std::ostream &output) {
    using namespace std;
    using namespace Json;
    Document doc = Load(input);
    const auto &data = doc.GetRoot().AsMap();
    const auto &out_requests = data.at("stat_requests").AsArray();
    const auto &serialization_settings = data.at("serialization_settings").AsMap();
    ProtoCatalog::TransportCatalog db = Deserialize(serialization_settings.at("file").AsString());
    Graph::Router router(db);
    Executor executor(db, router);
    Json::PrintValue<std::vector<Json::Node>>(executor.ExecuteRequests(out_requests), output);
}