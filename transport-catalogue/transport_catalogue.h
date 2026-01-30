#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <optional>
#include "geo.h"

namespace transport_catalogue {
    
    struct Stop {
        std::string name;
        Coordinates coordinates;
    };

    struct Bus {
        std::string name;
        std::vector<const Stop*> stops;
        bool is_roundtrip;
    };

    struct BusInfo {
        size_t stop_count = 0;
        size_t unique_stop_count = 0;
        double route_length = 0.0;
        double geo_length = 0.0;
        double curvature = 0.0;
        bool found = false;
    };

    class TransportCatalogue {
    public:
        void AddStop(std::string_view name, Coordinates coord);
        void AddBus(std::string_view name, const std::vector<std::string_view>& stop_names, bool is_roundtrip);
        std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;
        const Stop* FindStop(std::string_view name) const;
        const Bus* FindBus(std::string_view name) const;
        const std::unordered_set<const Bus*>& GetBusesByStop(std::string_view stop_name) const;
        void SetDistance(const Stop* from, const Stop* to, int distance);
        int GetDistance(const Stop* from, const Stop* to) const;
        const std::deque<Bus>& GetBuses() const;
        const std::deque<Stop>& GetStops() const;

    private:
        std::deque<Stop> stops_;
        std::deque<Bus> buses_;

        std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
        std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
        std::unordered_map<const Stop*, std::unordered_set<const Bus*>> stop_to_buses_;
        struct StopPairHasher {
            size_t operator()(const std::pair<const Stop*, const Stop*>& p) const {
                return std::hash<const void*>()(p.first) * 37 + std::hash<const void*>()(p.second);
            }
        };
        std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopPairHasher> distances_;
    };

} 