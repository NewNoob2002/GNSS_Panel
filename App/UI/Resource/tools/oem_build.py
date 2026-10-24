#!/usr/bin/env python3
"""从每客户 oem.toml 生成固件直接编译的 oem_logo.c / oem_config.h（Python 3.11+）。

图片（PNG 或迁移保留的 A8/RGB565 C）仅标准库：
  python3 -B App/UI/Resource/tools/oem_build.py App/UI/Resource/oem/singularxyz
文字仅在此模式导入 Pillow，使用仓库 TTF（Oswald 为 SIL OFL）：
  uv run --with pillow python3 -B App/UI/Resource/tools/oem_build.py App/UI/Resource/oem/demo
--output DIR 将全部生成物写到指定目录；固件 CMake 不运行生成器。

完整字段（路径相对 oem.toml；未知字段报错）：
name = "Demo"                        # 配置显示名，非运行期客户分支
[colors]                             # RRGGBB，可带 #
background = "101820"               # Logo 页底色
logo = "f2aa4c"                     # A8 着色；RGB565 忽略
text = "e8e8e8"                     # Startup 标题和初始化页脚
# footer_text = "a4adb5"            # 可选独立页脚颜色；省略时等于 text（兼容旧版配色）
[logo]                               # image / text 严格二选一
image = "logo.png"                  # PNG 自动检测源底色/前景，再按 colors 着色
# groups = "0,1|2"                  # 图片：分量编号，所有分量恰好出现一次
# timings = "0:0:250|1:50:300"      # cascade 组延迟/时长，相对 background_ms
# max_error = 20                    # PNG RGB565 回读最大通道误差，0..255
# fit = false                       # PNG 超屏时显式等比缩小
# text = "NOVA RTK"                 # 替代 image：每个非空白字符为一个 A8 零件
# font = "../../font/Oswald-Bold.ttf" # 文字必须指定字体
# size = 40                         # 字号，1..126；超过 294x126 报错
# groups = "NOVA|RTK"               # 文字：顺序子串（忽略空格），或字符零件编号
[device_name]                        # en 必填，其余缺省回退 en
# en = "NOVA RTK"
# ru = "NOVA RTK"
# footer_en = "NOVA GNSS Receiver"   # 必填
# footer_ru = "Приёмник NOVA GNSS"
[effect]
preset = "sweep"                    # cascade | soft | sweep | wipe
# total_ms = 1300                   # 整个 Logo 停留时长，1..60000
# background_ms = 150               # 底色淡入时长
# interval_ms = 50                  # cascade 逐组间隔
# duration_ms = 180                 # cascade 250 / soft 600 / sweep 180 / wipe 700
# rise_px = 0                       # cascade/soft 默认 6，其余固定 0
# highlight = true                  # cascade/sweep 默认开，soft/wipe 默认关；soft 必须关
# highlight_start_ms = 900          # 扫光起点，须在入场结束后
# highlight_duration_ms = 300       # 扫光时长

cascade 保留旧组时间覆盖；soft 同步 ease-out 上浮淡入；sweep 同步淡入后扫光；
wipe 线性展开 16 px 柔边。所有配置的入场/扫光必须在 total_ms 内结束。
设备名称逐字校验实际 Startup Oswald 18 与页脚 Oswald 12 的 C cmap（含稀疏映射），
按字体 advance、kerning 与 LVGL 逐字舍入计算宽度。上限来自 DeviceNameLayout.h，
生成器和 UI 共读同一常量；超宽报语言、原文、实际像素宽度与上限，不允许静默截断。
"""
from __future__ import annotations

import argparse
import importlib
import json
import re
import tomllib
from pathlib import Path

import convert_images_v9 as codec
import convert_logo_a8 as converter
import png_logo
import split_logo_a8 as splitter

RESOURCE = Path(__file__).resolve().parents[1]


def fields(table: dict, allowed: str, name: str) -> None:
    """配置边界严格校验，避免拼错字段而静默使用默认值。"""
    if not isinstance(table, dict) or set(table) - set(allowed.split()):
        raise ValueError(f'{name}: 无效表或未知字段 {set(table) - set(allowed.split()) if isinstance(table, dict) else table}')


def number(value, name: str, low: int = 0, high: int = 60000) -> int:
    """拒绝 TOML bool 冒充整数及越界时序。"""
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f'{name} 必须在 {low}..{high} 内')
    return value


def color(value: str) -> int:
    """颜色只接受 24 位十六进制字符串。"""
    if not isinstance(value, str) or not re.fullmatch(r'#?[0-9a-fA-F]{6}', value):
        raise ValueError(f'无效颜色 {value!r}，应为 RRGGBB')
    return int(value.lstrip('#'), 16)


def font_data(path: Path) -> tuple[str, dict[str, list[int]], dict[int, int]]:
    """读取真实 C 数组与 cmap 的 glyph ID，保留稀疏映射和共享字形。"""
    text = codec.strip_comments(path.read_text())
    arrays = {name: [int(v, 0) for v in re.findall(r'-?(?:0x[0-9a-fA-F]+|\b\d+\b)', data)]
              for name, data in re.findall(r'static const u?int(?:8|16|32)_t (\w+)\[\]\s*=\s*\{([^{}]+)\}', text)}
    found = re.search(r'cmaps\[\]\s*=\s*\{(.*?)\n\};', text, re.S)
    if not found:
        raise ValueError(f'字体缺少 cmap: {path}')
    glyphs = {}
    for entry in re.findall(r'\{([^{}]+)\}', found[1]):
        def get(key, entry=entry):
            match = re.search(rf'\.{key}\s*=\s*(\w+)', entry)
            if not match:
                raise ValueError(f'不支持的 cmap: {path}: {key}')
            return match[1]
        start, length, first = int(get('range_start')), int(get('range_length')), int(get('glyph_id_start'))
        kind = get('type')
        if kind in ('LV_FONT_FMT_TXT_CMAP_SPARSE_TINY', 'LV_FONT_FMT_TXT_CMAP_SPARSE_FULL'):
            offsets = arrays[get('unicode_list')]
        elif kind in ('LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY', 'LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL'):
            offsets = range(length)
        else:
            raise ValueError(f'不支持的 cmap 类型 {kind}: {path}')
        ids = arrays.get(get('glyph_id_ofs_list'))
        for index, offset in enumerate(offsets):
            if ids is not None and kind == 'LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL' and index and not ids[index]:
                continue
            glyph = first + (ids[index] if ids is not None else index if 'SPARSE' in kind else offset)
            if glyph:
                glyphs[start + offset] = glyph
    return text, arrays, glyphs


def font_codepoints(path: Path) -> set[int]:
    """覆盖范围由实际 cmap 决定，不能只读取字体生成命令的 range。"""
    return set(font_data(path)[2])


def font_width(path: Path, value: str) -> int:
    """复现 LVGL fmt_txt 的 8.4 advance、kerning 缩放及逐字舍入（字距为 0）。"""
    text, arrays, glyphs = font_data(path)
    found = re.search(r'\bglyph_dsc\[\]\s*=\s*\{(.*?)\n\};', text, re.S)
    if not found:
        raise ValueError(f'字体缺少 glyph_dsc: {path}')
    advances = [int(v) for v in re.findall(r'\.adv_w\s*=\s*(\d+)', found[1])]

    def field(key: str, body: str = text) -> str:
        match = re.search(rf'\.{key}\s*=\s*(&?\w+)', body)
        if not match:
            raise ValueError(f'字体缺少 {key}: {path}')
        return match[1].lstrip('&')

    kern = field('kern_dsc')
    scale = int(field('kern_scale'))
    table = ''
    if kern != 'NULL':
        descriptor = re.search(rf'\b{re.escape(kern)}\s*=\s*\{{([^{{}}]*)\}}', text)
        if not descriptor:
            raise ValueError(f'字体缺少 kerning 表 {kern}: {path}')
        table = descriptor[1]
    width = 0
    for index, ch in enumerate(value):
        glyph = glyphs[ord(ch)]
        following = glyphs.get(ord(value[index + 1]), 0) if index + 1 < len(value) else 0
        adjustment = 0
        if table and following:
            if int(field('kern_classes')):
                left = arrays[field('left_class_mapping', table)][glyph]
                right = arrays[field('right_class_mapping', table)][following]
                if left and right:
                    adjustment = arrays[field('class_pair_values', table)][
                        (left - 1) * int(field('right_class_cnt', table)) + right - 1]
            else:
                ids = arrays[field('glyph_ids', table)]
                values = arrays[field('values', table)]
                for pair in range(int(field('pair_cnt', table))):
                    if ids[pair * 2:pair * 2 + 2] == [glyph, following]:
                        adjustment = values[pair]
                        break
        width += (max(0, advances[glyph] + ((adjustment * scale) >> 4)) + 8) >> 4
    return width


def device_name_styles() -> list[tuple[str, Path, int]]:
    """UI 和生成器共读非生成头文件，避免 224/116 在 Python 中重复维护。"""
    header = (RESOURCE / 'DeviceNameLayout.h').read_text()
    result = []
    for prefix, font, macro in (('', 'font_oswaldBold_18.c', 'OEM_STARTUP_TITLE_WIDTH'),
                               ('footer_', 'font_oswaldBold_12.c', 'OEM_LOADING_FOOTER_WIDTH')):
        match = re.search(rf'^#define {macro}\s+(\d+)\s*$', header, re.M)
        if not match:
            raise ValueError(f'DeviceNameLayout.h 缺少整数常量 {macro}')
        result.append((prefix, RESOURCE / 'font' / font, int(match[1])))
    return result


def device_names(config: dict) -> dict:
    """写文件前校验实际字体与标签宽度，缺失俄语回退对应英语。"""
    fields(config, 'en ru footer_en footer_ru', 'device_name')
    result = {}
    for prefix, font, limit in device_name_styles():
        covered = font_codepoints(font)
        for language in ('en', 'ru'):
            key = prefix + language
            value = config.get(key, config.get(prefix + 'en'))
            if not isinstance(value, str) or not value or any(ord(ch) < 32 for ch in value):
                raise ValueError(f'device_name.{key} 必须是非空单行文字，缺省回退 en')
            for ch in value:
                if ord(ch) not in covered:
                    raise ValueError(f'device_name.{key}: 字符 {ch!r} U+{ord(ch):04X} 未被 {font.name} 覆盖')
            width = font_width(font, value)
            if width > limit:
                raise ValueError(f'device_name.{key} ({language}): {value!r} 宽度 {width} px 超出上限 {limit} px ({font.name})')
            result[key] = value
    return result


def text_parts(config: dict) -> tuple[codec.Parsed9, list[splitter.Part], tuple[int, int]]:
    """按 TTF kerning 和同一基线逐字符生成，零件保持原排版位置。"""
    Image = importlib.import_module('PIL.Image')
    ImageDraw = importlib.import_module('PIL.ImageDraw')
    ImageFont = importlib.import_module('PIL.ImageFont')
    value = config['text']
    if not isinstance(value, str) or not value.strip() or any(ord(ch) < 32 for ch in value):
        raise ValueError('logo.text 必须是非空单行文字')
    font = ImageFont.truetype(str(config['font']), number(config.get('size', 40), 'logo.size', 1, 126),
                              layout_engine=ImageFont.Layout.BASIC)
    glyphs = []
    for index, ch in enumerate(value):
        if ch.isspace():
            continue
        left, top, right, bottom = font.getbbox(ch, anchor='ls')
        if right <= left or bottom <= top:
            raise ValueError(f'logo.text 字符没有可见图形: {ch!r}')
        mask = Image.new('L', (right - left, bottom - top))
        ImageDraw.Draw(mask).text((-left, -top), ch, font=font, fill=255, anchor='ls')
        bounds = mask.getbbox()
        if bounds is None:
            raise ValueError(f'logo.text 字符没有可见图形: {ch!r}')
        mask = mask.crop(bounds)
        x = round(font.getlength(value[:index + 1]) - font.getlength(ch)) + left + bounds[0]
        glyphs.append((ch, x, top + bounds[1], mask))
    left, top = min(g[1] for g in glyphs), min(g[2] for g in glyphs)
    right = max(g[1] + g[3].width for g in glyphs)
    bottom = max(g[2] + g[3].height for g in glyphs)
    width, height = right - left, bottom - top
    if width > 294 or height > 126:
        raise ValueError(f'文字 Logo {width}x{height} 超出 294x126，请减小 size')
    groups = config.get('groups')
    if groups is not None and not re.fullmatch(r'[\d,|]+', groups):
        segments = groups.split('|')
        if any(not s for s in segments) or ''.join(segments) != ''.join(g[0] for g in glyphs):
            raise ValueError('文字 groups 必须按顺序包含所有非空白字符，如 NOVA|RTK')
        cursor, indices = 0, []
        for segment in segments:
            indices.append(','.join(str(i) for i in range(cursor, cursor + len(segment))))
            cursor += len(segment)
        groups = '|'.join(indices)
    ids = splitter.groups_for(len(glyphs), groups)
    parts = [splitter.Part(codec.Image9(f'oem_logo_part{i}', mask.width, mask.height, 'A8', mask.width,
                                       mask.tobytes()), x - left, y - top, ids[i])
             for i, (_, x, y, mask) in enumerate(glyphs)]
    source, = codec.parse_v9_text(codec.render_c_file([
        codec.Image9('oem_logo', width, height, 'A8', width, bytes(width * height))]))
    return source, parts, ((294 - width) // 2, (126 - height) // 2)


def effect(config: dict, parts: list[splitter.Part], timings: str, can_highlight: bool) -> tuple[str, int]:
    """预设展开成静态整数参数，并拒绝超出停留窗口的入场或高光。"""
    fields(config, 'preset total_ms background_ms interval_ms duration_ms rise_px highlight highlight_start_ms highlight_duration_ms', 'effect')
    preset = config.get('preset', 'cascade')
    if preset not in ('cascade', 'soft', 'sweep', 'wipe'):
        raise ValueError(f'未知 effect.preset {preset!r}')
    total = number(config.get('total_ms', 1300), 'total_ms', 1)
    bg = number(config.get('background_ms', 150), 'background_ms')
    interval = number(config.get('interval_ms', 50), 'interval_ms')
    duration = number(config.get('duration_ms', {'cascade': 250, 'soft': 600, 'sweep': 180, 'wipe': 700}[preset]), 'duration_ms', 1)
    rise = number(config.get('rise_px', 6 if preset in ('cascade', 'soft') else 0), 'rise_px', 0, 126)
    highlight = config.get('highlight', preset in ('cascade', 'sweep'))
    if (type(highlight) is not bool or (preset == 'soft' and highlight)
            or (preset in ('sweep', 'wipe') and rise)):
        raise ValueError('highlight 必须为 bool；soft 禁止扫光；sweep/wipe 的 rise_px 必须为 0')
    end = bg + duration
    if preset == 'cascade':
        overrides = {int(g): (int(d), int(t)) for g, d, t in (s.split(':') for s in timings.split('|'))} if timings else {}
        end = max(bg + overrides.get(p.group, (p.group * interval, duration))[0]
                  + overrides.get(p.group, (0, duration))[1] for p in parts)
    start = number(config.get('highlight_start_ms', max(900, end)), 'highlight_start_ms')
    length = number(config.get('highlight_duration_ms', 300), 'highlight_duration_ms', 1)
    if end > total or (highlight and can_highlight and (start < end or start + length > total)):
        raise ValueError('入场/扫光超出 total_ms，或扫光早于入场结束')
    text = f'{{{bg}, {interval}, {duration}, {rise}, {str(highlight).lower()}, {start}, {length}, {total}, LOGO_EFFECT_{preset.upper()}}}'
    return text, total


def build(directory: Path, output: Path | None = None) -> dict[str, bytes]:
    """验证所有输入及 C 回读后才写生成物；返回可逐字节比较的稳定输出。"""
    with (directory / 'oem.toml').open('rb') as file:
        config = tomllib.load(file)
    fields(config, 'name colors logo device_name effect', 'oem')
    if not isinstance(config.get('name'), str) or not config['name'].strip():
        raise ValueError('name 必须是非空字符串')
    colors, logo = config.get('colors', {}), config.get('logo', {})
    fields(colors, 'background logo text footer_text', 'colors')
    bg, fg, text_color = [color(colors[key]) for key in ('background', 'logo', 'text')]
    footer_color = color(colors.get('footer_text', colors['text']))
    fields(logo, 'image text font size groups timings max_error fit', 'logo')
    if ('image' in logo) == ('text' in logo):
        raise ValueError('logo.image 与 logo.text 必须二选一')
    names = device_names(config.get('device_name', {}))
    timings = logo.get('timings', '')
    if not isinstance(timings, str) or ('groups' in logo and not isinstance(logo['groups'], str)):
        raise ValueError('logo.timings/groups 必须是字符串')
    recolor = True
    if 'text' in logo:
        if 'font' not in logo or any(k in logo for k in ('fit', 'max_error')):
            raise ValueError('文字必须指定 font；fit/max_error 仅适用于 PNG')
        source, parts, (x, y) = text_parts({**logo, 'font': directory / logo['font']})
    else:
        if any(k in logo for k in ('font', 'size')):
            raise ValueError('font/size 仅适用于文字')
        path = directory / logo['image']
        if path.suffix.lower() == '.png':
            if type(logo.get('fit', False)) is not bool:
                raise ValueError('logo.fit 必须为 bool')
            decoded = png_logo.read(path, fit=logo.get('fit', False))
            converted = converter.convert(decoded)
            maximum = number(logo.get('max_error', 20), 'logo.max_error', 0, 255)
            print(f'{directory.name}: PNG 最大误差 {converted.maximum}/255，平均 {converted.mean:.6f}/255')
            if converted.maximum > maximum:
                raise ValueError(f'PNG 单色回读误差 {converted.maximum} 超出 max_error {maximum}')
            source, = codec.parse_v9_text(codec.render_c_file([converted.image]))
            x, y = converted.x + (294 - decoded.w) // 2, converted.y + (126 - decoded.h) // 2
        elif path.suffix.lower() == '.c':
            if any(k in logo for k in ('fit', 'max_error')):
                raise ValueError('fit/max_error 仅适用于 PNG')
            original = path.read_text()
            source, = codec.parse_v9_text(original)
            _, _, x, y = splitter.layout_from(original, source.symbol)
            recolor = source.cf == 'A8'
            if source.cf not in ('A8', 'RGB565'):
                raise ValueError('迁移 C 源只接受 A8/RGB565')
        else:
            raise ValueError('logo.image 只支持 PNG 或迁移 C 源')
        source.symbol = 'oem_logo'
        if recolor:
            parts = splitter.split(source, logo.get('groups'))
        else:
            if logo.get('groups') or timings:
                raise ValueError('RGB565 保持单零件，不接受 groups/timings')
            parts = [splitter.Part(codec.Image9('oem_logo_part0', source.w, source.h, source.cf,
                                               source.stride, source.data), 0, 0, 0)]
    if not 0 <= x <= 294 - source.w or not 0 <= y <= 126 - source.h:
        raise ValueError('Logo 包围盒超出 294x126')
    parameters, total = effect(config.get('effect', {}), parts, timings, recolor)
    # 复用拆分器的 C 资源/组表生成；RGB565 跳过拆分，字节保持原样。
    rendered = splitter.render(source, (bg, fg, x, y), parts, timings,
                               highlight=recolor, effect=parameters, recolor=recolor)
    if [p.data for p in codec.parse_v9_text(rendered)] != [p.image.data for p in parts]:
        raise ValueError('生成 C 回读不一致')
    header = ('/** @file oem_config.h @brief 由 oem_build.py 生成，请修改 oem.toml 后重新生成。 */\n'
              '#ifndef APP_OEM_CONFIG_H\n#define APP_OEM_CONFIG_H\n\n')
    for key, value in names.items():
        prefix = 'footer_' if key.startswith('footer_') else ''
        font = next(font for part, font, _ in device_name_styles() if part == prefix)
        header += (f'#define OEM_DEVICE_{key.upper()} {json.dumps(value, ensure_ascii=False)}\n'
                   f'#define OEM_DEVICE_{key.upper()}_WIDTH {font_width(font, value)}\n')
    header += (f'#define OEM_TEXT_COLOR 0x{text_color:06x}\n#define OEM_FOOTER_COLOR 0x{footer_color:06x}\n'
               f'#define OEM_LOGO_TOTAL_MS {total}\n\n#endif\n')
    files = {'oem_logo.c': rendered.encode(), 'oem_config.h': header.encode()}
    target = output if output is not None else directory / 'generated'
    target.mkdir(parents=True, exist_ok=True)
    for name, data in files.items():
        (target / name).write_bytes(data)
    return files


def main() -> int:
    """命令行失败返回非零，文字缺 Pillow 时给出可直接使用的 uv 命令。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    try:
        build(args.directory, args.output)
    except ImportError as exc:
        parser.exit(1, f'ERROR: 文字模式需要 Pillow；用 uv run --with pillow python3 -B {__file__} {args.directory}: {exc}\n')
    except (OSError, ValueError, KeyError, TypeError, codec.ConvertError) as exc:
        parser.exit(1, f'ERROR: {exc}\n')
    print(f'{args.directory}: OEM 生成完成')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
