#ifndef __NSASM_H_
#define __NSASM_H_


#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <map>

#define USE_MULTITHREAD

#ifdef USE_MULTITHREAD
#include <mutex>
#endif

namespace NSASM {

	using namespace std;

	class NSASM {

	public:
		static string ver() { return "0.61"; }

		enum Result {
			RES_OK, RES_ERR, RES_ETC
		};
		
		enum RegType {
			REG_CHAR, REG_STR, REG_INT, REG_FLOAT,
			REG_CODE, REG_MAP, REG_PAR, REG_NUL
		};

	#ifdef USE_MULTITHREAD
		template<typename T>
		class SafePool {
			private:
				vector<T> pool;
				vector<int> pos;
				mutex lock;
				int now;

			public:
				SafePool() : pool(), pos(), lock() { now = 0; }
				~SafePool() { pool.clear(); pos.clear(); }

				int count() {
					lock.lock();
					int s = pool.size();
					lock.unlock();
					return s;
				}

				void add(T value) {
					lock.lock();
					pool.push_back(value);
					pos.push_back(now);
					now += 1;
					lock.unlock();
				}

				void insert(int index, T value) {
					lock.lock();
					pool.push_back(value);
					pos.push_back(index);
					lock.unlock();
				}

				T operator[](const int index) {
					lock.lock();
					int i;
					for (i = 0; i < pos.size(); i++)
						if (pos[i] == index)
							break;
					if (i >= pos.size()) return T();
					T t = pool[i];
					lock.unlock();
					return t;
				}
		};
	#endif

		class Register {
		public:
			Register() {
				this->type = RegType::REG_NUL;
				this->readOnly = false;
				this->gcFlag = false;
				this->n.c = 0;
				this->n.i = 0;
				this->n.f = 0;
				this->s = "";
			}
			Register(const Register& reg) {
				gcFlag = false;
				type = reg.type; sp = reg.sp; readOnly = reg.readOnly;
				switch (type) {
				case RegType::REG_INT: n.i = reg.n.i; break;
				case RegType::REG_CHAR: n.c = reg.n.c; break;
				case RegType::REG_FLOAT: n.f = reg.n.f; break;
				case RegType::REG_STR: s = reg.s; break;
				case RegType::REG_CODE: s = reg.s; break;
				case RegType::REG_MAP: m = reg.m; break;
				case RegType::REG_PAR: s = reg.s; break;
				}
			}
			~Register() {
				if (this->type == RegType::REG_STR || this->type == RegType::REG_CODE)
					this->s.clear();
				this->m.clear();
			}

		public:
			Register& operator=(const Register& reg) {
				gcFlag = false;
				type = reg.type; sp = reg.sp; readOnly = reg.readOnly;
				switch (type) {
				case RegType::REG_INT: n.i = reg.n.i; break;
				case RegType::REG_CHAR: n.c = reg.n.c; break;
				case RegType::REG_FLOAT: n.f = reg.n.f; break;
				case RegType::REG_STR: s = reg.s; break;
				case RegType::REG_CODE: s = reg.s; break;
				case RegType::REG_MAP: m = reg.m; break;
				case RegType::REG_PAR: s = reg.s; break;
				}
				return *this;
			}
			Register& operator>>(string& s) {
				stringstream parser; parser.clear();
				switch (this->type) {
				case RegType::REG_INT: parser << this->n.i; s = parser.str(); break;
				case RegType::REG_CHAR: parser << this->n.c; s = parser.str(); break;
				case RegType::REG_FLOAT: parser << this->n.f; s = parser.str(); break;
				case RegType::REG_STR: s = this->s.substr(this->sp); break;
				case RegType::REG_CODE: s = "(\n" + this->s + "\n)"; break;
				case RegType::REG_PAR: s = this->s; break;
				case RegType::REG_MAP:
					string a = "", b = ""; s = "M(\n"; Register reg;
					for (auto it = this->m.begin(); it != this->m.end(); it++) {
						reg = it->first; reg >> a; it->second >> b;
						s += (a + "->" + b + "\n");
					}
					s += ")";
					break;
				}
				return *this;
			}
			friend bool operator<(const Register& left, const Register& right) {
				hash<string> h; float l, r;
				switch (left.type) {
				case RegType::REG_INT:
					l = (float) left.n.i; break;
				case RegType::REG_CHAR:
					l = (float) left.n.c; break;
				case RegType::REG_FLOAT:
					l = left.n.f; break;
				case RegType::REG_STR:
					l = (float) h(left.s); break;
				case RegType::REG_CODE:
					l = (float) h(left.s); break;
				case RegType::REG_MAP:
					return false;
				case RegType::REG_PAR:
					l = (float) h(left.s); break;
				default:
					return false;
				}
				switch (right.type) {
				case RegType::REG_INT:
					r = (float) right.n.i; break;
				case RegType::REG_CHAR:
					r = (float) right.n.c; break;
				case RegType::REG_FLOAT:
					r = right.n.f; break;
				case RegType::REG_STR:
					r = (float) h(right.s); break;
				case RegType::REG_CODE:
					r = (float) h(right.s); break;
				case RegType::REG_MAP:
					return false;
				case RegType::REG_PAR:
					r = (float) h(right.s); break;
				default:
					return false;
				}
				return l < r;
			}

		public:
			RegType type;
			union Num {
			public:
				Num() { i = 0; }
				~Num() {}
			public:
				int i; char c; float f;
			} n;
			string s; int sp = 0;
			map<Register, Register> m;
			bool readOnly; bool gcFlag;
		};

		typedef function<Result(Register*, Register*, Register*)> Operator;
		#define $OP_ [&](Register* dst, Register* src, Register* ext) -> Result

		typedef function<Register*(Register*)> Param; // if reg is null, it's read, else write
		#define $PA_ [&](Register* reg) -> Register*

		Result execute(string var);
		Register* run();
		void call(string segName);

		NSASM(int heapSize, int stackSize, int regCnt, map<string, string>& code);
		~NSASM();

#ifdef USE_MULTITHREAD
	protected:
		void setArgument(Register* reg);
	private:
		Register* argReg;
#endif

	protected:
		Register* useReg = nullptr;
		int regCnt;
		vector<Register> regGroup;
		map<string, Operator> funcList;
		map<string, Param> paramList;

		virtual NSASM* instance(NSASM& super, map<string, string>& code);
		Register* eval(Register* reg);
		void loadFuncList();
		void loadParamList();

		NSASM(NSASM& super, map<string, string>& code);

	private:
		enum WordType {
			WD_REG, WD_CHAR, WD_STR, WD_INT,
			WD_FLOAT, WD_VAR, WD_TAG, WD_SEG,
			WD_CODE, WD_MAP, WD_PAR
		};

		map<string, Register> heapManager;
		stack<Register> stackManager;
		int heapSize, stackSize;
		Register stateReg;
		Register prevDstReg;

		stack<int> backupReg;
		int progSeg, tmpSeg;
		int progCnt, tmpCnt;

		vector<string> segs;
		map<string, vector<string>> code;

		bool verifyBound(string var, char left, char right);
		bool verifyWord(string var, WordType type);
		Register* getRegister(string var);
		vector<string> convToArray(string var);
		Result appendCode(map<string, string>& code);
		void copyRegGroup(NSASM& super);

		Result calc(int* dst, int src, char type);
		Result calc(char* dst, char src, char type);
		Result calc(float* dst, float src, char type);
		Result calc(Register* dst, int src, char type);
		Result calc(Register* dst, Register* src, char type);

	};

}


#endif
