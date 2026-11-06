#!/system/bin/sh
# 在 Android/clang 上构建并测试 NSASM
#   用法: bash build-android.sh [--test]
#
# 两个必须绕开的坑：
#   1) 源码目录在 sdcard(FUSE) 上，直接在那里编译实测要 8 分钟；先同步到 $HOME
#      再编译，只要 14 秒。
#   2) 产物不能放 sdcard（noexec 挂载，执行会 Permission denied），所以输出到
#      $HOME/build/nsasm。
set -eu

SRC=$(cd "$(dirname "$0")" && pwd)
WORK="$HOME/build/nsasm-src"
OUT="${OUT:-$HOME/build/nsasm}"
: "${PREFIX:?PREFIX 未设置}"

mkdir -p "$WORK/ng" "$WORK/tests" "$OUT"

# 只同步构建需要的文件（跳过 .git 和历史遗留文件）
for f in Launcher.cpp Util.cpp Util.h NSASM.h funcList.cpp paramList.cpp; do
	cp "$SRC/$f" "$WORK/"
done
cp "$SRC"/ng/*.h "$SRC"/ng/*.cpp "$WORK/ng/"
cp "$SRC"/tests/*.ns "$WORK/tests/" 2>/dev/null || true
cp "$SRC"/tests/run.sh "$WORK/tests/"

CXXFLAGS="-std=c++17 -O2 -pthread -Wall -I$PREFIX/extensions/clang/include/c++/v1"

echo "compiling (from $WORK) ..."
clang++ $CXXFLAGS -I"$WORK" \
	"$WORK/Launcher.cpp" \
	"$WORK/ng/Front.cpp" "$WORK/ng/Interp.cpp" "$WORK/ng/Calc.cpp" \
	"$WORK/Util.cpp" "$WORK/funcList.cpp" "$WORK/paramList.cpp" \
	-o "$OUT/nsasm"

echo "built: $OUT/nsasm"

if [ "${1:-}" = "--test" ]; then
	echo
	sh "$SRC/tests/run.sh" "$OUT/nsasm"
fi
