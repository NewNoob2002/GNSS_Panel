#!/usr/bin/env python3
"""LVGL 8 图片资源 -> LVGL 9.6 ``lv_image_dsc_t`` 转换脚本（仅用 Python 3 标准库）。

用途
----
读取 ``App/Resource/img/*.c`` 正式 v9 资源，独立解码回读并校验头字段、像素和生成文本。
也支持显式 ``--src`` 传入历史 v8 C 数组并转换到独立 ``--dst`` 目录。
设计依据为 ``docs/lvgl9_migration.md`` 第 1、3 节。文件名与符号名（``img_src_*``）保持不变。

用法（在仓库根目录执行）::

    python3 App/Resource/tools/convert_images_v9.py              # 规范化正式 v9 资源并回读校验
    python3 App/Resource/tools/convert_images_v9.py --check      # 只校验，不写任何文件
    python3 App/Resource/tools/convert_images_v9.py --format SystemLoading_Neutral=A8
    python3 App/Resource/tools/convert_images_v9.py --self-test  # 解码器/编码器/预处理器的内置自检

选项
----
``--src DIR`` / ``--dst DIR``  源目录和输出目录默认均为 ``App/Resource/img``；历史 v8 输入须指定独立输出目录。
``--format NAME=FMT``         覆盖单张图片的目标格式，可重复。FMT 为 ``RGB565`` / ``RGB565A8`` / ``A8``。
                              NAME 可写符号名（``img_src_startupLogo``、``startupLogo``）或文件名
                              （``img_src_SystemLoading_XYZ``、``SystemLoading_XYZ``）。
                              ``A8`` 只保留 alpha（单色图配合 recolor 使用），要求可见像素颜色唯一，否则报错。

默认格式策略（可被 ``--format`` 覆盖）
------------------------------------
1. ``ALPHA_8BIT`` -> ``A8``：保留遮罩，由调用处的 image_recolor 提供颜色。
2. ``TRUE_COLOR`` -> ``RGB565``；``TRUE_COLOR_CHROMA_KEYED`` -> ``RGB565A8``。
3. ``TRUE_COLOR_ALPHA`` -> 全部像素不透明则 ``RGB565``，否则 ``RGB565A8``。
4. ``INDEXED_*`` -> 实际像素全部不透明则 ``RGB565``，否则 ``RGB565A8``（未被像素引用的调色板项不影响结果）。
   不保留索引格式，避免 v9 绘制时整图解码到堆。规则不依赖图片名称。

解码约定（与设备上 v8 的实际渲染一致）
----------------------------------
* 取 ``LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0`` 分支；同时解析 ``LV_COLOR_16_SWAP != 0`` 分支，
  要求它逐像素等于前者的字节交换（RGB565 的 2 字节交换，alpha 字节不动），否则报错。
* 索引色调色板为 B,G,R,A 各 8 位；颜色量化为 RGB565 时按 v8 ``LV_COLOR_MAKE16`` 截断（r>>3, g>>2, b>>3）。
* v8 的 RGB565 像素扩展到 8 位用位复制，再量化回 RGB565 时是恒等变换，即 RGB565 来源的图片无损。

* 色键图按本仓库三端 lv_conf.h 的 ``LV_COLOR_CHROMA_KEY = 0x00ff00`` 解码；
  仅 ``TRUE_COLOR_CHROMA_KEYED`` 应用色键，普通 ``TRUE_COLOR`` 不应用。
* 源目录按实际文件发现，不假设图片数量、尺寸或符号等于设计文档中的历史清单。
* v9 文件保持已有行尾；新文件使用 LF。历史 v8 输入与输出目录不得重叠。

v9 输出格式
----------
* ``header.magic = LV_IMAGE_HEADER_MAGIC (0x19)``，``header.cf = LV_COLOR_FORMAT_*``，``header.flags = 0``。
* ``RGB565``：小端 2 字节/像素，``stride = w*2``，``data_size = stride*h``。
* ``RGB565A8``：先 w*h 个小端 RGB565（``stride = w*2``），后 w*h 个 A8，``data_size = w*h*3``。
* ``A8``：``stride = w``，``data_size = w*h``。
* 数组为 ``static const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST ... uint8_t <符号>_map[]``，
  描述符为 ``const lv_image_dsc_t <符号>``，与 v9 ``scripts/LVGLImage.py`` 生成的文件风格一致。

校验（任一失败即以非零状态退出）
------------------------------
对每个输出文件重新解析 C 文本、按 v9 头字段计算数据长度并解码，与原 v8 解码结果逐像素比较：
alpha 必须精确相等，颜色在 RGB565 精度下必须相等（``A8`` 只比较 alpha）；``data_size`` 必须等于格式要求的长度
且等于数组实际长度。``--check`` 额外要求 ``--dst`` 中的文件齐全、无多余文件、且与当前策略的生成结果逐字节一致。

* A8 覆盖只保留透明度；必须在调用处设置单一可见颜色的 image_recolor。

``Vendor/Linux/tools/generate_ui_icons.py --v9`` 复用本脚本的编码器与校验，默认输出 A8；
``--v9 --format RGB565A8`` 可显式生成带白色 RGB565 平面的图标。
"""
from __future__ import annotations

import argparse
import ast
import operator
import random
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

HERE = Path(__file__).resolve().parent
DEFAULT_SRC = HERE.parent / "img"
DEFAULT_DST = HERE.parent / "img"

# v9.6 include/lvgl/draw/lv_image_dsc.h: LV_IMAGE_HEADER_MAGIC；include/lvgl/draw/lv_color.h: 颜色格式枚举值。
LV_IMAGE_HEADER_MAGIC = 0x19
V9_FORMATS = {"RGB565": 0x12, "RGB565A8": 0x14, "A8": 0x0E}
# HC32、Linux 和 Simulator 的 lv_conf.h 均定义纯绿色键；只有 CHROMA_KEYED 格式使用。
V8_CHROMA_KEY_RGB = (0, 255, 0)
# 16 位色深下的 v8 常量：LV_IMG_PX_SIZE_ALPHA_BYTE = 3，LV_COLOR_SIZE = 16（位）。
V8_CONSTANTS = {"LV_IMG_PX_SIZE_ALPHA_BYTE": 3, "LV_COLOR_SIZE": 16}
# 32 位目标上 sizeof(lv_img_dsc_t) = 4（头）+ 4 + 4；sizeof(lv_image_dsc_t) = 12（头）+ 4 + 4 + 4 + 4。
V8_DESC_BYTES = 12
V9_DESC_BYTES = 28


class ConvertError(Exception):
    """Raised when a source file cannot be parsed or converted without loss."""


# --------------------------------------------------------------------------------------------------
# C 文本解析工具
# --------------------------------------------------------------------------------------------------

def strip_comments(text: str) -> str:
    """Remove C comments. 删除 C 块注释与行注释，以空格分隔相邻标记。"""
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def match_brace(text: str, open_index: int) -> int:
    """Find the matching brace. 返回配对右花括号下标，不平衡时抛出 ConvertError。"""
    depth = 0
    for index in range(open_index, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return index
    raise ConvertError("花括号不匹配")


def eval_condition(expr: str, defs: Dict[str, int]) -> bool:
    """Evaluate a preprocessor condition. 只允许已知宏、整数、布尔和比较操作。"""
    expr = re.sub(r"\bdefined\s*\(\s*(\w+)\s*\)", lambda m: "1" if m.group(1) in defs else "0", expr)
    expr = expr.replace("&&", " and ").replace("||", " or ")
    expr = re.sub(r"!(?!=)", " not ", expr)

    def walk(node: ast.AST) -> int:
        """Evaluate only boolean and comparison nodes. 仅接受布尔和比较表达式。"""
        if isinstance(node, ast.Expression):
            return walk(node.body)
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return node.value
        if isinstance(node, ast.Name) and node.id in defs:
            return defs[node.id]
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.Not):
            return int(not walk(node.operand))
        if isinstance(node, ast.BoolOp) and isinstance(node.op, (ast.And, ast.Or)):
            values = [walk(value) for value in node.values]
            return int(all(values) if isinstance(node.op, ast.And) else any(values))
        comparisons = {ast.Eq: operator.eq, ast.NotEq: operator.ne, ast.Lt: operator.lt,
                       ast.LtE: operator.le, ast.Gt: operator.gt, ast.GtE: operator.ge}
        if isinstance(node, ast.Compare):
            values = [walk(node.left)] + [walk(value) for value in node.comparators]
            if all(type(op) in comparisons for op in node.ops):
                return int(all(comparisons[type(op)](values[i], values[i + 1])
                               for i, op in enumerate(node.ops)))
        raise ConvertError(f"不支持的预处理表达式: {expr!r}")

    try:
        return bool(walk(ast.parse(expr.strip(), mode="eval")))
    except SyntaxError as exc:
        raise ConvertError(f"无效预处理表达式: {expr!r}") from exc


def select_branch(body: str, defs: Dict[str, int]) -> str:
    """Select active preprocessor branches. 从数组体选取指定宏配置对应的字节文本。

    支持嵌套 #if/#ifdef/#ifndef/#elif/#else/#endif，不支持的指令或不平衡结构报错。
    """
    out: List[str] = []
    stack: List[List[bool]] = []  # 每项 [父级是否有效, 本组是否已有分支命中, 当前分支是否有效]
    for line in body.split("\n"):
        stripped = line.strip()
        directive = re.match(r"#\s*(\w+)\s*(.*)", stripped)
        if not directive:
            if all(item[2] for item in stack):
                out.append(line)
            continue
        kind, rest = directive.group(1), directive.group(2)
        parent_active = all(item[2] for item in stack)
        if kind in ("if", "ifdef", "ifndef"):
            if kind == "if":
                value = eval_condition(rest, defs)
            else:
                value = (rest.strip() in defs) == (kind == "ifdef")
            stack.append([parent_active, value, value])
        elif kind == "elif":
            if not stack:
                raise ConvertError("#elif 没有对应的 #if")
            top = stack[-1]
            value = (not top[1]) and eval_condition(rest, defs)
            top[1] = top[1] or value
            top[2] = value
        elif kind == "else":
            if not stack:
                raise ConvertError("#else 没有对应的 #if")
            top = stack[-1]
            top[2] = not top[1]
            top[1] = True
        elif kind == "endif":
            if not stack:
                raise ConvertError("#endif 没有对应的 #if")
            stack.pop()
        else:
            raise ConvertError(f"数组内出现不支持的预处理指令 #{kind}")
    if stack:
        raise ConvertError("#if 没有对应的 #endif")
    return "\n".join(out)


def hex_bytes(text: str) -> bytes:
    """Parse byte literals strictly. 只接受逗号分隔的 0..255 整数字面量，禁止静默跳过内容。"""
    tokens = text.strip().split(",")
    if tokens and not tokens[-1].strip():
        tokens.pop()
    values = []
    for token in tokens:
        token = token.strip()
        if not re.fullmatch(r"0[xX][0-9a-fA-F]+|0|[1-9][0-9]*", token):
            raise ConvertError(f"数组含非字节字面量: {token!r}")
        value = int(token, 0)
        if not 0 <= value <= 255:
            raise ConvertError("数组元素超出 0..255")
        values.append(value)
    return bytes(values)


_OPS = {ast.Add: operator.add, ast.Sub: operator.sub, ast.Mult: operator.mul,
        ast.Div: operator.floordiv, ast.FloorDiv: operator.floordiv}


def eval_arith(expr: str, names: Dict[str, int]) -> int:
    """Evaluate integer size arithmetic safely. 计算 data_size；仅支持已知常量和 + - * /。

    除法采用整数除法，不支持的语法和无效运算抛出 ConvertError。
    """
    def walk(node: ast.AST) -> int:
        """Evaluate whitelisted integer nodes. 只计算白名单中的整数节点。"""
        if isinstance(node, ast.Expression):
            return walk(node.body)
        if isinstance(node, ast.Constant) and isinstance(node.value, int):
            return node.value
        if isinstance(node, ast.Name) and node.id in names:
            return names[node.id]
        if isinstance(node, ast.BinOp) and type(node.op) in _OPS:
            return _OPS[type(node.op)](walk(node.left), walk(node.right))
        raise ConvertError(f"不支持的表达式: {expr!r}")

    try:
        return walk(ast.parse(expr.strip(), mode="eval"))
    except (SyntaxError, ZeroDivisionError) as exc:
        raise ConvertError(f"无效整数表达式: {expr!r}") from exc


# --------------------------------------------------------------------------------------------------
# v8 解析与解码
# --------------------------------------------------------------------------------------------------

@dataclass
class Image8:
    """v8 解码结果：RGBA 为 w*h*4 字节，data_bytes 含索引格式的调色板。"""
    path: Path
    symbol: str
    cf: str
    w: int
    h: int
    data_bytes: int
    rgba: bytes
    palette_opaque: bool = True

    @property
    def opaque(self) -> bool:
        """Check full opacity. 判断全部实际像素的 alpha 是否为 255。"""
        return all(a == 255 for a in self.rgba[3::4])

    @property
    def aliases(self) -> List[str]:
        """List accepted aliases. 提供符号名、文件名及去除 img_src_ 前缀的覆盖名称。"""
        stem = self.path.stem
        names = [self.symbol, stem]
        for name in (self.symbol, stem):
            if name.startswith("img_src_"):
                names.append(name[len("img_src_"):])
        return names


def expected_v8_len(cf: str, w: int, h: int) -> int:
    """Compute v8 payload size at 16-bit depth. 返回数据长度，索引图含调色板和逐行填充。"""
    if cf in ("LV_IMG_CF_TRUE_COLOR", "LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED"):
        return w * h * 2
    if cf == "LV_IMG_CF_TRUE_COLOR_ALPHA":
        return w * h * 3
    if cf == "LV_IMG_CF_ALPHA_8BIT":
        return w * h
    match = re.fullmatch(r"LV_IMG_CF_INDEXED_(\d)BIT", cf)
    if match and int(match.group(1)) in (1, 2, 4, 8):
        bits = int(match.group(1))
        return 4 * (1 << bits) + h * ((w * bits + 7) // 8)
    raise ConvertError(f"不支持的 v8 格式 {cf}")


def rgb565_to_rgb888(value: int) -> Tuple[int, int, int]:
    """Expand RGB565 channels by bit replication. 位复制扩展为 8 位 RGB，保证再次量化可逆。"""
    r5, g6, b5 = value >> 11, (value >> 5) & 0x3F, value & 0x1F
    return (r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2)


def quantize565(r: int, g: int, b: int) -> int:
    """Quantize RGB888 to RGB565. 使用与 v8 LV_COLOR_MAKE16 相同的截断方式。"""
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def decode_v8(cf: str, w: int, h: int, data: bytes) -> bytes:
    """Decode unswapped v8 pixels to RGBA. 解码 16 位非交换分支，返回 w*h*4 字节。

    索引调色板按 BGRA 读取，索引高位在前，每行单独向字节边界对齐。
    A8 图按白色处理，由调用处 recolor 提供颜色；格式或长度不符时报错。
    """
    if not 0 < w <= 65535 or not 0 < h <= 65535:
        raise ConvertError(f"无效尺寸 {w}x{h}")
    if len(data) != expected_v8_len(cf, w, h):
        raise ConvertError(f"{cf} {w}x{h} 数据长度 {len(data)} 与期望 {expected_v8_len(cf, w, h)} 不符")
    out = bytearray(w * h * 4)
    if cf in ("LV_IMG_CF_TRUE_COLOR", "LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED", "LV_IMG_CF_TRUE_COLOR_ALPHA"):
        step = 3 if cf == "LV_IMG_CF_TRUE_COLOR_ALPHA" else 2
        for i in range(w * h):
            value = data[i * step] | (data[i * step + 1] << 8)
            out[i * 4:i * 4 + 3] = bytes(rgb565_to_rgb888(value))
            alpha = 255 if step == 2 else data[i * step + 2]
            if cf == "LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED" and value == quantize565(*V8_CHROMA_KEY_RGB):
                alpha = 0
            out[i * 4 + 3] = alpha
    elif cf == "LV_IMG_CF_ALPHA_8BIT":
        for i in range(w * h):
            out[i * 4:i * 4 + 4] = bytes((255, 255, 255, data[i]))  # 无颜色信息：按白色，由 recolor 着色
    else:
        bits = int(cf[len("LV_IMG_CF_INDEXED_")])
        palette_bytes = 4 * (1 << bits)
        row_bytes = (w * bits + 7) // 8
        mask = (1 << bits) - 1
        for y in range(h):
            row = palette_bytes + y * row_bytes
            for x in range(w):
                bit = x * bits
                index = (data[row + bit // 8] >> (8 - bits - bit % 8)) & mask  # 高位在前
                b, g, r, a = data[index * 4:index * 4 + 4]
                out[(y * w + x) * 4:(y * w + x) * 4 + 4] = bytes((r, g, b, a))
    return bytes(out)


def parse_v8_file(path: Path) -> List[Image8]:
    """Parse all v8 descriptors in a C file. 依据描述符关联数组、校验 data_size 后解码。

    同时解析交换与非交换分支，确认仅 RGB565 两字节交换、alpha 不变。
    保留真实文件名与符号名，允许一个 C 文件含多个描述符。
    """
    text = strip_comments(path.read_bytes().decode("utf-8").replace("\r\n", "\n"))
    arrays: Dict[str, str] = {}
    for match in re.finditer(r"uint8_t\s+(\w+)\s*\[\s*\]\s*=\s*\{", text):
        end = match_brace(text, match.end() - 1)
        arrays[match.group(1)] = text[match.end():end]
    images: List[Image8] = []
    for match in re.finditer(r"const\s+lv_img_dsc_t\s+(\w+)\s*=\s*\{", text):
        symbol = match.group(1)
        desc = text[match.end():match_brace(text, match.end() - 1)]
        try:
            cf = re.search(r"\bcf\s*=\s*(LV_IMG_CF_\w+)", desc).group(1)  # type: ignore[union-attr]
            w = int(re.search(r"\bw\s*=\s*(\d+)", desc).group(1))  # type: ignore[union-attr]
            h = int(re.search(r"\bh\s*=\s*(\d+)", desc).group(1))  # type: ignore[union-attr]
            size_expr = re.search(r"\.data_size\s*=\s*([^,]+),", desc).group(1)  # type: ignore[union-attr]
            array_name = re.search(r"\.data\s*=\s*(\w+)", desc).group(1)  # type: ignore[union-attr]
        except AttributeError:
            raise ConvertError(f"{path.name}: 描述符 {symbol} 缺少 cf/w/h/data_size/data 字段") from None
        if array_name not in arrays:
            raise ConvertError(f"{path.name}: 找不到数组 {array_name}")
        plain = hex_bytes(select_branch(arrays[array_name], {"LV_COLOR_DEPTH": 16, "LV_COLOR_16_SWAP": 0}))
        swapped = hex_bytes(select_branch(arrays[array_name], {"LV_COLOR_DEPTH": 16, "LV_COLOR_16_SWAP": 1}))
        check_swap_branch(path.name, cf, plain, swapped)
        size_expr = re.sub(r"sizeof\s*\(\s*(\w+)\s*\)",
                           lambda m: str(len(hex_bytes(select_branch(arrays[m.group(1)], {"LV_COLOR_DEPTH": 16,
                                                                                         "LV_COLOR_16_SWAP": 0})))),
                           size_expr)
        data_size = eval_arith(size_expr, V8_CONSTANTS)
        if data_size != len(plain):
            raise ConvertError(f"{path.name}: {symbol} data_size={data_size} 与数组长度 {len(plain)} 不符")
        palette_opaque = True
        if cf.startswith("LV_IMG_CF_INDEXED_"):
            palette_bytes = 4 * (1 << int(cf[len("LV_IMG_CF_INDEXED_")]))
            palette_opaque = all(alpha == 255 for alpha in plain[3:palette_bytes:4])
        images.append(Image8(path, symbol, cf, w, h, data_size, decode_v8(cf, w, h, plain), palette_opaque))
    if not images:
        raise ConvertError(f"{path.name}: 未找到 lv_img_dsc_t 描述符")
    return images


def check_swap_branch(name: str, cf: str, plain: bytes, swapped: bytes) -> None:
    """Verify byte-swapped branches. 校验两个分支仅交换 RGB565 字节，非 RGB565 数据必须相同。"""
    if len(plain) != len(swapped):
        raise ConvertError(f"{name}: 两个 LV_COLOR_16_SWAP 分支长度不同 ({len(plain)} vs {len(swapped)})")
    step = {"LV_IMG_CF_TRUE_COLOR": 2, "LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED": 2,
            "LV_IMG_CF_TRUE_COLOR_ALPHA": 3}.get(cf)
    if step is None:
        # 非逐像素 RGB565 的格式（索引色、A8）没有交换分支，两种配置必须得到同一份数据
        if plain != swapped:
            raise ConvertError(f"{name}: {cf} 的两个 LV_COLOR_16_SWAP 分支内容不同")
        return
    if len(plain) % step:
        raise ConvertError(f"{name}: RGB565 数据不是完整像素")
    expected = bytearray(plain)
    for i in range(0, len(plain), step):
        expected[i], expected[i + 1] = plain[i + 1], plain[i]
    if bytes(expected) != swapped:
        raise ConvertError(f"{name}: LV_COLOR_16_SWAP 分支不是非交换分支的字节交换")


# --------------------------------------------------------------------------------------------------
# v9 编码、生成与回读
# --------------------------------------------------------------------------------------------------

@dataclass
class Image9:
    """v9 编码结果，stride 以字节计，不含行填充。"""
    symbol: str
    w: int
    h: int
    fmt: str
    stride: int
    data: bytes


def v9_expected_size(fmt: str, w: int, h: int, stride: int) -> int:
    """Compute v9 data_size from header fields. RGB565A8 为 w*h*3，其余为 stride*h。"""
    if fmt == "RGB565A8":
        return w * h * 3
    if fmt in ("RGB565", "A8"):
        return stride * h
    raise ConvertError(f"不支持的 v9 格式 {fmt}")


def v9_expected_stride(fmt: str, w: int) -> int:
    """Compute tightly packed stride. A8 为 w，其余支持的格式为 w*2 字节。"""
    if fmt in ("RGB565", "RGB565A8"):
        return w * 2
    if fmt == "A8":
        return w
    raise ConvertError(f"不支持的 v9 格式 {fmt}")


def encode_v9(symbol: str, w: int, h: int, rgba: bytes, fmt: str) -> Image9:
    """Encode RGBA to a v9 payload. 编码为小端 RGB565、平面 RGB565A8 或单色 A8。

    拒绝 RGB565 丢失 alpha、多色图转 A8，以及超出头字段范围的尺寸和 stride。
    """
    if (not 0 < w <= 65535 or not 0 < h <= 65535 or
            len(rgba) != w * h * 4 or v9_expected_stride(fmt, w) > 65535):
        raise ConvertError(f"{symbol}: 尺寸、stride 或 RGBA 数据长度无效")
    pixels = [(rgba[i], rgba[i + 1], rgba[i + 2], rgba[i + 3]) for i in range(0, w * h * 4, 4)]
    colors = bytearray()
    for r, g, b, _ in pixels:
        colors += quantize565(r, g, b).to_bytes(2, "little")
    alpha = bytes(p[3] for p in pixels)
    if fmt == "RGB565":
        if any(a != 255 for a in alpha):
            raise ConvertError(f"{symbol}: 含透明像素，不能存为 RGB565")
        data = bytes(colors)
    elif fmt == "RGB565A8":
        data = bytes(colors) + alpha
    elif fmt == "A8":
        visible = {quantize565(r, g, b) for r, g, b, a in pixels if a}
        if len(visible) > 1:
            raise ConvertError(f"{symbol}: 可见像素有 {len(visible)} 种颜色，存为 A8 会丢失颜色")
        data = alpha
    else:
        raise ConvertError(f"不支持的 v9 格式 {fmt}")
    return Image9(symbol, w, h, fmt, v9_expected_stride(fmt, w), data)


C_HEADER = """/* LVGL 9 图片资源，由 App/Resource/tools/convert_images_v9.py 的编码器生成，请勿手改。 */
#ifdef __has_include
    #if __has_include("lvgl.h")
        #if !defined(LV_LVGL_H_INCLUDE_SIMPLE) && !defined(LV_LVGL_H_INCLUDE_SYSTEM) && !defined(LV_BUILD_TEST)
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#elif defined(LV_LVGL_H_INCLUDE_SYSTEM)
#include <lvgl.h>
#elif defined(LV_BUILD_TEST)
#include "../lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif
"""


def format_bytes(data: bytes, per_line: int) -> str:
    """Format byte literals for C. 按指定行宽生成缩进的十六进制数组文本。"""
    return "\n".join("    " + ", ".join(f"0x{value:02x}" for value in data[i:i + per_line]) + ","
                     for i in range(0, len(data), per_line))


def render_c_file(images: Sequence[Image9]) -> str:
    """Render v9 descriptors and arrays. 按上游风格生成同一文件内全部图片，使用 LF 行尾。"""
    parts = [C_HEADER]
    for img in images:
        macro = "LV_ATTRIBUTE_" + img.symbol.upper()
        # 每行字节数：行宽不超过 32 时按图像行对齐，便于肉眼核对；更宽时固定 32
        plane_row = img.w if img.fmt == "A8" else img.w * 2
        per_line = plane_row if plane_row <= 32 else 32
        if img.fmt == "RGB565A8":
            color_len = img.w * img.h * 2
            body = format_bytes(img.data[:color_len], per_line) + "\n" + format_bytes(img.data[color_len:],
                                                                                      img.w if img.w <= 32 else 32)
        else:
            body = format_bytes(img.data, per_line)
        parts.append(f"""
#ifndef {macro}
#define {macro}
#endif

static const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST {macro} uint8_t {img.symbol}_map[] = {{
{body}
}};

const lv_image_dsc_t {img.symbol} = {{
  .header = {{
    .magic = LV_IMAGE_HEADER_MAGIC,
    .cf = LV_COLOR_FORMAT_{img.fmt},
    .flags = 0,
    .w = {img.w},
    .h = {img.h},
    .stride = {img.stride},
    .reserved_2 = 0,
  }},
  .data_size = sizeof({img.symbol}_map),
  .data = {img.symbol}_map,
  .reserved = NULL,
  .reserved_2 = NULL,
}};
""")
    return "".join(parts)


@dataclass
class Parsed9:
    """从生成的 C 文本重新读取的 v9 头字段与像素数据。"""
    symbol: str
    magic: str
    cf: str
    flags: int
    w: int
    h: int
    stride: int
    reserved_2: int
    data_size: int
    data: bytes
    array_declared_large_const: bool


def parse_v9_text(text: str) -> List[Parsed9]:
    """Parse v9 C text independently. 回读所有描述符与数组，缺少必需字段或数组时报错。"""
    text = strip_comments(text.replace("\r\n", "\n"))
    arrays: Dict[str, Tuple[bytes, bool]] = {}
    # 行首锚定避免在大型数组内部反复扫描整段初始化器（旧正则为平方复杂度）。
    declaration = r"^[ \t]*((?:(?:static|const|LV_ATTRIBUTE_\w+)\s+)*)uint8_t\s+(\w+)\s*\[\s*\]\s*=\s*\{"
    for match in re.finditer(declaration, text, flags=re.M):
        end = match_brace(text, match.end() - 1)
        declared = "LV_ATTRIBUTE_LARGE_CONST" in match.group(1) and "const" in match.group(1)
        arrays[match.group(2)] = (hex_bytes(text[match.end():end]), declared)
    result: List[Parsed9] = []
    for match in re.finditer(r"const\s+lv_image_dsc_t\s+(\w+)\s*=\s*\{", text):
        symbol = match.group(1)
        desc = text[match.end():match_brace(text, match.end() - 1)]

        def field(name: str) -> str:
            """Read one required descriptor field. 读取必需的描述符字段。"""
            found = re.search(rf"\.{name}\s*=\s*([^,}}]+?)\s*[,}}\n]", desc + "\n")
            if not found:
                raise ConvertError(f"{symbol}: 缺少字段 .{name}")
            return found.group(1).strip()

        array_name = field("data")
        if array_name not in arrays:
            raise ConvertError(f"{symbol}: 找不到数组 {array_name}")
        data, declared = arrays[array_name]
        size_text = field("data_size")
        sizeof = re.fullmatch(r"sizeof\s*\(\s*(\w+)\s*\)", size_text)
        if sizeof:
            if sizeof.group(1) not in arrays:
                raise ConvertError(f"{symbol}: sizeof 引用了未知数组 {sizeof.group(1)}")
            data_size = len(arrays[sizeof.group(1)][0])
        else:
            data_size = int(size_text, 0)
        cf = field("cf")
        if not cf.startswith("LV_COLOR_FORMAT_"):
            raise ConvertError(f"{symbol}: 非 v9 颜色格式 {cf}")
        result.append(Parsed9(symbol, field("magic"), cf[len("LV_COLOR_FORMAT_"):], int(field("flags"), 0),
                              int(field("w")), int(field("h")), int(field("stride")), int(field("reserved_2")),
                              data_size, data, declared))
    if not result:
        raise ConvertError("未找到 lv_image_dsc_t 描述符")
    return result


def verify_pair(src: Image8, out: Parsed9) -> List[str]:
    """Verify headers and every decoded pixel. 比较回读结果与原图，返回错误列表。

    颜色按 RGB565 精度比较，alpha 必须精确；A8 只比较 alpha，并要求可见像素单色。
    """
    errors: List[str] = []
    name = out.symbol
    if out.symbol != src.symbol:
        errors.append(f"{name}: 符号名应为 {src.symbol}")
    if out.magic != "LV_IMAGE_HEADER_MAGIC":
        errors.append(f"{name}: header.magic 应为 LV_IMAGE_HEADER_MAGIC，实际 {out.magic}")
    if out.cf not in V9_FORMATS:
        return errors + [f"{name}: 不支持的格式 {out.cf}"]
    if out.flags != 0 or out.reserved_2 != 0:
        errors.append(f"{name}: flags/reserved_2 应为 0")
    if (out.w, out.h) != (src.w, src.h):
        errors.append(f"{name}: 尺寸 {out.w}x{out.h} 与原图 {src.w}x{src.h} 不符")
        return errors
    if not out.array_declared_large_const:
        errors.append(f"{name}: 数组缺少 const LV_ATTRIBUTE_LARGE_CONST")
    if out.stride != v9_expected_stride(out.cf, out.w):
        errors.append(f"{name}: stride={out.stride}，{out.cf} 应为 {v9_expected_stride(out.cf, out.w)}")
    expected = v9_expected_size(out.cf, out.w, out.h, out.stride)
    if out.data_size != expected or len(out.data) != expected:
        errors.append(f"{name}: data_size={out.data_size}/数组 {len(out.data)} 字节，{out.cf} 要求 {expected}")
        return errors
    pixels = out.w * out.h
    bad_alpha = bad_color = 0
    for i in range(pixels):
        r, g, b, a = src.rgba[i * 4:i * 4 + 4]
        if out.cf == "RGB565":
            new_a, new_c = 255, out.data[i * 2] | (out.data[i * 2 + 1] << 8)
        elif out.cf == "RGB565A8":
            new_a, new_c = out.data[pixels * 2 + i], out.data[i * 2] | (out.data[i * 2 + 1] << 8)
        else:
            new_a, new_c = out.data[i], None
        bad_alpha += new_a != a
        bad_color += new_c is not None and new_c != quantize565(r, g, b)
    if bad_alpha:
        errors.append(f"{name}: {bad_alpha}/{pixels} 个像素 alpha 与原图不一致")
    if bad_color:
        errors.append(f"{name}: {bad_color}/{pixels} 个像素颜色（RGB565）与原图不一致")
    if out.cf == "A8":
        visible = {quantize565(*src.rgba[i * 4:i * 4 + 3]) for i in range(pixels) if src.rgba[i * 4 + 3]}
        if len(visible) > 1:
            errors.append(f"{name}: A8 丢失了 {len(visible)} 种可见颜色")
    return errors


def verify_text(sources: Sequence[Image8], text: str) -> List[str]:
    """Verify a generated file from scratch. 独立解析文本，校验描述符清单并逐张比较。"""
    try:
        parsed = parse_v9_text(text)
    except ConvertError as exc:
        return [f"回读解析失败: {exc}"]
    if [p.symbol for p in parsed] != [s.symbol for s in sources]:
        return [f"描述符 {[p.symbol for p in parsed]} 与源 {[s.symbol for s in sources]} 不一致"]
    errors: List[str] = []
    for src, out in zip(sources, parsed):
        errors += verify_pair(src, out)
    return errors


# --------------------------------------------------------------------------------------------------
# 格式策略与主流程
# --------------------------------------------------------------------------------------------------

def parse_overrides(items: Sequence[str]) -> Dict[str, str]:
    """Parse repeated NAME=FMT options. 校验格式覆盖参数，返回名称到格式的映射。"""
    result: Dict[str, str] = {}
    for item in items:
        name, sep, fmt = item.partition("=")
        if not sep or not name or fmt not in V9_FORMATS:
            raise ConvertError(f"--format 需要 NAME=FMT，FMT 为 {'/'.join(V9_FORMATS)}: {item!r}")
        result[name] = fmt
    return result


def choose_format(img: Image8, overrides: Dict[str, str]) -> str:
    """Choose by source format and opacity. 按源格式推导目标格式，别名覆盖优先且不得冲突。"""
    requested = {overrides[alias] for alias in img.aliases if alias in overrides}
    if len(requested) > 1:
        raise ConvertError(f"{img.symbol}: 文件名与符号名的格式覆盖互相冲突")
    if requested:
        return requested.pop()
    if img.cf == "LV_IMG_CF_ALPHA_8BIT":
        return "A8"
    if img.cf == "LV_IMG_CF_TRUE_COLOR":
        return "RGB565"
    if img.cf == "LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED":
        return "RGB565A8"
    if img.cf == "LV_IMG_CF_TRUE_COLOR_ALPHA" or img.cf.startswith("LV_IMG_CF_INDEXED_"):
        # 只看实际像素：未被引用的非不透明调色板项不会出现在画面上，不能因此多付一个 A8 平面。
        return "RGB565" if img.opaque else "RGB565A8"
    raise ConvertError(f"{img.symbol}: 不支持的源格式 {img.cf}")


def parse_source_file(path: Path) -> List[Image8]:
    """Parse v8 or deployed v9 resources. 兼容历史输入，并对正式 v9 资源独立回读为 RGBA。"""
    text = path.read_bytes().decode("utf-8")
    if "const lv_image_dsc_t" not in text:
        return parse_v8_file(path)
    result = []
    formats = {"A8": "LV_IMG_CF_ALPHA_8BIT", "RGB565": "LV_IMG_CF_TRUE_COLOR",
               "RGB565A8": "LV_IMG_CF_TRUE_COLOR_ALPHA"}
    for image in parse_v9_text(text):
        if image.cf not in formats:
            raise ConvertError(f"{path.name}: 不支持的 v9 格式 {image.cf}")
        size = v9_expected_size(image.cf, image.w, image.h, image.stride)
        if len(image.data) != size or image.data_size != size:
            raise ConvertError(f"{image.symbol}: v9 数据长度与头字段不符")
        rgba = bytearray()
        for index in range(image.w * image.h):
            color = (255, 255, 255) if image.cf == "A8" else rgb565_to_rgb888(
                image.data[index * 2] | image.data[index * 2 + 1] << 8)
            alpha = image.data[index] if image.cf == "A8" else (
                image.data[image.w * image.h * 2 + index] if image.cf == "RGB565A8" else 255)
            rgba.extend((*color, alpha))
        source = Image8(path, image.symbol, formats[image.cf], image.w, image.h, size, bytes(rgba))
        errors = verify_pair(source, image)
        if errors:
            raise ConvertError("; ".join(errors))
        result.append(source)
    return result


def run(src_dir: Path, dst_dir: Path, overrides: Dict[str, str], check_only: bool) -> int:
    """Convert or check all images. 转换并磁盘回读，或只读校验现有输出；失败返回非零。

    输入错误在写入前统一报告。报告包含像素数据大小、C 文件大小及 32 位描述符增量估算。
    """
    errors: List[str] = []
    src_dir, dst_dir = src_dir.resolve(), dst_dir.resolve()
    if src_dir != dst_dir and (src_dir in dst_dir.parents or dst_dir in src_dir.parents):
        raise ConvertError("源目录与输出目录不得互相包含")
    if src_dir == dst_dir and any("const lv_image_dsc_t" not in path.read_text()
                                  for path in src_dir.glob("*.c")):
        raise ConvertError("v8 输入必须使用独立输出目录，禁止覆盖历史原图")
    paths = sorted(src_dir.glob("*.c"))
    if not paths:
        raise ConvertError(f"{src_dir}: 没有图片 C 文件")
    source_is_v9 = all("const lv_image_dsc_t" in path.read_text() for path in paths)
    files: List[Tuple[Path, List[Image8]]] = []
    for path in paths:
        try:
            files.append((path, parse_source_file(path)))
        except ConvertError as exc:
            errors.append(f"{path.name}: 解析失败: {exc}")
    symbols = [img.symbol for _, imgs in files for img in imgs]
    if len(symbols) != len(set(symbols)):
        errors.append("源目录中存在重复图片符号")
    known = {alias for _, imgs in files for img in imgs for alias in img.aliases}
    for key in overrides:
        if key not in known:
            errors.append(f"--format 指定的名称 {key!r} 不对应任何图片")

    # 先完成所有输入和编码校验，再写入暂存目录，避免输入错误造成半套输出。
    prepared = []
    for path, imgs in files:
        encoded: List[Image9] = []
        for img in imgs:
            try:
                fmt = choose_format(img, overrides)
                encoded.append(encode_v9(img.symbol, img.w, img.h, img.rgba, fmt))
            except ConvertError as exc:
                errors.append(f"{path.name}: {exc}")
        if len(encoded) != len(imgs):
            continue
        text = render_c_file(encoded)
        # 专用 Logo 工具的布局常量不参与像素编码，规范化时原样保留。
        marker = "/* Logo 布局：底色、前景色及原画面中的裁剪偏移。 */"
        original = path.read_bytes().decode("utf-8").replace("\r\n", "\n")
        if marker in original:
            text += "\n" + original[original.index(marker):]
        errors += [f"{path.name}: {message}" for message in verify_text(imgs, text)]
        prepared.append((path, imgs, encoded, text))
    if errors:
        for message in errors:
            print(f"ERROR: {message}", file=sys.stderr)
        return 1

    rows = []
    for path, imgs, encoded, text in prepared:
        target = dst_dir / path.name
        if check_only:
            if not target.exists():
                errors.append(f"{target}: 不存在，请先运行转换")
                continue
            disk = target.read_bytes().decode("utf-8")
            stale = disk.replace("\r\n", "\n") != text
            if stale:
                errors.append(f"{target}: 与当前策略的生成结果不一致（请传入生成时相同的 --format）")
        else:
            dst_dir.mkdir(parents=True, exist_ok=True)
            # 已有输出保持行尾；首次生成使用 LF。
            newline = "\r\n" if target.exists() and b"\r\n" in target.read_bytes() else "\n"
            target.write_bytes(text.replace("\n", newline).encode("utf-8"))
            disk = target.read_bytes().decode("utf-8")
            stale = False
        verified = verify_text(imgs, disk)
        errors += [f"{path.name}: {message}" for message in verified]
        for img, new in zip(imgs, encoded):
            rows.append((img, new, not verified and not stale, target.stat().st_size))
    if dst_dir.is_dir():
        expected_names = {path.name for path, _ in files}
        for extra in sorted(dst_dir.glob("*.c")):
            if extra.name not in expected_names:
                errors.append(f"{extra}: 源目录中没有对应文件")

    print(f"{'symbol':34}{'size':>9}  {'source format':>26} -> {'v9':9}{'source B':>8}{'v9 B':>8}{'C file B':>10}  readback")
    total_old = total_new = 0
    for img, new, ok, c_size in rows:
        total_old += img.data_bytes
        total_new += len(new.data)
        source_format = new.fmt if source_is_v9 else img.cf[len('LV_IMG_CF_'):]
        print(f"{img.symbol:34}{f'{img.w}x{img.h}':>9}  {source_format:>26} -> {new.fmt:9}"
              f"{img.data_bytes:8}{len(new.data):8}{c_size:10}  {'OK' if ok else 'FAIL'}")
    if source_is_v9:
        print(f"\n{len(files)} 个 C 文件，{len(rows)} 张 v9 图片；像素数据 {total_new} B，描述符 {V9_DESC_BYTES} B/张"
              "（32 位目标估算，未计链接裁剪及对齐）")
    else:
        desc_delta = (V9_DESC_BYTES - V8_DESC_BYTES) * len(rows)
        print(f"\n{len(files)} 个 C 文件，{len(rows)} 张图片；像素数据 v8 {total_old} B -> v9 {total_new} B（{total_new - total_old:+d} B），"
              f"描述符 {V8_DESC_BYTES} -> {V9_DESC_BYTES} B/张（{desc_delta:+d} B），合计 {total_new - total_old + desc_delta:+d} B"
              "（全量参考，未计链接裁剪及对齐）")
    for message in errors:
        print(f"ERROR: {message}", file=sys.stderr)
    mode = "校验" if check_only else "转换并回读校验"
    print(f"{mode}{'失败' if errors else '通过'}：{len(rows)} 张图片，{len(errors)} 个错误")
    return 1 if errors else 0


# --------------------------------------------------------------------------------------------------
# 自检
# --------------------------------------------------------------------------------------------------

def self_test() -> int:
    """Run deterministic built-in checks. 自检预处理、索引解包、编码及回读，失败返回非零。"""
    rng = random.Random(1)
    failures: List[str] = []

    def expect(condition: bool, message: str) -> None:
        """Collect a self-test failure. 汇总自检失败项。"""
        if not condition:
            failures.append(message)

    # 预处理：#if / #elif / #else 嵌套选择
    body = ("#if LV_COLOR_DEPTH == 1 || LV_COLOR_DEPTH == 8\n0x01,\n#endif\n"
            "#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0\n0x02,\n#endif\n"
            "#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP != 0\n0x03,\n#endif\n"
            "#if LV_COLOR_DEPTH == 32\n0x04,\n#endif\n")
    expect(hex_bytes(select_branch(body, {"LV_COLOR_DEPTH": 16, "LV_COLOR_16_SWAP": 0})) == b"\x02", "预处理 swap=0")
    expect(hex_bytes(select_branch(body, {"LV_COLOR_DEPTH": 16, "LV_COLOR_16_SWAP": 1})) == b"\x03", "预处理 swap=1")
    expect(hex_bytes(select_branch("#if 0\n1,\n#elif 1\n2,\n#else\n3,\n#endif\n", {})) == b"\x02", "预处理 #elif")
    expect(eval_arith("256 * 4 + 37044", {}) == 38068, "算术表达式")
    expect(eval_arith("336 * LV_COLOR_SIZE / 8", V8_CONSTANTS) == 672, "算术表达式（LV_COLOR_SIZE）")

    # 索引色解包：1/2/4/8 位，宽度故意取非字节对齐
    for bits in (1, 2, 4, 8):
        w, h = 7, 3
        palette = bytes(rng.randrange(256) for _ in range(4 * (1 << bits)))
        indexes = [[rng.randrange(1 << bits) for _ in range(w)] for _ in range(h)]
        rows = bytearray()
        for row in indexes:
            packed = bytearray((w * bits + 7) // 8)
            for x, index in enumerate(row):
                bit = x * bits
                packed[bit // 8] |= index << (8 - bits - bit % 8)
            rows += packed
        rgba = decode_v8(f"LV_IMG_CF_INDEXED_{bits}BIT", w, h, palette + bytes(rows))
        ok = all(rgba[(y * w + x) * 4:(y * w + x) * 4 + 4] ==
                 bytes((palette[indexes[y][x] * 4 + 2], palette[indexes[y][x] * 4 + 1], palette[indexes[y][x] * 4],
                        palette[indexes[y][x] * 4 + 3])) for y in range(h) for x in range(w))
        expect(ok, f"索引色 {bits} 位解包")

    # 编码 -> 生成 -> 回读 -> 校验：随机 RGB565 来源图，三种格式
    w, h = 13, 5
    rgba = bytearray()
    for _ in range(w * h):
        r, g, b = rgb565_to_rgb888(rng.randrange(65536))
        rgba += bytes((r, g, b, rng.choice((0, 77, 255))))
    src = Image8(Path("img_src_t.c"), "img_src_t", "LV_IMG_CF_TRUE_COLOR_ALPHA", w, h, w * h * 3, bytes(rgba))
    text = render_c_file([encode_v9(src.symbol, w, h, src.rgba, "RGB565A8")])
    expect(not verify_text([src], text), "RGB565A8 回读校验应通过")
    expect("sizeof(img_src_t_map)" in text and "LV_IMAGE_HEADER_MAGIC" in text, "生成文本含 sizeof 与 magic")
    corrupt_alpha = text.replace("0x4d,", "0x4e,", 1)
    expect(bool(verify_text([src], corrupt_alpha)) and corrupt_alpha != text, "alpha 被改动应被检出")
    expect(bool(verify_text([src], text.replace(".stride = 26", ".stride = 27"))), "stride 错误应被检出")
    expect(bool(verify_text([src], text.replace("LV_COLOR_FORMAT_RGB565A8", "LV_COLOR_FORMAT_RGB565"))), "格式错误应被检出")
    try:
        encode_v9(src.symbol, w, h, src.rgba, "RGB565")
        failures.append("含透明像素时编码 RGB565 应报错")
    except ConvertError:
        pass
    solid = Image8(Path("img_src_s.c"), "img_src_s", "LV_IMG_CF_TRUE_COLOR", 4, 4, 32, bytes((255, 0, 0, 255)) * 16)
    expect(not verify_text([solid], render_c_file([encode_v9("img_src_s", 4, 4, solid.rgba, "RGB565")])), "RGB565 回读")
    expect(not verify_text([solid], render_c_file([encode_v9("img_src_s", 4, 4, solid.rgba, "A8")])), "单色 A8 回读")

    # 颜色交换分支校验
    try:
        check_swap_branch("t", "LV_IMG_CF_TRUE_COLOR_ALPHA", b"\x01\x02\x03", b"\x02\x01\x03")
        check_swap_branch("t", "LV_IMG_CF_TRUE_COLOR", b"\x01\x02", b"\x02\x01")
    except ConvertError:
        failures.append("合法的交换分支被拒绝")
    try:
        check_swap_branch("t", "LV_IMG_CF_TRUE_COLOR_ALPHA", b"\x01\x02\x03", b"\x01\x02\x03")
        failures.append("未交换的交换分支应报错")
    except ConvertError:
        pass

    for message in failures:
        print(f"SELF-TEST FAIL: {message}", file=sys.stderr)
    print("自检通过" if not failures else f"自检失败：{len(failures)} 项")
    return 1 if failures else 0


def main(argv: Optional[Sequence[str]] = None) -> int:
    """Run the CLI. 解析参数并返回进程退出码；用法见模块文档字符串。"""
    parser = argparse.ArgumentParser(description="LVGL 8 图片 C 数组 -> LVGL 9.6 lv_image_dsc_t（详见脚本文档字符串）")
    parser.add_argument("--src", type=Path, default=DEFAULT_SRC,
                        help="源目录（默认 App/Resource/img，支持 v8/v9）")
    parser.add_argument("--dst", type=Path, default=DEFAULT_DST, help="v9 输出目录（默认 App/Resource/img）")
    parser.add_argument("--check", action="store_true", help="只校验 --dst 中已有文件，不写任何文件")
    parser.add_argument("--format", action="append", default=[], metavar="NAME=FMT",
                        help="覆盖某张图片的目标格式（RGB565/RGB565A8/A8），可重复")
    parser.add_argument("--self-test", action="store_true", help="运行内置自检后退出")
    args = parser.parse_args(argv)
    if args.self_test:
        return self_test()
    try:
        overrides = parse_overrides(args.format)
    except ConvertError as exc:
        parser.error(str(exc))
    try:
        return run(args.src, args.dst, overrides, args.check)
    except (ConvertError, OSError, UnicodeError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
