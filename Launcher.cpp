#include "NSASM.h"
#include "Util.h"

#include <fstream>
using namespace std;

#include <string.h>
#include <time.h>

int main(int argc, char* argv[]) {
	NSASM::Util::I().FileInput = [](string path) -> string {
		ifstream reader; stringstream str; string buf;
		reader.open(path, ifstream::in); str.clear();
		while (!reader.eof()) {
			getline(reader, buf);
			str << buf << endl;
		}
		reader.close();
		return str.str();
	};

	NSASM::Util::print("NyaSama Assembly Script Module\n");
	NSASM::Util::print("Version: ");
	NSASM::Util::print(NSASM::NSASM::ver());
	NSASM::Util::print("\n\n");

	if (argc <= 1) {
		NSASM::Util::print("Usage: nsasm [c/r] [FILE]\n\n");
		NSASM::Util::interactive();
	} else {
		if (argc == 3) {
			if (strcmp(argv[1], "r") == 0) {
				clock_t begin, end; float t;
				string path = argv[2];
				begin = clock();
				NSASM::Util::run(path);
				end = clock();
				t = (float)(end - begin) / (float)CLOCKS_PER_SEC * 1000.0F;
				NSASM::Util::print("This script took ");
				NSASM::Util::print(t);
				NSASM::Util::print("ms.\n");
				return 0;
			} else {
				clock_t begin, end; float t;
				string seg = argv[1]; string path = argv[2];
				string code = NSASM::Util::read(path);
				auto segs = NSASM::Util::getSegments(code);
				NSASM::NSASM nsasm(64, 32, 32, segs);
				begin = clock();
				nsasm.call(seg);
				end = clock();
				t = (float)(end - begin) / (float)CLOCKS_PER_SEC * 1000.0F;
				NSASM::Util::print("This script took ");
				NSASM::Util::print(t);
				NSASM::Util::print("ms.\n");
				return 0;
			}
		}
		if (strcmp(argv[1], "c") == 0) {
			NSASM::Util::interactive();
			return 0;
		}
		string path = argv[1];
		NSASM::Util::run(path);
	}

	return 0;
}
