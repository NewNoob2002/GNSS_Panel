#!/usr/bin/env bash
# 把 ATK_RK3506 仓库 application/App 的指定提交原样同步到 Shared/App（共享应用的唯一源码）。
# 用法：scripts/sync_shared_app.sh <ATK_RK3506 仓库路径> [提交，默认 HEAD]
# 只保留 HC32 自有文件（KEEP 列表）；其余文件以上游为准，HC32 不得直接修改，改动应先提交到上游。
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "usage: $0 <ATK_RK3506 repo> [commit]" >&2
    exit 2
fi
upstream=$1
commit=${2:-HEAD}
root=$(cd "$(dirname "$0")/.." && pwd)
dest="$root/Shared/App"
# HC32 自有：来源提交与禁止就地格式化的配置；CMake 与源码都复用上游。
KEEP=(UPSTREAM .clang-format)

sha=$(git -C "$upstream" rev-parse --verify "$commit^{commit}")
if [[ -n "$(git -C "$upstream" status --porcelain -- application/App)" ]]; then
    echo "note: $upstream has uncommitted changes under application/App; syncing committed $sha only" >&2
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
git -C "$upstream" archive "$sha" application/App | tar -x -C "$work"
src="$work/application/App"
for keep in "${KEEP[@]}"; do
    rm -f "$src/$keep"
    if [[ -e "$dest/$keep" ]]; then
        cp -p "$dest/$keep" "$src/$keep"
    fi
done

rm -rf "$dest"
mv "$src" "$dest"
{
    echo "ATK_RK3506 application/App"
    echo "commit $sha"
    echo "subject $(git -C "$upstream" log -1 --format=%s "$sha")"
} > "$dest/UPSTREAM"
echo "Shared/App synced from $sha"
