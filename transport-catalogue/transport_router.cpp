#include "transport_router.h"
#include "geo.h"

namespace transport {

TransportRouter::TransportRouter(const RoutingSettings& settings,
                                const transport_catalogue::TransportCatalogue& catalogue)
    : settings_(settings)
    , catalogue_(catalogue) {}

void TransportRouter::BuildGraph() {
    stops_.clear();
    stop_ids_.clear();
    edges_.clear();
    edge_to_bus_info_.clear();
    graph_.reset();
    router_.reset();

    InitializeStops();
    ProcessBusRoutes();

    router_ = std::make_unique<graph::Router<double>>(*graph_);
}

void TransportRouter::InitializeStops() {
    const auto& all_stops = catalogue_.GetStops();
    stops_.reserve(all_stops.size());

    for (const auto& stop : all_stops) {
        stop_ids_[stop.name] = stops_.size();
        stops_.push_back(stop.name);
    }

    const size_t vertex_count = stops_.size() * 2;
    graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(vertex_count);

    // Добавляем ребра ожидания на каждой остановке (from waiting vertex to boarding vertex)
    for (size_t i = 0; i < stops_.size(); ++i) {
        graph_->AddEdge({
            2 * i,        // from waiting vertex
            2 * i + 1,    // to boarding vertex
            settings_.bus_wait_time
        });
    }
}

void TransportRouter::ProcessBusRoutes() {
    const auto& all_buses = catalogue_.GetBuses();

    for (const auto& bus : all_buses) {
        if (bus.stops.empty()) {
            continue;
        }

        if (bus.is_roundtrip) {
            ProcessRoundTripBus(bus);
        } else {
            ProcessLinearBus(bus);
        }
    }
}

void TransportRouter::ProcessRoundTripBus(const transport_catalogue::Bus& bus) {
    const auto& stops = bus.stops;
    const size_t stop_count = stops.size();

    for (size_t i = 0; i < stop_count; ++i) {
        double total_distance = 0.0;

        for (size_t j = i + 1; j < stop_count; ++j) {
            int segment_distance = catalogue_.GetDistance(stops[j - 1], stops[j]);
            if (segment_distance == 0) {
                segment_distance = static_cast<int>(ComputeDistance(
                    stops[j - 1]->coordinates,
                    stops[j]->coordinates));
            }

            total_distance += segment_distance;
            double time = total_distance / (settings_.bus_velocity * VELOCITY_COEF);

            size_t from_vertex = stop_ids_.at(stops[i]->name) * 2 + 1;
            size_t to_vertex = stop_ids_.at(stops[j]->name) * 2;

            auto edge_id = graph_->AddEdge({from_vertex, to_vertex, time});

            BusEdge edge{
                stops[i]->name,
                stops[j]->name,
                bus.name,
                static_cast<int>(j - i),
                time
            };

            edges_.push_back(edge);
            edge_to_bus_info_[edge_id] = edge;
        }
    }
}

void TransportRouter::ProcessLinearBus(const transport_catalogue::Bus& bus) {
    const auto& stops = bus.stops;
    const size_t stop_count = stops.size();

    // Вперёд
    for (size_t i = 0; i < stop_count; ++i) {
        double total_distance = 0.0;

        for (size_t j = i + 1; j < stop_count; ++j) {
            int segment_distance = catalogue_.GetDistance(stops[j - 1], stops[j]);
            if (segment_distance == 0) {
                segment_distance = static_cast<int>(ComputeDistance(
                    stops[j - 1]->coordinates,
                    stops[j]->coordinates));
            }

            total_distance += segment_distance;
            double time = total_distance / (settings_.bus_velocity * VELOCITY_COEF);

            size_t from_vertex = stop_ids_.at(stops[i]->name) * 2 + 1;
            size_t to_vertex = stop_ids_.at(stops[j]->name) * 2;

            auto edge_id = graph_->AddEdge({from_vertex, to_vertex, time});

            BusEdge edge{
                stops[i]->name,
                stops[j]->name,
                bus.name,
                static_cast<int>(j - i),
                time
            };

            edges_.push_back(edge);
            edge_to_bus_info_[edge_id] = edge;
        }
    }

    // Назад
    for (size_t i = stop_count - 1; i > 0; --i) {
        double total_distance = 0.0;

        for (size_t j = i - 1; j < i; --j) {
            int segment_distance = catalogue_.GetDistance(stops[j + 1], stops[j]);
            if (segment_distance == 0) {
                segment_distance = static_cast<int>(ComputeDistance(
                    stops[j + 1]->coordinates,
                    stops[j]->coordinates));
            }

            total_distance += segment_distance;
            double time = total_distance / (settings_.bus_velocity * VELOCITY_COEF);

            size_t from_vertex = stop_ids_.at(stops[i]->name) * 2 + 1;
            size_t to_vertex = stop_ids_.at(stops[j]->name) * 2;

            auto edge_id = graph_->AddEdge({from_vertex, to_vertex, time});

            BusEdge edge{
                stops[i]->name,
                stops[j]->name,
                bus.name,
                static_cast<int>(i - j),
                time
            };

            edges_.push_back(edge);
            edge_to_bus_info_[edge_id] = edge;

            if (j == 0) break;
        }
    }
}

std::optional<TransportRouter::RouteInfo> TransportRouter::FindRoute(const std::string& from, const std::string& to) const {
    if (from == to) {
        return RouteInfo{0.0, {}};
    }

    if (!stop_ids_.count(from) || !stop_ids_.count(to)) {
        return std::nullopt;
    }

    size_t from_vertex = stop_ids_.at(from) * 2;
    size_t to_vertex = stop_ids_.at(to) * 2;

    auto route = router_->BuildRoute(from_vertex, to_vertex);
    if (!route) {
        return std::nullopt;
    }

    RouteInfo result;
    result.total_time = route->weight;

    for (auto edge_id : route->edges) {
        const auto& edge = graph_->GetEdge(edge_id);

        if (edge.from % 2 == 0 && edge.to == edge.from + 1) {
            result.items.push_back(RouteItem{
                RouteItem::Type::WAIT,
                stops_[edge.from / 2],
                "",
                edge.weight,
                0
            });
        } else {
            auto it = edge_to_bus_info_.find(edge_id);
            if (it != edge_to_bus_info_.end()) {
                const auto& bus_edge = it->second;
                result.items.push_back(RouteItem{
                    RouteItem::Type::BUS,
                    "",
                    bus_edge.bus_name,
                    bus_edge.time,
                    bus_edge.span_count
                });
            }
        }
    }

    return result;
}

}  // namespace transport
