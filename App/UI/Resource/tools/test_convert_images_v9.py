#!/usr/bin/env python3
"""图片迁移回归测试（标准库）：python3 -B App/UI/Resource/tools/test_convert_images_v9.py。"""
import contextlib
import io
import random
import re
from pathlib import Path
import struct
import tempfile
import unittest
import zlib
from unittest.mock import patch

import convert_images_v9 as converter
import convert_logo_a8 as logo_converter
import png_logo
import split_logo_a8 as splitter
import test_oem_build


def load_tests(loader, tests, pattern):
    """资源工具统一执行 OEM 校验；Pillow 缺失仅跳过文字项并明确报告。"""
    tests.addTests(loader.loadTestsFromModule(test_oem_build))
    return tests


class ConversionTests(unittest.TestCase):
    """Exercise independent pixel fixtures and failure paths. 覆盖已知像素与失败路径。"""

    def test_planar_layout_and_quantization(self):
        """Check known little-endian RGB565 and planar alpha. 校验已知字节布局。"""
        rgba = bytes((255, 0, 0, 7, 0, 255, 0, 128, 0, 0, 255, 255))
        image = converter.encode_v9("img_src_t", 3, 1, rgba, "RGB565A8")
        self.assertEqual(image.data, bytes.fromhex("00 f8 e0 07 1f 00 07 80 ff"))
        self.assertEqual(image.stride, 6)
        self.assertEqual(converter.quantize565(127, 129, 7), 0x7C00)
        self.assertEqual(converter.decode_v8("LV_IMG_CF_TRUE_COLOR_ALPHA", 3, 1,
                                           bytes.fromhex("00 f8 07 e0 07 80 1f 00 ff")), rgba)

    def test_indexed_palette_and_row_padding(self):
        """Check BGRA palettes, MSB indexes and odd row widths. 校验调色板及逐行填充。"""
        palette = bytes.fromhex("01 02 03 04 10 20 30 40 50 60 70 80 90 a0 b0 c0")
        rgba = converter.decode_v8("LV_IMG_CF_INDEXED_2BIT", 3, 2,
                                   palette + bytes.fromhex("1b e7"))
        expected = bytes.fromhex("03 02 01 04 30 20 10 40 70 60 50 80 "
                                 "b0 a0 90 c0 70 60 50 80 30 20 10 40")
        self.assertEqual(rgba, expected)
        palette4 = palette + bytes(48)
        rgba4 = converter.decode_v8("LV_IMG_CF_INDEXED_4BIT", 3, 2,
                                    palette4 + bytes.fromhex("01 2f 32 1f"))
        self.assertEqual(rgba4, expected)

    def test_swap_validation(self):
        """Reject altered colors or alpha in swapped branches. 拒绝非字节交换的分支。"""
        plain = bytes.fromhex("12 34 56 78 9a bc")
        converter.check_swap_branch("t", "LV_IMG_CF_TRUE_COLOR_ALPHA", plain,
                                    bytes.fromhex("34 12 56 9a 78 bc"))
        for wrong in (plain, bytes.fromhex("34 12 57 9a 78 bc"), b"\x00"):
            with self.assertRaises(converter.ConvertError):
                converter.check_swap_branch("t", "LV_IMG_CF_TRUE_COLOR_ALPHA", plain, wrong)

    def test_nested_preprocessor_and_strict_literals(self):
        """Select nested branches and reject unsupported C tokens. 校验嵌套分支与严格解析。"""
        source = ("#if LV_COLOR_DEPTH == 32\n0x01,\n#if 1\n0x02,\n#endif\n"
                  "#elif LV_COLOR_DEPTH == 16\n#if !LV_COLOR_16_SWAP\n0x03,\n"
                  "#else\n0x04,\n#endif\n#else\n0x05,\n#endif\n")
        for swap, expected in ((0, b"\x03"), (1, b"\x04")):
            selected = converter.select_branch(source, {"LV_COLOR_DEPTH": 16, "LV_COLOR_16_SWAP": swap})
            self.assertEqual(converter.hex_bytes(selected), expected)
        for bad in ("-1,", "0x01 + 0x02,", "0x00, BAD,", "0x100,", "0x01,,", "0x01 garbage"):
            with self.assertRaises(converter.ConvertError):
                converter.hex_bytes(bad)
        with self.assertRaises(converter.ConvertError):
            converter.eval_condition("__import__('os')", {})

    def test_format_policy(self):
        """Derive formats without name exceptions and honor overrides. 校验按源格式推导。"""
        source = converter.Image8(Path("img_src_SystemLoading_RGK.c"), "img_src_RGKLogo",
                                  "LV_IMG_CF_TRUE_COLOR", 1, 1, 2, b"\xff\xff\xff\xff")
        cases = (("TRUE_COLOR", "RGB565", "RGB565"),
                 ("TRUE_COLOR_CHROMA_KEYED", "RGB565A8", "RGB565A8"),
                 ("TRUE_COLOR_ALPHA", "RGB565", "RGB565A8"),
                 ("ALPHA_8BIT", "A8", "A8"),
                 ("INDEXED_1BIT", "RGB565", "RGB565A8"),
                 ("INDEXED_2BIT", "RGB565", "RGB565A8"),
                 ("INDEXED_4BIT", "RGB565", "RGB565A8"),
                 ("INDEXED_8BIT", "RGB565", "RGB565A8"))
        for cf, opaque_format, alpha_format in cases:
            source.cf = "LV_IMG_CF_" + cf
            for alpha, expected in ((255, opaque_format), (127, alpha_format)):
                with self.subTest(cf=cf, alpha=alpha):
                    source.rgba = bytes((255, 255, 255, alpha))
                    self.assertEqual(converter.choose_format(source, {}), expected)
                    self.assertEqual(converter.choose_format(source, {"RGKLogo": "A8"}), "A8")
        with self.assertRaises(converter.ConvertError):
            converter.choose_format(source, {"RGKLogo": "A8", "SystemLoading_RGK": "RGB565"})

    def test_chroma_key_is_limited_to_keyed_source(self):
        """Apply the project green key only to keyed sources. 普通真彩色不得变透明。"""
        data = bytes.fromhex("e0 07 00 f8")
        for cf, alpha in (("TRUE_COLOR", 255), ("TRUE_COLOR_CHROMA_KEYED", 0)):
            source_format = "LV_IMG_CF_" + cf
            rgba = converter.decode_v8(source_format, 2, 1, data)
            self.assertEqual(rgba, bytes((0, 255, 0, alpha, 255, 0, 0, 255)))
            converter.check_swap_branch("t", source_format, data, bytes.fromhex("07 e0 f8 00"))
            source = converter.Image8(Path("t.c"), "img_src_t", source_format, 2, 1, 4, rgba)
            encoded = converter.encode_v9(source.symbol, 2, 1, rgba, converter.choose_format(source, {}))
            self.assertFalse(converter.verify_text([source], converter.render_c_file([encoded])))

    def test_unused_transparent_palette_entry(self):
        """Ignore unused transparent palette entries. 只按实际像素判断，未使用的透明调色板项不保留 alpha。"""
        root = converter.HERE.parents[3] / "build"
        root.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="images-v9-palette-", dir=root) as directory:
            path = Path(directory) / "img_src_t.c"
            path.write_text("const uint8_t pixels[] = {0, 0, 255, 255, 0, 0, 0, 0, 0};\n"
                            "const lv_img_dsc_t img_src_t = {\n"
                            ".header.cf = LV_IMG_CF_INDEXED_1BIT, .header.w = 1, .header.h = 1,\n"
                            ".data_size = 9, .data = pixels,\n};\n")
            source, = converter.parse_v8_file(path)
            self.assertTrue(source.opaque)
            self.assertFalse(source.palette_opaque)
            self.assertEqual(converter.choose_format(source, {}), "RGB565")
            path.write_text(path.read_text().replace("255, 0, 0, 0, 0, 0", "255, 0, 0, 0, 255, 0"))
            source, = converter.parse_v8_file(path)
            self.assertTrue(source.palette_opaque)
            self.assertEqual(converter.choose_format(source, {}), "RGB565")

    def test_lossy_or_invalid_encoding_rejected(self):
        """Reject alpha loss, multicolor A8 and invalid dimensions. 拒绝有损或越界输入。"""
        for rgba, fmt in ((b"\xff\x00\x00\x80", "RGB565"), (b"", "RGB565A8")):
            with self.assertRaises(converter.ConvertError):
                converter.encode_v9("t", 1, 1, rgba, fmt)
        with self.assertRaises(converter.ConvertError):
            converter.encode_v9("t", 2, 1, bytes.fromhex("ff 00 00 ff 00 ff 00 ff"), "A8")
        with self.assertRaises(converter.ConvertError):
            converter.encode_v9("t", 0, 1, b"", "RGB565")

    def test_readback_detects_corruption(self):
        """Detect every header constraint and pixel-plane corruption. 检出头字段及像素损坏。"""
        rgba = bytes.fromhex("ff 00 00 11 00 ff 00 22")
        source = converter.Image8(Path("img_src_t.c"), "img_src_t", "LV_IMG_CF_TRUE_COLOR_ALPHA",
                                  2, 1, 6, rgba)
        text = converter.render_c_file([converter.encode_v9(source.symbol, 2, 1, rgba, "RGB565A8")])
        self.assertFalse(converter.verify_text([source], text))
        substitutions = ((".magic = LV_IMAGE_HEADER_MAGIC", ".magic = 0"),
                         (".flags = 0", ".flags = 1"), (".reserved_2 = 0", ".reserved_2 = 1"),
                         (".w = 2", ".w = 3"), (".h = 1", ".h = 2"),
                         (".stride = 4", ".stride = 5"),
                         ("sizeof(img_src_t_map)", "5"),
                         ("LV_ATTRIBUTE_LARGE_CONST", ""),
                         ("0xf8", "0xf0"), ("0x11", "0x12"))
        for old, new in substitutions:
            with self.subTest(field=old):
                self.assertTrue(converter.verify_text([source], text.replace(old, new)))

    def test_file_roundtrip_and_check_mode(self):
        """Check disk readback, CRLF preservation and failure exits. 校验磁盘回读与只读模式。"""
        root = converter.HERE.parents[3] / "build"
        root.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="images-v9-test-", dir=root) as directory:
            base = Path(directory)
            src, dst = base / "src", base / "dst"
            src.mkdir()
            fixture = ("const uint8_t pixels[] = {0x00, 0xf8, 0x80};\n"
                       "const lv_img_dsc_t img_src_t = {\n"
                       ".header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA, .header.w = 1, .header.h = 1,\n"
                       ".data_size = 3, .data = pixels,\n};\n")
            # 逐像素色为红色，需另外提供真正交换后的分支。
            fixture = fixture.replace("{0x00, 0xf8, 0x80}", "{\n#if LV_COLOR_16_SWAP == 0\n"
                                      "0x00, 0xf8, 0x80,\n#else\n0xf8, 0x00, 0x80,\n#endif\n}")
            path = src / "img_src_t.c"
            path.write_bytes(fixture.replace("\n", "\r\n").encode())
            before = path.read_bytes()
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(converter.run(src, dst, {}, False), 0)
                target = dst / path.name
                target.write_bytes(target.read_bytes().replace(b"\n", b"\r\n"))
                self.assertEqual(converter.run(src, dst, {}, False), 0)
                existing = target.read_bytes()
                self.assertIn(b"\r\n", existing)
                self.assertEqual(converter.run(src, dst, {}, True), 0)
                self.assertEqual(target.read_bytes(), existing)
                target.write_bytes(existing.replace(b".stride = 2", b".stride = 3"))
                corrupted = target.read_bytes()
                self.assertEqual(converter.run(src, dst, {}, True), 1)
                self.assertEqual(target.read_bytes(), corrupted)
                self.assertEqual(converter.run(src, dst, {"unknown": "A8"}, False), 1)
                self.assertEqual(target.read_bytes(), corrupted)
                target.unlink()
                self.assertEqual(converter.run(src, dst, {}, True), 1)
                self.assertFalse(target.exists())
            self.assertEqual(path.read_bytes(), before)
            with self.assertRaises(converter.ConvertError):
                converter.run(src, src, {}, False)
            with self.assertRaises(converter.ConvertError):
                converter.run(dst, base / "empty", {}, False)


    def test_logo_crop_readback_and_color_rejection(self):
        """覆盖裁剪余量、透明边界、误差门槛、残留列及确定性。"""
        pixels = bytearray(bytes((0, 0, 0, 255)) * 100)
        for y in range(4, 6):
            for x in range(4, 6):
                pixels[(y * 10 + x) * 4:(y * 10 + x + 1) * 4] = bytes((255, 255, 255, 255))
        source = converter.Image8(Path("t.c"), "img_src_t", "LV_IMG_CF_TRUE_COLOR",
                                  10, 10, 200, bytes(pixels))
        logo = logo_converter.convert(source)
        self.assertEqual((logo.x, logo.y, logo.image.w, logo.image.h), (2, 2, 6, 6))
        self.assertEqual(logo.maximum, 0)
        self.assertEqual(logo.image.data[2 * 6 + 2], 255)
        self.assertEqual(logo.image.data[0], 0)
        self.assertEqual(logo_converter.render(logo), logo_converter.render(logo_converter.convert(source)))
        for y in range(10):
            pixels[y * 40:y * 40 + 4] = bytes((255, 255, 255, 255))
        source.rgba = bytes(pixels)
        excluded = logo_converter.convert(source, True)
        self.assertEqual(excluded.image, logo.image)
        source.rgba = bytes((0, 0, 0, 255)) * 8 + bytes((255, 0, 0, 255, 0, 255, 0, 255))
        source.w, source.h = 10, 1
        self.assertGreater(logo_converter.convert(source).maximum, 20)
        source.rgba = bytes((0, 0, 0, 255)) * 10
        with self.assertRaises(converter.ConvertError):
            logo_converter.convert(source)
        source.rgba = bytes((0, 0, 0, 128)) * 10
        with self.assertRaises(converter.ConvertError):
            logo_converter.convert(source)

    def test_logo_lvgl_blend_and_metadata_preservation(self):
        """已知 RGB565 混色与部署资源规范化保留布局常量。"""
        self.assertEqual(logo_converter.blend565(0xffff, 0, 128), 0x7bef)
        self.assertEqual(logo_converter.blend565(0xffff, 0, 2), 0)
        self.assertEqual(logo_converter.blend565(0xffff, 0, 253), 0xffff)
        image = converter.encode_v9("img_src_t", 1, 1, bytes((255, 255, 255, 255)), "A8")
        logo = logo_converter.Logo(image, (0, 0, 0), (255, 255, 255), 2, 3, 0, 0)
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            path = base / "img_src_t.c"
            path.write_text(logo_converter.render(logo))
            self.assertIn('#include "UI/Resource/LogoLayout.h"', path.read_text())
            self.assertNotIn("Pages/SystemLoading", path.read_text())
            original = path.read_bytes()
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(converter.run(base, base, {}, True), 0)
                self.assertEqual(converter.run(base, base, {}, False), 0)
            self.assertEqual(path.read_bytes(), original)

    def test_logo_cli_rejects_loss_and_preserves_color_on_request(self):
        """超限默认失败且不写文件；显式彩色回退保留像素与原有 CRLF。"""
        rgba = bytes((0, 0, 0, 255)) * 8 + bytes((255, 0, 0, 255, 0, 255, 0, 255))
        image = converter.encode_v9("img_src_t", 10, 1, rgba, "RGB565")
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            source, target = base / "source.c", base / "target.c"
            source.write_text(converter.render_c_file([image]))
            target.write_bytes(b"unchanged\r\n")
            arguments = ["convert_logo_a8.py", str(source), str(target)]
            with patch("sys.argv", arguments), contextlib.redirect_stdout(io.StringIO()), \
                    contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as failed:
                    logo_converter.main()
                self.assertEqual(failed.exception.code, 1)
            self.assertEqual(target.read_bytes(), b"unchanged\r\n")
            with patch("sys.argv", arguments + ["--keep-rgb565-on-error"]), \
                    contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(logo_converter.main(), 0)
                original = target.read_bytes()
                self.assertEqual(logo_converter.main(), 0)
            self.assertEqual(target.read_bytes(), original)
            self.assertNotIn(b"\n", original.replace(b"\r\n", b""))
            parsed, = converter.parse_v9_text(target.read_text())
            self.assertEqual(parsed.data, image.data)
            self.assertEqual(parsed.cf, "RGB565")

    def test_logo_parts_exact_reconstruction(self):
        """部署零件从 C 独立回读，max 与相加重建都逐字节等于旧数组。"""
        for customer, count in (('singularxyz', 10), ('neutral', 1)):
            original_path = converter.HERE.parent / 'oem' / customer / 'logo.c'
            original, = converter.parse_v9_text(original_path.read_text())
            parts = splitter.split(original)
            self.assertEqual(len(parts), count)
            deployed = original_path.parent / 'generated' / 'oem_logo.c'
            deployed_text = deployed.read_text()
            images = {p.symbol: p for p in converter.parse_v9_text(deployed_text)}
            rows = re.findall(r'\{&(\w+),\s*(-?\d+),\s*(-?\d+),\s*(\d+)\}', deployed_text)
            self.assertEqual(len(rows), count)
            restored = []
            for symbol, x, y, group in rows:
                im = images[symbol]
                restored.append(splitter.Part(converter.Image9(im.symbol, im.w, im.h, im.cf, im.stride, im.data),
                                               int(x), int(y), int(group)))
            self.assertEqual([p.image.data for p in parts], [p.image.data for p in restored])
            splitter.verify(original, restored)
            if count == 10:
                bounds = [(p.x + min(n % p.image.w for n, a in enumerate(p.image.data) if a > 24),
                           p.x + max(n % p.image.w for n, a in enumerate(p.image.data) if a > 24)) for p in restored]
                self.assertEqual(bounds, [(2, 28), (27, 39), (36, 59), (54, 83), (83, 105),
                                          (104, 116), (114, 139), (137, 154), (154, 241), (248, 260)])
                self.assertLess(parts[1].x, parts[0].x + parts[0].image.w)  # 斜体列重叠仍不混字。

    def test_logo_groups_and_weak_edges(self):
        """斜线的八邻域连接、弱边缘归属及显式分组不丢像素。"""
        im = converter.Image9('img_src_t', 7, 3, 'A8', 7,
                              bytes((255, 0, 0, 0, 0, 0, 255, 0, 255, 0, 5, 0, 255, 0, 0, 0, 255, 0, 255, 0, 0)))
        parsed, = converter.parse_v9_text(converter.render_c_file([im]))
        parts = splitter.split(parsed, '0,1')
        self.assertEqual(len(parts), 2)
        self.assertEqual([p.group for p in parts], [0, 0])
        splitter.verify(parsed, parts)
        for bad in ('0', '0|0,1', '0|2', '|0,1'):
            with self.assertRaises(ValueError):
                splitter.split(parsed, bad)

    @staticmethod
    def png(width, height, color, pixels, filters=None, depth=8, interlace=0, palette=None, transparency=None):
        """用 zlib 生成 PNG fixture，反向编码所有五种行滤波。"""
        def chunk(kind, body):
            return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))
        channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[color]
        stride = (width * channels * depth + 7) // 8
        bpp = max(1, (channels * depth + 7) // 8)
        raw = bytearray()
        previous = bytes(stride)
        for y in range(height):
            row = pixels[y * stride:(y + 1) * stride]
            mode = filters[y] if filters else 0
            raw.append(mode)
            for i, value in enumerate(row):
                left = row[i - bpp] if i >= bpp else 0
                up, corner = previous[i], previous[i - bpp] if i >= bpp else 0
                pred = left + up - corner
                distances = abs(pred - left), abs(pred - up), abs(pred - corner)
                paeth = (left, up, corner)[distances.index(min(distances))]
                raw.append((value - (0, left, up, (left + up) // 2, paeth)[mode]) & 255)
            previous = row
        return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, depth, color, 0, 0, interlace))
                + (chunk(b'PLTE', palette) if palette is not None else b'')
                + (chunk(b'tRNS', transparency) if transparency is not None else b'')
                + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))

    def test_png_modes_filters_and_failures(self):
        """四类 PNG 的五种行滤波、透明合成、CRC/位深/隔行错误及显式 fit。"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'logo.png'
            for color, channels in ((0, 1), (2, 3), (4, 2), (6, 4)):
                samples = bytes(v for y in range(5) for x in range(4)
                                for v in ((x * 63,) * (channels - (color in (4, 6)))
                                          + ((255,) if color in (4, 6) else ())))
                path.write_bytes(self.png(4, 5, color, samples, list(range(5))))
                image = png_logo.read(path)
                self.assertEqual(image.rgba, bytes(v for y in range(5) for x in range(4) for v in (x * 63,) * 3 + (255,)))
            path.write_bytes(self.png(1, 1, 6, bytes((255, 255, 255, 128))))
            self.assertEqual(png_logo.read(path, (0, 0, 0)).rgba, bytes((128, 128, 128, 255)))
            valid = self.png(1, 1, 0, b'\xff')
            for bad in (valid[:-1], valid.replace(b'IDAT', b'IDXT'),
                        self.png(1, 1, 0, b'\xff', depth=16), self.png(1, 1, 0, b'\xff', interlace=1)):
                path.write_bytes(bad)
                with self.assertRaises(ValueError):
                    png_logo.read(path)
            path.write_bytes(self.png(300, 10, 0, bytes(3000)))
            with self.assertRaises(ValueError):
                png_logo.read(path)
            self.assertEqual(png_logo.read(path, fit=True).w, 294)

    def test_png_palette_bit_depths_and_transparency(self):
        """索引色各位深、奇数行填充、五种滤波和缺省不透明的 tRNS 尾项。"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'indexed.png'
            for depth in (1, 2, 4, 8):
                count = 2 if depth == 1 else 4
                palette = bytes((255, 255, 255, 255, 0, 0, 0, 0, 255, 0, 255, 0))[:count * 3]
                transparency = bytes((0, 128))
                packed, expected = bytearray(), bytearray()
                for y in range(5):
                    row = bytearray((5 * depth + 7) // 8)
                    for x in range(5):
                        index = (x + y) % count
                        row[x * depth // 8] |= index << (8 - depth - x * depth % 8)
                        rgb = palette[index * 3:index * 3 + 3]
                        alpha = transparency[index] if index < len(transparency) else 255
                        expected.extend((*((v * alpha + 127) // 255 for v in rgb), 255))
                    packed.extend(row)
                path.write_bytes(self.png(5, 5, 3, packed, list(range(5)), depth=depth,
                                          palette=palette, transparency=transparency))
                self.assertEqual(png_logo.read(path, (0, 0, 0)).rgba, bytes(expected))
            for data in (self.png(1, 1, 3, b'\0'),
                         self.png(1, 1, 3, b'\x02', palette=b'\xff\0\0'),
                         self.png(1, 1, 3, b'\0', palette=b'\xff\0\0', transparency=b'\xff\xff')):
                path.write_bytes(data)
                with self.assertRaises(ValueError):
                    png_logo.read(path)

    def test_png_16_bit_modes_and_color_key(self):
        """四类 16 位采样在滤波还原后取高字节；透明色键按原 16 位值比较。"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'sixteen.png'
            for color, values in ((0, (0x1234,)), (2, (0x1234, 0x5678, 0x9abc)),
                                  (4, (0xabcd, 0x8000)), (6, (0x1234, 0x5678, 0x9abc, 0x8000))):
                samples = struct.pack('>' + 'H' * len(values), *values) * 15
                path.write_bytes(self.png(3, 5, color, samples, list(range(5)), depth=16))
                rgb = [v >> 8 for v in values[:3]] if color in (2, 6) else [values[0] >> 8] * 3
                alpha = values[-1] >> 8 if color in (4, 6) else 255
                expected = bytes((*((v * alpha + 127) // 255 for v in rgb), 255)) * 15
                self.assertEqual(png_logo.read(path, (0, 0, 0)).rgba, expected)
            path.write_bytes(self.png(2, 1, 0, struct.pack('>HH', 0x1234, 0x12ff), depth=16,
                                      transparency=struct.pack('>H', 0x1234)))
            self.assertEqual(png_logo.read(path, (0, 0, 0)).rgba, bytes((0, 0, 0, 255, 18, 18, 18, 255)))

    def test_png_noisy_logo_error_override(self):
        """随机噪点单色 Logo 默认拒绝；显式门槛成功且输出实际最大/平均误差。"""
        rng = random.Random(1234)
        pixels = bytearray()
        for y in range(10):
            for x in range(16):
                color = (220, 30, 20) if x in (4, 11) and 3 <= y <= 6 else (255, 255, 255)
                pixels.extend((*[min(255, max(0, v + rng.randint(-40, 40))) for v in color], 255))
        with tempfile.TemporaryDirectory() as directory:
            source, target = Path(directory) / 'noisy.png', Path(directory) / 'result.c'
            source.write_bytes(self.png(16, 10, 6, pixels))
            target.write_text('unchanged')
            args = ['split_logo_a8.py', str(source), str(target), '--background', 'ffffff', '--foreground', 'dc1e14']
            with patch('sys.argv', args), contextlib.redirect_stdout(io.StringIO()), \
                    contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
                splitter.main()
            self.assertEqual(target.read_text(), 'unchanged')
            output = io.StringIO()
            with patch('sys.argv', args + ['--max-error', '80']), contextlib.redirect_stdout(output):
                self.assertEqual(splitter.main(), 0)
            self.assertIn('最大', output.getvalue())
            self.assertIn('平均', output.getvalue())
            self.assertIn('继续近似单色', output.getvalue())
            self.assertIn('logo_parts_t', target.read_text())
            for bad in ('-1', '256'):
                with patch('sys.argv', args + ['--max-error', bad]), contextlib.redirect_stderr(io.StringIO()), \
                        self.assertRaises(SystemExit):
                    splitter.main()

    def test_png_to_parts_cli(self):
        """PNG 直接转换、显式颜色/分组与 layout，拒绝多色及错误组号。"""
        with tempfile.TemporaryDirectory() as directory:
            source, target = Path(directory) / 'designer.png', Path(directory) / 'result.c'
            pixels = bytes(v for y in range(8) for x in range(12)
                           for v in ((255 if x in (3, 8) and 2 <= y <= 5 else 0,) * 3 + (255,)))
            source.write_bytes(self.png(12, 8, 6, pixels))
            args = ['split_logo_a8.py', str(source), str(target), '--groups', '0,1',
                    '--symbol', 'img_src_NewLogo', '--background', '000000', '--foreground', 'ffffff']
            with patch('sys.argv', args), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(splitter.main(), 0)
            self.assertIn('img_src_NewLogo_parts', target.read_text())
            self.assertEqual(splitter.layout_from(target.read_text(), 'img_src_NewLogo'), (0, 0xffffff, 1, 0))
            self.assertEqual(len(converter.parse_v9_text(target.read_text())), 2)
            before = target.read_bytes()
            with patch('sys.argv', args + ['--timings', '7:0:250']), contextlib.redirect_stderr(io.StringIO()), \
                    contextlib.redirect_stdout(io.StringIO()), self.assertRaises(SystemExit):
                splitter.main()
            self.assertEqual(target.read_bytes(), before)

    def test_deployed_v9_check(self):
        """Check deployed v9 resources in place. 正式资源支持同目录只读校验，拒绝错误头字段。"""
        root = converter.HERE.parents[3] / "build"
        root.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="deployed-v9-test-", dir=root) as directory:
            base = Path(directory)
            for fmt in ("A8", "RGB565", "RGB565A8"):
                rgba = bytes((255, 0, 0, 255 if fmt == "RGB565" else 128)) * 2
                image = converter.encode_v9("img_src_t", 2, 1, rgba, fmt)
                path = base / "img_src_t.c"
                path.write_bytes(converter.render_c_file([image]).replace("\n", "\r\n").encode())
                original = path.read_bytes()
                with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(converter.run(base, base, {}, True), 0)
                    self.assertEqual(path.read_bytes(), original)
                    path.write_bytes(original.replace(b".magic = LV_IMAGE_HEADER_MAGIC", b".magic = 0"))
                    self.assertEqual(converter.run(base, base, {}, True), 1)
                    self.assertEqual(path.read_bytes(), original.replace(b".magic = LV_IMAGE_HEADER_MAGIC",
                                                                         b".magic = 0"))


if __name__ == "__main__":
    unittest.main()
