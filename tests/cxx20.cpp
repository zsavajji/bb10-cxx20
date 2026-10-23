// C++20 on BB10: the language and library features a modern code base
// (WebKit's WTF, for one) leans on. Prints one line per check and exits
// non-zero if any fails.
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <charconv>
#include <chrono>
#include <cmath>
#include <compare>
#include <concepts>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <latch>
#include <barrier>
#include <semaphore>
#include <source_location>
#include <stop_token>
#include <map>
#include <memory>
#include <mutex>
#include <numbers>
#include <numeric>
#include <optional>
#include <ranges>
#include <regex>
#include <shared_mutex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <typeinfo>
#include <unordered_map>
#include <variant>
#include <vector>

static int failures = 0;

static void check(const char* name, bool ok)
{
    std::printf("%-28s %s\n", name, ok ? "ok" : "FAIL");
    if (!ok)
        ++failures;
}

template<typename T>
concept Number = std::integral<T> || std::floating_point<T>;

static constexpr auto twice(Number auto x) { return x + x; }

struct Version {
    int major, minor;
    auto operator<=>(const Version&) const = default;
};

struct Base {
    virtual ~Base() = default;
};
struct Derived : Base { };

thread_local int perThread = 1;

static int sum(std::span<const int> values)
{
    return std::accumulate(values.begin(), values.end(), 0);
}

int main()
{
    // Language.
    static_assert(twice(21) == 42);
    check("concepts", twice(1.5) == 3.0);
    check("three-way comparison", Version { 10, 3 } > Version { 10, 2 } && (Version { 1, 0 } <=> Version { 1, 0 }) == 0);
    constexpr auto squares = [] {
        std::array<int, 5> a {};
        for (int i = 0; i < 5; ++i)
            a[i] = i * i;
        return a;
    }();
    check("constexpr lambda", squares[4] == 16);

    // Library.
    int values[] = { 3, 1, 4, 1, 5, 9, 2, 6 };
    check("std::span", sum(values) == 31 && std::span(values).subspan(2, 2)[0] == 4);
    check("<bit>", std::popcount(0xF0u) == 4 && std::bit_width(255u) == 8 && std::countr_zero(8u) == 3
            && std::bit_cast<std::uint32_t>(1.0f) == 0x3f800000u);
    auto evens = values | std::views::filter([](int v) { return v % 2 == 0; }) | std::views::transform([](int v) { return v * 10; });
    std::vector<int> picked(evens.begin(), evens.end());
    check("ranges", (picked == std::vector { 40, 20, 60 }));
    std::ranges::sort(values);
    check("ranges::sort", std::ranges::is_sorted(values) && values[7] == 9);
    check("<numbers> <cmath>", std::abs(std::numbers::pi - 4 * std::atan(1.0)) < 1e-12 && std::lround(std::sqrt(2.0) * 1000) == 1414);

    char buffer[32];
    auto [end, ec] = std::to_chars(buffer, buffer + sizeof buffer, 123456789);
    int parsed = 0;
    std::from_chars(buffer, end, parsed);
    check("<charconv> integers", ec == std::errc() && parsed == 123456789);
    // (from_chars for floating point is libc++ 20.)
    end = std::to_chars(buffer, buffer + sizeof buffer, 2500.5).ptr;
    check("<charconv> double", std::string_view(buffer, end) == "2500.5");

    std::string s = "BlackBerry 10";
    std::string_view view = s;
    check("string_view", view.starts_with("Black") && view.ends_with("10") && view.substr(6, 4) == "Berr");
    check("std::to_string", std::to_string(42) + std::to_string(-1) == "42-1" && std::stod("0.25") == 0.25);

    std::variant<int, std::string> var = std::string("qnx");
    check("variant", std::visit([](auto& v) { return sizeof(v) > 0; }, var) && std::holds_alternative<std::string>(var));
    std::optional<int> none;
    check("optional", none.value_or(7) == 7);
    auto shared = std::make_shared<std::map<std::string, int>>();
    (*shared)["a"] = 1;
    std::unordered_map<std::string, int> hashed { { "x", 1 }, { "y", 2 } };
    check("containers", shared->at("a") == 1 && hashed.contains("y"));
    struct alignas(64) CacheLine {
        char bytes[64];
    };
    auto cacheLine = std::make_unique<CacheLine>();
    void* c11 = std::aligned_alloc(256, 1024);
    void* small = aligned_alloc(2, 16);
    check("aligned new, aligned_alloc", reinterpret_cast<std::uintptr_t>(cacheLine.get()) % 64 == 0
            && c11 && reinterpret_cast<std::uintptr_t>(c11) % 256 == 0 && small);
    std::free(c11);
    std::free(small);
    std::function<int(int)> f = [captured = 5](int x) { return x * captured; };
    check("std::function", f(3) == 15);

    // Run-time type information and exceptions, through libc++abi and libunwind.
    std::unique_ptr<Base> object = std::make_unique<Derived>();
    Base& base = *object;
    check("RTTI", dynamic_cast<Derived*>(&base) && typeid(base) == typeid(Derived));
    bool caught = false;
    try {
        std::vector<int>().at(1);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    check("exceptions", caught);
    std::string message;
    try {
        try {
            throw std::runtime_error("inner");
        } catch (...) {
            std::throw_with_nested(std::logic_error("outer"));
        }
    } catch (const std::logic_error& e) {
        message = e.what();
        try {
            std::rethrow_if_nested(e);
        } catch (const std::runtime_error& inner) {
            message += std::string("/") + inner.what();
        }
    }
    check("nested exceptions", message == "outer/inner");

    // Threads, on BB10's pthreads; thread_local through emulated TLS.
    std::atomic<int> counter = 0;
    std::atomic<std::uint64_t> wide = 0;
    std::mutex mutex;
    std::shared_mutex sharedMutex;
    int guarded = 0;
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t] {
            perThread = 100 + t;
            for (int i = 0; i < 10000; ++i) {
                counter.fetch_add(1, std::memory_order_relaxed);
                wide += 1;
                std::lock_guard lock(mutex);
                ++guarded;
            }
            std::shared_lock read(sharedMutex);
            if (perThread != 100 + t)
                counter = -1000000;
        });
    }
    for (auto& thread : threads)
        thread.join();
    check("std::thread + atomics", counter == 40000 && wide == 40000 && guarded == 40000);
    check("thread_local", perThread == 1);

    std::condition_variable ready;
    bool flag = false;
    std::thread signaller([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::lock_guard lock(mutex);
        flag = true;
        ready.notify_one();
    });
    auto start = std::chrono::steady_clock::now();
    {
        std::unique_lock lock(mutex);
        ready.wait(lock, [&] { return flag; });
    }
    signaller.join();
    auto waited = std::chrono::steady_clock::now() - start;
    check("condition_variable + chrono", flag && waited >= std::chrono::milliseconds(15));
    check("hardware_concurrency", std::thread::hardware_concurrency() >= 1);
    std::once_flag once;
    int calls = 0;
    for (int i = 0; i < 3; ++i)
        std::call_once(once, [&] { ++calls; });
    check("call_once", calls == 1);

    std::latch done(3);
    std::barrier sync(3);
    std::counting_semaphore<3> slots(2);
    std::atomic<int> phases = 0;
    {
        std::vector<std::jthread> workers;
        for (int t = 0; t < 3; ++t) {
            workers.emplace_back([&] {
                slots.acquire();
                slots.release();
                sync.arrive_and_wait();
                ++phases;
                done.count_down();
            });
        }
        done.wait();
    }
    check("latch barrier semaphore", phases == 3);
    std::atomic<bool> stopped = false;
    {
        std::jthread worker([&](std::stop_token token) {
            while (!token.stop_requested())
                std::this_thread::yield();
            stopped = true;
        });
    }
    check("jthread + stop_token", stopped);
    std::atomic<int> flagWord = 0;
    std::thread waker([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        flagWord = 1;
        flagWord.notify_one();
    });
    flagWord.wait(0);
    waker.join();
    check("atomic wait/notify", flagWord == 1);

    // Formatting, streams, regex and the file system.
    check("std::format", std::format("{:>6}|{:#x}|{:.2f}|{}", "bb", 255, 3.14159, true) == "    bb|0xff|3.14|true");
    check("source_location", std::source_location::current().line() > 0
            && std::string_view(std::source_location::current().function_name()).find("main") != std::string_view::npos);
    std::ostringstream out;
    out << "pi=" << 3.25 << ' ' << std::hex << 255;
    check("ostringstream", out.str() == "pi=3.25 ff");
    std::regex pattern(R"((\d+)\.(\d+))");
    std::smatch match;
    std::string versionText = "OS 10.3.3";
    check("regex", std::regex_search(versionText, match, pattern) && match[1] == "10" && match[2] == "3");

    namespace fs = std::filesystem;
    std::error_code error;
    fs::path dir = fs::temp_directory_path(error) / "bb10-cxx20-test";
    fs::remove_all(dir, error);
    bool created = fs::create_directories(dir / "a" / "b", error);
    {
        std::ofstream file(dir / "a" / "hello.txt");
        file << "hello BB10\n";
    }
    std::string line;
    {
        std::ifstream file(dir / "a" / "hello.txt");
        std::getline(file, line);
    }
    int entries = 0;
    for (auto& entry : fs::recursive_directory_iterator(dir, error)) {
        (void)entry;
        ++entries;
    }
    auto size = fs::file_size(dir / "a" / "hello.txt", error);
    fs::rename(dir / "a" / "hello.txt", dir / "hello.txt", error);
    bool moved = fs::exists(dir / "hello.txt");
    auto removed = fs::remove_all(dir, error);
    check("filesystem", created && line == "hello BB10" && entries == 3 && size == 11 && moved && removed == 4 && !fs::exists(dir));

    std::cout << "iostream " << std::string(23, ' ') << "ok" << std::endl;
    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
