#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Test {

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct Registrar {
    Registrar(std::string suite, std::string name, std::function<void()> fn) {
        registry().push_back({ std::move(suite), std::move(name), std::move(fn) });
    }
};

inline int& checkCount()   { static int n = 0; return n; }
inline int& failureCount() { static int n = 0; return n; }
inline std::string& currentTest() { static std::string s; return s; }

inline void fail(const char* file, int line, const std::string& msg) {
    std::cerr << "  FAIL [" << currentTest() << "] "
              << file << ":" << line << "\n"
              << "       " << msg << "\n";
    failureCount()++;
}

inline int run() {
    std::string lastSuite;
    for (auto& t : registry()) {
        if (t.suite != lastSuite) {
            std::cout << "=== " << t.suite << " ===\n";
            lastSuite = t.suite;
        }
        currentTest() = t.name;
        int before = failureCount();
        try {
            t.fn();
        } catch (const std::exception& e) {
            fail("<exception>", 0, std::string("threw: ") + e.what());
        } catch (...) {
            fail("<exception>", 0, "threw unknown");
        }
        if (failureCount() == before)
            std::cout << "  ok   " << t.name << "\n";
    }

    std::cout << "\n";
    std::cout << "Total checks:   " << checkCount()   << "\n";
    std::cout << "Total failures: " << failureCount() << "\n";
    return failureCount() == 0 ? 0 : 1;
}

} // namespace Test

#define TEST(suite, name)                                                \
    static void suite##_##name##_impl();                                 \
    static ::Test::Registrar suite##_##name##_reg(                       \
        #suite, #name, &suite##_##name##_impl);                          \
    static void suite##_##name##_impl()

#define CHECK(expr) do {                                                 \
    ::Test::checkCount()++;                                              \
    if (!(expr)) ::Test::fail(__FILE__, __LINE__,                        \
        std::string("CHECK failed: ") + #expr);                          \
} while (0)

#define CHECK_NEAR(a, b, eps) do {                                       \
    ::Test::checkCount()++;                                              \
    double _a = (double)(a);                                             \
    double _b = (double)(b);                                             \
    if (std::abs(_a - _b) > (double)(eps)) {                             \
        std::ostringstream _os;                                          \
        _os << "CHECK_NEAR failed: " #a " ≈ " #b                         \
            << " (" << _a << " vs " << _b                                \
            << ", diff=" << (_a - _b) << ")";                            \
        ::Test::fail(__FILE__, __LINE__, _os.str());                     \
    }                                                                    \
} while (0)
