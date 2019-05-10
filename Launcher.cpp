#include "NSASM.h"
#include "Util.h"

#include <fstream>
using namespace std;

#include <string.h>
#include <time.h>

#define CODE_HEADER "HELLO GENSOKYO!"
#define CODE_HEADER_LEN 16
#define CODE_MAX_SIZE 4096

const static unsigned char inlineCode[CODE_MAX_SIZE] = CODE_HEADER;

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
	NSASM::Util::I().BinaryInput = [](string path) ->vector<unsigned char> { 
		vector<unsigned char> res;
		ifstream reader; char buf;
		reader.open(path, ifstream::binary);
		reader.seekg(0, reader.end);
		int fileSiz = reader.tellg();
		reader.seekg(0, reader.beg);
		res.resize(fileSiz);
		reader.read((char*)res.data(), fileSiz);
		reader.close();
		return res;
	};
	NSASM::Util::I().BinaryOutput = [](string path, vector<unsigned char> data) -> void {
		ofstream writer;
		writer.open(path, ofstream::binary);
		writer.write((const char*)data.data(), data.size());
		writer.flush();
		writer.close();
	};

	char cdHeader[CODE_HEADER_LEN] = { 0 };
	memcpy(cdHeader, inlineCode, CODE_HEADER_LEN);
	if (strcmp(cdHeader, CODE_HEADER) != 0) {
		NSASM::Util::binary((unsigned char*)inlineCode, CODE_MAX_SIZE);
		return 0;
	}

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
				if (res != nulstr)
					NSASM::Util::print("Compilation OK.\n\n");
				return 0;
			}
		} else if (argc == 3) {
			if (strcmp(argv[1], "r") == 0) {
				clock_t begin, end; float t;
				string path = argv[2];
				begin = clock();
				NSASM::Util::run(path);
				end = clock();
				t = (float)(end - begin) / (float)CLOCKS_PER_SEC * 1000.0F;
				NSASM::Util::print("This script took ");
				NSASM::Util::print(t);
				NSASM::Util::print("ms.\n\n");
				return 0;
			} else if (strcmp(argv[1], "c") == 0) {
				string res = NSASM::Util::compile(argv[2], nulstr);
				NSASM::Util::print("\n" + res + "\n");
				return 0;
			} else {
				clock_t begin, end; float t;
				string path = argv[1]; string seg = argv[2];
				string code = NSASM::Util::read(path);
				auto segs = NSASM::Util::getSegments(code);
				NSASM::NSASM nsasm(64, 32, 32, segs);
				begin = clock();
				nsasm.call(seg);
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
		NSASM::Util::run(path);
	}

	return 0;
}
