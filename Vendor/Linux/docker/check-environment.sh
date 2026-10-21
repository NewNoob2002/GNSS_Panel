#!/bin/bash
set -euo pipefail

sdk=${1:?Usage: bash check-environment.sh /path/to/sdk}
source /etc/os-release
test "$ID:$VERSION_ID" = ubuntu:20.04
test "$(id -u)" -ne 0
test "$(id -u)" = "$(stat -c %u "$sdk")"
bash "$sdk/device/rockchip/common/scripts/check-sdk.sh"

for tool in gcc g++ make cmake python python2 python3 dtc bison flex rsync cpio fakeroot; do
    command -v "$tool"
done
cross="$sdk/prebuilts/gcc/linux-x86/arm/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf"
test "$("$cross-gcc" -dumpmachine)" = arm-none-linux-gnueabihf
"$cross-gcc" --version | head -n 1
"$cross-gcc" -print-sysroot

scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
printf '#include <stdio.h>\nint main(void) { return puts("toolchain-ok") < 0; }\n' > "$scratch/check.c"
gcc -Wall -Wextra -Werror "$scratch/check.c" -o "$scratch/host"
"$scratch/host"
gcc -m32 "$scratch/check.c" -o "$scratch/host32"
"$scratch/host32"
"$cross-gcc" -Wall -Wextra -Werror "$scratch/check.c" -o "$scratch/arm"
"$cross-readelf" -h "$scratch/arm" | grep -E 'Class:.*ELF32|Machine:.*ARM'
"$cross-readelf" -A "$scratch/arm" | grep 'Tag_ABI_VFP_args:.*VFP registers'
file "$scratch/arm"
echo "PASS: host tools, SDK preflight and ARM hard-float compile/link (no full SDK build)."
