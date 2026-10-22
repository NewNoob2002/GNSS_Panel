#!/usr/bin/env python3
"""图片迁移回归测试（标准库）：python3 -B App/Resource/tools/test_convert_images_v9.py。"""
import contextlib
import io
from pathlib import Path
import tempfile
import unittest

import convert_images_v9 as converter


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
        root = converter.HERE.parents[2] / "build"
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
        root = converter.HERE.parents[2] / "build"
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


    def test_deployed_v9_check(self):
        """Check deployed v9 resources in place. 正式资源支持同目录只读校验，拒绝错误头字段。"""
        root = converter.HERE.parents[2] / "build"
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
