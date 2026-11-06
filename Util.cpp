#include "Util.h"

#include "NSASM.h"

namespace NSASM {

	void Util::print(int value) {
		stringstream parser;
		parser << value;
		I().Output(parser.str());
	}
	void Util::print(char value) {
		stringstream parser;
		parser << value;
		I().Output(parser.str());
	}
	void Util::print(float value) {
		stringstream parser;
		parser << value;
		I().Output(parser.str());
	}
	void Util::print(string value) {
		I().Output(value);
	}

	string Util::scan() {
		return I().Input();
	}

	void Util::cleanSymbol(string& var, string symbol, string trash) {
		cycleReplace(var, symbol + trash, symbol);
		cycleReplace(var, trash + symbol, symbol);
	}

	void Util::cleanSymbol(string& var, string symbol, string trashA, string trashB) {
		cycleReplace(var, symbol + trashA, symbol);
		cycleReplace(var, symbol + trashB, symbol);
		cycleReplace(var, trashA + symbol, symbol);
		cycleReplace(var, trashB + symbol, symbol);
	}

	void Util::cleanSymbolLeft(string& var, string symbol, string trashA, string trashB) {
		cycleReplace(var, trashA + symbol, symbol);
		cycleReplace(var, trashB + symbol, symbol);
	}

	void Util::cleanSymbolRight(string& var, string symbol, string trashA, string trashB) {
		cycleReplace(var, symbol + trashA, symbol);
		cycleReplace(var, symbol + trashB, symbol);
	}

	// 仅保留给「直接构造字符串值」的调用方用；源码里的转义现在由词法器处理
	void Util::formatString(string& var) {
		replace(var, "\\\"", "\"");
		replace(var, "\\\'", "\'");
		replace(var, "\\\\", "\\");
		replace(var, "\\n", "\n");
		replace(var, "\\t", "\t");
	}

	string Util::read(string path) {
		stringstream reader;
		try {
			reader << I().FileInput(path);
		} catch (exception& e) {
			print("File open failed.\n");
			print("At file: " + path + "\n\n");
			return nulstr;
		}
		if (reader.str().empty()) return nulstr;

		string str = "", tmp;
		try {
			while (!reader.eof()) {
				getline(reader, tmp);
				str += (tmp + "\n");
			}
		} catch (exception& e) {
			print("File read error.\n");
			print("At file: " + path + "\n\n");
			return nulstr;
		}
		return str;
	}

	namespace {
		void printParseError(const string& where, const Diag& d) {
			Util::print("\nNSASM syntax error!\n");
			Util::print("At " + where + ", line " + to_string(d.line) +
			            ", column " + to_string(d.col) + ":\n");
			Util::print("  " + d.msg + "\n\n");
		}
	}

	int Util::run(string path) {
		string str = read(path);
		if (str == nulstr) {
			print("Cannot read file: " + path + "\n\n");
			return 2;
		}
		ParseResult pr = parse(str);
		if (!pr.diag.ok) {
			printParseError(path, pr.diag);
			return 2;
		}
		NSASM nsasm(pr.prog);
		if (nsasm.run() == Result::RES_ERR) {
			print("\nNSASM aborted.\n\n");
			return 1;
		}
		print("\nNSASM running finished.\n\n");
		return 0;
	}

	int Util::execute(string str) {
		if (str == nulstr) return 2;
		ParseResult pr = parse(str);
		if (!pr.diag.ok) {
			printParseError("local", pr.diag);
			return 2;
		}
		NSASM nsasm(pr.prog);
		if (nsasm.run() == Result::RES_ERR) {
			print("\nNSASM aborted.\n\n");
			return 1;
		}
		print("\nNSASM running finished.\n\n");
		return 0;
	}

	void Util::interactive() {
		print("Now in console mode. 输入 end 退出。\n\n");
		NSASM nsasm(64, 32, 16);
		int lines = 1;
		string buf;

		for (;;) {
			print(lines); print(" >>> ");
			buf = scan();
			if (buf.length() == 0) {
				lines += 1;
				continue;
			}
			if (buf == "end") break;
			if (nsasm.execLine(buf) == Result::RES_ERR) {
				print("\nNSASM running error!\n");
				print("At line "); print(lines); print(": " + buf + "\n");
				print("  " + nsasm.error() + "\n\n");
			}
			lines += 1;
		}
	}

	// 旧的字节码格式（compile/binary）已被移除：
	// 它序列化的是「预处理后的源码文本」，和 AST 前端是两套东西，先不保留。
	// 现在 c 模式只做语法检查，成功返回 nulstr。
	string Util::compile(string inPath, string outPath) {
		(void)outPath;
		string str = read(inPath);
		if (str == nulstr) return "cannot read: " + inPath;
		ParseResult pr = parse(str);
		if (!pr.diag.ok) {
			return "line " + to_string(pr.diag.line) + ", column " +
			       to_string(pr.diag.col) + ": " + pr.diag.msg;
		}
		return nulstr;
	}

}
