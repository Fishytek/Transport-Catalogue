#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace transport {

struct RoutingSettings {
    double bus_wait_time = 0;
    double bus_velocity = 0;
};

struct BusEdge {
    std::string from_stop;
    std::string to_stop;
    std::string bus_name;
    int span_count;
    double time;
};

class TransportRouter {
public:
    // Конструктор теперь принимает настройки и каталог
    TransportRouter(const RoutingSettings& settings,
                    const transport_catalogue::TransportCatalogue& catalogue);

    void BuildGraph();

    struct RouteItem {
        enum class Type { WAIT, BUS };
        Type type;
        std::string stop_name;
        std::string bus_name;
        double time;
        int span_count;
    };

    struct RouteInfo {
        double total_time;
        std::vector<RouteItem> items;
    };

    std::optional<RouteInfo> FindRoute(const std::string& from, const std::string& to) const;

private:
    void InitializeStops();
    void ProcessBusRoutes();
    void ProcessRoundTripBus(const transport_catalogue::Bus& bus);
    void ProcessLinearBus(const transport_catalogue::Bus& bus);

private:
    RoutingSettings settings_;
    const transport_catalogue::TransportCatalogue& catalogue_; // ссылка на каталог

    std::vector<std::string> stops_;
    std::unordered_map<std::string, size_t> stop_ids_;

    std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_;
    std::unique_ptr<graph::Router<double>> router_;

    std::vector<BusEdge> edges_;
    std::unordered_map<graph::EdgeId, BusEdge> edge_to_bus_info_;

    static constexpr double VELOCITY_COEF = 1000.0 / 60.0; // скорость в м/мин
};

}  // namespace transport
