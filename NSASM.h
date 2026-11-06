#ifndef __NSASM_H_
#define __NSASM_H_

#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "ng/Front.h"

// par 指令 / arg、tid 参数依赖线程
#define USE_MULTITHREAD

namespace NSASM {

	using namespace std;

	enum Result { RES_OK, RES_ERR, RES_ETC };

	enum RegType {
		REG_CHAR, REG_STR, REG_INT, REG_FLOAT,
		REG_CODE, REG_MAP, REG_PAR, REG_NUL
	};

	// 统一的值类型。
	//
	// 相比初版只有一处语义改动：operator< 不再把一切折成 float 比较。
	// 原来 int 16777216 和 16777217 会「相等」而被映射型变量静默合并（丢数据），
	// 字符串键则按 (float)hash 排序，撞键概率随条目数迅速上升。
	class Register {
	public:
		Register() {
			type = REG_NUL;
			readOnly = false;
			gcFlag = false;
			n.i = 0;
			s = "";
			sp = 0;
		}
		Register(const Register& reg) {
			type = reg.type;
			n = reg.n;
			s = reg.s;
			sp = reg.sp;
			m = reg.m;
			readOnly = reg.readOnly;
			gcFlag = false;  // 拷贝不继承所有权
		}
		Register& operator=(const Register& reg) {
			if (this == &reg) return *this;
			type = reg.type;
			n = reg.n;
			s = reg.s;
			sp = reg.sp;
			m = reg.m;
			readOnly = reg.readOnly;
			gcFlag = false;  // 拷贝不继承所有权
			return *this;
		}
		~Register() = default;

		// 序列化：打印、映射键显示都用它
		Register& operator>>(string& out) {
			switch (type) {
			case REG_INT: out = to_string(n.i); break;
			case REG_CHAR: out = string(1, n.c); break;
			case REG_FLOAT: {
				char buf[32];
				snprintf(buf, sizeof(buf), "%g", n.f);
				out = buf;
				break;
			}
			case REG_STR:
			case REG_PAR: out = s.substr(sp); break;
			case REG_CODE: out = "(\n" + s + "\n)"; break;
			case REG_MAP: {
				out = "M(\n";
				Register k, v;
				for (auto it = m.begin(); it != m.end(); it++) {
					k = it->first; v = it->second;
					string a, b;
					k >> a; v >> b;
					out += (a + "->" + b + "\n");
				}
				out += ")";
				break;
			}
			default: out = "null"; break;
			}
			return *this;
		}

		int rank() const {
			switch (type) {
			case REG_INT: case REG_CHAR: case REG_FLOAT: return 0;
			case REG_STR: return 1;
			case REG_CODE: return 2;
			case REG_PAR: return 3;
			case REG_MAP: return 4;
			default: return 5;
			}
		}
		double num() const {
			switch (type) {
			case REG_INT: return static_cast<double>(n.i);
			case REG_CHAR: return static_cast<double>(n.c);
			case REG_FLOAT: return static_cast<double>(n.f);
			default: return 0.0;
			}
		}

		// 映射键的严格弱序：数值按数值比（跨 int/char/float 相等即同一个键），
		// 其余按类型分组后精确比较内容。
		friend bool operator<(const Register& l, const Register& r) {
			const int lr = l.rank(), rr = r.rank();
			if (lr != rr) return lr < rr;
			if (lr == 0) {
				const double a = l.num(), b = r.num();
				const bool na = std::isnan(a), nb = std::isnan(b);
				if (na || nb) return na && !nb;
				if (a < b) return true;
				if (b < a) return false;
				return false;  // 数值相等 → 同一个键
			}
			return l.s < r.s;
		}

	public:
		RegType type;
		union Num {
			Num() { i = 0; }
			~Num() {}
			int i; char c; float f;
		} n;
		string s;
		int sp = 0;      // 字符串/代码的游标
		map<Register, Register> m;
		bool readOnly;
		bool gcFlag;     // 兼容旧指令实现，真正的所有权由解释器管
	};

	typedef function<Result(Register*, Register*, Register*)> Operator;
	#define $OP_ [&](Register* dst, Register* src, Register* ext) -> Result

	typedef function<Register*(Register*)> Param;
	#define $PA_ [&](Register* reg) -> Register*

	class NSASM;

	class NSASM {
	private:
		struct Frame {
			int seg;
			int pc;
		};
		// 一个操作数的解析结果
		struct Operand {
			Register* reg = nullptr;
			bool owned = false;        // reg 是否由本层分配（需要释放）
			string param;              // 非空表示是虚拟寄存器，操作后要写回
			string error;              // 解析失败原因
			bool missing = false;      // 该位置本来就没有操作数
		};

	public:
		// 直接给已解析的程序（ld / eval / par 都走这个）
		NSASM(const Program& prog);
		// 按配置构造空解释器，随后用 load 装入
		NSASM(int heapSize, int stackSize, int regCnt);
		virtual ~NSASM();

		static string ver();

		// 解析并追加源码（失败时 error() 给出原因，返回 false）
		bool load(const string& src);
		// 追加已解析的程序（段覆盖、重复段检查都在这里）
		bool append(const Program& prog);
		// 执行所有段
		Result run();
		// 交互模式：解析并立即执行一行（状态跨行保留）
		Result execLine(const string& line);
		// 只执行指定名字的段（nsasm FILE SEG 用法）
		Result runSegment(const string& name);

		Register* lastResult() { return &prevDstReg; }
		const string& error() const { return errMsg; }

	protected:
		Register* eval(Register* reg);
		virtual NSASM* instance(NSASM& super, const Program& prog);
		void loadFuncList();
		void loadParamList();
		void setArgument(Register* reg);

		// 名字是否可作为变量名（不依赖「首字母不是 r」这种旧规则）
		static bool isDeclarableName(const string& name);
		// 数值比较（供控制流使用，不再走「序列化成字符串再解析」）
		bool stateIsZero() const;
		int stateSign() const;
		// 把一段文本当立即数/表达式解析（in 指令读入、REPL 都用它）
		Register* getRegister(const string& var);
		Result calc(int* dst, int src, char type);
		Result calc(char* dst, char src, char type);
		Result calc(float* dst, float src, char type);
		Result calc(Register* dst, int src, char type);
		Result calc(Register* dst, Register* src, char type);
		// 三操作数（重载）形式 dst = src op ext：src 只读，计算在副本上进行
		Result triOp(const string& op, Register* dst, Register* src, Register* ext);

	protected:
		Register* useReg = nullptr;
		int regCnt = 0;
		vector<Register> regGroup;
		map<string, Operator> funcList;
		map<string, Param> paramList;
		Register* argReg = nullptr;

		map<string, Register> heapManager;
		stack<Register> stackManager;
		int heapSize = 64, stackSize = 32;
		Register stateReg;
		Register prevDstReg;

	private:
		Result runLoop(int stopSeg);
		int segIndex(const string& name) const;
		Result execInstr(const Stmt& st, const Segment& sg, const string& segName);
		Operand resolve(const ExprP& e);
		Operand evaluate(const ExprP& e);
		Register* buildCode(const Expr& e);
		Register* buildMap(const Expr& e);
		void writeBack(Operand& o);
		void release(Operand& o);
		Result controlFlow(const Stmt& st, const Segment& sg, const string& segName);
		Result jumpTo(const string& tag);
		void rebuildLabels();
		void fail(const string& msg, int line, int col);
		Result runChild(NSASM* child, Register* out);

		Program prog;
		Conf conf;
		map<string, pair<int, int>> labelAt;  // 标号 -> (段号, 语句号)
		vector<Frame> callStack;
		int curSeg = 0, curPc = 0;
		int jumpSeg = -1, jumpPc = -1;
		bool jumping = false;
		bool stopFlag = false;
		bool loadOk = true;      // append() 失败（重复段 / 覆盖段找不到目标）时为 false
		int callDepth = 0;
		string errMsg;
		string errWhere;
	};

}  // namespace NSASM

#endif
