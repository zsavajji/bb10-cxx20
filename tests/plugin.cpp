// plugin.so, linked with the static (hermetic) runtime and loaded with
// dlopen(): its C++ runtime stays private, and dlclose() runs its static
// destructors before it is unmapped.
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>

namespace {

struct Announcer {
    ~Announcer() { std::printf("plugin: static destructor\n"); }
} announcer;

std::map<std::string, int> table { { "one", 1 }, { "two", 2 } };

}

extern "C" int plugin_lookup(const char* key)
{
    try {
        return table.at(key);
    } catch (const std::out_of_range&) {
        return -1;
    }
}
