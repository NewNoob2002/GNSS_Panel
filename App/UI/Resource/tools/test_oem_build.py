#!/usr/bin/env python3
"""OEM 配置、字体、生成物一致性回归；图片模式无 Pillow 仍可执行。"""
import contextlib
import importlib.util
import io
import shutil
import struct
import subprocess
import tempfile
import tomllib
import unittest
import zlib
from pathlib import Path

import convert_images_v9 as codec
import oem_build

ROOT = oem_build.RESOURCE / 'oem'


class OemTests(unittest.TestCase):
    """覆盖所有客户再生成、配置拒绝、文字分组及真实字体稀疏范围。"""

    def regenerate(self, name):
        with (ROOT / name / 'oem.toml').open('rb') as file:
            config = tomllib.load(file)
        if 'text' in config['logo'] and importlib.util.find_spec('PIL') is None:
            print(f'SKIP: {name} 文字再生成需要 Pillow；用 uv run --with pillow python3 -B 运行本测试')
            self.skipTest('SKIP Pillow 未安装，文字再生成未验证')
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory)
            files = oem_build.build(ROOT / name, target)
            self.assertEqual(set(files), {p.name for p in (ROOT / name / 'generated').iterdir()})
            for filename in files:
                self.assertEqual((target / filename).read_bytes(), (ROOT / name / 'generated' / filename).read_bytes(),
                                 f'{name}/{filename} 与提交生成物不一致')

    def test_generated_all_oems(self):
        for config in sorted(ROOT.glob('*/oem.toml')):
            with self.subTest(oem=config.parent.name):
                self.regenerate(config.parent.name)

    def test_migrated_rgb565_bytes(self):
        source, = codec.parse_v9_text((ROOT / 'rgk/logo.c').read_text())
        image, = codec.parse_v9_text((ROOT / 'rgk/generated/oem_logo.c').read_text())
        self.assertEqual((source.cf, source.data), ('RGB565', image.data))
        self.assertIn('false, false,', (ROOT / 'rgk/generated/oem_logo.c').read_text())

    def test_text_generated_glyphs(self):
        parts = codec.parse_v9_text((ROOT / 'demo/generated/oem_logo.c').read_text())
        self.assertEqual(len(parts), 7)
        self.assertTrue(all(p.cf == 'A8' and any(p.data) for p in parts))

    def test_font_coverage_and_english_fallback(self):
        for size in (12, 18):
            points = oem_build.font_codepoints(oem_build.RESOURCE / 'font' / f'font_oswaldBold_{size}.c')
            self.assertIn(ord('ё'), points)
            self.assertIn(ord('Œ'), points)
            self.assertNotIn(340, points)  # 稀疏 cmap 的空洞不算覆盖。
        value = {'en': 'NOVA RTK', 'footer_en': 'NOVA GNSS Receiver'}
        names = oem_build.device_names(value)
        self.assertEqual(names['ru'], value['en'])
        self.assertEqual(names['footer_ru'], value['footer_en'])
        for key, font in [('en', '18'), ('footer_en', '12')]:
            with self.assertRaisesRegex(ValueError, rf"字.*'中'.*U\+4E2D.*font_oswaldBold_{font}"):
                oem_build.device_names({**value, key: '中'})
        for invalid in ('', 'a\nb', '\0'):
            with self.assertRaises(ValueError):
                oem_build.device_names({**value, 'en': invalid})

    def test_device_name_width_rejects_english_and_russian(self):
        names = {'en': 'NOVA RTK', 'ru': 'НОВА RTK', 'footer_en': 'NOVA GNSS', 'footer_ru': 'Приёмник NOVA'}
        for prefix, font, limit in oem_build.device_name_styles():
            for language, char in (('en', 'W'), ('ru', 'Ж')):
                key = prefix + language
                with self.subTest(field=key):
                    value = char * 80
                    with self.assertRaisesRegex(ValueError, rf'device_name\.{key} \({language}\): .* 宽度 \d+ px 超出上限 {limit} px'):
                        oem_build.device_names({**names, key: value})
                    fitting = char * (limit // oem_build.font_width(font, char))
                    self.assertEqual(oem_build.device_names({**names, key: fitting})[key], fitting)

    def test_font_advance_rounding_and_kerning(self):
        for _, font, _ in oem_build.device_name_styles():
            self.assertLess(oem_build.font_width(font, 'AV'),
                            oem_build.font_width(font, 'A') + oem_build.font_width(font, 'V'))
        common = ('static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {\n'
                  '{.adv_w=0}, {.adv_w=167}, {.adv_w=168}\n};\n'
                  'static const lv_font_fmt_txt_cmap_t cmaps[] = {\n'
                  '{.range_start=65, .range_length=2, .glyph_id_start=1, .unicode_list=NULL, '
                  '.glyph_id_ofs_list=NULL, .type=LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY}\n};\n')
        classes = ('static const uint8_t left[] = {0,1,0};\n'
                   'static const uint8_t right[] = {0,0,1};\n'
                   'static const int8_t values[] = {-16};\n'
                   'static const lv_font_fmt_txt_kern_classes_t kern = {'
                   '.class_pair_values=values, .left_class_mapping=left, .right_class_mapping=right, '
                   '.left_class_cnt=1, .right_class_cnt=1};\n'
                   'static const lv_font_fmt_txt_dsc_t font_dsc = {'
                   '.kern_dsc=&kern, .kern_scale=16, .kern_classes=1};\n')
        pairs = ('static const uint8_t ids[] = {1,2};\n'
                 'static const int8_t values[] = {-16};\n'
                 'static const lv_font_fmt_txt_kern_pair_t kern = {'
                 '.glyph_ids=ids, .values=values, .pair_cnt=1, .glyph_ids_size=0};\n'
                 'static const lv_font_fmt_txt_dsc_t font_dsc = {'
                 '.kern_dsc=&kern, .kern_scale=16, .kern_classes=0};\n')
        absent = ('static const lv_font_fmt_txt_dsc_t font_dsc = {'
                  '.kern_dsc=NULL, .kern_scale=16, .kern_classes=0};\n')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'font.c'
            for kerning, width in ((classes, 20), (pairs, 20), (absent, 21)):
                path.write_text(common + kerning)
                self.assertEqual(oem_build.font_width(path, 'A'), 10)
                self.assertEqual(oem_build.font_width(path, 'B'), 11)
                self.assertEqual(oem_build.font_width(path, 'AB'), width)

    def test_png_standard_library_and_all_presets(self):
        def chunk(kind, body):
            return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))
        pixels = bytes(v for y in range(8) for v in ([0] + [255 if x == 4 and 2 <= y <= 5 else 0 for x in range(12)]))
        png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 12, 8, 8, 0, 0, 0, 0))
               + chunk(b'IDAT', zlib.compress(pixels)) + chunk(b'IEND', b''))
        config = ('name="Test"\n[colors]\nbackground="101820"\nlogo="f2aa4c"\ntext="e8e8e8"\n'
                  '[logo]\nimage="logo.png"\n[device_name]\nen="Test"\nfooter_en="Receiver"\n')
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            (base / 'logo.png').write_bytes(png)
            for preset in ('cascade', 'soft', 'sweep', 'wipe'):
                (base / 'oem.toml').write_text(config + f'[effect]\npreset="{preset}"\n')
                with contextlib.redirect_stdout(io.StringIO()):
                    files = oem_build.build(base)
                self.assertIn(f'LOGO_EFFECT_{preset.upper()}'.encode(), files['oem_logo.c'])
                self.assertIn(b'OEM_FOOTER_COLOR 0xe8e8e8', files['oem_config.h'])
            previous = (base / 'generated/oem_logo.c').read_bytes()
            for bad in ('preset="unknown"', 'duration_ms=0', 'total_ms=100', 'highlight="yes"',
                        'rise_px=true', 'background_ms=60001', 'preset="soft"\nhighlight=true',
                        'preset="wipe"\nrise_px=6', 'highlight_start_ms=1', 'intervl_ms=9'):
                (base / 'oem.toml').write_text(config + '[effect]\n' + bad + '\n')
                with contextlib.redirect_stdout(io.StringIO()), self.assertRaises(ValueError):
                    oem_build.build(base)
                self.assertEqual(previous, (base / 'generated/oem_logo.c').read_bytes())
            for bad in ('image="logo.png"\ntext="x"', 'image="logo.png"\nmax_error=256',
                        'image="logo.png"\nfit="yes"', 'image="logo.png"\ngroups="5"'):
                altered = config.replace('image="logo.png"', bad)
                (base / 'oem.toml').write_text(altered)
                with contextlib.redirect_stdout(io.StringIO()), self.assertRaises(ValueError):
                    oem_build.build(base)

    def test_text_groups_baseline_and_failure(self):
        if importlib.util.find_spec('PIL') is None:
            print('SKIP: 文字 kerning/分组回归需要 Pillow')
            self.skipTest('SKIP Pillow 未安装')
        font = oem_build.RESOURCE / 'font/Oswald-Bold.ttf'
        source, parts, position = oem_build.text_parts({'text': 'NOVA RTK', 'font': font, 'size': 40, 'groups': 'NOVA|RTK'})
        self.assertEqual([p.group for p in parts], [0, 0, 0, 0, 1, 1, 1])
        self.assertEqual(position, ((294 - source.w) // 2, (126 - source.h) // 2))
        self.assertTrue(all(parts[i].x < parts[i + 1].x for i in range(6)))
        # 字形紧裁剪后 y 不同，但仍保持同一基线。
        _, chars, _ = oem_build.text_parts({'text': 'Ag', 'font': font})
        self.assertGreater(chars[1].y + chars[1].image.h, chars[0].y + chars[0].image.h)
        for config in ({'groups': 'NOVA|BAD'}, {'text': ' '}, {'text': 'NOVA' * 40}, {'size': 0}):
            with self.assertRaises(ValueError):
                oem_build.text_parts({'text': 'NOVA RTK', 'font': font, **config})

    def test_unknown_oem_cmake(self):
        cmake = shutil.which('cmake')
        if cmake is None:
            raise AssertionError('CMake 必须可用以验证未知 OEM')
        with tempfile.TemporaryDirectory() as directory:
            script = Path(directory) / 'select.cmake'
            script.write_text(f'cmake_minimum_required(VERSION 3.20)\ninclude("{oem_build.RESOURCE.parents[1] / "oem.cmake"}")\n')
            result = subprocess.run([cmake, '-DOEM=not-an-oem', '-P', str(script)], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            for text in ('Unknown OEM', 'Available OEM', 'singularxyz', 'demo', 'neutral', 'rgk'):
                self.assertIn(text, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main(verbosity=2)
