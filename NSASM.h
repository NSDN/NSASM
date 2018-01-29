#ifndef __NSASM_H_
#define __NSASM_H_


#include <functional>
#include <vector>
#include <stack>
#include <map>

namespace NSASM {

	using namespace std;

	class NSASM {

	public:
		const string version = "0.42";

		enum Result {
			RES_OK, RES_ERR, RES_ETC
		};
		
		enum RegType {
			REG_CHAR, REG_STR, REG_INT, REG_FLOAT, REG_CODE
		};

		class Register {
		public:
			Register() {
				this->type = RegType::REG_INT;
				this->readOnly = false;
				this->gcFlag = false;
				this->data.i = 0;
			}
			Register(const Register& reg) {
				this->gcFlag = false;
				type = reg.type;
				strPtr = reg.strPtr;
				readOnly = reg.readOnly;
				switch (type) {
				case RegType::REG_INT:
					data.i = reg.data.i;
					break;
				case RegType::REG_CHAR:
					data.c = reg.data.c;
					break;
				case RegType::REG_FLOAT:
					data.f = reg.data.f;
					break;
				case RegType::REG_STR:
					data.s = reg.data.s;
					break;
				case RegType::REG_CODE:
					data.s = reg.data.s;
					break;
				}
			}
			~Register() {
				if (this->type == RegType::REG_STR || this->type == RegType::REG_CODE)
					this->data.s.clear();
			}

		public:
			Register& operator=(const Register& reg) {
				this->gcFlag = false;
				type = reg.type;
				strPtr = reg.strPtr;
				readOnly = reg.readOnly;
				switch (type) {
				case RegType::REG_INT:
					data.i = reg.data.i;
					break;
				case RegType::REG_CHAR:
					data.c = reg.data.c;
					break;
				case RegType::REG_FLOAT:
					data.f = reg.data.f;
					break;
				case RegType::REG_STR:
					data.s = reg.data.s;
					break;
				case RegType::REG_CODE:
					data.s = reg.data.s;
					break;
				}
				return *this;
			}

		public:
			RegType type;
			union Data {
			public:
				Data() { i = 0; }
				~Data() {}
			public:
				int i; char c;
				float f; string s;

			} data;
			int strPtr = 0;
			bool readOnly;
			bool gcFlag;
		};

		typedef function<Result(Register*, Register*)> Operator;

		Result execute(string var);
		Register* run();
		void call(string segName);

		NSASM(int heapSize, int stackSize, int regCnt, map<string, string> code);
		~NSASM();

	protected:
		vector<Register> regGroup;
		map<string, Operator> funcList;

		Register* eval(Register* reg);
		void loadFuncList();

	private:
		enum WordType {
			WD_REG, WD_CHAR, WD_STR, WD_INT,
			WD_FLOAT, WD_VAR, WD_TAG, WD_SEG,
			WD_CODE
		};

		map<string, Register> heapManager;
		stack<Register> stackManager;
		int heapSize, stackSize;
		Register stateReg;
		Register prevDstReg;

		stack<int> backupReg;
		int progSeg, tmpSeg;
		int progCnt, tmpCnt;

		map<string, vector<string>> code;

		bool verifyBound(string var, char left, char right);
		bool verifyWord(string var, WordType type);
		Register* getRegister(string var);
		vector<string> convToArray(string var);
		Result appendCode(map<string, string> code);
		void copyRegGroup(NSASM super);

		Result calc(int* dst, int src, char type);
		Result calc(char* dst, char src, char type);
		Result calc(float* dst, float src, char type);
		Result calc(Register* dst, int src, char type);
		Result calc(Register* dst, Register* src, char type);

		NSASM(NSASM super, map<string, string> code);

	};

}


#endif
