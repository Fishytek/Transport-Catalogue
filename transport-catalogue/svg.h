#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

struct Rgb{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct Rgba{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    double opacity = 1;
};
using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline const Color NoneColor = std::monostate();

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

struct Point {
    Point() = default;
    Point(double x, double y) : x(x), y(y) {}
    double x = 0;
    double y = 0;
};

inline std::ostream& operator<<(std::ostream& out, const Rgb& color){
    out << "rgb(" << static_cast<int>(color.red) << "," << static_cast<int>(color.green) << "," << static_cast<int>(color.blue) << ")";
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const Rgba& color){
    out << "rgba(" << static_cast<int>(color.red) << "," << static_cast<int>(color.green) << "," << static_cast<int>(color.blue) << "," << color.opacity <<")";
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const Color& color){
    struct ColorPrinter{
        std::ostream& output;
        void operator()(std::monostate){
            output << "none";
        }
        void operator()(const std::string& str){
            output << str;
        }
        void operator()(const Rgb& rgb){
            output << rgb;
        }
        void operator()(const Rgba& rgba){
            output << rgba;
        }
    };
    std::visit(ColorPrinter{out}, color);
    return out;
}

inline std::ostream& operator<<(std::ostream& out, StrokeLineCap cap) {
    using namespace std::literals;
    switch (cap) {
        case StrokeLineCap::BUTT:
            out << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            out << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"sv;
            break;
    }
    return out;
}


inline std::ostream& operator<<(std::ostream& out, StrokeLineJoin join) {
    using namespace std::literals;
    switch (join) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
    }
    return out;
}

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeWidth(double width) {
        stroke_width_ = std::move(width);
        return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap linecap){
        stroke_linecap_ = std::move(linecap);
        return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin linejoin){
        stroke_linejoin_ = std::move(linejoin);
        return AsOwner();
    }

protected:
    ~PathProps() = default;
    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if(stroke_width_){
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if(stroke_linecap_){
            out << " stroke-linecap=\""sv << *stroke_linecap_ << "\""sv;
        }
        if(stroke_linejoin_){
            out << " stroke-linejoin=\""sv << *stroke_linejoin_ << "\""sv;
        }
    }

private:
    Owner& AsOwner() {
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_linecap_;
    std::optional<StrokeLineJoin> stroke_linejoin_;
};

struct RenderContext {
    RenderContext(std::ostream& out) : out(out) {}
    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out), indent_step(indent_step), indent(indent) {}

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

class Object {
public:
    void Render(const RenderContext& context) const;
    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;
    Point center_;
    double radius_ = 1.0;
};

class Polyline : public Object, public PathProps<Polyline> {
public:
    Polyline& AddPoint(Point point);
    void Render(std::ostream& out) const;

private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<Point> points_;
};

class Text : public Object, public PathProps<Text> {
public:
    Text& SetPosition(Point pos);
    Text& SetOffset(Point offset);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);
    Text& SetData(std::string data);
    void Render(std::ostream& out) const;

private:
    void RenderObject(const RenderContext& context) const override;
    Point pos_;
    Point offset_;
    uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

class ObjectContainer {
public:
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
    virtual ~ObjectContainer() = default;

    template <typename Obj>
    void Add(Obj obj) {
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    }
};

class Drawable {
public:
    virtual void Draw(ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};

class Document : public Object, public ObjectContainer {
public:
    void AddPtr(std::unique_ptr<Object>&& obj) override;
    void Render(std::ostream& out) const;

private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<std::unique_ptr<Object>> objects_;
};

} 