#include <fstream>
#include <string>
#include <vector>

#include "transport_catalog.h"
#include "transport_catalog.pb.h"
#include "transport_graph.h"
#include "render_builder.h"

namespace Serialize {
class Serializator {
   public:
    Serializator(const TransportCatalog::Catalog& db,
                 const TransportCatalog::TransportGraph& graph,
                 const Svg::RenderBuilder& render);

    void SerializeBuses(ProtoCatalog::TransportCatalog& data);
    void SerializeStops(ProtoCatalog::TransportCatalog& data);
    void BuildAndSerializeRouter(ProtoCatalog::TransportCatalog& data);
    void SerializeGraphInfo(ProtoCatalog::TransportCatalog& data);
    void SerializeRender(ProtoCatalog::TransportCatalog& data);
    void SerializeTo(const std::string& path);

   private:
    const TransportCatalog::Catalog& db;
    const TransportCatalog::TransportGraph& graph;
    const Svg::RenderBuilder& render;
};
}