#include "json_reader.h"
#include "map_renderer.h"
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

using namespace transport_catalogue;

void JsonReader::ParsingBaseRequests(const json::Array& base_requests) {
    std::vector<const json::Dict*> stops_with_distances;
    std::vector<const json::Dict*> buses;

    for (const auto& node : base_requests) {
        const auto& request = node.AsDict();
        const std::string& type = request.at("type").AsString();

        if (type == "Stop") {
            ProcessStop(request);
            if (request.count("road_distances")) {
                stops_with_distances.push_back(&request);
            }
        } else if (type == "Bus") {
            buses.push_back(&request);
        }
    }

    for (const auto* request : stops_with_distances) {
        ProcessRoadDistances(*request);
    }

    for (const auto* request : buses) {
        ProcessBus(*request);
    }
}

void JsonReader::ProcessStop(const json::Dict& request) {
    catalogue_.AddStop(
        request.at("name").AsString(),
        { request.at("latitude").AsDouble(),
          request.at("longitude").AsDouble() }
    );
}

void JsonReader::ProcessRoadDistances(const json::Dict& request) {
    const auto* from_stop = catalogue_.FindStop(request.at("name").AsString());
    for (const auto& kv : request.at("road_distances").AsDict()) {
        const auto* to_stop = catalogue_.FindStop(kv.first);
        if (from_stop && to_stop) {
            catalogue_.SetDistance(from_stop, to_stop, kv.second.AsInt());
        }
    }
}

void JsonReader::ProcessBus(const json::Dict& request) {
    std::vector<std::string_view> stops_view;
    for (const auto& stop_node : request.at("stops").AsArray()) {
        stops_view.push_back(stop_node.AsString());
    }

    bool is_roundtrip = request.at("is_roundtrip").AsBool();
    catalogue_.AddBus(request.at("name").AsString(), stops_view, is_roundtrip);
}

void JsonReader::ParsingRenderSettings(const json::Dict& reader_settings) {
    RenderSettings settings;
    settings.width                = reader_settings.at("width").AsDouble();
    settings.height               = reader_settings.at("height").AsDouble();
    settings.padding              = reader_settings.at("padding").AsDouble();
    settings.line_width           = reader_settings.at("line_width").AsDouble();
    settings.stop_radius          = reader_settings.at("stop_radius").AsDouble();
    settings.bus_label_font_size  = reader_settings.at("bus_label_font_size").AsInt();

    const auto& b_off = reader_settings.at("bus_label_offset").AsArray();
    settings.bus_label_offset = { b_off[0].AsDouble(), b_off[1].AsDouble() };

    settings.stop_label_font_size = reader_settings.at("stop_label_font_size").AsInt();
    const auto& s_off = reader_settings.at("stop_label_offset").AsArray();
    settings.stop_label_offset = { s_off[0].AsDouble(), s_off[1].AsDouble() };

    settings.underlayer_color = ParseColor(reader_settings.at("underlayer_color"));
    settings.underlayer_width = reader_settings.at("underlayer_width").AsDouble();

    for (const auto& color_node : reader_settings.at("color_palette").AsArray()) {
        settings.color_palette.push_back(ParseColor(color_node));
    }

    render_settings_ = std::move(settings);
}

svg::Color JsonReader::ParseColor(const json::Node& node) {
    if (node.IsString()) {
        return node.AsString();
    }
    else if (node.IsArray()) {
        const auto& arr = node.AsArray();
        if (arr.size() == 3) {
            return svg::Rgb{
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt())
            };
        }
        else if (arr.size() == 4) {
            return svg::Rgba{
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt()),
                arr[3].AsDouble()
            };
        }
    }
    return svg::NoneColor;
}

json::Document JsonReader::ParsingStatRequests(const json::Array& stat_requests) const {
    json::Builder builder;
    auto arr_ctx = builder.StartArray();

    for (const auto& node : stat_requests) {
        const auto& request = node.AsDict();
        int request_id = request.at("id").AsInt();
        const std::string& type = request.at("type").AsString();

        if (type == "Bus") {
            auto info_opt = catalogue_.GetBusInfo(request.at("name").AsString());
            if (info_opt) {
                const auto& info = *info_opt;
                arr_ctx.StartDict()
                    .Key("request_id").Value(request_id)
                    .Key("stop_count").Value(static_cast<int>(info.stop_count))
                    .Key("unique_stop_count").Value(static_cast<int>(info.unique_stop_count))
                    .Key("route_length").Value(info.route_length)
                    .Key("curvature").Value(info.curvature)
                .EndDict();
            } else {
                arr_ctx.StartDict()
                    .Key("request_id").Value(request_id)
                    .Key("error_message").Value("not found")
                .EndDict();
            }
        } else if (type == "Stop") {
            const auto* stop = catalogue_.FindStop(request.at("name").AsString());
            if (!stop) {
                arr_ctx.StartDict()
                    .Key("request_id").Value(request_id)
                    .Key("error_message").Value("not found")
                .EndDict();
            } else {
                auto buses = catalogue_.GetBusesByStop(stop->name);
                std::vector<std::string> bus_names;
                bus_names.reserve(buses.size());
                for (const auto* b : buses) {
                    bus_names.push_back(b->name);
                }
                std::sort(bus_names.begin(), bus_names.end());

                arr_ctx.StartDict()
                    .Key("request_id").Value(request_id)
                    .Key("buses").StartArray();
                
                for (const auto& name : bus_names) {
                    arr_ctx.Value(name);
                }

                arr_ctx.EndArray()
                .EndDict();
            }
        } else if (type == "Map") {
            svg::Document svg_doc;
            MapRenderer renderer(catalogue_, render_settings_);
            renderer.Render(svg_doc);

            std::ostringstream svg_stream;
            svg_doc.Render(svg_stream);

            arr_ctx.StartDict()
                .Key("request_id").Value(request_id)
                .Key("map").Value(svg_stream.str())
            .EndDict();
        }  else if (type == "Route") {
    const std::string& from = request.at("from").AsString();
    const std::string& to = request.at("to").AsString();
    
    auto route_info = router_.FindRoute(from, to);
    
    if (route_info) {
        arr_ctx.StartDict()
            .Key("request_id").Value(request_id)
            .Key("total_time").Value(route_info->total_time)
            .Key("items").StartArray();
        
        for (const auto& item : route_info->items) {
            if (item.type == transport::TransportRouter::RouteItem::Type::WAIT) {
                arr_ctx.StartDict()
                    .Key("type").Value("Wait")
                    .Key("stop_name").Value(item.stop_name)
                    .Key("time").Value(item.time)
                .EndDict();
            } else {
                arr_ctx.StartDict()
                    .Key("type").Value("Bus")
                    .Key("bus").Value(item.bus_name)
                    .Key("span_count").Value(static_cast<int>(item.span_count))
                    .Key("time").Value(item.time)
                .EndDict();
            }
        }
        
        arr_ctx.EndArray()
        .EndDict();
    } else {
        arr_ctx.StartDict()
            .Key("request_id").Value(request_id)
            .Key("error_message").Value("not found")
        .EndDict();
    }
        } else {
            arr_ctx.StartDict()
                .Key("request_id").Value(request_id)
                .Key("error_message").Value("unknown request type")
            .EndDict();
        }
    }

    return json::Document(arr_ctx.EndArray().Build());
}