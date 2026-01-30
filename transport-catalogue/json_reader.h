#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include "svg.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "json_builder.h"

class JsonReader {
public:
    JsonReader(transport_catalogue::TransportCatalogue& catalogue, 
               transport::TransportRouter& router)
        : catalogue_(catalogue), router_(router) {}

    void ParsingBaseRequests(const json::Array& base_requests);
    void ParsingRenderSettings(const json::Dict& reader_settings);
    json::Document ParsingStatRequests(const json::Array& stat_requests) const;
    svg::Color ParseColor(const json::Node& node);
    const RenderSettings& GetRenderSettings() const { return render_settings_; }

private:
    transport_catalogue::TransportCatalogue& catalogue_;
    transport::TransportRouter& router_;
    RenderSettings render_settings_;

    void ProcessStop(const json::Dict& request);              
    void ProcessRoadDistances(const json::Dict& request);      
    void ProcessBus(const json::Dict& request);               
};