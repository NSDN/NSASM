#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <map>

namespace NSASM {

	using namespace std;

	class Util {

		typedef void(*Printer)(string);
		typedef string(*Scanner)();
		typedef string(*FileReader)(string);

	public:
		static Printer Output;
		static Scanner Input;
		static FileReader FileInput;

		Util() {
			Output = NULL;
			Input = NULL;
			FileInput = NULL;
		}

	private:
		static inline void replace(string& var, string src, string dst) {
			size_t pos, begin = -1;
			while (pos = var.find(src) != var.npos) {
				var = var.replace(pos, (src).length(), dst);
				if (begin == -1) begin = pos;
				else if (pos == begin) break;
			}
		}

		static inline void cycleReplace(string& var, string src, string dst) {
			size_t pos;
			while (pos = var.find(src) != var.npos) {
				var = var.replace(pos, (src).length(), dst);
			}
		}

	public:
		static void print(string value) {
			if (Output != NULL) Output(value);
		}

		static string scan() {
			if (Input != NULL) return Input();
			return "";
		}

		static void cleanSymbol(string& var, string symbol, string trash) {
			cycleReplace(var, symbol + trash, symbol);
			cycleReplace(var, trash + symbol, symbol);
		}

		static void cleanSymbol(string& var, string symbol, string trashA, string trashB) {
			cycleReplace(var, symbol + trashA, symbol);
			cycleReplace(var, symbol + trashB, symbol);
			cycleReplace(var, trashA + symbol, symbol);
			cycleReplace(var, trashB + symbol, symbol);
		}

		static string formatLine(string var) {
			if (var.length() == 0) return "";
			
			cycleReplace(var, "\r", "");
			while (var[0] == '\t' || var[0] == ' ') {
				var = var.substr(1);
				if (var.length() == 0) return "";
			}

			string left, right; size_t pos;
			if (pos = var.find("\'") != var.npos) {
				left = var.substr(0, pos);
				right = var.substr(left.length());
			} else if (pos = var.find("\"") != var.npos) {
				left = var.substr(0, pos);
				right = var.substr(left.length());
				if (pos = right.substr(1).find("\"") != right.npos && (pos + 1) < right.length()) {
					if (right.find("*", pos + 1) != right.npos) {
						cleanSymbol(right, "*", "\t", " ");
					}
				}
			} else {
				left = var;
				right = "";
			}
			cycleReplace(left, "\t", " "); cycleReplace(left, "  ", " ");
			cleanSymbol(left, ",", " "); cleanSymbol(left, "=", " ");
			cleanSymbol(left, "{", "\t", " "); cleanSymbol(left, "}", "\t", " ");
			cleanSymbol(left, "(", "\t", " "); cleanSymbol(left, ")", "\t", " ");

			return left + right;
		}

		static string formatCode(string var) {
			string buf = "", tmp = "";
			stringstream reader(var);
			while (!reader.eof()) {
				getline(reader, tmp);
				buf += formatLine(tmp + "\n");
			}
			cycleReplace(buf, "\n\n", "\n");
			reader.clear();
			return buf;
		}

		static void repairBrackets(string& var, string left, string right) {
			cycleReplace(var, "\n" + left, left);
			replace(var, left, left + "\n");
			replace(var, right, "\n" + right);
			cycleReplace(var, "\n\n", "\n");
			cycleReplace(var, left + " ", left);
			cycleReplace(var, " \n" + right, "\n" + right);
		}

		static void encodeLambda(string& var) {
			replace(var, "\n", "\f");
		}

		static void decodeLambda(string var) {
			replace(var, "\f", "\n");
		}

		static void formatString(string var) {
			replace(var, "\\\"", "\"");
			replace(var, "\\\'", "\'");
			replace(var, "\\\\", "\\");
			replace(var, "\\n", "\n");
			replace(var, "\\t", "\t");
		}

		static string formatLambda(string var) {
			const int IDLE = 0, RUN = 1, DONE = 2;
			int state = IDLE, count = 0, begin = 0, end = 0;
			string a = "", b = "", c = "";

			for (int i = 0; i < var.length; i++) {
				switch (state) {
				case IDLE:
					count = begin = end = 0;
					if (var[i] == '(')
					{
						begin = i;
						count += 1;
						state = RUN;
					}
					break;
				case RUN:
					if (var[i] == '(')
						count += 1;
					else if (var[i] == ')')
						count -= 1;
					if (count == 0)
					{
						end = i;
						state = DONE;
					}
					break;
				case DONE:
					a = var.substr(0, begin);
					b = var.substr(begin, end - begin + 1);
					c = var.substr(end + 1);
					encodeLambda(b);
					var = a + b + c;
					state = IDLE;
					break;
				default:
					return var;
				}
			}
		
			return var;
		}

		static map<string, string> getSegments(string var) {
			map<string, string> segs;
			string pub = "", buf = var;

			buf = formatCode(buf);
			repairBrackets(buf, "{", "}");
			repairBrackets(buf, "(", ")");
			buf = formatCode(buf);

			buf = formatLambda(buf);

			stringstream reader(buf);
			string head, body = "", tmp;

			return segs;
		}

	};

}
