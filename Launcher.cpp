#include "NSASM.h"
#include "Util.h"

#include <fstream>
#include <mutex>
using namespace std;

#include <string.h>
#include <time.h>

static mutex io_lock;

int main(int argc, char* argv[]) {
	NSASM::Util::I().FileInput = [&](string path) -> string {
		io_lock.lock();
		ifstream reader; stringstream str; string buf;
		reader.open(path, ifstream::in); str.clear();
		while (!reader.eof()) {
			getline(reader, buf);
			str << buf << endl;
		}
		reader.close();
		string s = str.str();
		io_lock.unlock();
		return s;
	};
	NSASM::Util::I().Output = [&](string str) -> void {
		io_lock.lock();
		cout << str;
		fflush(stdout);
		io_lock.unlock();
	};
	NSASM::Util::I().Input = [&](void) -> string {
		io_lock.lock();
		string s = "";
		getline(cin, s);
		io_lock.unlock();
		return s;
	};

	NSASM::Util::print("NyaSama Assembly Script Module\n");
	NSASM::Util::print("Version: ");
	NSASM::Util::print(NSASM::NSASM::ver());
	NSASM::Util::print("\n\n");

	if (argc <= 1) {
		NSASM::Util::print("Usage: nsasm [c/r/i] [FILE]\n\n");
		NSASM::Util::interactive();
	} else {
		if (argc == 4) {
			if (strcmp(argv[1], "c") == 0) {
				string res = NSASM::Util::compile(argv[2], argv[3]);
				if (res == nulstr) {
					NSASM::Util::print("Compilation OK.\n\n");
					return 0;
				}
				NSASM::Util::print("Compilation failed.\n  " + res + "\n\n");
				return 2;
			}
		} else if (argc == 3) {
			if (strcmp(argv[1], "r") == 0) {
				clock_t begin, end; float t;
				string path = argv[2];
				begin = clock();
				int rc = NSASM::Util::run(path);
				end = clock();
				t = (float)(end - begin) / (float)CLOCKS_PER_SEC * 1000.0F;
				NSASM::Util::print("This script took ");
				NSASM::Util::print(t);
				NSASM::Util::print("ms.\n\n");
				return rc;
			} else if (strcmp(argv[1], "c") == 0) {
				string res = NSASM::Util::compile(argv[2], nulstr);
				if (res == nulstr) {
					NSASM::Util::print("\nCompilation OK.\n\n");
					return 0;
				}
				NSASM::Util::print("\nCompilation failed.\n  " + res + "\n\n");
				return 2;
			} else {
				clock_t begin, end; float t;
				string path = argv[1]; string seg = argv[2];
				string code = NSASM::Util::read(path);
				auto parsed = NSASM::parse(code);
				if (!parsed.diag.ok) {
					NSASM::Util::print("Syntax error at line " +
						std::to_string(parsed.diag.line) + ", column " +
						std::to_string(parsed.diag.col) + ":\n  " +
						parsed.diag.msg + "\n\n");
					return 1;
				}
				NSASM::NSASM nsasm(parsed.prog);
				begin = clock();
				nsasm.runSegment(seg);
				end = clock();
				t = (float)(end - begin) / (float)CLOCKS_PER_SEC * 1000.0F;
				NSASM::Util::print("This script took ");
				NSASM::Util::print(t);
				NSASM::Util::print("ms.\n\n");
				return 0;
			}
		}
		if (strcmp(argv[1], "i") == 0) {
			NSASM::Util::interactive();
			return 0;
		}
		string path = argv[1];
		return NSASM::Util::run(path);
	}

	return 0;
}
