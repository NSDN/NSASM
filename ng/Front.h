// NSASM 新一代前端：词法 / AST / 语法
//
// 取代原来「把整份源码当字符串做模板替换」的做法：
//   * 词法把源码切成 Token（带行号列号，报错终于能定位）
//   * 语法直接产出 AST，运行期不再 substr / parseArgs
//   * 宏展开在 Token 层做，支持嵌套（原来是文本层一次性替换）
//
// AST 节点用 shared_ptr 连接，整个 Program 因此可拷贝 —— par 指令要按线程
// 复制一份程序，而不是像原来那样每个线程重新解析文本。
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace NSASM {

enum class Tok : uint8_t {
	End, Newline, Ident, Int, Float, Char, Str,
	Comma, LParen, RParen, LBrace, RBrace, LBracket, RBracket,
	LAngle, RAngle, Assign, Plus, Minus, Star, Slash, Percent, Dot, At,
};

const char* tokName(Tok t);

struct Token {
	Tok kind = Tok::End;
	std::string text;   // Ident 名 / Str 内容 / Char 原文
	long long i = 0;
	double f = 0;
	char ch = 0;
	int line = 1, col = 1;
};

enum class ExprKind : uint8_t {
	Int, Float, Char, Str,     // 字面量
	Reg,                       // r0 这类寄存器
	Var,                       // 变量读
	Name,                      // 变量声明里的名字（不查表）
	Label,                     // [tag]
	Seg,                       // <seg>
	Code,                      // (...) 代码字面量，s 为源码文本
	Map,                       // M(...)
	Binary, Unary,
};

struct Expr;
using ExprP = std::shared_ptr<Expr>;

struct Expr {
	ExprKind kind = ExprKind::Int;
	long long i = 0;
	double f = 0;
	char ch = 0;
	std::string s;   // Var/Name/Reg 名，或 Code/Map 的源码文本
	int reg = -1;    // Reg 的编号
	char op = 0;     // Binary / Unary 的运算符
	ExprP l, r;
	int line = 1, col = 1;
};

enum class StmtKind : uint8_t { Instr, Label };

struct Stmt {
	StmtKind kind = StmtKind::Instr;
	std::string op;       // 指令名（已转小写）或带方括号的标号名
	ExprP a, b, c;        // 最多三个操作数
	std::string text;     // 原始源码行，报错时回显
	int line = 1, col = 1;
};

struct Segment {
	std::string name;
	bool pub = false;     // 公共段（段外代码）
	bool conf = false;    // .<conf>
	bool over = false;    // @<seg> 覆盖段
	std::vector<Stmt> body;
	int line = 1;
};

struct Conf {
	int heap = 64, stack = 32, regs = 16;
};

struct Program {
	std::vector<Segment> segs;
	Conf conf;
	bool empty() const { return segs.empty(); }
};

struct Diag {
	bool ok = true;
	std::string msg;
	int line = 0, col = 0;
};

// ---- 词法 ----
struct LexResult {
	std::vector<Token> toks;
	Diag diag;
};

LexResult lex(const std::string& src);

// ---- 语法 ----
struct ParseResult {
	Program prog;
	Diag diag;
};

// 解析整份源码。宏展开、段切分、指令/操作数/表达式解析都在这里完成。
ParseResult parse(const std::string& src);

// 只解析一个表达式（in 指令读入的文本、REPL 单行都用它）
ExprP parseExpression(const std::string& src, Diag& d);

// 把 Token 区间还原成可再次解析的源码文本（代码字面量、诊断用）
std::string tokensToText(const std::vector<Token>& toks, size_t begin, size_t end);

}  // namespace NSASM
