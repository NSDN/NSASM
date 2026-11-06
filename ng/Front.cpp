#include "Front.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace NSASM {

namespace {

bool isIdentStart(char c) {
	return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
bool isIdentChar(char c) {
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}
bool isRegName(const std::string& s) {
	if (s.size() < 2) return false;
	if (s[0] != 'r' && s[0] != 'R') return false;
	for (size_t i = 1; i < s.size(); i++)
		if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
	return true;
}
std::string lower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}
void escapeTo(std::string& out, char c, char quote) {
	switch (c) {
	case '\\': out += "\\\\"; break;
	case '\n': out += "\\n"; break;
	case '\r': out += "\\r"; break;
	case '\t': out += "\\t"; break;
	case '\b': out += "\\b"; break;
	case '\0': out += "\\0"; break;
	default:
		if (c == quote) { out += '\\'; out += c; }
		else out += c;
	}
}

}  // namespace

const char* tokName(Tok t) {
	switch (t) {
	case Tok::End: return "文件结束";
	case Tok::Newline: return "换行";
	case Tok::Ident: return "标识符";
	case Tok::Int: return "整数";
	case Tok::Float: return "浮点数";
	case Tok::Char: return "字符";
	case Tok::Str: return "字符串";
	case Tok::Comma: return ",";
	case Tok::LParen: return "(";
	case Tok::RParen: return ")";
	case Tok::LBrace: return "{";
	case Tok::RBrace: return "}";
	case Tok::LBracket: return "[";
	case Tok::RBracket: return "]";
	case Tok::LAngle: return "<";
	case Tok::RAngle: return ">";
	case Tok::Assign: return "=";
	case Tok::Plus: return "+";
	case Tok::Minus: return "-";
	case Tok::Star: return "*";
	case Tok::Slash: return "/";
	case Tok::Percent: return "%";
	case Tok::Dot: return ".";
	case Tok::At: return "@";
	}
	return "?";
}

// ---------------------------------------------------------------- 词法

LexResult lex(const std::string& src) {
	LexResult r;
	int line = 1, col = 1;

	auto fail = [&](const std::string& m) {
		r.diag.ok = false;
		r.diag.msg = m;
		r.diag.line = line;
		r.diag.col = col;
	};

	size_t i = 0;
	while (i < src.size()) {
		const char c = src[i];

		if (c == '\n') {
			Token t; t.kind = Tok::Newline; t.line = line; t.col = col;
			r.toks.push_back(t);
			i++; line++; col = 1;
			continue;
		}
		if (c == ' ' || c == '\t' || c == '\r') { i++; col++; continue; }

		// 注释：// 到行尾；/* ... */ 可跨行（不嵌套）
		// 注释按空白处理，但块注释内部的换行保留为 Newline token，
		// 避免把注释前后的两条语句粘在一起。
		if (c == '/' && i + 1 < src.size() && src[i + 1] == '/') {
			i += 2; col += 2;
			while (i < src.size() && src[i] != '\n') { i++; col++; }
			continue;
		}
		if (c == '/' && i + 1 < src.size() && src[i + 1] == '*') {
			const int cLine = line, cCol = col;
			i += 2; col += 2;
			bool closed = false;
			while (i < src.size()) {
				if (src[i] == '*' && i + 1 < src.size() && src[i + 1] == '/') {
					i += 2; col += 2; closed = true; break;
				}
				if (src[i] == '\n') {
					Token nl; nl.kind = Tok::Newline; nl.line = line; nl.col = col;
					r.toks.push_back(nl);
					i++; line++; col = 1;
				} else { i++; col++; }
			}
			if (!closed) {
				line = cLine; col = cCol;  // 让诊断指向注释开头
				fail("块注释没有结束的 */");
				return r;
			}
			continue;
		}

		const int tline = line, tcol = col;
		auto push = [&](Tok k, const std::string& text) {
			Token t; t.kind = k; t.text = text; t.line = tline; t.col = tcol;
			r.toks.push_back(t);
		};

		// 标识符 / 关键字
		if (isIdentStart(c)) {
			size_t b = i;
			while (i < src.size() && isIdentChar(src[i])) { i++; col++; }
			r.toks.push_back(Token{Tok::Ident, src.substr(b, i - b), 0, 0, 0, tline, tcol});
			continue;
		}

		// 数字：0x1F / 123 / 123h / 1.5 / 1.5F / 1F
		if (std::isdigit(static_cast<unsigned char>(c))) {
			size_t b = i;
			bool isHex = false, isFloat = false;
			if (c == '0' && i + 1 < src.size() && (src[i + 1] == 'x' || src[i + 1] == 'X')) {
				isHex = true;
				i += 2; col += 2;
				while (i < src.size() && std::isxdigit(static_cast<unsigned char>(src[i]))) { i++; col++; }
			} else {
				while (i < src.size() && std::isdigit(static_cast<unsigned char>(src[i]))) { i++; col++; }
				if (i < src.size() && src[i] == '.') {
					isFloat = true;
					i++; col++;
					while (i < src.size() && std::isdigit(static_cast<unsigned char>(src[i]))) { i++; col++; }
				}
				if (i < src.size() && (src[i] == 'f' || src[i] == 'F')) { isFloat = true; i++; col++; }
				else if (i < src.size() && (src[i] == 'h' || src[i] == 'H')) { isHex = true; i++; col++; }
			}
			std::string text = src.substr(b, i - b);
			Token t; t.line = tline; t.col = tcol;
			if (isFloat) {
				std::string num = text;
				if (!num.empty() && (num.back() == 'f' || num.back() == 'F')) num.pop_back();
				t.kind = Tok::Float;
				t.f = std::strtod(num.c_str(), nullptr);
			} else if (isHex) {
				std::string num = text;
				if (!num.empty() && (num.back() == 'h' || num.back() == 'H')) num.pop_back();
				t.kind = Tok::Int;
				t.i = static_cast<long long>(std::strtoll(num.c_str(), nullptr, 16));
			} else {
				t.kind = Tok::Int;
				t.i = std::strtoll(text.c_str(), nullptr, 10);
			}
			t.text = text;
			r.toks.push_back(t);
			continue;
		}

		// 字符字面量
		if (c == '\'') {
			i++; col++;
			char val = 0; bool got = false;
			while (i < src.size() && src[i] != '\'') {
				char ch = src[i];
				if (ch == '\\' && i + 1 < src.size()) {
					char e = src[i + 1];
					switch (e) {
					case '0': ch = '\0'; break;
					case 'b': ch = '\b'; break;
					case 'n': ch = '\n'; break;
					case 'r': ch = '\r'; break;
					case 't': ch = '\t'; break;
					case '\\': ch = '\\'; break;
					case '\'': ch = '\''; break;
					case '"': ch = '"'; break;
					default: ch = e; break;
					}
					i += 2; col += 2;
				} else { i++; col++; }
				if (!got) { val = ch; got = true; }
			}
			if (i >= src.size()) { fail("字符字面量没有结束的引号"); return r; }
			i++; col++;
			Token t; t.kind = Tok::Char; t.ch = val; t.line = tline; t.col = tcol;
			char buf[2] = {val, 0};
			t.text = buf;
			r.toks.push_back(t);
			continue;
		}

		// 字符串字面量
		if (c == '"') {
			i++; col++;
			std::string val;
			bool closed = false;
			while (i < src.size()) {
				char ch = src[i];
				if (ch == '"') { i++; col++; closed = true; break; }
				if (ch == '\\' && i + 1 < src.size()) {
					char e = src[i + 1];
					switch (e) {
					case '0': val += '\0'; break;
					case 'b': val += '\b'; break;
					case 'n': val += '\n'; break;
					case 'r': val += '\r'; break;
					case 't': val += '\t'; break;
					case '\\': val += '\\'; break;
					case '"': val += '"'; break;
					case '\'': val += '\''; break;
					default: val += e; break;
					}
					i += 2; col += 2;
					continue;
				}
				if (ch == '\n') { line++; col = 1; }
				else col++;
				val += ch;
				i++;
			}
			if (!closed) { fail("字符串字面量没有结束的引号"); return r; }
			Token t; t.kind = Tok::Str; t.text = val; t.line = tline; t.col = tcol;
			r.toks.push_back(t);
			continue;
		}

		Tok k;
		switch (c) {
		case ',': k = Tok::Comma; break;
		case '(': k = Tok::LParen; break;
		case ')': k = Tok::RParen; break;
		case '{': k = Tok::LBrace; break;
		case '}': k = Tok::RBrace; break;
		case '[': k = Tok::LBracket; break;
		case ']': k = Tok::RBracket; break;
		case '<': k = Tok::LAngle; break;
		case '>': k = Tok::RAngle; break;
		case '=': k = Tok::Assign; break;
		case '+': k = Tok::Plus; break;
		case '-': k = Tok::Minus; break;
		case '*': k = Tok::Star; break;
		case '/': k = Tok::Slash; break;
		case '%': k = Tok::Percent; break;
		case '.': k = Tok::Dot; break;
		case '@': k = Tok::At; break;
		default:
			fail(std::string("无法识别的字符 '") + c + "'");
			return r;
		}
		push(k, std::string(1, c));
		i++; col++;
	}

	Token end; end.kind = Tok::End; end.line = line; end.col = col;
	r.toks.push_back(end);
	r.diag.ok = true;
	return r;
}

// ------------------------------------------------------------ Token → 文本

std::string tokensToText(const std::vector<Token>& toks, size_t begin, size_t end) {
	// 重新排版：标点前后不留多余空格，方便诊断回显和代码字面量再解析
	auto noSpaceBefore = [](Tok k) {
		return k == Tok::Comma || k == Tok::RParen || k == Tok::RBracket ||
		       k == Tok::RAngle || k == Tok::Dot;
	};
	auto noSpaceAfter = [](Tok k) {
		return k == Tok::LParen || k == Tok::LBracket || k == Tok::LAngle ||
		       k == Tok::Dot || k == Tok::At;
	};

	std::string out;
	Tok prev = Tok::End;
	bool first = true;
	for (size_t i = begin; i < end && i < toks.size(); i++) {
		const Token& t = toks[i];
		if (t.kind == Tok::Newline) {
			while (!out.empty() && out.back() == ' ') out.pop_back();
			out += '\n';
			first = true;
			prev = Tok::End;
			continue;
		}

		std::string text;
		switch (t.kind) {
		case Tok::Ident: text = t.text; break;
		case Tok::Int: {
			std::ostringstream oss; oss << t.i; text = oss.str(); break;
		}
		case Tok::Float: {
			std::ostringstream oss; oss.precision(17); oss << t.f;
			text = oss.str();
			if (text.find('.') == std::string::npos && text.find('e') == std::string::npos &&
				text.find("inf") == std::string::npos && text.find("nan") == std::string::npos)
				text += ".0";
			break;
		}
		case Tok::Char: {
			text = "'";
			escapeTo(text, t.ch, '\'');
			text += "'";
			break;
		}
		case Tok::Str: {
			text = "\"";
			for (char c : t.text) escapeTo(text, c, '"');
			text += "\"";
			break;
		}
		default: text = t.text; break;
		}

		if (!first && !noSpaceBefore(t.kind) && !noSpaceAfter(prev)) out += ' ';
		out += text;
		prev = t.kind;
		first = false;
	}
	while (!out.empty() && out.back() == ' ') out.pop_back();
	return out;
}

// ---------------------------------------------------------------- 语法

namespace {

struct MacroDef {
	std::string name;
	std::vector<std::string> params;
	std::vector<Token> body;
	int line = 0;
};

class Parser {
public:
	Parser(std::vector<Token> toks, std::vector<MacroDef> macros)
		: t(std::move(toks)), m(std::move(macros)) {}

	Program prog;
	Diag diag;

	// 只解析一个表达式（供 parseExpression 用）
	ExprP parseExprOnly(Diag& out) {
		skipNewlines();
		ExprP e = parseExpr();
		if (!e) { out = diag; return nullptr; }
		skipNewlines();
		if (cur() != Tok::End) {
			fail(std::string("表达式之后有多余内容: ") + tokName(cur()));
			out = diag;
			return nullptr;
		}
		out.ok = true;
		return e;
	}
	bool run() {
		expand();
		if (!diag.ok) return false;
		if (!parseProgram()) return false;
		return true;
	}

private:
	std::vector<Token> t;
	std::vector<MacroDef> m;
	size_t p = 0;
	Conf conf;

	// ---- 基础 ----
	bool atEnd() const { return p >= t.size() || t[p].kind == Tok::End; }
	Tok cur() const { return atEnd() ? Tok::End : t[p].kind; }
	Tok ahead(size_t n) const {
		size_t q = p + n;
		return q >= t.size() ? Tok::End : t[q].kind;
	}
	const Token& tok() const { static Token e; return atEnd() ? e : t[p]; }
	bool accept(Tok k) { if (cur() == k) { p++; return true; } return false; }
	bool skipNewlines() {
		bool any = false;
		while (cur() == Tok::Newline) { p++; any = true; }
		return any;
	}
	bool fail(const std::string& msg) {
		if (!diag.ok) return false;
		diag.ok = false;
		diag.msg = msg;
		diag.line = tok().line;
		diag.col = tok().col;
		return false;
	}
	bool failAt(const std::string& msg, int line, int col) {
		if (!diag.ok) return false;
		diag.ok = false;
		diag.msg = msg;
		diag.line = line;
		diag.col = col;
		return false;
	}
	bool expect(Tok k, const char* what) {
		if (cur() == k) { p++; return true; }
		return fail(std::string("期望 ") + what + "，实际是 " + tokName(cur()));
	}

	// ---- 宏展开（Token 层，可嵌套）----
	void expand() {
		std::vector<Token> out;
		emit(t, 0, t.size(), 0, out);
		t.swap(out);
	}

	void emit(const std::vector<Token>& src, size_t b, size_t e, int depth,
	          std::vector<Token>& out) {
		if (!diag.ok) return;
		if (depth > 32) {
			failAt("宏展开层数过深（可能存在递归定义）", src.empty() ? 1 : src[b].line, 1);
			return;
		}
		for (size_t i = b; i < e; i++) {
			if (src[i].kind == Tok::Ident && i + 1 < e && src[i + 1].kind == Tok::LAngle) {
				const MacroDef* def = findMacro(src[i].text);
				if (def != nullptr) {
					size_t close = matchAngle(src, i + 1, e);
					if (close == std::string::npos) {
						failAt("宏调用 " + src[i].text + " 缺少 '>'", src[i].line, src[i].col);
						return;
					}
					std::vector<std::pair<size_t, size_t>> args;
					if (!splitArgs(src, i + 2, close, args)) return;
					if (args.size() != def->params.size()) {
						failAt("宏 " + def->name + " 需要 " + std::to_string(def->params.size()) +
						       " 个参数，实际 " + std::to_string(args.size()) + " 个",
						       src[i].line, src[i].col);
						return;
					}
					std::vector<Token> body;
					substituteArgs(*def, src, args, body);
					emit(body, 0, body.size(), depth + 1, out);
					i = close;
					continue;
				}
			}
			out.push_back(src[i]);
		}
	}

	void substituteArgs(const MacroDef& def, const std::vector<Token>& src,
	                    const std::vector<std::pair<size_t, size_t>>& args,
	                    std::vector<Token>& body) {
		for (const Token& bt : def.body) {
			bool replaced = false;
			if (bt.kind == Tok::Ident) {
				for (size_t k = 0; k < def.params.size(); k++) {
					if (def.params[k] == bt.text) {
						for (size_t q = args[k].first; q < args[k].second; q++) body.push_back(src[q]);
						replaced = true;
						break;
					}
				}
			}
			if (!replaced) body.push_back(bt);
		}
	}

	const MacroDef* findMacro(const std::string& name) const {
		for (const auto& d : m)
			if (d.name == name) return &d;
		return nullptr;
	}

	size_t matchAngle(const std::vector<Token>& src, size_t open, size_t e) const {
		int depth = 0;
		for (size_t i = open; i < e; i++) {
			if (src[i].kind == Tok::LAngle) depth++;
			else if (src[i].kind == Tok::RAngle) {
				if (--depth == 0) return i;
			}
		}
		return std::string::npos;
	}

	bool splitArgs(const std::vector<Token>& src, size_t b, size_t e,
	               std::vector<std::pair<size_t, size_t>>& args) {
		if (b >= e) return true;  // 无参
		int depth = 0;
		size_t start = b;
		for (size_t i = b; i <= e; i++) {
			Tok k = (i == e) ? Tok::Comma : src[i].kind;
			if (k == Tok::LParen || k == Tok::LBracket || k == Tok::LBrace) depth++;
			else if (k == Tok::RParen || k == Tok::RBracket || k == Tok::RBrace) depth--;
			if (k == Tok::Comma && depth == 0) {
				// 去掉首尾空白 token
				size_t s = start, f = i;
				while (s < f && (src[s].kind == Tok::Newline)) s++;
				while (f > s && (src[f - 1].kind == Tok::Newline)) f--;
				args.push_back({s, f});
				start = i + 1;
			}
		}
		return true;
	}

	// ---- 程序结构 ----
	bool parseProgram() {
		std::vector<Stmt> pub;
		std::vector<Segment> segs;

		for (;;) {
			skipNewlines();
			if (cur() == Tok::End) break;

			if (cur() == Tok::Dot && ahead(1) == Tok::LAngle) {
				Conf c;
				if (!parseConfSeg(c)) return false;
				conf = c;
				continue;
			}
			if (cur() == Tok::At && ahead(1) == Tok::LAngle) {
				p++;  // '@'
				Segment s;
				if (!parseSegBody(s, true)) return false;
				segs.push_back(std::move(s));
				continue;
			}
			if (cur() == Tok::LAngle) {
				Segment s;
				if (!parseSegBody(s, false)) return false;
				segs.push_back(std::move(s));
				continue;
			}

			Stmt st;
			if (!parseStmt(st)) return false;
			pub.push_back(std::move(st));
		}

		prog.conf = conf;
		if (!pub.empty()) {
			Segment s;
			s.name = "_pub_";
			s.pub = true;
			s.body = std::move(pub);
			prog.segs.push_back(std::move(s));
		}
		for (auto& s : segs) prog.segs.push_back(std::move(s));

		// 段名唯一性检查
		for (size_t i = 0; i < prog.segs.size(); i++)
			for (size_t j = i + 1; j < prog.segs.size(); j++)
				if (!prog.segs[j].over && prog.segs[i].name == prog.segs[j].name)
					return failAt("段 <" + prog.segs[j].name + "> 重复定义", prog.segs[j].line, 1);
		return true;
	}

	bool parseConfSeg(Conf& out) {
		const int startLine = tok().line;
		p++;  // '.'
		if (!expect(Tok::LAngle, "'<'")) return false;
		if (cur() != Tok::Ident) return fail("配置段缺少名字");
		std::string name = tok().text;
		p++;
		if (!expect(Tok::RAngle, "'>'")) return false;
		if (!expect(Tok::LBrace, "'{'")) return false;

		Conf c = out;
		skipNewlines();
		while (cur() != Tok::RBrace && cur() != Tok::End) {
			if (cur() != Tok::Ident) return fail("配置段内只允许 heap/stack/reg");
			std::string key = lower(tok().text);
			p++;
			if (cur() != Tok::Int) return fail("配置项 " + key + " 后面需要正整数");
			int v = static_cast<int>(tok().i);
			p++;
			if (key == "heap") c.heap = v;
			else if (key == "stack") c.stack = v;
			else if (key == "reg") c.regs = v;
			else return fail("未知配置项: " + key);
			accept(Tok::Newline);
			skipNewlines();
		}
		if (!expect(Tok::RBrace, "'}'")) return false;
		(void)startLine;
		(void)name;
		out = c;
		return true;
	}

	bool parseSegBody(Segment& s, bool over) {
		s.over = over;
		s.line = tok().line;
		if (!expect(Tok::LAngle, "'<'")) return false;
		if (cur() != Tok::Ident) return fail("段声明缺少名字");
		s.name = tok().text;
		p++;
		if (!expect(Tok::RAngle, "'>'")) return false;
		if (!expect(Tok::LBrace, "'{'")) return false;

		for (;;) {
			skipNewlines();
			if (cur() == Tok::RBrace) { p++; return true; }
			if (cur() == Tok::End) return fail("段 <" + s.name + "> 缺少 '}'");
			Stmt st;
			if (!parseStmt(st)) return false;
			s.body.push_back(std::move(st));
		}
	}

	// ---- 语句 ----
	bool isDecl(const std::string& op) const {
		return op == "var" || op == "int" || op == "char" || op == "float" ||
		       op == "str" || op == "code" || op == "map";
	}

	bool parseStmt(Stmt& st) {
		const size_t begin = p;
		st.line = tok().line;
		st.col = tok().col;

		if (cur() == Tok::LBracket) {
			p++;
			if (cur() != Tok::Ident) return fail("标号 [ ] 内需要标识符");
			st.kind = StmtKind::Label;
			st.op = "[" + tok().text + "]";
			p++;
			if (!expect(Tok::RBracket, "']'")) return false;
			st.text = tokensToText(t, begin, p);
			accept(Tok::Newline);
			return true;
		}

		if (cur() != Tok::Ident) return fail("语句需要一个指令名");
		st.kind = StmtKind::Instr;
		st.op = lower(tok().text);
		p++;

		// 变量声明：var x = <expr>
		if (isDecl(st.op) && cur() == Tok::Ident && ahead(1) == Tok::Assign) {
			st.a = std::make_shared<Expr>();
			st.a->kind = ExprKind::Name;
			st.a->s = tok().text;
			st.a->line = tok().line;
			st.a->col = tok().col;
			p += 2;  // 名字 + '='
			st.b = parseExpr();
			if (!st.b) return false;
		} else if (cur() != Tok::Newline && cur() != Tok::RBrace && cur() != Tok::End) {
			st.a = parseExpr();
			if (!st.a) return false;
			if (accept(Tok::Comma)) {
				st.b = parseExpr();
				if (!st.b) return false;
				if (accept(Tok::Comma)) {
					st.c = parseExpr();
					if (!st.c) return false;
				}
			}
		}

		st.text = tokensToText(t, begin, p);
		if (cur() == Tok::Newline) p++;
		else if (cur() != Tok::RBrace && cur() != Tok::End)
			return fail(std::string("语句结束后出现了多余的 ") + tokName(cur()));
		return true;
	}

	// ---- 表达式（四则运算 + 优先级）----
	ExprP parseExpr() { return parseAdd(); }

	ExprP mkBin(char op, ExprP l, ExprP r, int line, int col) {
		auto e = std::make_shared<Expr>();
		e->kind = ExprKind::Binary;
		e->op = op;
		e->l = std::move(l);
		e->r = std::move(r);
		e->line = line;
		e->col = col;
		return e;
	}

	ExprP parseAdd() {
		ExprP l = parseMul();
		if (!l) return nullptr;
		for (;;) {
			Tok k = cur();
			if (k != Tok::Plus && k != Tok::Minus) break;
			int line = tok().line, col = tok().col;
			p++;
			ExprP r = parseMul();
			if (!r) return nullptr;
			l = mkBin(k == Tok::Plus ? '+' : '-', l, r, line, col);
		}
		return l;
	}

	ExprP parseMul() {
		ExprP l = parseUnary();
		if (!l) return nullptr;
		for (;;) {
			Tok k = cur();
			if (k != Tok::Star && k != Tok::Slash && k != Tok::Percent) break;
			int line = tok().line, col = tok().col;
			p++;
			ExprP r = parseUnary();
			if (!r) return nullptr;
			l = mkBin(k == Tok::Star ? '*' : (k == Tok::Slash ? '/' : '%'), l, r, line, col);
		}
		return l;
	}

	ExprP parseUnary() {
		if (cur() == Tok::Minus) {
			int line = tok().line, col = tok().col;
			p++;
			ExprP l = parseUnary();
			if (!l) return nullptr;
			auto e = std::make_shared<Expr>();
			e->kind = ExprKind::Unary;
			e->op = '-';
			e->l = std::move(l);
			e->line = line;
			e->col = col;
			return e;
		}
		if (cur() == Tok::Plus) { p++; return parseUnary(); }
		return parsePrimary();
	}

	ExprP parsePrimary() {
		int line = tok().line, col = tok().col;
		switch (cur()) {
		case Tok::Int: {
			auto e = std::make_shared<Expr>();
			e->kind = ExprKind::Int;
			e->i = tok().i;
			e->line = line; e->col = col;
			p++;
			return e;
		}
		case Tok::Float: {
			auto e = std::make_shared<Expr>();
			e->kind = ExprKind::Float;
			e->f = tok().f;
			e->line = line; e->col = col;
			p++;
			return e;
		}
		case Tok::Char: {
			auto e = std::make_shared<Expr>();
			e->kind = ExprKind::Char;
			e->ch = tok().ch;
			e->line = line; e->col = col;
			p++;
			return e;
		}
		case Tok::Str: {
			auto e = std::make_shared<Expr>();
			e->kind = ExprKind::Str;
			e->s = tok().text;
			e->line = line; e->col = col;
			p++;
			return e;
		}
		case Tok::LBracket: {
			p++;
			if (cur() != Tok::Ident) { fail("标号 [ ] 内需要标识符"); return nullptr; }
			std::string name = tok().text;
			p++;
			if (!expect(Tok::RBracket, "']'")) return nullptr;
			auto e = std::make_shared<Expr>();
			e->kind = ExprKind::Label;
			e->s = "[" + name + "]";
			e->line = line; e->col = col;
			return e;
		}
		case Tok::LAngle: {
			p++;
			if (cur() != Tok::Ident) { fail("段引用 < > 内需要标识符"); return nullptr; }
			std::string name = tok().text;
			p++;
			if (!expect(Tok::RAngle, "'>'")) return nullptr;
			auto e = std::make_shared<Expr>();
			e->kind = ExprKind::Seg;
			e->s = name;
			e->line = line; e->col = col;
			return e;
		}
		case Tok::LParen: return parseParenOrCode();
		case Tok::Ident: {
			std::string name = tok().text;
			// m( ... ) / M( ... ) 映射字面量
			if ((name == "m" || name == "M") && ahead(1) == Tok::LParen) {
				p++;
				auto e = std::make_shared<Expr>();
				e->kind = ExprKind::Map;
				if (!takeBalanced(Tok::LParen, Tok::RParen, e->s)) return nullptr;
				e->line = line; e->col = col;
				return e;
			}
			if (isRegName(name)) {
				auto e = std::make_shared<Expr>();
				e->kind = ExprKind::Reg;
				e->s = name;
				e->reg = std::atoi(name.c_str() + 1);
				e->line = line; e->col = col;
				p++;
				return e;
			}
			auto e = std::make_shared<Expr>();
			e->kind = ExprKind::Var;
			e->s = name;
			e->line = line; e->col = col;
			p++;
			return e;
		}
		default:
			fail(std::string("缺少操作数，遇到了 ") + tokName(cur()));
			return nullptr;
		}
	}

	// '(' 之后：分「算术括号」和「代码字面量」两种
	//
	// 判定规则（写进 DESIGN.md）：
	//   * 括号内出现换行        -> 代码字面量
	//   * 括号内深度 0 有运算符 -> 算术分组，例如 (a + b) * 2
	//   * 否则                  -> 代码字面量，例如 (nop) / ( prt "x" )
	ExprP parseParenOrCode() {
		const size_t open = p;
		int line = tok().line, col = tok().col;

		// 找到配对的 ')'
		int depth = 0;
		size_t close = t.size();
		for (size_t i = open; i < t.size(); i++) {
			if (t[i].kind == Tok::LParen) depth++;
			else if (t[i].kind == Tok::RParen) {
				if (--depth == 0) { close = i; break; }
			} else if (t[i].kind == Tok::End) break;
		}
		if (close >= t.size()) { fail("'(' 没有配对的 ')'"); return nullptr; }

		bool code = false;
		int d = 1;
		// 「指令名 + 操作数」的样子（Ident 后紧跟 Ident/字面量/寄存器）也当代码段，
		// 这样单行 (mov r0, 1 + 2) 不会被误当成算术表达式
		for (size_t i = open + 1; i + 1 < close; i++) {
			if (t[i].kind != Tok::Ident) continue;
			Tok nx = t[i + 1].kind;
			if (nx == Tok::Ident || nx == Tok::Int || nx == Tok::Float ||
			    nx == Tok::Str || nx == Tok::Char) {
				code = true;
				break;
			}
		}
		if (!code) {
			for (size_t i = open + 1; i < close; i++) {
				Tok k = t[i].kind;
				if (k == Tok::LParen || k == Tok::LBracket) d++;
				else if (k == Tok::RParen || k == Tok::RBracket) d--;
				if (k == Tok::Newline && d == 1) { code = true; break; }
				if (d == 1 && (k == Tok::Plus || k == Tok::Minus || k == Tok::Star ||
				               k == Tok::Slash || k == Tok::Percent)) {
					code = false;
					break;
				}
			}
		}

		if (code) {
			auto e = std::make_shared<Expr>();
			e->kind = ExprKind::Code;
			e->s = tokensToText(t, open + 1, close);
			e->line = line; e->col = col;
			p = close + 1;
			return e;
		}

		p++;  // '('
		ExprP inner = parseExpr();
		if (!inner) return nullptr;
		if (!expect(Tok::RParen, "')'")) return nullptr;
		return inner;
	}

	// 当前位置是 open 括号，取到配对收括号为止的文本，p 移到收括号之后
	bool takeBalanced(Tok openK, Tok closeK, std::string& out) {
		int depth = 0;
		size_t b = p;
		for (size_t i = p; i < t.size(); i++) {
			if (t[i].kind == openK) depth++;
			else if (t[i].kind == closeK) {
				if (--depth == 0) {
					out = tokensToText(t, b + 1, i);
					p = i + 1;
					return true;
				}
			} else if (t[i].kind == Tok::End) break;
		}
		return fail(std::string("'") + tokName(openK) + "' 没有配对的 '" + tokName(closeK) + "'");
	}
};

// 收集宏定义，并把它们从 Token 流里摘掉
bool collectMacros(std::vector<Token>& toks, std::vector<MacroDef>& macros, Diag& d) {
	std::vector<Token> kept;
	for (size_t i = 0; i < toks.size(); i++) {
		if (toks[i].kind == Tok::Dot && i + 2 < toks.size() &&
		    toks[i + 1].kind == Tok::Ident && toks[i + 2].kind == Tok::LAngle) {
			MacroDef def;
			def.name = toks[i + 1].text;
			def.line = toks[i].line;
			size_t j = i + 3;
			bool ok = true;
			while (j < toks.size() && toks[j].kind != Tok::RAngle) {
				if (toks[j].kind == Tok::Ident) def.params.push_back(toks[j].text);
				else if (toks[j].kind != Tok::Comma && toks[j].kind != Tok::Newline) { ok = false; break; }
				j++;
			}
			if (!ok || j >= toks.size()) {
				d.ok = false; d.msg = "宏定义 " + def.name + " 的参数表格式错误";
				d.line = toks[i].line; d.col = toks[i].col;
				return false;
			}
			j++;  // '>'
			if (j >= toks.size() || toks[j].kind != Tok::LBrace) {
				d.ok = false; d.msg = "宏定义 " + def.name + " 缺少 '{'";
				d.line = toks[i].line; d.col = toks[i].col;
				return false;
			}
			int depth = 0;
			size_t bodyBegin = j + 1, bodyEnd = j + 1;
			for (size_t k = j; k < toks.size(); k++) {
				if (toks[k].kind == Tok::LBrace) depth++;
				else if (toks[k].kind == Tok::RBrace) {
					if (--depth == 0) { bodyEnd = k; j = k; break; }
				}
			}
			if (depth != 0) {
				d.ok = false; d.msg = "宏定义 " + def.name + " 缺少 '}'";
				d.line = toks[i].line; d.col = toks[i].col;
				return false;
			}
			for (size_t k = bodyBegin; k < bodyEnd; k++) def.body.push_back(toks[k]);
			macros.push_back(std::move(def));
			i = j;  // 跳过整个定义
			// 定义后面的换行保留，避免把两条语句粘在一起
			continue;
		}
		kept.push_back(toks[i]);
	}
	toks.swap(kept);
	return true;
}

}  // namespace

ParseResult parse(const std::string& src) {
	ParseResult r;
	LexResult lr = lex(src);
	if (!lr.diag.ok) { r.diag = lr.diag; return r; }

	std::vector<MacroDef> macros;
	if (!collectMacros(lr.toks, macros, r.diag)) return r;

	Parser ps(std::move(lr.toks), std::move(macros));
	if (!ps.run()) { r.diag = ps.diag; return r; }
	r.prog = std::move(ps.prog);
	r.diag.ok = true;
	return r;
}

ExprP parseExpression(const std::string& src, Diag& d) {
	LexResult lr = lex(src);
	if (!lr.diag.ok) { d = lr.diag; return nullptr; }
	Parser ps(std::move(lr.toks), {});
	return ps.parseExprOnly(d);
}

}  // namespace NSASM
