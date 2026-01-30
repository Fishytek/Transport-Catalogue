#include "json_reader.h"
#include "transport_router.h"
#include <iostream>

int main() {
    transport_catalogue::TransportCatalogue catalogue;
    transport::RoutingSettings routing_settings;

    auto doc = json::Load(std::cin);
    const auto& root_map = doc.GetRoot().AsDict();

    if (root_map.count("routing_settings")) {
        const auto& rs = root_map.at("routing_settings").AsDict();
        routing_settings.bus_wait_time = rs.at("bus_wait_time").AsInt();
        routing_settings.bus_velocity = rs.at("bus_velocity").AsDouble();
    }

    // Передаём в конструктор и настройки, и каталог
    transport::TransportRouter router(routing_settings, catalogue);
    JsonReader reader(catalogue, router);

    if (root_map.count("base_requests")) {
        reader.ParsingBaseRequests(root_map.at("base_requests").AsArray());
    }

    // BuildGraph без параметров
    router.BuildGraph();

    if (root_map.count("render_settings")) {
        reader.ParsingRenderSettings(root_map.at("render_settings").AsDict());
    }

    if (root_map.count("stat_requests")) {
        auto response = reader.ParsingStatRequests(root_map.at("stat_requests").AsArray());
        json::Print(response, std::cout);
    }

    return 0;
}
