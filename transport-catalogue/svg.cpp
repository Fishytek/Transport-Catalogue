#include "svg.h"
#include <sstream>

namespace svg {


using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    RenderObject(context);
    context.out << std::endl;
}

Circle& Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }


Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool first = true;
    for (const auto& point : points_) {
        if (!first) out << " "sv;
        out << point.x << ","sv << point.y;
        first = false;
    }
    out << "\"";
    RenderAttrs(out); 
    out << " />"sv;
}


void Polyline::Render(std::ostream& out) const {
    RenderContext ctx(out, 2);
    RenderObject(ctx);
}



Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::Render(std::ostream& out) const {
    RenderContext ctx(out, 2);
    RenderObject(ctx);
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"
        << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\""
        << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""
        << " font-size=\""sv << font_size_ << "\"";
    if (!font_family_.empty()) {
        out << " font-family=\""sv << font_family_ << "\"";
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\"";
    }
    RenderAttrs(out);
    out << ">"sv;

    for (char c : data_) {
        switch (c) {
            case '"': out << "&quot;"sv; break;
            case '\'': out << "&apos;"sv; break;
            case '<': out << "&lt;"sv; break;
            case '>': out << "&gt;"sv; break;
            case '&': out << "&amp;"sv; break;
            default: out << c;
        }
    }

    out << "</text>"sv;
}


void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";

    RenderContext ctx(out, 2);
    RenderObject(ctx);

    out << "</svg>"sv;
}

void Document::RenderObject(const RenderContext& context) const {
    for (const auto& obj : objects_) {
        obj->Render(context);
    }
}

}