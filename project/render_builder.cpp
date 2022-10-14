#include "render_builder.h"
#include <algorithm>

using namespace TransportCatalog;

namespace Svg {

static Svg::Point ParsePoint(const Json::Node &json) {
    const auto &array = json.AsArray();
    return {
        array[0].AsDouble(),
        array[1].AsDouble()};
}

static Svg::Color ParseColor(const Json::Node &json) {
    if (json.IsString()) {
        return json.AsString();
    }
    const auto &array = json.AsArray();
    Svg::Rgb rgb{
        static_cast<uint8_t>(array[0].AsInt()),
        static_cast<uint8_t>(array[1].AsInt()),
        static_cast<uint8_t>(array[2].AsInt())};
    if (array.size() == 3) {
        return rgb;
    } else {
        return Svg::Rgba{rgb, array[3].AsDouble()};
    }
}

static vector<Svg::Color> ParseColors(const Json::Node &json) {
    const auto &array = json.AsArray();
    vector<Svg::Color> colors;
    colors.reserve(array.size());
    transform(begin(array), end(array), back_inserter(colors), ParseColor);
    return colors;
}

RenderSettings ParseRenderSettings(const Json::Dict &json) {
    RenderSettings result;
    result.max_width = json.at("width").AsDouble();
    result.max_height = json.at("height").AsDouble();
    result.padding = json.at("padding").AsDouble();
    result.outer_margin = json.at("outer_margin").AsDouble();
    result.palette = ParseColors(json.at("color_palette"));
    result.line_width = json.at("line_width").AsDouble();
    result.underlayer_color = ParseColor(json.at("underlayer_color"));
    result.underlayer_width = json.at("underlayer_width").AsDouble();
    result.stop_radius = json.at("stop_radius").AsDouble();
    result.bus_label_offset = ParsePoint(json.at("bus_label_offset"));
    result.bus_label_font_size = json.at("bus_label_font_size").AsInt();
    result.stop_label_offset = ParsePoint(json.at("stop_label_offset"));
    result.stop_label_font_size = json.at("stop_label_font_size").AsInt();

    const auto &layers_array = json.at("layers").AsArray();
    result.layers.reserve(layers_array.size());
    for (const auto &layer_node : layers_array) {
        result.layers.push_back(layer_node.AsString());
    }

    return result;
}

struct CompressPositions {
    const TransportCatalog::Catalog &db;
    const RenderSettings& settings;
    CompressPositions(const TransportCatalog::Catalog &db, const RenderSettings& settings) : db(db), settings(settings) {}

    struct StopWithUniformArrangement {
        double longitude;
        double latitude;
    };

    bool IsNeighbours(const Catalog::Stop *stop1, const Catalog::Stop *stop2) {
        for (const auto &[bus, positions] : stop1->pos_in_routes) {
            if (stop2->pos_in_routes.count(bus)) {
                for (const auto &pos1 : positions) {
                    for (const auto &pos2 : stop2->pos_in_routes.at(bus)) {
                        if (std::abs(int(pos1) - int(pos2)) == 1) return true;
                    }
                }
            }
        }
        return false;
    }

    std::map<int32_t, std::vector<const Catalog::Stop *>> Glue(std::vector<std::pair<double, const Catalog::Stop *>> &sorted) {
        std::map<int32_t, std::vector<const Catalog::Stop *>> glued;
        std::unordered_map<std::string, int32_t> met;
        for (const auto &[_, stop] : sorted) {
            int32_t insert = -1;
            for (const auto &[prev_name, id] : met) {
                if (IsNeighbours(stop, &db.GetStop(prev_name))) {
                    insert = std::max(insert, id);
                }
            }
            glued[++insert].push_back(stop);
            met[stop->name] = insert;
        }
        return glued;
    }

    std::map<std::string, StopWithUniformArrangement> ComputeUniformArrangements() {
        std::map<std::string, StopWithUniformArrangement> uniform_stops;
        for (const auto &[name, bus] : db.GetBuses()) {
            size_t i = 0;
            const auto &route = bus.route;
            size_t b = bus.end_points.first;
            size_t len = bus.end_points.second + 1;
            for (size_t j = 1; j < len; ++j) {
                Catalog::Stop *stop = route[j];
                if (j == 0 || j == route.size() - 1 || stop->pos_in_routes.size() > 1 || (stop->pos_in_routes[name].size() * (bus.is_rounded ? 1 : 2)) > 2) {
                    double lon_step = (stop->geo_pos.longitude - route[i]->geo_pos.longitude) / (j - i);
                    double lat_step = (stop->geo_pos.latitude - route[i]->geo_pos.latitude) / (j - i);
                    stop = route[i];
                    for (size_t k = i; k < j; ++k) {
                        uniform_stops[route[k]->name] = {
                            .longitude = stop->geo_pos.longitude + lon_step * (k - i),
                            .latitude = stop->geo_pos.latitude + lat_step * (k - i)};
                    }
                    stop = route[j];
                    uniform_stops[stop->name] = {
                        .longitude = stop->geo_pos.longitude,
                        .latitude = stop->geo_pos.latitude};
                    i = j;
                }
            }
        }
        return uniform_stops;
    }

    void AddStopsWithNoBuses(std::map<std::string, StopWithUniformArrangement> &uniform_stops) {
        for (const auto &[name, stop] : db.GetStops()) {
            if (stop.pos_in_routes.size() == 0) {
                uniform_stops[name] = {
                    stop.geo_pos.longitude,
                    stop.geo_pos.latitude};
            }
        }
    }

    std::map<StopName, Svg::Point> ConstructStopsPoints() {
        if (db.StopsCount() == 0) return {};
        auto uniform_stops = ComputeUniformArrangements();
        AddStopsWithNoBuses(uniform_stops);
        std::vector<std::pair<double, const Catalog::Stop *>> lon_sorted, lat_sorted;
        bool first = true;
        for (const auto &[name, stop] : db.GetStops()) {
            lon_sorted.push_back({uniform_stops[name].longitude, &stop});
            lat_sorted.push_back({uniform_stops[name].latitude, &stop});
        }
        auto compare_by_double = [](const auto &lhs, const auto &rhs) {
            return lhs.first < rhs.first;
        };
        std::sort(lon_sorted.begin(), lon_sorted.end(), compare_by_double);
        std::sort(lat_sorted.begin(), lat_sorted.end(), compare_by_double);
        auto glued_by_lon = Glue(lon_sorted);
        auto glued_by_lat = Glue(lat_sorted);
        double x_step = glued_by_lon.size() - 1 ? (settings.max_width - 2 * settings.padding) / (glued_by_lon.size() - 1) : 0.0;
        double y_step = glued_by_lat.size() - 1 ? (settings.max_height - 2 * settings.padding) / (glued_by_lat.size() - 1) : 0.0;
        std::map<StopName, Svg::Point> stops_points;
        for (const auto &[idx, stops] : glued_by_lon) {
            for (const auto &stop : stops) {
                stops_points[stop->name].x = idx * x_step + settings.padding;
            }
        }
        for (const auto &[idy, stops] : glued_by_lat) {
            for (const auto &stop : stops) {
                stops_points[stop->name].y = settings.max_height - settings.padding - idy * y_step;
            }
        }
        return stops_points;
    }
};

std::map<BusName, Svg::Color> ConstructBusesColors(const TransportCatalog::Catalog &db, const std::vector<Svg::Color>& palette) {
    size_t cnt_color = 0;
    std::map<BusName, Svg::Color> buses_colors;
    for (const auto &[name, _] : db.GetBuses()) {
        buses_colors[name] = palette[cnt_color];
        cnt_color = (cnt_color + 1) % palette.size();
    }
    return buses_colors;
}

RenderBuilder::RenderBuilder(const TransportCatalog::Catalog &db, const Json::Dict &settings)
    : settings_(ParseRenderSettings(settings)), db(db), stops_points(CompressPositions(db, settings_).ConstructStopsPoints()), 
    buses_colors(ConstructBusesColors(db, settings_.palette)) {}
}  // namespace Svg