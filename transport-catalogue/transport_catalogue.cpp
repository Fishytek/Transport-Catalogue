#include "transport_catalogue.h"
#include "geo.h"
#include <algorithm>
#include <optional>
using namespace std;
namespace transport_catalogue {


    void TransportCatalogue::AddStop(string_view name, Coordinates coord) {
        stops_.push_back({ std::string(name), coord.lat, coord.lng });
        stopname_to_stop_[stops_.back().name] = &stops_.back();
    }

    void TransportCatalogue::AddBus(string_view name, const vector<string_view>& stop_names, bool is_roundtrip) {
        std::vector<const Stop*> bus_stops;
        bus_stops.reserve(stop_names.size());

        for (auto stop_name : stop_names) {
            const Stop* stop = FindStop(stop_name);
            if (stop) {
                bus_stops.push_back(stop);
            }
        }

        buses_.push_back({ std::string(name), std::move(bus_stops), is_roundtrip });
        busname_to_bus_[buses_.back().name] = &buses_.back();

        for (const Stop* stop : buses_.back().stops) {
            if (stop) {
                stop_to_buses_[stop].insert(&buses_.back());
            }
        }

    }


    const Stop* TransportCatalogue::FindStop(string_view name) const {
        if (auto it = stopname_to_stop_.find(name); it != stopname_to_stop_.end()) {
            return it->second;
        }
        return nullptr;
    }

    const Bus* TransportCatalogue::FindBus(string_view name) const {
        if (auto it = busname_to_bus_.find(name); it != busname_to_bus_.end()) {
            return it->second;
        }
        return nullptr;
    }

optional<BusInfo> TransportCatalogue::GetBusInfo(string_view bus_name) const {
    const Bus* bus = FindBus(bus_name);
    if (!bus) {
        return nullopt;
    }
    BusInfo info;

    if (bus->is_roundtrip) {
        info.stop_count = bus->stops.size();
    } else {
        info.stop_count = bus->stops.size() * 2 - 1;
    }

    std::unordered_set<const Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
    info.unique_stop_count = unique_stops.size();

    int road = 0;
    double geo = 0.0;

    if (bus->is_roundtrip) {
        for (size_t i = 1; i < bus->stops.size(); ++i) {
            road += GetDistance(bus->stops[i - 1], bus->stops[i]);
            geo += ComputeDistance(bus->stops[i - 1]->coordinates, bus->stops[i]->coordinates);
        }
    } else {
        for (size_t i = 1; i < bus->stops.size(); ++i) {
            road += GetDistance(bus->stops[i - 1], bus->stops[i]);
            geo += ComputeDistance(bus->stops[i - 1]->coordinates, bus->stops[i]->coordinates);
        }
        for (size_t i = bus->stops.size() - 1; i > 0; --i) {
            road += GetDistance(bus->stops[i], bus->stops[i - 1]);
            geo += ComputeDistance(bus->stops[i]->coordinates, bus->stops[i - 1]->coordinates);
        }
    }

    info.route_length = road;
    info.geo_length = geo;
    info.curvature = geo > 0 ? static_cast<double>(road) / geo : 0.0;

    return info;
}


const std::unordered_set<const Bus*>& TransportCatalogue::GetBusesByStop(std::string_view stop_name) const {
    const Stop* stop = FindStop(stop_name);
    if (!stop) {
        static const std::unordered_set<const Bus*> empty_set;
        return empty_set;
    }

    auto it = stop_to_buses_.find(stop);
    if (it == stop_to_buses_.end()) {
        static const std::unordered_set<const Bus*> empty_set;
        return empty_set;
    }

    return it->second;
}
    void TransportCatalogue::SetDistance(const Stop* from, const Stop* to, int distance) {
        distances_[{from, to}] = distance;
    }

    int TransportCatalogue::GetDistance(const Stop* from, const Stop* to) const {
        if (auto it = distances_.find({ from, to }); it != distances_.end()) {
            return it->second;
        }
        if (auto rev = distances_.find({ to, from }); rev != distances_.end()) {
            return rev->second;
        }
        return 0;
    }
    const std::deque<Bus>& TransportCatalogue::GetBuses() const { 
        return buses_; 
        }
    const std::deque<Stop>& TransportCatalogue::GetStops() const { 
        return stops_;
    }
}