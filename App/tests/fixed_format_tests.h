/** @file fixed_format_tests.h 各数值页面格式的逐字节回归扫描，无 LVGL 依赖。 */
#ifndef APP_FIXED_FORMAT_TESTS_H
#define APP_FIXED_FORMAT_TESTS_H
#include "../Utils/FixedFormat.h"
#include <cassert>
#include <cfloat>
#include <initializer_list>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <limits>

inline void test_fixed_format() {
    unsigned comparisons = 0, placeholders = 0;
    const auto check = [&](double value, unsigned precision) {
        char guarded[app::kFixedTextCapacity + 2];
        std::memset(guarded, 0x5a, sizeof(guarded));
        char* actual = guarded + 1;
        char expected[app::kFixedTextCapacity];
        const int size = app::format_fixed(actual, app::kFixedTextCapacity, value, precision);
        const bool in_range = value > -app::kFixedLimit && value < app::kFixedLimit;
        const int reference = in_range
                                  ? std::snprintf(expected, sizeof(expected), "%.*f", static_cast<int>(precision), value)
                                  : std::snprintf(expected, sizeof(expected), "---");
        assert(guarded[0] == 0x5a && guarded[app::kFixedTextCapacity + 1] == 0x5a);
        placeholders += !in_range;
        if (size != reference || std::strcmp(actual, expected)) {
            std::fprintf(stderr, "定点格式不同: %a p=%u actual=%s expected=%s\n", value, precision, actual, expected);
            assert(false);
        }
        ++comparisons;
    };
    // 经纬度整个合法区间，含每个六位小数半值两侧；海拔正负、跨整数进位。
    for (int i = -18000; i <= 18000; ++i) {
        check(i / 100.0, 6);
        const double half = (i + 0.5) / 1000000.0;
        check(half, 6);
        check(std::nextafter(half, -INFINITY), 6);
        check(std::nextafter(half, INFINITY), 6);
        check(i / 10.0, 2);
    }
    // 电池 uint16 原始电压/温度、百分比，保留 float 到 double 的原有提升。
    for (unsigned i = 0; i <= 65535; ++i) {
        check(static_cast<float>(i) / 1000.0f, 2);
        check(static_cast<float>(i) / 10.0f - 273.15f, 1);
        if (i <= 10000)
            check(static_cast<float>(i) / 100.0f, 1);
    }
    // 扫常用 MB 范围与所有 binary32 指数；超出显示范围时验证确定的占位输出。
    for (unsigned i = 0; i <= 50000; ++i) {
        check(static_cast<float>(i) / 8.0f, 0);
        check(static_cast<float>(i) / 8.0f, 1);
    }
    uint64_t random = 0x123456789abcdefULL;
    for (unsigned i = 0; i < 4096; ++i) {
        random ^= random << 13;
        random ^= random >> 7;
        random ^= random << 17;
        const uint32_t bits = static_cast<uint32_t>(random);
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        check(value, 0);
        check(value, 1);
        double altitude;
        std::memcpy(&altitude, &random, sizeof(altitude));
        check(altitude, 2);
    }
    for (unsigned precision = 0; precision <= app::kFixedMaxPrecision; ++precision) {
        for (double value : {0.0, -0.0, 0.5, -0.5, 1.25, -1.25, 2.5, 3.5, 9.9999995,
                             std::nextafter(app::kFixedLimit, 0.0), -std::nextafter(app::kFixedLimit, 0.0),
                             app::kFixedLimit - 0.5, -app::kFixedLimit + 0.5, app::kFixedLimit,
                             -app::kFixedLimit, std::nextafter(app::kFixedLimit, INFINITY),
                             -std::nextafter(app::kFixedLimit, INFINITY),
                             std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),
                             std::numeric_limits<double>::denorm_min(), -std::numeric_limits<double>::denorm_min(),
                             std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::quiet_NaN(), -std::numeric_limits<double>::quiet_NaN()})
            check(value, precision);
        // 各数量级的正负半值以及上下相邻浮点，验证精确舍入和高位进位。
        for (double integer : {0.0, 1.0, 9.0, 999.0, 999999999999999.0}) {
            const double half = integer + 0.5;
            for (double value : {half, std::nextafter(half, -INFINITY), std::nextafter(half, INFINITY)}) {
                check(value, precision);
                check(-value, precision);
            }
        }
    }
    // 单行截断、每个调用点的后缀、多行组合和两种语言的模板也逐字节比较。
    for (double value : {-0.0, -1.25, double(7.605f), double(99.95f), double(1228.8f), 99999.5,
                         std::nextafter(app::kFixedLimit, 0.0)}) {
        for (const char* suffix : {" V", " MB", " m", " C", "%"}) {
            for (unsigned precision : {0U, 1U, 2U, 6U}) {
                char actual[24], expected[24];
                app::format_fixed(actual, sizeof(actual), value, precision, suffix);
                std::snprintf(expected, sizeof(expected), "%.*f%s", int(precision), value, suffix);
                assert(std::strcmp(actual, expected) == 0);
                for (std::size_t size = 0; size <= 24; ++size) {
                    const int a = app::format_fixed(actual, size, value, precision);
                    const int b = std::snprintf(expected, size, "%.*f", int(precision), value);
                    assert(a == b && (!size || std::strcmp(actual, expected) == 0));
                }
            }
        }
        char actual[256], expected[256];
        std::snprintf(actual, sizeof(actual), "%s\n%s\n%s m", app::Fixed(value, 6).c_str(),
                      app::Fixed(-value, 6).c_str(), app::Fixed(value, 2).c_str());
        std::snprintf(expected, sizeof(expected), "%.6f\n%.6f\n%.2f m", value, -value, value);
        assert(std::strcmp(actual, expected) == 0);
        std::snprintf(actual, sizeof(actual), "%s%%\n%s V\n%s C\n%s", app::Fixed(value, 1).c_str(),
                      app::Fixed(value, 2).c_str(), app::Fixed(value, 1).c_str(), "fastCharge");
        std::snprintf(expected, sizeof(expected), "%.1f%%\n%.2f V\n%.1f C\n%s", value, value, value, "fastCharge");
        assert(std::strcmp(actual, expected) == 0);
        std::snprintf(actual, sizeof(actual), "%s\n%s\n%s MB\n%s\n%s", "ON", "record", app::Fixed(value, 1).c_str(), "type", "1s");
        std::snprintf(expected, sizeof(expected), "%s\n%s\n%.1f MB\n%s\n%s", "ON", "record", value, "type", "1s");
        assert(std::strcmp(actual, expected) == 0);
        for (bool russian : {false, true}) {
            std::snprintf(actual, sizeof(actual), russian ? "БАТАРЕЯ %s C - ОТКЛЮЧИТЕ ПИТАНИЕ" : "BATTERY HOT %s C - PLACE IN COOL PLACE", app::Fixed(value, 1).c_str());
            std::snprintf(expected, sizeof(expected), russian ? "БАТАРЕЯ %.1f C - ОТКЛЮЧИТЕ ПИТАНИЕ" : "BATTERY HOT %.1f C - PLACE IN COOL PLACE", value);
            assert(std::strcmp(actual, expected) == 0);
        }
    }
    for (double value : {app::kFixedLimit, -app::kFixedLimit, double(FLT_MAX), -double(FLT_MAX),
                         std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN(), -std::numeric_limits<double>::quiet_NaN()}) {
        for (unsigned precision = 0; precision <= app::kFixedMaxPrecision; ++precision) {
            assert(std::strcmp(app::Fixed(value, precision).c_str(), "---") == 0);
            for (std::size_t size = 0; size <= app::kFixedTextCapacity; ++size) {
                char actual[app::kFixedTextCapacity], expected[app::kFixedTextCapacity];
                const int a = app::format_fixed(actual, size, value, precision, " MB");
                const int b = std::snprintf(expected, size, "--- MB");
                assert(a == b && (!size || std::strcmp(actual, expected) == 0));
            }
        }
    }
    char invalid = 'x';
    assert(app::format_fixed(&invalid, 1, 1.0, 7) == -1 && invalid == '\0');
    assert(app::format_fixed(nullptr, 0, -0.0, 6) == 9);
    static_assert(sizeof(app::Fixed) == 24, "定点临时文本必须保持 24 字节");
    std::printf("定点格式扫描：%u 次范围内比较、%u 次占位比较及全部页面模板/截断检查通过\n",
                comparisons - placeholders, placeholders);
}
#endif
