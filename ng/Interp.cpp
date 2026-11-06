// 新一代解释器：直接执行 AST，不再把指令拼成字符串再拆开解析。
//
// 与初版的行为差异（都是修正，见 DESIGN.md）：
//   * 操作数在装载期就解析完毕；运行期只做取值，没有 substr/parseArgs/replace
//   * 跳转按标号表定位（原来是全代码文本扫描 + 两个 int 计数器）
//   * 出错带段名/行/列/原因；未知指令直接报错（原来是静默跳过）
//   * 四则运算：操作数可以是表达式，如 add r0, (a + b) * 2 - 1
//   * par 不再用 SafePool + 忙等，改成每线程独立实例 + 一次 join

#include <thread>

#include "NSASM.h"
#include "Util.h"

namespace NSASM {

	namespace {
		bool isDeclOp(const string& op) {
			return op == "var" || op == "int" || op == "char" || op == "float" ||
				op == "str" || op == "code" || op == "map";
		}
		bool isCtrlOp(const string& op) {
			return op == "jmp" || op == "jz" || op == "jnz" || op == "jg" || op == "jl" ||
				op == "loop" || op == "run" || op == "call" || op == "ret" || op == "end" ||
				op == "rst" || op == "nop" || op == "ld" || op == "eval" || op == "par";
		}
		bool isNumeric(RegType t) {
			return t == REG_INT || t == REG_CHAR || t == REG_FLOAT;
		}
	}

	// ------------------------------------------------------------ 生命周期

	NSASM::NSASM(int heapSize, int stackSize, int regCnt) {
		this->heapSize = heapSize;
		this->stackSize = stackSize;
		this->regCnt = regCnt;

		stateReg.type = REG_INT;
		stateReg.readOnly = false;
		stateReg.n.i = 0;
		prevDstReg.readOnly = false;

		for (int i = 0; i < regCnt + 1; i++) {
			Register r;
			r.type = REG_INT;
			r.readOnly = false;
			r.n.i = 0;
			regGroup.push_back(r);
		}
		useReg = &regGroup[regCnt];

		loadFuncList();
		loadParamList();
	}

	NSASM::NSASM(const Program& prog) : NSASM(prog.conf.heap, prog.conf.stack, prog.conf.regs) {
		// 装载失败必须在 run() 时暴露出来，不能像原来那样被构造器吞掉
		if (!append(prog)) loadOk = false;
	}

	NSASM::~NSASM() {
		regGroup.clear();
		heapManager.clear();
		funcList.clear();
		paramList.clear();
		delete argReg;
	}

	string NSASM::ver() { return "0.70-ast"; }

	bool NSASM::isDeclarableName(const string& name) {
		if (name.empty()) return false;
		if (name[0] >= '0' && name[0] <= '9') return false;
		for (char c : name)
			if (!(isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
		// r0 / R12 这种留给寄存器
		if (name[0] == 'r' || name[0] == 'R') {
			bool digits = name.size() > 1;
			for (size_t i = 1; i < name.size(); i++)
				if (!isdigit(static_cast<unsigned char>(name[i]))) { digits = false; break; }
			if (digits) return false;
		}
		return true;
	}

	void NSASM::fail(const string& msg, int line, int col) {
		if (!errMsg.empty()) return;  // 保留最内层的原因
		errMsg = msg + " (行 " + to_string(line) + ", 列 " + to_string(col) + ")";
	}

	// ------------------------------------------------------------ 装载

	bool NSASM::append(const Program& p) {
		if (p.segs.empty()) return true;
		// 首次装载时应用配置段
		if (prog.segs.empty() && heapSize == 64 && stackSize == 32 && regCnt == 16) {
			conf = p.conf;
		}
		for (const Segment& s : p.segs) {
			if (s.conf) continue;
			if (s.over) {
				bool found = false;
				for (Segment& dst : prog.segs) {
					if (dst.name == s.name) { dst.body = s.body; found = true; break; }
				}
				if (!found) {
					errMsg = "覆盖段 <" + s.name + "> 找不到同名段";
					return false;
				}
				continue;
			}
			if (s.pub) {
				bool merged = false;
				for (Segment& dst : prog.segs) {
					if (dst.pub) { dst.body.insert(dst.body.end(), s.body.begin(), s.body.end()); merged = true; break; }
				}
				if (!merged) prog.segs.push_back(s);
				continue;
			}
			bool dup = false;
			for (const Segment& dst : prog.segs)
				if (dst.name == s.name) { dup = true; break; }
			if (dup) {
				errMsg = "段 <" + s.name + "> 重复定义";
				return false;
			}
			prog.segs.push_back(s);
		}
		rebuildLabels();
		return true;
	}

	bool NSASM::load(const string& src) {
		ParseResult pr = parse(src);
		if (!pr.diag.ok) {
			errMsg = pr.diag.msg + " (行 " + to_string(pr.diag.line) + ", 列 " + to_string(pr.diag.col) + ")";
			return false;
		}
		errMsg.clear();
		return append(pr.prog);
	}

	void NSASM::rebuildLabels() {
		labelAt.clear();
		for (int si = 0; si < static_cast<int>(prog.segs.size()); si++) {
			const Segment& sg = prog.segs[si];
			for (int pc = 0; pc < static_cast<int>(sg.body.size()); pc++) {
				const Stmt& st = sg.body[pc];
				if (st.kind != StmtKind::Label) continue;
				if (labelAt.count(st.op) == 0) labelAt[st.op] = make_pair(si, pc);
			}
		}
	}

	int NSASM::segIndex(const string& name) const {
		for (int i = 0; i < static_cast<int>(prog.segs.size()); i++)
			if (prog.segs[i].name == name) return i;
		return -1;
	}

	// ------------------------------------------------------------ 执行

	Result NSASM::run() {
		if (!loadOk) {
			Util::print("\nNSASM loading error!\n  " + errMsg + "\n\n");
			return RES_ERR;
		}
		errMsg.clear();
		stopFlag = false;
		jumping = false;
		jumpSeg = jumpPc = -1;
		callStack.clear();
		curSeg = 0;
		curPc = 0;
		return runLoop(-1);
	}

	// 交互模式：把一行源码当成一个临时段跑掉，寄存器/变量状态保留
	Result NSASM::execLine(const string& line) {
		ParseResult pr = parse(line);
		if (!pr.diag.ok) {
			errMsg = pr.diag.msg + " (行 " + to_string(pr.diag.line) +
			         ", 列 " + to_string(pr.diag.col) + ")";
			return RES_ERR;
		}
		Segment seg;
		seg.name = "_repl_";
		for (const Segment& s : pr.prog.segs)
			seg.body.insert(seg.body.end(), s.body.begin(), s.body.end());
		if (seg.body.empty()) return RES_OK;

		prog.segs.push_back(seg);
		rebuildLabels();
		const int idx = static_cast<int>(prog.segs.size()) - 1;

		errMsg.clear();
		stopFlag = false;
		jumping = false;
		jumpSeg = jumpPc = -1;
		callStack.clear();
		curSeg = idx;
		curPc = 0;

		Result r = runLoop(idx);

		prog.segs.pop_back();
		rebuildLabels();
		return r;
	}

	Result NSASM::runSegment(const string& name) {
		if (!loadOk) {
			Util::print("\nNSASM loading error!\n  " + errMsg + "\n\n");
			return RES_ERR;
		}
		const int idx = segIndex(name);
		if (idx < 0) {
			errMsg = "找不到段 <" + name + ">";
			return RES_ERR;
		}
		errMsg.clear();
		stopFlag = false;
		jumping = false;
		jumpSeg = jumpPc = -1;
		callStack.clear();
		curSeg = idx;
		curPc = 0;
		return runLoop(idx);
	}

	Result NSASM::runLoop(int stopSeg) {
		for (;;) {
			if (stopFlag) break;
			if (stopSeg >= 0 && curSeg > stopSeg) break;
			if (curSeg < 0 || curSeg >= static_cast<int>(prog.segs.size())) break;

			const Segment& sg = prog.segs[curSeg];
			if (curPc >= static_cast<int>(sg.body.size())) {
				// 段执行完：有调用帧则返回调用点，否则顺序进入下一段
				if (!callStack.empty()) {
					Frame f = callStack.back();
					callStack.pop_back();
					curSeg = f.seg;
					curPc = f.pc;
					continue;
				}
				curSeg++;
				curPc = 0;
				continue;
			}

			const Stmt& st = sg.body[curPc];
			if (st.kind == StmtKind::Label) { curPc++; continue; }

			const int resume = curPc + 1;
			Result r = isCtrlOp(st.op) ? controlFlow(st, sg, sg.name)
			                           : execInstr(st, sg, sg.name);
			if (r == RES_ERR) {
				if (errMsg.empty()) fail("执行失败", st.line, st.col);
				Util::print("\nNSASM running error!\n");
				Util::print("At " + sg.name + ", line " + to_string(st.line) + ":\n");
				Util::print("    " + st.text + "\n");
				Util::print("  " + errMsg + "\n\n");
				return RES_ERR;
			}
			if (jumping) {
				curSeg = jumpSeg;
				curPc = jumpPc;
				jumping = false;
				continue;
			}
			if (stopFlag) break;
			curPc = resume;
		}

		prevDstReg.readOnly = false;
		return RES_OK;
	}

	Result NSASM::jumpTo(const string& tag) {
		auto it = labelAt.find(tag);
		if (it == labelAt.end()) {
			errMsg = "找不到标号 " + tag;
			return RES_ERR;
		}
		jumpSeg = it->second.first;
		jumpPc = it->second.second;
		jumping = true;
		return RES_OK;
	}

	bool NSASM::stateIsZero() const { return stateReg.num() == 0.0; }
	int NSASM::stateSign() const {
		const double v = stateReg.num();
		return v > 0 ? 1 : (v < 0 ? -1 : 0);
	}

	// 控制流指令：直接改解释器状态，不再走「文本扫描找标号」
	Result NSASM::controlFlow(const Stmt& st, const Segment& sg, const string& segName) {
		const string& op = st.op;

		if (op == "nop") return RES_OK;

		if (op == "jmp" || op == "jz" || op == "jnz" || op == "jg" || op == "jl") {
			bool take = true;
			if (op == "jz") take = stateIsZero();
			else if (op == "jnz") take = !stateIsZero();
			else if (op == "jg") take = stateSign() > 0;
			else if (op == "jl") take = stateSign() < 0;
			if (!take) return RES_OK;
			if (!st.a || st.a->kind != ExprKind::Label) {
				fail(op + " 需要一个 [标号]", st.line, st.col);
				return RES_ERR;
			}
			return jumpTo(st.a->s);
		}

		if (op == "loop") {
			// loop reg, times, [tag]  ==  inc/dec reg; cmp reg, times; jnz [tag]
			if (!st.a || !st.b || !st.c) {
				fail("loop 需要三个操作数：寄存器, 次数, [标号]", st.line, st.col);
				return RES_ERR;
			}
			Operand step = resolve(st.a);
			Operand limit = resolve(st.b);
			if (!step.reg || !limit.reg) {
				fail(step.error.empty() ? limit.error : step.error, st.line, st.col);
				release(step);
				release(limit);
				return RES_ERR;
			}
			if (!isNumeric(step.reg->type) || !isNumeric(limit.reg->type)) {
				fail("loop 的操作数必须是数值", st.line, st.col);
				release(step); release(limit);
				return RES_ERR;
			}
			const double lim = limit.reg->num();
			// 旧语义：上限 > 0 时递增，否则递减（0 或负数表示倒着数到 0）
			Result r = (lim > 0.0 ? funcList["inc"] : funcList["dec"])(step.reg, nullptr, nullptr);
			if (r != RES_ERR) {
				Register tmp;
				tmp.type = REG_INT;
				tmp.n.i = static_cast<int>(lim);
				r = funcList["cmp"](step.reg, &tmp, nullptr);
			}
			if (r == RES_ERR) {
				fail("loop 的计数操作失败", st.line, st.col);
				release(step); release(limit);
				return RES_ERR;
			}
			release(step);
			release(limit);
			if (stateIsZero()) return RES_OK;
			if (st.c->kind != ExprKind::Label) {
				fail("loop 的第三个操作数必须是 [标号]", st.line, st.col);
				return RES_ERR;
			}
			return jumpTo(st.c->s);
		}

		if (op == "run" || op == "call") {
			Operand o = resolve(st.a);
			if (!o.reg || o.reg->type != REG_STR) {
				fail(op + " 需要一个 <段名>", st.line, st.col);
				release(o);
				return RES_ERR;
			}
			string name = o.reg->s;
			if (!name.empty() && name.front() == '<' && name.back() == '>')
				name = name.substr(1, name.size() - 2);
			release(o);
			const int idx = segIndex(name);
			if (idx < 0) {
				fail("找不到段 <" + name + ">", st.line, st.col);
				return RES_ERR;
			}
			if (op == "call") callStack.push_back(Frame{curSeg, curPc + 1});
			jumpSeg = idx;
			jumpPc = 0;
			jumping = true;
			return RES_OK;
		}

		if (op == "ret") {
			if (st.a) {
				Operand o = resolve(st.a);
				if (!o.reg) {
					fail(o.error, st.line, st.col);
					return RES_ERR;
				}
				prevDstReg = *o.reg;
				release(o);
			} else {
				prevDstReg = regGroup[0];
			}
			prevDstReg.readOnly = false;
			stopFlag = true;
			return RES_OK;
		}

		if (op == "end") {
			prevDstReg.readOnly = false;
			stopFlag = true;
			return RES_OK;
		}

		if (op == "rst") {
			curSeg = 0;
			curPc = 0;
			callStack.clear();
			return RES_OK;
		}

		if (op == "ld") {
			Operand o = resolve(st.a);
			if (!o.reg) {
				fail(o.error, st.line, st.col);
				return RES_ERR;
			}
			string path;
			if (o.reg->type == REG_STR) path = o.reg->s;
			else if (o.reg->type == REG_CODE) {
				Register* r = eval(o.reg);
				if (r == nullptr) {
					fail("ld 的代码段求值失败", st.line, st.col);
					release(o);
					return RES_ERR;
				}
				path = r->s;
				delete r;
			} else {
				fail("ld 需要一个字符串或代码段", st.line, st.col);
				release(o);
				return RES_ERR;
			}
			release(o);
			string src = Util::read(path);
			if (src == nulstr) {
				fail("无法读取文件: " + path, st.line, st.col);
				return RES_ERR;
			}
			if (!load(src)) return RES_ERR;
			return RES_OK;
		}

		if (op == "eval") {
			// eval [dst,] src
			Operand dst, src;
			if (st.b) {
				dst = resolve(st.a);
				src = resolve(st.b);
			} else {
				src = resolve(st.a);
			}
			Result rc = RES_OK;
			if (!src.reg) {
				fail(src.error, st.line, st.col);
				rc = RES_ERR;
			} else if (src.reg->type != REG_CODE) {
				fail("eval 的操作数必须是代码段", st.line, st.col);
				rc = RES_ERR;
			} else {
				Register* r = eval(src.reg);
				if (r == nullptr) {
					fail("代码段执行失败", st.line, st.col);
					rc = RES_ERR;
				} else {
					if (dst.reg && st.b) {
						if (dst.reg->readOnly) {
							fail("eval 的目标操作数不可写", st.line, st.col);
							rc = RES_ERR;
						} else {
							*dst.reg = *r;
							dst.reg->readOnly = false;
							prevDstReg = *r;
						}
					}
					delete r;
				}
			}
			if (st.b) writeBack(dst);
			release(dst);
			release(src);
			return rc;
		}

		if (op == "par") {
			if (!st.a || !st.b || !st.c) {
				fail("par 需要三个操作数：结果, 代码段, 参数映射", st.line, st.col);
				return RES_ERR;
			}
			Operand dst = resolve(st.a);
			Operand code = resolve(st.b);
			Operand args = resolve(st.c);
			if (!dst.reg || !code.reg || !args.reg ||
			    code.reg->type != REG_CODE || args.reg->type != REG_MAP) {
				fail("par 的操作数类型错误", st.line, st.col);
				release(dst); release(code); release(args);
				return RES_ERR;
			}
			ParseResult pr = parse(code.reg->s);
			if (!pr.diag.ok) {
				fail("par 的代码段解析失败: " + pr.diag.msg, st.line, st.col);
				release(dst); release(code); release(args);
				return RES_ERR;
			}

			vector<Register> keys;
			vector<Register> values;
			for (auto it = args.reg->m.begin(); it != args.reg->m.end(); ++it) {
				keys.push_back(it->first);
				values.push_back(it->second);
			}

			// 每个线程一个独立实例：先全部建好（复制寄存器），再并发跑
			vector<NSASM*> runners;
			for (size_t i = 0; i < values.size(); i++) {
				NSASM* core = instance(*this, pr.prog);
				core->setArgument(&values[i]);
				core->callDepth = callDepth + 1;
				runners.push_back(core);
			}

			vector<Register> results(values.size());
			{
				vector<thread> threads;
				threads.reserve(runners.size());
				for (size_t i = 0; i < runners.size(); i++) {
					threads.emplace_back([&, i]() {
						runners[i]->run();
						results[i] = *runners[i]->lastResult();
					});
				}
				for (auto& t : threads) t.join();
			}
			for (NSASM* core : runners) delete core;

			dst.reg->type = REG_MAP;
			dst.reg->readOnly = false;
			dst.reg->m.clear();
			for (size_t i = 0; i < keys.size() && i < results.size(); i++)
				dst.reg->m[keys[i]] = results[i];
			prevDstReg = *dst.reg;

			writeBack(dst);
			release(dst); release(code); release(args);
			return RES_OK;
		}

		fail("未实现的控制流指令: " + op, st.line, st.col);
		return RES_ERR;
	}

	// 普通指令：解析操作数 → 查指令表 → 写回虚拟寄存器 → 释放临时值
	Result NSASM::execInstr(const Stmt& st, const Segment& sg, const string& segName) {
		const string& op = st.op;

		if (isDeclOp(op)) {
			if (!st.a || st.a->kind != ExprKind::Name) {
				fail(op + " 需要一个变量名", st.line, st.col);
				return RES_ERR;
			}
			// .<conf> 里的 heap 以前只被解析、从未生效，这里让它真正起作用
			if (heapManager.count(st.a->s) == 0 &&
			    static_cast<int>(heapManager.size()) >= heapSize) {
				fail("变量数超过 .<conf> 配置的 heap " + to_string(heapSize), st.line, st.col);
				return RES_ERR;
			}
			Operand name;
			name.reg = new Register();
			name.owned = true;
			name.reg->type = REG_STR;
			name.reg->readOnly = true;
			name.reg->gcFlag = true;
			name.reg->s = st.a->s;

			Operand val = resolve(st.b);
			if (val.reg == nullptr && !st.b) {
				fail(op + " 需要一个初始值", st.line, st.col);
				release(name);
				return RES_ERR;
			}
			if (val.reg == nullptr) {
				fail(val.error, st.line, st.col);
				release(name);
				return RES_ERR;
			}
			prevDstReg = *name.reg;
			Result r = funcList[op](name.reg, val.reg, nullptr);
			release(val);
			release(name);
			if (r == RES_ERR) {
				fail("声明 " + st.a->s + " 失败（类型不匹配，或该名字已被声明）", st.line, st.col);
				return RES_ERR;
			}
			return RES_OK;
		}

		auto it = funcList.find(op);
		if (it == funcList.end()) {
			fail("未知指令: " + op, st.line, st.col);
			return RES_ERR;
		}

		Operand a = resolve(st.a);
		Operand b = resolve(st.b);
		Operand c = resolve(st.c);
		if ((st.a && !a.reg) || (st.b && !b.reg) || (st.c && !c.reg)) {
			fail(!a.error.empty() ? a.error : (!b.error.empty() ? b.error : c.error), st.line, st.col);
			release(a); release(b); release(c);
			return RES_ERR;
		}

		if (a.reg != nullptr) prevDstReg = *a.reg;
		Result r = it->second(a.reg, b.reg, c.reg);

		writeBack(a);
		writeBack(b);
		writeBack(c);
		release(a);
		release(b);
		release(c);

		if (r == RES_ERR) {
			if (errMsg.empty()) fail("指令 " + op + " 执行失败（操作数类型或可写性不满足）", st.line, st.col);
			return RES_ERR;
		}
		return RES_OK;
	}

	// ------------------------------------------------------------ 操作数解析

	void NSASM::writeBack(Operand& o) {
		if (o.param.empty() || o.reg == nullptr) return;
		Register* ret = paramList[o.param](o.reg);
		// 只读 param 的「写入」会新建一个没人要的值，回收掉
		if (ret != nullptr && ret != o.reg) delete ret;
	}

	void NSASM::release(Operand& o) {
		if (o.owned) delete o.reg;
		o.reg = nullptr;
		o.owned = false;
	}

	NSASM::Operand NSASM::resolve(const ExprP& e) {
		Operand o;
		if (!e) { o.missing = true; return o; }
		if (e->kind == ExprKind::Binary || e->kind == ExprKind::Unary) return evaluate(e);

		switch (e->kind) {
		case ExprKind::Int: {
			Register* r = new Register();
			r->type = REG_INT;
			r->readOnly = true;
			r->gcFlag = true;
			r->n.i = static_cast<int>(e->i);
			o.reg = r; o.owned = true;
			break;
		}
		case ExprKind::Float: {
			Register* r = new Register();
			r->type = REG_FLOAT;
			r->readOnly = true;
			r->gcFlag = true;
			r->n.f = static_cast<float>(e->f);
			o.reg = r; o.owned = true;
			break;
		}
		case ExprKind::Char: {
			Register* r = new Register();
			r->type = REG_CHAR;
			r->readOnly = true;
			r->gcFlag = true;
			r->n.c = e->ch;
			o.reg = r; o.owned = true;
			break;
		}
		case ExprKind::Str: {
			Register* r = new Register();
			r->type = REG_STR;
			r->readOnly = true;
			r->gcFlag = true;
			r->s = e->s;
			o.reg = r; o.owned = true;
			break;
		}
		case ExprKind::Reg: {
			if (e->reg < 0 || e->reg >= static_cast<int>(regGroup.size())) {
				o.error = "寄存器编号越界: " + e->s;
				break;
			}
			o.reg = &regGroup[e->reg];
			break;
		}
		case ExprKind::Var: {
			auto pit = paramList.find(e->s);
			if (pit != paramList.end()) {
				Register* r = pit->second(nullptr);
				if (r == nullptr) { o.error = "虚拟寄存器 " + e->s + " 取值失败"; break; }
				o.reg = r;
				o.owned = true;
				o.param = e->s;
				break;
			}
			auto hit = heapManager.find(e->s);
			if (hit == heapManager.end()) {
				o.error = "未定义的变量: " + e->s;
				break;
			}
			o.reg = &hit->second;
			break;
		}
		case ExprKind::Name: {
			Register* r = new Register();
			r->type = REG_STR;
			r->readOnly = true;
			r->gcFlag = true;
			r->s = e->s;
			o.reg = r; o.owned = true;
			break;
		}
		case ExprKind::Label: {
			Register* r = new Register();
			r->type = REG_STR;
			r->readOnly = true;
			r->gcFlag = true;
			r->s = e->s;
			o.reg = r; o.owned = true;
			break;
		}
		case ExprKind::Seg: {
			Register* r = new Register();
			r->type = REG_STR;
			r->readOnly = true;
			r->gcFlag = true;
			r->s = e->s;
			o.reg = r; o.owned = true;
			break;
		}
		case ExprKind::Code: {
			o.reg = buildCode(*e);
			o.owned = true;
			break;
		}
		case ExprKind::Map: {
			o.reg = buildMap(*e);
			if (o.reg == nullptr) o.error = "映射字面量构造失败";
			else o.owned = true;
			break;
		}
		default:
			o.error = "无法解析的操作数";
			break;
		}
		return o;
	}

	Register* NSASM::buildCode(const Expr& e) {
		Register* r = new Register();
		r->type = REG_CODE;
		r->readOnly = true;
		r->gcFlag = true;
		r->s = e.s;
		return r;
	}

	// 映射字面量：在一个子解释器里跑 M(...) 里的 put 序列，跑完取其「映射寄存器」
	Register* NSASM::buildMap(const Expr& e) {
		ParseResult pr = parse(e.s);
		if (!pr.diag.ok) {
			errMsg = "映射字面量解析失败: " + pr.diag.msg;
			return nullptr;
		}
		if (callDepth > 64) {
			errMsg = "映射字面量嵌套过深";
			return nullptr;
		}
		NSASM* child = instance(*this, pr.prog);
		child->callDepth = callDepth + 1;
		child->useReg->type = REG_MAP;
		child->useReg->readOnly = false;
		child->useReg->m.clear();
		child->run();
		Register* out = new Register(*child->useReg);
		delete child;
		return out;
	}

	// 表达式求值：四则运算在这里，类型提升按「有 float 就是 float」
	NSASM::Operand NSASM::evaluate(const ExprP& e) {
		Operand o;

		if (e->kind == ExprKind::Unary) {
			Operand a = resolve(e->l);
			if (!a.reg) { o.error = a.error; return o; }
			if (!isNumeric(a.reg->type)) {
				o.error = "一元负号的操作数必须是数值";
				release(a);
				return o;
			}
			Register* r = new Register();
			r->readOnly = true;
			r->gcFlag = true;
			if (a.reg->type == REG_FLOAT) { r->type = REG_FLOAT; r->n.f = -a.reg->n.f; }
			else { r->type = REG_INT; r->n.i = -a.reg->n.i; }
			o.reg = r;
			o.owned = true;
			release(a);
			return o;
		}

		Operand l = resolve(e->l);
		Operand r = resolve(e->r);
		if (!l.reg || !r.reg) {
			o.error = !l.reg ? l.error : r.error;
			release(l);
			release(r);
			return o;
		}

		const RegType lt = l.reg->type, rt = r.reg->type;
		const bool lStr = lt == REG_STR, rStr = rt == REG_STR;
		const bool lNum = isNumeric(lt), rNum = isNumeric(rt);

		Register* out = new Register();
		out->readOnly = true;
		out->gcFlag = true;

		auto failExpr = [&](const string& msg) {
			o.error = msg;
			delete out;
			release(l);
			release(r);
			return Operand(o);
		};

		// 字符串：+ 连接，* 重复（第二操作数须为整数）
		if (lStr || rStr) {
			if (e->op == '+') {
				string s;
				if (lStr) s += l.reg->s.substr(l.reg->sp);
				else if (lt == REG_CHAR) s += l.reg->n.c;
				else return failExpr("字符串只能与字符或字符串相加");
				if (rStr) s += r.reg->s.substr(r.reg->sp);
				else if (rt == REG_CHAR) s += r.reg->n.c;
				else return failExpr("字符串只能与字符或字符串相加");
				out->type = REG_STR;
				out->s = s;
			} else if (e->op == '*' && lStr) {
				if (!rNum) return failExpr("字符串重复次数必须是整数");
				if (rt == REG_FLOAT) return failExpr("字符串重复次数必须是整数");
				const int n = static_cast<int>(r.reg->num());
				if (n < 0) return failExpr("字符串重复次数不能为负");
				string base = l.reg->s.substr(l.reg->sp);
				string s;
				for (int i = 0; i < n; i++) s += base;
				out->type = REG_STR;
				out->s = s;
			} else {
				return failExpr(string("字符串不支持运算符 ") + e->op);
			}
			o.reg = out;
			o.owned = true;
			release(l);
			release(r);
			return o;
		}

		if (!lNum || !rNum) return failExpr(string("运算符 ") + e->op + " 需要数值操作数");

		const bool asFloat = (lt == REG_FLOAT || rt == REG_FLOAT);
		if (e->op == '/' || e->op == '%') {
			const double d = r.reg->num();
			if (d == 0.0) return failExpr("除数不能为 0");
		}
		if (e->op == '%' && asFloat) return failExpr("% 只支持整数");

		if (asFloat) {
			const double a = l.reg->num(), b = r.reg->num();
			double v = 0;
			switch (e->op) {
			case '+': v = a + b; break;
			case '-': v = a - b; break;
			case '*': v = a * b; break;
			case '/': v = a / b; break;
			case '%': v = std::fmod(a, b); break;
			default: return failExpr(string("未知运算符 ") + e->op);
			}
			out->type = REG_FLOAT;
			out->n.f = static_cast<float>(v);
		} else {
			// 整数运算保持 int 语义（溢出按 32 位回绕，与初版一致）
			const long long a = static_cast<long long>(l.reg->num());
			const long long b = static_cast<long long>(r.reg->num());
			long long v = 0;
			switch (e->op) {
			case '+': v = a + b; break;
			case '-': v = a - b; break;
			case '*': v = a * b; break;
			case '/': v = a / b; break;
			case '%': v = a % b; break;
			default: return failExpr(string("未知运算符 ") + e->op);
			}
			out->type = REG_INT;
			out->n.i = static_cast<int>(v);
		}

		o.reg = out;
		o.owned = true;
		release(l);
		release(r);
		return o;
	}

	// ------------------------------------------------------------ 子解释器

	NSASM* NSASM::instance(NSASM& super, const Program& p) {
		NSASM* child = new NSASM(super.heapSize, super.stackSize, super.regCnt);
		// 继承父实例的寄存器组拷贝（与初版一致的语义）
		for (int i = 0; i < static_cast<int>(super.regGroup.size()) &&
		                i < static_cast<int>(child->regGroup.size()); i++)
			child->regGroup[i] = super.regGroup[i];
		child->append(p);
		return child;
	}

	Register* NSASM::eval(Register* reg) {
		if (reg == nullptr || reg->type != REG_CODE) return nullptr;
		if (callDepth > 64) {
			errMsg = "eval 递归过深（超过 64 层）";
			return nullptr;
		}
		ParseResult pr = parse(reg->s);
		if (!pr.diag.ok) {
			errMsg = "代码段解析失败: " + pr.diag.msg;
			return nullptr;
		}
		NSASM* child = instance(*this, pr.prog);
		child->callDepth = callDepth + 1;
		Result r = child->run();
		if (r == RES_ERR) {
			errMsg = child->errMsg.empty() ? "代码段执行出错" : child->errMsg;
			delete child;
			return nullptr;
		}
		Register* out = new Register(*child->lastResult());
		delete child;
		return out;
	}

	void NSASM::setArgument(Register* reg) {
		delete argReg;
		argReg = nullptr;
		if (reg != nullptr) argReg = new Register(*reg);
	}

}  // namespace NSASM
