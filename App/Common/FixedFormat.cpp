/** @file FixedFormat.cpp 无堆分配的定点文本转换实现，由 app_core 统一编译。 */
#include "FixedFormat.h"
#include <cstdint>
#include <cstring>
#include <limits>

namespace app {
namespace {
/** @brief 将数值和后缀写入同一有界缓冲，并累计未截断长度。 */
struct FixedOutput {
    char* data;
    std::size_t capacity;
    std::size_t written = 0;

    void put(char c) {
        if (written + 1 < capacity)
            data[written] = c;
        ++written;
    }
    int finish(const char* suffix) {
        for (const char* p = suffix; *p; ++p)
            put(*p);
        if (capacity)
            data[written < capacity ? written : capacity - 1] = '\0';
        return static_cast<int>(written);
    }
};
} // namespace

int format_fixed(char* output, std::size_t capacity, double value, unsigned precision, const char* suffix) {
    static_assert(sizeof(double) == sizeof(uint64_t) && std::numeric_limits<double>::is_iec559,
                  "需要 IEEE-754 binary64");
    FixedOutput text{output, capacity};
    if (precision > kFixedMaxPrecision) {
        if (capacity)
            output[0] = '\0';
        return -1;
    }
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    uint64_t limit_bits;
    std::memcpy(&limit_bits, &kFixedLimit, sizeof(limit_bits));
    // IEEE-754 正数位模式单调；同一检查同时挡住超范围值、NaN 和 Inf，不先输出符号。
    if ((bits & ~(uint64_t{1} << 63)) >= limit_bits) {
        text.put('-');
        text.put('-');
        text.put('-');
        return text.finish(suffix);
    }
    if (bits >> 63)
        text.put('-');
    const unsigned exponent = static_cast<unsigned>((bits >> 52) & 0x7ffU);
    uint64_t mantissa = bits & ((uint64_t{1} << 52) - 1);
    if (exponent)
        mantissa |= uint64_t{1} << 52;
    // 低位在前的十进制整数：先乘 10^precision，再按二进制指数精确移位。
    // 初始尾数最多 16 位，加 6 位小数共 22 位；右移只缩短，舍入最多再进一位。
    uint8_t digits[16 + kFixedMaxPrecision + 1]{};
    unsigned count = precision;
    do {
        digits[count++] = static_cast<uint8_t>(mantissa % 10);
        mantissa /= 10;
    } while (mantissa);
    // |value| < 1e15 < 2^52，正规数指数减 52 后必为负，无需左移扩容。
    int shift = exponent ? static_cast<int>(exponent) - 1023 - 52 : -1074;
    bool guard = false, sticky = false;
    while (shift < 0) {
        unsigned carry = 0;
        for (unsigned i = count; i > 0; --i) {
            const unsigned digit = carry * 10U + digits[i - 1];
            digits[i - 1] = static_cast<uint8_t>(digit / 2);
            carry = digit % 2;
        }
        sticky = sticky || guard;
        guard = carry != 0;
        if (count > 1 && digits[count - 1] == 0)
            --count;
        ++shift;
    }
    if (guard && (sticky || (digits[0] & 1U))) {
        unsigned i = 0;
        while (i < count && digits[i] == 9)
            digits[i++] = 0;
        if (i == count)
            ++count;
        ++digits[i];
    }
    while (count <= precision)
        digits[count++] = 0;
    for (unsigned i = count; i > 0; --i) {
        text.put(static_cast<char>('0' + digits[i - 1]));
        if (precision && i == precision + 1)
            text.put('.');
    }
    return text.finish(suffix);
}
} // namespace app
