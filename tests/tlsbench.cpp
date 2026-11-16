// thread_local on BB10 (emulated TLS: every access calls __emutls_get_address,
// src-rt/emutls.c): what an access costs, and that it still works. Prints the
// time per access of a thread_local int in the main thread and in two threads
// running at once, against an ordinary global as baseline, then one line per
// check, and exits non-zero if any fails.
//
//   tlsbench [accesses per loop, default 50000000]
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <latch>
#include <thread>
#include <utility>
#include <vector>

static int failures = 0;

static void check(const char* name, bool ok)
{
    std::printf("%-34s %s\n", name, ok ? "ok" : "FAIL");
    if (!ok)
        ++failures;
}

// ---- Speed -----------------------------------------------------------------

static int plain;
thread_local int counter;

// The address comes from a call the compiler can neither inline nor move out of
// the loop (the empty asm is a side effect), so every iteration pays for it,
// for the global (the baseline) as for the thread_local.
__attribute__((noinline)) static int* plainAddress()
{
    int* address = &plain;
    asm volatile("" : "+r"(address)::"memory"); // else the callers learn the address
    return address;
}

__attribute__((noinline)) static int* counterAddress()
{
    asm volatile("" ::: "memory");
    return &counter;
}

// n reads and increments through address(), in this thread: ns per access.
template<int* (*address)()>
static double run(int n, bool& counted)
{
    *address() = 0; // the thread's first use of the variable, outside the timing
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i)
        ++*address();
    auto elapsed = std::chrono::steady_clock::now() - start;
    counted = *address() == n;
    return std::chrono::duration<double, std::nano>(elapsed).count() / n;
}

static void report(const char* name, double ns)
{
    std::printf("%-34s %7.2f ns/access\n", name, ns);
}

// ---- Behaviour -------------------------------------------------------------

// 100 thread_locals, with initial values, so that a thread's array of them
// grows (it starts with room for a dozen or so).
template<int N>
struct Slot {
    static thread_local int value;
};
template<int N>
thread_local int Slot<N>::value = N;

template<int N>
__attribute__((noinline)) static int& slot()
{
    return Slot<N>::value;
}

constexpr int slotCount = 100;

template<int... Is>
static constexpr std::array<int& (*)(), sizeof...(Is)> makeSlots(std::integer_sequence<int, Is...>)
{
    return { &slot<Is>... };
}

static constexpr auto slots = makeSlots(std::make_integer_sequence<int, slotCount>());

// The first touch of each: holds the initial value. Then the thread's own values.
static bool touchSlots(int salt, bool descending)
{
    bool good = true;
    for (int n = 0; n < slotCount; ++n) {
        int i = descending ? slotCount - 1 - n : n;
        good &= slots[i]() == i;
        slots[i]() = i + salt;
    }
    return good;
}

static bool slotsHold(int salt)
{
    bool good = true;
    for (int i = 0; i < slotCount; ++i)
        good &= slots[i]() == i + salt;
    return good;
}

static std::atomic<int> constructed, destroyed, destroyedIntact;
thread_local int mark;

// An aligned object with a constructor and a destructor. The destructor runs
// when its thread exits, and must find the thread's other thread_locals as
// the thread left them (mark): they are not freed before it runs.
struct alignas(16) Tracked {
    int value;
    int id = 0;
    Tracked() : value(42) { ++constructed; }
    ~Tracked()
    {
        ++destroyed;
        if (value == 42 && id != 0 && mark == id)
            ++destroyedIntact;
    }
};

thread_local Tracked tracked;

static bool aligned(const void* p)
{
    return reinterpret_cast<std::uintptr_t>(p) % 16 == 0;
}

// A thread that sets all its thread_locals to its own values, waits until the
// others have too, and finds them unchanged.
static void worker(int id, bool descending, std::latch& written, bool& ok)
{
    bool good = aligned(&tracked) && tracked.value == 42 && tracked.id == 0 && mark == 0 && counter == 0;
    tracked.id = mark = counter = id;
    good &= touchSlots(id * 1000, descending);
    written.arrive_and_wait();
    ok = good && tracked.id == id && mark == id && counter == id && slotsHold(id * 1000);
}

// A short-lived thread that touches everything, then exits.
static void churn(std::atomic<int>& bad)
{
    tracked.id = mark = 7;
    if (!aligned(&tracked) || !touchSlots(1, false) || !slotsHold(1))
        ++bad;
}

int main(int argc, char** argv)
{
    int n = argc > 1 ? std::atoi(argv[1]) : 50000000;
    std::printf("%d accesses per loop\n", n);
    bool counted[4];

    double baseline = run<plainAddress>(n, counted[0]);
    double main1 = run<counterAddress>(n, counted[1]);
    double thread1, thread2;
    {
        std::thread a([&] { thread1 = run<counterAddress>(n, counted[2]); });
        std::thread b([&] { thread2 = run<counterAddress>(n, counted[3]); });
        a.join();
        b.join();
    }
    report("global (baseline)", baseline);
    report("thread_local, main thread", main1);
    report("thread_local, thread 1 of 2", thread1);
    report("thread_local, thread 2 of 2", thread2);
    std::printf("%-34s %7.2f ns\n", "thread_local over the baseline", (main1 + thread1 + thread2) / 3 - baseline);
    check("loops counted", counted[0] && counted[1] && counted[2] && counted[3]);

    // The main thread's values, which the others must not disturb.
    bool mainFirst = aligned(&tracked) && tracked.value == 42 && touchSlots(555, false);
    tracked.id = mark = 99;
    check("main: first use, aligned 16", mainFirst);

    const int threads = 4;
    bool ok[threads];
    std::latch written(threads);
    {
        std::vector<std::thread> pool;
        for (int i = 0; i < threads; ++i)
            pool.emplace_back(worker, i + 1, i % 2 == 1, std::ref(written), std::ref(ok[i]));
        for (auto& t : pool)
            t.join();
    }
    check("threads: independent values", ok[0] && ok[1] && ok[2] && ok[3]);
    check("main: unchanged by them", tracked.id == 99 && mark == 99 && slotsHold(555));
    check("constructors, one per thread", constructed == 1 + threads);
    check("destructors ran at thread exit", destroyed == threads);
    check("destructors saw their thread_locals", destroyedIntact == threads);

    // Many threads that come and go, a few at a time.
    std::atomic<int> bad{0};
    for (int round = 0; round < 10; ++round) {
        std::vector<std::thread> pool;
        for (int i = 0; i < 8; ++i)
            pool.emplace_back(churn, std::ref(bad));
        for (auto& t : pool)
            t.join();
    }
    check("80 threads, exit without crash", bad == 0);
    check("destructors, all of them", constructed == 1 + threads + 80 && destroyed == threads + 80
            && destroyedIntact == threads + 80);

    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
