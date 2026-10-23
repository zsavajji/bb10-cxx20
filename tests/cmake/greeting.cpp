#include <format>
#include <string>

std::string greeting(std::string_view who)
{
    return std::format("hello, {}", who);
}
