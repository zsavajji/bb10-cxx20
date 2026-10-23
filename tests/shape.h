// The interface between modules.cpp and libshape.so.
#pragma once
#include <memory>
#include <stdexcept>
#include <string>

struct Shape {
    virtual ~Shape() = default;
    virtual double area() const = 0;
    virtual std::string name() const = 0;
};

struct BadShape : std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

std::unique_ptr<Shape> makeShape(const std::string& description); // throws BadShape
int& shapeThreadCounter(); // a thread_local in the library
