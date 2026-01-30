#pragma once

#include "transport_catalogue.h"
#include "svg.h"
#include "geo.h"
#include <vector>
#include <utility>

struct RenderSettings {
    double width;
    double height;
    double padding;
    double line_width;
    double stop_radius;
    
    int bus_label_font_size;
    std::pair<double, double> bus_label_offset;
    
    int stop_label_font_size;
    std::pair<double, double> stop_label_offset;
    
    svg::Color underlayer_color;
    double underlayer_width;
    
    std::vector<svg::Color> color_palette;
};


class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding);

    svg::Point operator()(Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    MapRenderer(const transport_catalogue::TransportCatalogue& catalogue,
                const RenderSettings& render_settings);

    void Render(svg::Document& doc) const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;
    RenderSettings render_settings_;

    std::vector<Coordinates> CollectAllCoordinates() const;

    SphereProjector CreateProjector(const std::vector<Coordinates>& coordinates) const;

    std::vector<const transport_catalogue::Bus*> GetSortedNonEmptyBuses() const;

    std::vector<const transport_catalogue::Stop*> GetSortedBusStops() const;

    void RenderBusRoutes(svg::Document& doc,
                         const std::vector<const transport_catalogue::Bus*>& buses,
                         const SphereProjector& projector) const;

    void RenderBusLabels(svg::Document& doc,
                         const std::vector<const transport_catalogue::Bus*>& buses,
                         const SphereProjector& projector) const;

    void RenderBusLabelForStop(svg::Document& doc,
                               const transport_catalogue::Bus* bus,
                               const transport_catalogue::Stop* stop,
                               const svg::Color& color,
                               const SphereProjector& projector) const;

    void RenderStopCircles(svg::Document& doc,
                           const std::vector<const transport_catalogue::Stop*>& stops,
                           const SphereProjector& projector) const;

    void RenderStopLabels(svg::Document& doc,
                          const std::vector<const transport_catalogue::Stop*>& stops,
                          const SphereProjector& projector) const;
};