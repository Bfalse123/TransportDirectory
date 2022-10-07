#include <fstream>
#include <string>
#include <vector>

#include "transport_catalog.h"
#include "transport_catalog.pb.h"
#include "transport_graph.h"

namespace Serialize {
class Serializator {
   public:
    Serializator(const TransportCatalog::Catalog& db,
                 const TransportCatalog::TransportGraph& graph);

    void LoadBuses(ProtoCatalog::TransportCatalog& data);
    void LoadStops(ProtoCatalog::TransportCatalog& data);
    void BuildAndLoadRouter(ProtoCatalog::TransportCatalog& data);
    void LoadGraphInfo(ProtoCatalog::TransportCatalog& data);
    void SerializeTo(const std::string& path);

   private:
    const TransportCatalog::Catalog& db;
    const TransportCatalog::TransportGraph& graph;
};
}