#pragma once
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <memory>

using namespace std;

namespace Svg {

  struct Point {
    double x = 0.0, y = 0.0;
  };

  struct Rgb {
    uint8_t red, green, blue;
  };

  struct Rgba {
    Rgb rgb;
    double a;
  };

  using Color = variant<monostate, string, Rgb, Rgba>;
  const Color NoneColor{};

  struct Render {
    static void RenderColor(ostream& out, monostate) {
      out << "none";
    }

    static void RenderColor(ostream& out, const string& value) {
      out << value;
    }

    static void RenderColor(ostream& out, Rgb rgb) {
      out << "rgb(" << static_cast<int>(rgb.red)
          << "," << static_cast<int>(rgb.green)
          << "," << static_cast<int>(rgb.blue) << ")";
    }

    static void RenderColor(ostream& out, Rgba rgba) {
      out << "rgba(" << static_cast<int>(rgba.rgb.red)
          << "," << static_cast<int>(rgba.rgb.green)
          << "," << static_cast<int>(rgba.rgb.blue) 
          << "," << rgba.a << ")";
    }

    static void RenderColor(ostream& out, const Color& color) {
      visit([&out](const auto& value) { RenderColor(out, value); },
            color);
    }
  };


  class Object {
  public:
    virtual void Render(ostream& out) const = 0;
    virtual ~Object() = default;
  };

  template <typename Owner>
  class BaseAttrs {
  public:
    Owner& SetFillColor(const Color& color) {
      fill_color_ = color;
      return AsOwner();
    }

    Owner& SetStrokeColor(const Color& color) {
      stroke_color_ = color;
      return AsOwner();
    }

    Owner& SetStrokeWidth(double value) {
      stroke_width_ = value;
      return AsOwner();
    }
    Owner& SetStrokeLineCap(const string& value) {
      stroke_line_cap_ = value;
      return AsOwner();
    }
    Owner& SetStrokeLineJoin(const string& value) {
      stroke_line_join_ = value;
      return AsOwner();
    }
    void RenderAttrs(ostream& out) const {
    out << "fill=\\\"";
    Render::RenderColor(out, fill_color_);
    out << "\\\" ";
    out << "stroke=\\\"";
    Render::RenderColor(out, stroke_color_);
    out << "\\\" ";
    out << "stroke-width=\\\"" << stroke_width_ << "\\\" ";
    if (stroke_line_cap_) {
        out << "stroke-linecap=\\\"" << *stroke_line_cap_ << "\\\" ";
    }
    if (stroke_line_join_) {
        out << "stroke-linejoin=\\\"" << *stroke_line_join_ << "\\\" ";
    }
    }
    
  private:
    Color fill_color_;
    Color stroke_color_;
    double stroke_width_ = 1.0;
    optional<string> stroke_line_cap_;
    optional<string> stroke_line_join_;

    Owner& AsOwner() {
      return static_cast<Owner&>(*this);
    }
  };

  class Circle : public Object, public BaseAttrs<Circle> {
  public:
    Circle& SetCenter(Point point);
    Circle& SetRadius(double radius);
    void Render(ostream& out) const override;

  private:
    Point center_;
    double radius_ = 1;
  };

  class Polyline : public Object, public BaseAttrs<Polyline> {
  public:
    Polyline& AddPoint(Point point);
    void Render(ostream& out) const override;

  private:
    vector<Point> points_;
  };

  class Text : public Object, public BaseAttrs<Text> {
  public:
    Text& SetPoint(Point point);
    Text& SetOffset(Point point);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(const string& value);
    Text& SetData(const string& data);
    void Render(ostream& out) const override;

  private:
    Point point_;
    Point offset_;
    uint32_t font_size_ = 1;
    optional<string> font_family_;
    string data_;
  };

  class Document : public Object {
  public:
    template <typename ObjectType>
    void Add(ObjectType object) {
      objects_.push_back(make_unique<ObjectType>(move(object)));
    }

    void Render(ostream& out) const override;

  private:
    vector<unique_ptr<Object>> objects_;
  };
}
