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
	NSASM::Util::I().BinaryInput = [&](string path) ->vector<unsigned char> { 
		io_lock.lock();
		vector<unsigned char> res;
		ifstream reader; char buf;
		reader.open(path, ifstream::binary);
		reader.seekg(0, reader.end);
		int fileSiz = reader.tellg();
		reader.seekg(0, reader.beg);
		res.resize(fileSiz);
		reader.read((char*)res.data(), fileSiz);
		reader.close();
		io_lock.unlock();
		return res;
	};
	NSASM::Util::I().BinaryOutput = [&](string path, vector<unsigned char> data) -> void {
		io_lock.lock();
		ofstream writer;
		writer.open(path, ofstream::binary);
		writer.write((const char*)data.data(), data.size());
		writer.flush();
		writer.close();
		io_lock.unlock();
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
