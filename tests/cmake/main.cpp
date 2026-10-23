#include <cstdio>
#include <string>
#include <thread>

std::string greeting(std::string_view who);
extern "C" int counter_next(void);

int main()
{
    std::jthread([] { counter_next(); }).join();
    bool ok = greeting("BB10") == "hello, BB10" && counter_next() == 2;
    std::printf("%-28s %s\n%s\n", "cmake build", ok ? "ok" : "FAIL", ok ? "all passed" : "FAILED");
    return ok ? 0 : 1;
}
