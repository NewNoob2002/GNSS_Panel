#!/usr/bin/env python3
"""源码父目录包含规则回归：python3 -B tests/test_check_layers.py。"""
import tempfile
import unittest
from pathlib import Path

import check_layers


class ParentIncludeTests(unittest.TestCase):
    """检查扫描范围、精确例外和新增违规的文件行号。"""

    def test_owned_sources_and_excluded_directories(self):
        """自有代码报错，第三方与构建目录不参与扫描。"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("App/test.cpp", "Frameworks/test.hpp", "Vendor/Linux/test.c",
                         "External/test.c", "build/test.c", "Vendor/HC32/Libraries/a/test.c",
                         "Vendor/HC32/Platform/DDL/test.c", "Vendor/HC32/Platform/CMSIS/test.c",
                         "App/build/test.c"):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('\n#include "../bad.h"\n')
            errors = check_layers.check_parent_includes(directory)
            self.assertEqual(len(errors), 3)
            self.assertTrue(all(':2: 违规 (规则 2)' in error for error in errors))

    def test_handwritten_parent_includes_have_no_exceptions(self):
        """手写源码没有父目录路径豁免，即使位于曾经包含旧路径的文件。"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for filename, include in (
                    ("App/UI/StatusBar/StatusBar.h", "../../Utils/lv_anim_label/numberFlow.h"),
                    ("Vendor/Linux/tests/linux_button_test.c", "../platform/linux_button.h")):
                path = root / filename
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(f'#include "{include}"\n')
            errors = check_layers.check_parent_includes(directory)
            self.assertEqual(len(errors), 2)

    def test_generated_lvgl_fallback_is_limited_to_exact_branch(self):
        """仅生成图像的原有测试分支可回退，其他路径或分支均拒绝。"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / "App/UI/Resource/img/img_src_t.c"
            path.parent.mkdir(parents=True)
            fallback = '#elif defined(LV_BUILD_TEST)\n#include "../lvgl.h"\n'
            path.write_text(fallback)
            self.assertEqual(check_layers.check_parent_includes(directory), [])
            for invalid in ('#include "../lvgl.h"\n', fallback.replace('../lvgl.h', '../other.h')):
                path.write_text(invalid)
                self.assertEqual(len(check_layers.check_parent_includes(directory)), 1)
            path.write_text('#include "UI/Resource/LogoLayout.h"\n')
            self.assertEqual(check_layers.check_parent_includes(directory), [])


class UiHalIncludeTests(unittest.TestCase):
    """UI 只能直接依赖 DataProc，平台与领域层仍可包含 HAL。"""

    def test_all_hal_include_spellings_and_static_entry(self):
        """完整、缩短、相对路径及尖括号都拒绝，报错保留行号。"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / "App/Service/HAL/Def/HAL_Init.h"
            header.parent.mkdir(parents=True)
            header.write_text("#pragma once\n")
            manager = root / "App/Service/HAL/HAL_Manager.h"
            manager.write_text("#pragma once\n")
            path = root / "App/UI/SystemLoading/Model.cpp"
            path.parent.mkdir(parents=True)
            for include in ("App/Service/HAL/Def/HAL_Init.h", "Service/HAL/Def/HAL_Init.h",
                            "HAL/Def/HAL_Init.h", "Def/HAL_Init.h", "HAL_Init.h", "HAL_Manager.h",
                            "../../Service/HAL/Def/HAL_Init.h", "HAL/Def/Future.h"):
                for opener, closer in (('"', '"'), ('<', '>')):
                    path.write_text(f'\n# include {opener}{include}{closer}\n')
                    errors = check_layers.check_ui_hal_includes(directory)
                    self.assertEqual(len(errors), 1, include)
                    self.assertIn("App/UI/SystemLoading/Model.cpp:2:", errors[0])
                    self.assertIn("UI → DataProc → HAL", errors[0])
                    self.assertIn(errors[0], check_layers.check_static_rules(directory))

    def test_domains_platforms_and_non_source_files_are_allowed(self):
        """扫描限于 UI 源文件，DataProc 头与普通 UI 资源均允许。"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("App/Service/HAL/HAL_Manager.h", "App/Service/DataProc/Node.h",
                         "Vendor/HC32/Device.cpp", "App/UI/notes.md", "App/UI/build/generated.cpp"):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('#include "HAL/HAL_Manager.h"\n')
            path = root / "App/UI/Model.hpp"
            path.write_text('#include "Def/DP_Init.h"\n#include "UI/Resource/ResourcePool.h"\n')
            self.assertEqual(check_layers.check_ui_hal_includes(directory), [])


if __name__ == "__main__":
    unittest.main()
