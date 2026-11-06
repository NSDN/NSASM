#!/system/bin/sh
# NSASM 一致性测试
#   用法: sh tests/run.sh [二进制路径]
#   默认用 $NSASM_BIN，否则 $HOME/build/nsasm/nsasm-ast
#
# 约定：
#   tests/*.ns            正常用例，输出（去掉头尾装饰后）与 expected/*.out 逐字比对
#   tests/errors/*.ns     错误用例，要求退出码非 0，且输出包含 *.expect 里的关键词
set -u

HERE=$(cd "$(dirname "$0")" && pwd)
BIN="${1:-${NSASM_BIN:-$HOME/build/nsasm/nsasm}}"

if [ ! -x "$BIN" ]; then
	echo "找不到可执行文件: $BIN"
	echo "先构建：bash build-android.sh   （或在仓库根目录用 cmake）"
	exit 2
fi

# 统一用绝对路径：ld 指令按相对路径读文件，后面会 cd 进 tests/
case "$BIN" in
/*) ;;
*) BIN=$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN") ;;
esac
cd "$HERE" || exit 2

# 去掉版本横幅、结束语、计时行和空行；线程 id 归一化
decorate() {
	grep -vE '^(NyaSama|Version:|NSASM running finished|NSASM aborted|This script took|$)' \
		| sed -E 's/Thread-[0-9-]+/Thread-N/g'
}

pass=0
fail=0
failed_names=""

for f in "$HERE"/*.ns; do
	[ -f "$f" ] || continue
	name=$(basename "$f" .ns)
	exp="$HERE/expected/$name.out"
	[ -f "$exp" ] || continue

	# 需要键盘输入的用例放在 tests/stdin/<name>.in
	in_file="$HERE/stdin/$name.in"
	[ -f "$in_file" ] || in_file=/dev/null

	# 加超时：脚本里的死循环（比如修好之前的 loop）不应该把测试卡死
	got=$(timeout "${TIMEOUT:-20}" "$BIN" r "$f" < "$in_file" 2>&1 | decorate)
	want=$(cat "$exp")
	if [ "$got" = "$want" ]; then
		pass=$((pass + 1))
		echo "  PASS  $name"
	else
		fail=$((fail + 1))
		failed_names="$failed_names $name"
		echo "  FAIL  $name"
		echo "-------- 期望 --------"
		echo "$want"
		echo "-------- 实际 --------"
		echo "$got"
		echo "----------------------"
	fi
done

for f in "$HERE"/errors/*.ns; do
	[ -f "$f" ] || continue
	name="errors/$(basename "$f" .ns)"
	kw_file="${f%.ns}.expect"
	[ -f "$kw_file" ] || continue
	kw=$(cat "$kw_file")

	out=$(timeout "${TIMEOUT:-20}" "$BIN" r "$f" < /dev/null 2>&1)
	code=$?
	if [ "$code" -eq 0 ]; then
		fail=$((fail + 1))
		failed_names="$failed_names $name"
		echo "  FAIL  $name (退出码为 0，应当报错)"
		continue
	fi
	case "$out" in
	*"$kw"*)
		pass=$((pass + 1))
		echo "  PASS  $name (exit $code)"
		;;
	*)
		fail=$((fail + 1))
		failed_names="$failed_names $name"
		echo "  FAIL  $name (输出里没有 '$kw')"
		echo "$out"
		;;
	esac
done

echo
echo "通过 $pass，失败 $fail"
[ "$fail" -eq 0 ] || echo "失败用例:$failed_names"
[ "$fail" -eq 0 ]
