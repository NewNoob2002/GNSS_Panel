"""在 SDL dummy 模式验证启动及 SIGTERM 触发的页面清理。"""
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def main() -> None:
    executable = Path(sys.argv[1]).resolve(strict=True)
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software")
    with tempfile.TemporaryFile(mode="w+", encoding="utf-8") as log:
        process = subprocess.Popen([str(executable)], env=environment, stdout=log, stderr=subprocess.STDOUT)
        try:
            time.sleep(2)
            if process.poll() is not None:
                raise RuntimeError("模拟器在启动观察期内提前退出")
            process.terminate()
            status = process.wait(timeout=5)
            if status != 0:
                raise RuntimeError(f"模拟器未正常清理退出：{status}")
            log.seek(0)
            output = log.read()
            if "Pages/Startup" not in output or "Stack clear done" not in output:
                raise RuntimeError("未观察到启动页面和页面栈清理")
        except BaseException:
            log.seek(0)
            print(log.read(), file=sys.stderr)
            raise
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
    print("模拟器启动与页面清理通过")


if __name__ == "__main__":
    main()
