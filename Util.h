#ifndef __UTIL_H_
#define __UTIL_H_

#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#define nulstr ""

namespace NSASM {

	using namespace std;

	class Util {

	public:
		static Util& I() {
			static Util inst;
			return inst;
		}

		typedef function<void(string)> Printer;
		typedef function<string(void)> Scanner;
		typedef function<string(string)> FileReader;

		Printer Output;
		Scanner Input;
		FileReader FileInput;

		Util() {
			Output = [](string str) -> void { cout << str; };
			Input = [](void) -> string { string s = ""; getline(cin, s); return s; };
			FileInput = [](string path) -> string { return ""; };
		}

	public:
		static inline void replace(string& var, string src, string dst) {
			size_t pos = 0; stack<size_t> s;
			while ((pos = var.find(src, pos)) != var.npos) {
				s.push(pos); pos += 1;
			}
			while (!s.empty()) {
				var = var.replace(s.top(), src.length(), dst); s.pop();
			}
		}

		static inline void cycleReplace(string& var, string src, string dst) {
			if (dst.find(src) != dst.npos) return;
			size_t pos;
			while ((pos = var.find(src)) != var.npos) {
				var = var.replace(pos, src.length(), dst);
			}
		}

	public:
		static void print(int value);
		static void print(char value);
		static void print(float value);
		static void print(string value);
		static string scan();
		static void cleanSymbol(string& var, string symbol, string trash);
		static void cleanSymbol(string& var, string symbol, string trashA, string trashB);
		static void cleanSymbolLeft(string& var, string symbol, string trashA, string trashB);
		static void cleanSymbolRight(string& var, string symbol, string trashA, string trashB);
		static void formatString(string& var);
		static string read(string path);
		// 返回 0 表示成功，非 0 表示语法或运行错误（供 CLI 设置退出码）
		static int run(string path);
		static int execute(string str);
		static void interactive();
		// 只做语法检查：成功返回 nulstr，失败返回诊断信息
		static string compile(string inPath, string outPath);
	};

}

#endif
