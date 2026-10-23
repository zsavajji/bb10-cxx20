// Linked with -shared-libc++ against libshape.so: C++ objects, RTTI,
// exceptions and thread_local across a shared library boundary, all on the
// one libc++_shared.so.
#include "shape.h"

#include <cmath>
#include <cstdio>
#include <thread>
#include <typeinfo>

static int failures = 0;

static void check(const char* name, bool ok)
{
    std::printf("%-28s %s\n", name, ok ? "ok" : "FAIL");
    if (!ok)
        ++failures;
}

int main()
{
    auto circle = makeShape("circle 2");
    check("virtual call", std::abs(circle->area() - 12.566370614359172) < 1e-9 && circle->name() == "circle");
    const Shape& shape = *circle;
    check("RTTI across modules", std::string(typeid(shape).name()).find("Circle") != std::string::npos);

    std::string caught;
    try {
        makeShape("hexagon 1");
    } catch (const std::logic_error& e) { // BadShape, thrown in libshape.so
        caught = e.what();
        check("caught by base class", dynamic_cast<const BadShape*>(&e) != nullptr);
    }
    check("exception across modules", caught == "unknown shape: hexagon 1");

    shapeThreadCounter() = 5;
    int other = -1;
    std::thread([&] { other = ++shapeThreadCounter(); }).join();
    check("thread_local across modules", shapeThreadCounter() == 5 && other == 1);

    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
