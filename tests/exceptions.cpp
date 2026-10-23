// C++ exceptions, one step at a time: each step is named before it runs, so
// a crash shows where. tests/build.sh builds it twice: `exceptions` with the
// toolchain's default unwinder (LLVM's libunwind, linked in), and
// `exceptions-sysunwind` with BB10's own (libgcc's, in libc.so.3/libm.so.2).
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

static int failures = 0;

template<typename Step>
static void step(const char* name, Step body)
{
    std::printf("%-36s", name);
    std::fflush(stdout);
    bool ok = body();
    std::printf("%s\n", ok ? "ok" : "FAIL");
    if (!ok)
        ++failures;
}

[[gnu::noinline]] static void thrower(int depth)
{
    std::string frame = "frame " + std::to_string(depth); // a cleanup in every frame
    if (!depth)
        throw std::runtime_error("deep");
    thrower(depth - 1);
    std::printf("%s", frame.c_str()); // not reached
}

struct Guard {
    bool& destroyed;
    ~Guard() { destroyed = true; }
};

int main()
{
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    step("throw and catch in one function", [] {
        try {
            throw 42;
        } catch (int value) {
            return value == 42;
        }
        return false;
    });
    step("through 6 frames with cleanups", [] {
        try {
            thrower(5);
        } catch (const std::runtime_error& e) {
            return std::string(e.what()) == "deep";
        }
        return false;
    });
    step("destructor while unwinding", [] {
        bool destroyed = false;
        try {
            Guard guard { destroyed };
            throw std::logic_error("x");
        } catch (const std::logic_error&) {
        }
        return destroyed;
    });
    step("std::out_of_range from libc++", [] {
        try {
            std::vector<int>().at(1);
        } catch (const std::out_of_range&) {
            return true;
        }
        return false;
    });
    step("rethrow", [] {
        try {
            try {
                throw std::runtime_error("again");
            } catch (...) {
                throw;
            }
        } catch (const std::runtime_error& e) {
            return std::string(e.what()) == "again";
        }
        return false;
    });
    step("exception_ptr", [] {
        std::exception_ptr saved;
        try {
            throw std::invalid_argument("kept");
        } catch (...) {
            saved = std::current_exception();
        }
        try {
            std::rethrow_exception(saved);
        } catch (const std::invalid_argument& e) {
            return std::string(e.what()) == "kept";
        }
        return false;
    });
    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
