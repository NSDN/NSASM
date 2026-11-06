# NSASM 0.70-ast：前端重写与语义修正

这份文档记录「为什么改」「改成什么」「哪些行为变了」。改动的目标是让这套东西
**从「靠字符串约定跑起来」变成「有一层能检查的前端」**，而不是重写一个语言。

## 1. 旧链路的问题

原来的执行路径是：整份源码 → 字符串模板替换（占位符/格式化/括号修复）→ 按行切段 →
运行时把 `"add r0, 1"` 这种字符串 `substr` 成 `op / dst / src / ext` → 再解析每个操作数
（每次都 `new Register`）→ 查 `funcList`。跳转则是**遍历全部代码文本**找 `[tag]` 那一行。

具体后果：

| 症状 | 原因 |
|---|---|
| 报错只有「At seg, line N」，没有列和原因 | 前端不分层，错误信息无法保留位置 |
| 每条指令都要切字符串、`replace`、分配临时值 | 运行期反复解析同一行 |
| `jmp` 的复杂度是 O(全部代码行数) | 标号靠文本扫描定位 |
| 宏只能展开一层，参数会误替换字符串里的同名文字 | 宏在文本层用 `replace` 做 |
| 单行 `(mov r0, 1)` 要靠 `encodeLambda` 把空格换成特殊字符才不被切段 | 没有词法，靠「不能有空格」的约定 |
| 段执行顺序是**段名字典序**，和 Java/C# 的文件序不一致 | `std::map<string,string>` 的遍历顺序，文档还写成了「随机」 |
| `.<conf> heap N` 只被解析，从未生效 | `heapSize` 存了但没人用 |
| 16 线程跑 test.ns 直接 `std::terminate` | `tid` 把 pthread 句柄当 int 用 `stoi` |
| 映射键会静默合并 | 键比较把一切折成 `float`（`int 16777216` 与 `16777217` 相等；字符串键用 `(float)hash`） |

## 2. 新结构

```
ng/Front.h  ng/Front.cpp    词法 Token → AST → 语法分析 + 宏展开
ng/Interp.cpp               NSASM 执行器：操作数解析、控制流、eval/par/ld
ng/Calc.cpp                 数值运算与「文本→立即数」解析（指令表依赖）
funcList.cpp                值操作指令表（保留，删除控制流部分）
paramList.cpp               虚拟寄存器（保留）
Util.cpp / Util.h           打印、读文件、CLI（删掉了整块字符串前端）
```

关键点：**`funcList` 那 1000 行值操作逻辑没有重写**。执行器的约定没变 —— 仍然把
`(dst, src, ext)` 三个 `Register*` 交给指令表，仍然支持虚拟寄存器的「读一次、写回一次」
协议。所以改动集中在「谁来决定这三个指针是什么」。

- 操作数在**装载期**解析成 AST 节点，运行期只做求值（`resolve`）。
- 跳转改为标号表 `labelAt[tag] = (段, 语句)`，O(log n)。
- 段顺序改为**文件顺序**（公共段永远第一个），与 Java/C# 版一致。
- 代码字面量 `(...)` 的文本由 Token 重新排版生成（`tokensToText`），
  不再需要 `encodeLambda` 那种「把空格换成不可见字符」的技巧。

## 3. 语言新增：四则运算

操作数现在可以是表达式，优先级与 C 一致：

```
add   r0, 1 + 2 * 3          ; 7
mov   r1, (1 + 2) * 3        ; 9
mov   r2, 100 / 7            ; 14（整数除法）
mov   r3, 100 % 7            ; 2
mov   r4, 1.5 * 2 + 0.25     ; 3.25（有 float 就按 float）
mov   r5, -(3 - 10)          ; 7
int   a = 7
int   b = 5
mov   r6, a * b - (a + b)    ; 23（可以引用变量和寄存器）
mov   r7, "ab" * 3           ; ababab（字符串重复，沿用原语法）
mov   r8, "x" + "y" + 'z'    ; xyz（字符串连接）
```

类型规则：`char` 提升为 `int`；任一侧是 `float` 就按 `float` 算；整数除法/取模按整数。
除数为 0、浮点取模、字符串参与 `-` 等都会报错而不是崩溃。

**`(...)` 到底是算术括号还是代码字面量**，判定规则（写死在这里，避免以后猜）：

1. 括号内出现换行 → 代码字面量（多行 `code c = (...)`）
2. 括号内第一个符号是「标识符 + 操作数」的样子（如 `mov r0`）→ 代码字面量
3. 括号内深度 0 出现 `+ - * / %` → 算术分组
4. 其余（如 `(nop)`、`( prt "x" )`、`()`）→ 代码字面量

所以想要「纯括号分组」时里面必须有运算符；`(a)` 这种请直接写 `a`。

## 4. 有意的语义变化

| 项目 | 旧行为 | 新行为 | 兼容性影响 |
|---|---|---|---|
| 段执行顺序 | 段名字典序 | 文件顺序（公共段优先） | 与 Java/C# 一致；依赖字典序的脚本会变 |
| 映射键比较 | 一律折成 `float` | int/char/float 按数值，字符串按内容精确比较 | 大整数键不再合并（修复丢数据） |
| 映射迭代顺序 | 按 `(float)hash` | 按数值 / 字符串字典序 | 字符串键的 `M(...)` 打印顺序变了（但稳定可预期） |
| 变量名 | 首字母是 `r` 就不能用（`result` 会被当成寄存器） | 只排除 `r0` 这种纯寄存器名 | 原来写不了的变量名现在可以 |
| `.<conf> heap N` | 解析但不生效 | 变量数超过上限报错 | 新约束 |
| 整数除零 | `SIGFPE` 崩溃 | 报错「除数不能为 0」 | 新错误 |
| 未知指令 | 报错但只说「运行错误」 | 报错并写明指令名、行、列 | 诊断更强 |
| 错误输出 | 无原因 | `At <段>, line N:` + 源码回显 + 原因 | — |
| CLI 退出码 | 恒为 0 | 运行错误 1，语法/读取错误 2 | 便于脚本和 CI |
| `nsasm c in out` | 生成二进制 `.nsb` | 只做语法检查（字节码格式移除） | 功能缩减，见 §7 |
| `nsasm FILE SEG` | 硬编码 `64/32/32` | 用文件里的 `.<conf>` | — |
| `par` 实现 | `SafePool` + 忙等 `nop` + 一把大锁 | 每线程一个独立实例，`join` 后合并结果 | 少了一层锁和忙等；`SafePool::operator[]` 持锁早退的死锁隐患随之消失 |
| `Register` 拷贝 | 拷贝会把 `gcFlag` 一起带过来 | 拷贝不继承所有权 | 避免「复制品被当成需要 delete」 |
| `eval` 递归 | 无限制 | 64 层上限 | 递归死循环变成可诊断错误 |

补充：**映射型变量不能作为键**。`operator<` 对所有 map 都返回「相等」，所以
`put` 一个 map 键会静默覆盖前一个 —— 这与手册里「键不能是函数型变量」的说明一致，
但仍是个限制，见 §7。

## 5. 兼容性验收

`tests/mat.ns`、`tests/demo.ns` 就是仓库自带的 `mat.ns`、`test.ns`。用旧引擎
（`nsasm-legacy`，含 `stoi` 补丁）和新引擎各跑一遍，去掉版本号/计时/线程 id 后
**输出逐字节一致**：

```sh
# 旧
$HOME/build/nsasm/nsasm-legacy r test.ns < /dev/null > old.out
# 新
$HOME/build/nsasm/nsasm r test.ns < /dev/null > new.out
diff <(sed 's/took .*//' old.out) <(sed 's/took .*//' new.out)
```

宏展开、`code`/`map` 字面量、`eval`、`call`、`par` 16 线程、`in` 读输入，全部保持原样。

## 6. 性能

同一台设备（aarch64，clang 21，`-O2`）：

| 用例 | 旧 | 新 |
|---|---|---|
| `mat.ns`（512 行输出 + 循环） | 11.0 ms | 5.35 ms |
| `test.ns`（宏 + map + 16 线程） | 9.4 ms | 6.4 ms |

另外一条与语言无关但很影响手感的发现：**源码在 sdcard(FUSE) 上直接编译要 8 分钟，
先拷到 `$HOME` 再编译只要 14 秒**。`build-android.sh` 就是干这个的。

## 7. 还没做（诚实清单）

- **字节码**：旧的 `compile`/`binary` 序列化的是「预处理后的源码」，与 AST 前端是两套
  表示。现在 `c` 模式只做语法检查；真正的字节码应该序列化 AST（或编译到闭包表）。
- **宏的卫生性**：参数按 Token 替换（不会误替换字符串里的同名文字了），但参数名仍会
  捕获目标代码里的同名标识符；没有 `gensym`。
- **多行 REPL**：`execLine` 每次只解析一行，`{}`/`()` 跨行输入不支持。
- **值语义的拷贝成本**：`Register` 仍然「一个值带一个 `std::map`」，大映射传参会整份拷贝。
  下一步应该换成 `shared_ptr` + 写时复制。
- **错误恢复**：仍是一遇错就中止整个脚本，没有 try/catch 之类的语言级手段。
- **`prt dst, src` 的行编辑语义**仍在 `funcList` 里用字符串拼接实现，没有走 AST。
- **未覆盖的角落**：`out 0x00, x` 的显式地址形式、`type` 作用在虚拟寄存器上、
  浮点特殊值（NaN/Inf）打印、`par` 与 `ld` 同时使用。

## 8. 怎么构建和测试

```sh
# Android（推荐）：先把源码同步到 $HOME（sdcard 上直接编译慢一个数量级），
# 再编译并跑一致性测试
bash build-android.sh --test

# 通用 CMake（toolchain-dsh.cmake 会处理好 libc++ 头路径和 make 的 recipe shell）
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=toolchain-dsh.cmake -DCMAKE_SYSTEM_VERSION=24
cmake --build build && ctest --test-dir build
```

`-DCMAKE_SYSTEM_VERSION=24` 不能省：CMake 在 Android 宿主上会读
`$PREFIX/include/android/api-level.h`，而该文件在 `extensions/clang/include/` 下，
读失败会直接中止 configure。

测试用例约定见 `tests/run.sh`：`tests/*.ns` 与 `tests/expected/*.out` 逐字比对，
`tests/errors/*.ns` 要求非 0 退出码且输出含 `*.expect` 里的关键词。
需要键盘输入的用例放 `tests/stdin/<name>.in`；`mat.ns`/`test.ns` 作为兼容性回归
放在 `tests/` 里（线程 id 已归一化）。

## 9. 测试：48 个用例，以及它们挖出来的 6 个 bug

用例分两类：**26 个正常用例**逐字比对输出，**22 个错误用例**要求非 0 退出码
且输出含指定关键词（`TIMEOUT` 环境变量给每个用例设上限，防止死循环卡死测试）。

覆盖范围：

| 用例 | 覆盖内容 |
|---|---|
| `datatypes` | 五种值类型、`type`、各类型打印 |
| `strings` | `len`/`ctn`/`equ`/`cat`/`dog`/游标 `add u, n`/字符与字符串互转 |
| `chars` | 字符算术、`inc`/`dec`、字符比较、表达式里 char→int |
| `control` | `cmp`+`jz/jnz/jg/jl`、`loop` 递增与递减、嵌套循环 |
| `calls` | `call` 返回、两层嵌套调用 |
| `runseg` | `run` 不返回（目标段结束后按文件顺序继续） |
| `rst` | `rst` 回到第一条指令且寄存器保持 |
| `codeappend` | 代码段作可变字符串：追加字符串/代码段、`'\b'` 删最后一行 |
| `codeval` | 代码段作源操作数自动执行、`eval` 的两种形式 |
| `prtcode` | `prt`/`out` 单操作数为代码段 |
| `stack` | `push`/`pop`、表达式操作数 |
| `ops` | 位运算、移位、`not`、`test`、三操作数 `add`、`out 0xFF` 调试口、三操作数打印 |
| `maps`/`mapcat`/`mapkeys` | 字面量、`put`/`get`/`len`/`ctn`、循环构造、嵌套映射、`cat`/`dog` 合并删除、大整数键 |
| `exprs2`/`arith` | 优先级组合、一元负号、int/float 分界、字符串 `+`/`*`、指令第三操作数里的表达式 |
| `order` | 段执行顺序 = 文件顺序 |
| `ld-override` | `ld` 加载外部文件 + `@<seg>` 覆盖 |
| `stdin`/`in-eof` | `in` 按目标类型解析输入、类型不符时寄存器不变 |
| `macros` | 嵌套宏展开、参数不误替换字符串内容 |
| `par3` | `par` 3 线程，结果映射稳定 |
| `mat`/`demo` | 仓库自带脚本的兼容性回归 |

### 测试挖出来的 bug

| 用例 | 问题 | 谁的责任 |
|---|---|---|
| `prtcode` | `prt <代码段>` 永远失败：函数里写成 `eval(src)`，而该分支 `src` 必为 `null` | 原版 |
| `ops` | `not` 指令**必然 segfault**：`calc(dst, nullptr, '~')` 一进函数就解引用 `src->type` | 原版 |
| `badcodeop` | 任意 `add/sub/.../cmp` 的代码段操作数求值失败后，`calc(dst, nullptr, ...)` 空指针崩溃（12 处） | 原版 |
| `control` | `loop` 上限 ≤0 时应递减，我重写时漏了方向判断 → 死循环 | 本次重写 |
| `override` | `@<不存在的段>` 的装载错误被构造函数吞掉，静默 exit 0 | 本次重写 |
| `mapkey` | 映射当键会静默合并（`operator<` 认为所有 map 相等）→ 改为显式报错 | 原版设计 |

前四个里有两个是「手册里写着能用、实际一用就崩」的指令（`not`、`prt <code>`），
说明原来完全没有针对指令表的测试。

### 对照实验：这套用例能区分两个引擎

拿同一套用例跑**旧引擎**（`nsasm-legacy`）：

```
新引擎：通过 48，失败 0
旧引擎：通过 19，失败 29      ← 13 个正常用例 + 16 个错误用例
```

旧引擎失败的正是预期中的那些：没有表达式（`arith`/`exprs2`/`codeval`）、
段顺序是字典序（`order`）、映射键被 float 截断（`mapkeys`）、宏不能嵌套（`macros`）、
退出码恒为 0（全部 16 个错误用例）、以及 `not`/`prt <code>` 的崩溃。

复现：

```sh
sh tests/run.sh                              # 新引擎
sh tests/run.sh ~/build/nsasm/nsasm-legacy   # 旧引擎
```

## 10. Rust 移植

Rust 实现已拆分到独立仓库 `nsasm-rs`，移植记录（分层映射、扩展性硬约束、
与 C++ 的差异、性能、构建注意）见该仓库的 `DESIGN.md`。
