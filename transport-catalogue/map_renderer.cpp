#include "map_renderer.h"
#include "geo.h"
#include "svg.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

using namespace std;

inline const double EPSILON = 1e-6;
bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

template <typename PointInputIt>
SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                               double max_width, double max_height, double padding)
    : padding_(padding)
{
    if (points_begin == points_end) {
        return;
    }

    const auto [left_it, right_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    const auto [bottom_it, top_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
        zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
        zoom_coeff_ = *height_zoom;
    }
}

svg::Point SphereProjector::operator()(Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

MapRenderer::MapRenderer(const transport_catalogue::TransportCatalogue& catalogue,
                       const RenderSettings& render_settings)
    : catalogue_(catalogue), render_settings_(render_settings) {}

void MapRenderer::Render(svg::Document& doc) const {
    vector<Coordinates> allCoordinates = CollectAllCoordinates();
    SphereProjector projector = CreateProjector(allCoordinates);

    vector<const transport_catalogue::Bus*> buses = GetSortedNonEmptyBuses();
    RenderBusRoutes(doc, buses, projector);
    RenderBusLabels(doc, buses, projector);

    vector<const transport_catalogue::Stop*> stopsToRender = GetSortedBusStops();
    RenderStopCircles(doc, stopsToRender, projector);
    RenderStopLabels(doc, stopsToRender, projector);
}

vector<Coordinates> MapRenderer::CollectAllCoordinates() const {
    vector<Coordinates> coordinates;
    unordered_set<const transport_catalogue::Stop*> uniqueStops;

    for (const auto& bus : catalogue_.GetBuses()) {
        for (const auto* stop : bus.stops) {
            if (uniqueStops.insert(stop).second) {
                coordinates.push_back(stop->coordinates);
            }
        }
    }
    return coordinates;
}

SphereProjector MapRenderer::CreateProjector(const vector<Coordinates>& coordinates) const {
    return SphereProjector(coordinates.begin(), coordinates.end(),
                           render_settings_.width,
                           render_settings_.height,
                           render_settings_.padding);
}

vector<const transport_catalogue::Bus*> MapRenderer::GetSortedNonEmptyBuses() const {
    vector<const transport_catalogue::Bus*> buses;
    for (const auto& bus : catalogue_.GetBuses()) {
        if (!bus.stops.empty()) {
            buses.push_back(&bus);
        }
    }
    sort(buses.begin(), buses.end(), [](const auto* lhs, const auto* rhs) {
        return lhs->name < rhs->name;
    });
    return buses;
}

vector<const transport_catalogue::Stop*> MapRenderer::GetSortedBusStops() const {
    unordered_set<const transport_catalogue::Stop*> busStops;
    for (const auto& bus : catalogue_.GetBuses()) {
        for (const auto* stop : bus.stops) {
            busStops.insert(stop);
        }
    }

    vector<const transport_catalogue::Stop*> sortedStops(busStops.begin(), busStops.end());
    sort(sortedStops.begin(), sortedStops.end(), [](const auto* lhs, const auto* rhs) {
        return lhs->name < rhs->name;
    });
    return sortedStops;
}

void MapRenderer::RenderBusRoutes(svg::Document& doc,
                                  const vector<const transport_catalogue::Bus*>& buses,
                                  const SphereProjector& projector) const {
    for (size_t i = 0; i < buses.size(); ++i) {
        const auto* bus = buses[i];
        svg::Polyline polyline;
        polyline.SetStrokeColor(render_settings_.color_palette[i % render_settings_.color_palette.size()])
               .SetFillColor("none")
               .SetStrokeWidth(render_settings_.line_width)
               .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
               .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        for (const auto* stop : bus->stops) {
            polyline.AddPoint(projector(stop->coordinates));
        }
        if (!bus->is_roundtrip) {
            for (auto it = bus->stops.rbegin() + 1; it != bus->stops.rend(); ++it) {
                polyline.AddPoint(projector((*it)->coordinates));
            }
        }
        doc.Add(std::move(polyline));
    }
}

void MapRenderer::RenderBusLabels(svg::Document& doc,
                                  const vector<const transport_catalogue::Bus*>& buses,
                                  const SphereProjector& projector) const {
    for (size_t i = 0; i < buses.size(); ++i) {
        const auto* bus = buses[i];
        if (bus->stops.empty()) continue;

        const auto& color = render_settings_.color_palette[i % render_settings_.color_palette.size()];
        RenderBusLabelForStop(doc, bus, bus->stops.front(), color, projector);

        if (!bus->is_roundtrip && bus->stops.front() != bus->stops.back()) {
            RenderBusLabelForStop(doc, bus, bus->stops.back(), color, projector);
        }
    }
}

void MapRenderer::RenderBusLabelForStop(svg::Document& doc,
                                        const transport_catalogue::Bus* bus,
                                        const transport_catalogue::Stop* stop,
                                        const svg::Color& color,
                                        const SphereProjector& projector) const {
    auto point = projector(stop->coordinates);

    svg::Text underlayer;
    underlayer.SetFillColor(render_settings_.underlayer_color)
             .SetStrokeColor(render_settings_.underlayer_color)
             .SetStrokeWidth(render_settings_.underlayer_width)
             .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
             .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
             .SetPosition(point)
             .SetOffset({render_settings_.bus_label_offset.first, render_settings_.bus_label_offset.second})
             .SetFontSize(render_settings_.bus_label_font_size)
             .SetFontFamily("Verdana")
             .SetFontWeight("bold")
             .SetData(bus->name);
    doc.Add(std::move(underlayer));

    svg::Text text;
    text.SetFillColor(color)
        .SetPosition(point)
        .SetOffset({render_settings_.bus_label_offset.first, render_settings_.bus_label_offset.second})
        .SetFontSize(render_settings_.bus_label_font_size)
        .SetFontFamily("Verdana")
        .SetFontWeight("bold")
        .SetData(bus->name);
    doc.Add(std::move(text));
}

void MapRenderer::RenderStopCircles(svg::Document& doc,
                                    const vector<const transport_catalogue::Stop*>& stops,
                                    const SphereProjector& projector) const {
    for (const auto* stop : stops) {
        auto point = projector(stop->coordinates);
        svg::Circle circle;
        circle.SetCenter(point)
              .SetRadius(render_settings_.stop_radius)
              .SetFillColor("white");
        doc.Add(std::move(circle));
    }
}

void MapRenderer::RenderStopLabels(svg::Document& doc,
                                   const vector<const transport_catalogue::Stop*>& stops,
                                   const SphereProjector& projector) const {
    for (const auto* stop : stops) {
        auto point = projector(stop->coordinates);

        svg::Text underlayer;
        underlayer.SetFillColor(render_settings_.underlayer_color)
                 .SetStrokeColor(render_settings_.underlayer_color)
                 .SetStrokeWidth(render_settings_.underlayer_width)
                 .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                 .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                 .SetPosition(point)
                 .SetOffset({render_settings_.stop_label_offset.first, render_settings_.stop_label_offset.second})
                 .SetFontSize(render_settings_.stop_label_font_size)
                 .SetFontFamily("Verdana")
                 .SetData(stop->name);
        doc.Add(std::move(underlayer));

        svg::Text text;
        text.SetFillColor("black")
            .SetPosition(point)
            .SetOffset({render_settings_.stop_label_offset.first, render_settings_.stop_label_offset.second})
            .SetFontSize(render_settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(stop->name);
        doc.Add(std::move(text));
    }
}