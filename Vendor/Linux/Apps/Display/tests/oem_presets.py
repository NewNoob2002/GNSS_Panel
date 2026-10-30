#!/usr/bin/env python3
"""用四个临时 OEM 运行全部 Linux 测试；复用一个构建目录，只编译一次 LVGL。

uv run --with pillow python3 -B Vendor/Linux/Apps/Display/tests/oem_presets.py \
    --build-dir build/oem-presets --log-dir /tmp/oem-presets
不加入默认 CTest（防止递归）；临时客户退出时删除，配置和生成物保留在日志目录。
"""
from __future__ import annotations

import argparse
import importlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(REPO / 'App/UI/Resource/tools'))
oem_build = importlib.import_module('oem_build')


def main() -> int:
    """真实选择每个预设，不使用运行期特殊分支或放宽布局断言。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=REPO / 'build/oem-presets')
    parser.add_argument('--log-dir', type=Path, default=REPO / 'build/oem-presets/logs')
    args = parser.parse_args()
    build, logs = args.build_dir.resolve(), args.log_dir.resolve()
    logs.mkdir(parents=True, exist_ok=True)
    results = []

    def run(name: str, command: list[str]) -> None:
        with (logs / (name + '.log')).open('w') as output:
            result = subprocess.run(command, cwd=REPO, stdout=output, stderr=subprocess.STDOUT)
        results.append({'name': name, 'argv': command, 'code': result.returncode})
        (logs / 'results.json').write_text(json.dumps(results, ensure_ascii=False, indent=2))
        if result.returncode:
            raise RuntimeError(f'{name} 失败，见 {logs / (name + ".log")}')
        print(f'{name}: PASS', flush=True)

    for preset in ('cascade', 'soft', 'sweep', 'wipe'):
        with tempfile.TemporaryDirectory(prefix='test-preset-', dir=oem_build.RESOURCE / 'oem') as directory:
            customer = Path(directory)
            config = ('name = "Preset regression"\n'
                      '[colors]\nbackground = "101820"\nlogo = "f2aa4c"\ntext = "e8e8e8"\n'
                      '[logo]\ntext = "ORBIT GEO"\nfont = "../../font/Oswald-Bold.ttf"\n'
                      'size = 44\ngroups = "ORBIT|GEO"\n'
                      '[device_name]\nen = "ORBIT GEO"\nru = "ОРБИТ GEO"\n'
                      'footer_en = "ORBIT GNSS"\nfooter_ru = "Приёмник ORBIT"\n'
                      f'[effect]\npreset = "{preset}"\n')
            (customer / 'oem.toml').write_text(config)
            oem_build.build(customer)
            shutil.copytree(customer, logs / preset, dirs_exist_ok=True)
            run(preset + '-configure', ['cmake', '-S', str(REPO), '-B', str(build), '-G', 'Ninja',
                                       '-DPLATFORM=Linux', '-DCMAKE_BUILD_TYPE=Debug', '-DOEM=' + customer.name])
            run(preset + '-build', ['cmake', '--build', str(build), '-j', '6'])
            run(preset + '-ctest', ['ctest', '--test-dir', str(build), '--output-on-failure'])
    print('All Linux tests passed under cascade, soft, sweep and wipe', flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
