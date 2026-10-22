/** @file FixedFormat.h 无堆分配的二进制浮点到十进制定点文本转换。 */
#ifndef APP_FIXED_FORMAT_H
#define APP_FIXED_FORMAT_H

#include <cstddef>

namespace app {
/** @brief 定点显示的排他上限、小数位上限和完整数值文本容量（含结尾零）。 */
inline constexpr double kFixedLimit = 1.0e15;
inline constexpr unsigned kFixedMaxPrecision = 6;
inline constexpr std::size_t kFixedTextCapacity = 1 + 15 + 1 + kFixedMaxPrecision + 1;
static_assert(kFixedLimit == 1000000000000000.0 && kFixedMaxPrecision == 6
                  && kFixedTextCapacity == 24 && kFixedTextCapacity >= 1 + 16 + 1,
              "范围变更时须重新核实整数位、舍入进位和缓冲容量");
/**
 * @brief 将 IEEE-754 double 按 C locale 的 %.*f 输出，返回未截断的字符数。
 * @param output 调用者缓冲；capacity 为零时允许为空，否则必须有效。
 * @param capacity 缓冲容量（含结尾零）；不足时与 snprintf 一样截断并终止。
 * @param value 支持 |value| < 1e15；范围内保留负号（含 -0.0），范围外及 NaN/Inf 统一输出 ---。
 * @param precision 小数位数 0..6；超出范围返回 -1，缓冲置空。
 * @param suffix 追加的固定后缀（默认空串），与数值一起按容量截断。
 * @details 使用整数运算精确缩放，最近舍入、半值取偶，与 newlib 默认舍入一致；
 *          不依赖浮点乘法的中间舍入或当前 fenv。完整数值含零最多 24 字节，后缀另计。
 *          该显示范围远大于实际经纬度、海拔、电池与存储容量；协议可传入更大值，按占位处理。
 *          精度零可能舍入成 16 位整数，但无小数点，仍能放入同一缓冲。
 *          实现由 app_core 统一编译，调用方不保留转换实现。
 */
int format_fixed(char* output, std::size_t capacity, double value, unsigned precision, const char* suffix = "");

/** @brief 保存完整定点文本的栈对象；c_str() 在对象存活期间有效，无堆分配。 */
class Fixed {
  public:
    /** @brief |value| < 1e15、小数位 0..6；超范围/非有限值为 ---，非法精度为空串。 */
    Fixed(double value, unsigned precision) { format_fixed(text_, sizeof(text_), value, precision); }
    /** @brief 返回以零结尾的文本，只读借用，不得越过对象生命期保存。 */
    const char* c_str() const { return text_; }

  private:
    char text_[kFixedTextCapacity];
};
} // namespace app
#endif
