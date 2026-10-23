// libshape.so, linked with -shared-libc++: hands C++ objects and exceptions
// to the program that uses it.
#include "shape.h"

#include <cstdio>
#include <numbers>
#include <sstream>

namespace {

struct Circle final : Shape {
    explicit Circle(double r) : radius(r) { }
    double area() const override { return std::numbers::pi * radius * radius; }
    std::string name() const override { return "circle"; }
    double radius;
};

struct Rectangle final : Shape {
    Rectangle(double w, double h) : width(w), height(h) { }
    double area() const override { return width * height; }
    std::string name() const override { return "rectangle"; }
    double width, height;
};

struct Announcer {
    ~Announcer() { std::printf("libshape: static destructor\n"); }
} announcer;

thread_local int counter = 0;

}

std::unique_ptr<Shape> makeShape(const std::string& description)
{
    std::istringstream in(description);
    std::string kind;
    in >> kind;
    double a = 0, b = 0;
    if (kind == "circle" && in >> a)
        return std::make_unique<Circle>(a);
    if (kind == "rectangle" && in >> a >> b)
        return std::make_unique<Rectangle>(a, b);
    throw BadShape("unknown shape: " + description);
}

int& shapeThreadCounter()
{
    return counter;
}
