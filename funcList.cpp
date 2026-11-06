#include "NSASM.h"

#include "Util.h"

#ifdef USE_MULTITHREAD
#include <thread>
#endif

namespace NSASM {

	// 三操作数（重载）形式：dst = src op ext
	// 旧实现是 push/mutate/mov/pop 微码，要求 src 可写：字面量、表达式结果、
	// 只读全局参数（rand/null/cin* 等）都会失败，而且 dst == src 时 pop 会把结果还原掉。
	// 现在改成「复制一份可写的 src，算完再 mov 到 dst」，src 本身不再被改写。
	Result NSASM::triOp(const string& op, Register* dst, Register* src, Register* ext) {
		if (dst == nullptr || src == nullptr || ext == nullptr) return Result::RES_ERR;
		Register tmp(*src);   // 拷贝构造不继承 gcFlag，也不共享所有权
		tmp.readOnly = false;
		if (funcList[op](&tmp, ext, nullptr) == Result::RES_ERR)
			return Result::RES_ERR;
		return funcList["mov"](dst, &tmp, nullptr);
	}

	void NSASM::loadFuncList() {

		funcList["rem"] = $OP_{
			return Result::RES_OK;
		};

		funcList["var"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!isDeclarableName(dst->s)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			if (heapManager[dst->s].type != RegType::REG_STR)
				heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["int"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!isDeclarableName(dst->s)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_INT) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["char"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!isDeclarableName(dst->s)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_CHAR) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["float"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!isDeclarableName(dst->s)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_FLOAT) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["str"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!isDeclarableName(dst->s)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_STR) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = true;
			return Result::RES_OK;
		};

		funcList["code"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!isDeclarableName(dst->s)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_CODE) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["map"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!isDeclarableName(dst->s)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_MAP) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["mov"] = $OP_{
			if (ext != nullptr) {
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (dst->type == RegType::REG_CHAR && src->type == RegType::REG_STR) {
				dst->n.c = src->s[src->sp];
			} else if (dst->type == RegType::REG_STR && src->type == RegType::REG_CHAR) {
				dst->s[dst->sp] = src->n.c;
			} else {
				*dst = *src;
				if (dst->readOnly) dst->readOnly = false;
			}
			return Result::RES_OK;
		};

		funcList["push"] = $OP_{
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (stackManager.size() >= stackSize) return Result::RES_ERR;
			stackManager.push(*dst);
			return Result::RES_OK;
		};

		funcList["pop"] = $OP_{
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			*dst = stackManager.top(); stackManager.pop();
			return Result::RES_OK;
		};

		funcList["in"] = $OP_{
			string buf; Register* reg;
			if (src == nullptr) {
				if (dst == nullptr) return Result::RES_ERR;
				if (dst->readOnly && dst->type != RegType::REG_STR) return Result::RES_ERR;
				buf = Util::scan();
				switch (dst->type) {
				case RegType::REG_INT:
					reg = getRegister(buf);
					if (reg == nullptr) return Result::RES_OK;
					if (reg->type != RegType::REG_INT) return Result::RES_OK;
					dst->n.i = reg->n.i;
					delete reg;
					break;
				case RegType::REG_CHAR:
					if (buf.length() < 1) return Result::RES_OK;
					dst->n.c = buf[0];
					break;
				case RegType::REG_FLOAT:
					reg = getRegister(buf);
					if (reg == nullptr) return Result::RES_OK;
					if (reg->type != RegType::REG_FLOAT) return Result::RES_OK;
					dst->n.f = reg->n.f;
					delete reg;
					break;
				case RegType::REG_STR:
					if (buf.length() < 1) return Result::RES_OK;
					dst->s = buf;
					dst->sp = 0;
					break;
				}
			} else {
				if (dst == nullptr) return Result::RES_ERR;
				if (src->type != RegType::REG_INT) return Result::RES_ERR;
				switch (src->n.i) {
				case 0x00:
					if (dst->readOnly && dst->type != RegType::REG_STR) return Result::RES_ERR;
					buf = Util::scan();
					switch (dst->type) {
					case RegType::REG_INT:
						reg = getRegister(buf);
						if (reg == nullptr) return Result::RES_OK;
						if (reg->type != RegType::REG_INT) return Result::RES_OK;
						dst->n.i = reg->n.i;
						delete reg;
						break;
					case RegType::REG_CHAR:
						if (buf.length() < 1) return Result::RES_OK;
						dst->n.c = buf[0];
						break;
					case RegType::REG_FLOAT:
						reg = getRegister(buf);
						if (reg == nullptr) return Result::RES_OK;
						if (reg->type != RegType::REG_FLOAT) return Result::RES_OK;
						dst->n.f = reg->n.f;
						delete reg;
						break;
					case RegType::REG_STR:
						if (buf.length() < 1) return Result::RES_OK;
						dst->s = buf;
						dst->sp = 0;
						break;
					}
					break;
				case 0xFF:
					Util::print("[DEBUG] <<< ");
					if (dst->readOnly && dst->type != RegType::REG_STR) return Result::RES_ERR;
					buf = Util::scan();
					switch (dst->type) {
					case RegType::REG_INT:
						reg = getRegister(buf);
						if (reg == nullptr) return Result::RES_OK;
						if (reg->type != RegType::REG_INT) return Result::RES_OK;
						dst->n.i = reg->n.i;
						delete reg;
						break;
					case RegType::REG_CHAR:
						if (buf.length() < 1) return Result::RES_OK;
						dst->n.c = buf[0];
						break;
					case RegType::REG_FLOAT:
						reg = getRegister(buf);
						if (reg == nullptr) return Result::RES_OK;
						if (reg->type != RegType::REG_FLOAT) return Result::RES_OK;
						dst->n.f = reg->n.f;
						delete reg;
						break;
					case RegType::REG_STR:
						if (buf.length() < 1) return Result::RES_OK;
						dst->s = buf;
						dst->sp = 0;
						break;
					}
					break;
				default:
					return Result::RES_ERR;
				}
			}
			return Result::RES_OK;
		};

		funcList["out"] = $OP_{
			if (dst == nullptr) return Result::RES_ERR;
			string s = "";
			if (src == nullptr) {
				if (dst->type == RegType::REG_CODE) {
					Register* reg = eval(dst);
					if (reg == nullptr) return Result::RES_ERR;
					*reg >> s; if (reg->gcFlag) delete reg;
				} else *dst >> s;
				Util::print(s);
			} else {
				if (dst->type != RegType::REG_INT) return Result::RES_ERR;
				switch (dst->n.i) {
				case 0x00:
					if (src->type == RegType::REG_CODE) {
						Register* reg = eval(src);
						if (reg == nullptr) return Result::RES_ERR;
						*reg >> s; if (reg->gcFlag) delete reg;
					} else *src >> s;
					Util::print(s);
					break;
				case 0xFF:
					Util::print("[DEBUG] >>> ");
					if (src->type == RegType::REG_CODE) {
						Register* reg = eval(src);
						if (reg == nullptr) return Result::RES_ERR;
						*reg >> s; if (reg->gcFlag) delete reg;
					} else *src >> s;
					Util::print(s);
					Util::print('\n');
					break;
				default:
					return Result::RES_ERR;
				}
			}
			return Result::RES_OK;
		};

		funcList["prt"] = $OP_{
			if (dst == nullptr) return Result::RES_ERR;
			stringstream ss; string s;
			if (src != nullptr) {
				if (ext != nullptr) {
					string a, b, c;
					*dst >> a; *src >> b; *ext >> c;
					Util::print(a + b + c + '\n');
					return Result::RES_OK;
				}
				if (dst->type == RegType::REG_STR) {
					if (dst->readOnly) return Result::RES_ERR;
					if (src->type == RegType::REG_CHAR && src->n.c == '\b') {
						if (dst->s.find('\n') != dst->s.npos) {
							string buf = "", res = "";
							ss << dst->s; int count = 0;
							while (!ss.eof()) {
								getline(ss, buf);
								count += 1;
							}
							ss.clear(); ss << dst->s;
							for (int i = 0; i < count - 1; i++) {
								getline(ss, buf);
								res += buf;
								if (i < count - 2) res += "\n";
							}
							dst->s = res;
						}
					} else if (src->type == RegType::REG_CODE) {
						Register* reg = eval(src);
						if (reg == nullptr) return Result::RES_ERR;
						*reg >> s; if (reg->gcFlag) delete reg;
						dst->s += ("\n" + s);
					} else if (src->type == RegType::REG_STR) {
						dst->s += ("\n" + src->s.substr(src->sp));
					} else return Result::RES_ERR;
				} else if (dst->type == RegType::REG_CODE) {
					if (dst->readOnly) return Result::RES_ERR;
					if (src->type == RegType::REG_CHAR && src->n.c == '\b') {
						if (dst->s.find('\n') != dst->s.npos) {
							string buf = "", res = "";
							ss << dst->s; int count = 0;
							while (!ss.eof()) {
								getline(ss, buf);
								count += 1;
							}
							ss.clear(); ss << dst->s;
							for (int i = 0; i < count - 1; i++) {
								getline(ss, buf);
								res += buf;
								if (i < count - 2) res += "\n";
							}
							dst->s = res;
						}
					} else if (src->type == RegType::REG_CODE) {
						dst->s += ("\n" + src->s);
					} else if (src->type == RegType::REG_STR) {
						dst->s += ("\n" + src->s.substr(src->sp));
					} else return Result::RES_ERR;
				} else return Result::RES_ERR;
			} else {
				if (dst == nullptr) return Result::RES_ERR;
				if (dst->type == RegType::REG_CODE) {
					// 原来是 eval(src)（这里 src 必为 null），导致 prt <代码段> 永远失败
					Register* reg = eval(dst);
					if (reg == nullptr) return Result::RES_ERR;
					*reg >> s; if (reg->gcFlag) delete reg;
				} else *dst >> s;
				Util::print(s + '\n');
			}
			return Result::RES_OK;
		};

		funcList["add"] = $OP_{
			if (ext != nullptr) return triOp("add", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				Result res =  calc(dst, reg, '+');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '+');
		};

		funcList["inc"] = $OP_{
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			Register reg;
			reg.type = RegType::REG_INT;
			reg.n.i = 0x01;
			reg.readOnly = true;
			reg.gcFlag = true;
			return calc(dst, &reg, '+');
		};

		funcList["sub"] = $OP_{
			if (ext != nullptr) return triOp("sub", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				Result res = calc(dst, reg, '-');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '-');
		};

		funcList["dec"] = $OP_{
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			Register reg;
			reg.type = RegType::REG_INT;
			reg.n.i = 0x01;
			reg.readOnly = true;
			reg.gcFlag = true;
			return calc(dst, &reg, '-');
		};

		funcList["mul"] = $OP_{
			if (ext != nullptr) return triOp("mul", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				Result res = calc(dst, reg, '*');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '*');
		};

		funcList["div"] = $OP_{
			if (ext != nullptr) return triOp("div", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				Result res = calc(dst, reg, '/');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '/');
		};

		funcList["mod"] = $OP_{
			if (ext != nullptr) return triOp("mod", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				Result res = calc(dst, reg, '%');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '%');
		};

		funcList["and"] = $OP_{
			if (ext != nullptr) return triOp("and", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				Result res = calc(dst, reg, '&');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '&');
		};

		funcList["or"] = $OP_{
			if (ext != nullptr) return triOp("or", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				Result res = calc(dst, reg, '|');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '|');
		};

		funcList["xor"] = $OP_{
			if (ext != nullptr) return triOp("xor", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				Result res = calc(dst, reg, '^');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '^');
		};

		funcList["not"] = $OP_{
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			return calc(dst, nullptr, '~');
		};

		funcList["shl"] = $OP_{
			if (ext != nullptr) return triOp("shl", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				Result res = calc(dst, reg, '<');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '<');
		};

		funcList["shr"] = $OP_{
			if (ext != nullptr) return triOp("shr", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				Result res = calc(dst, reg, '>');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '>');
		};

		funcList["cmp"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (funcList["mov"](&stateReg, dst, nullptr) == Result::RES_ERR)
				return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				if (funcList["sub"](&stateReg, reg, nullptr) == Result::RES_ERR) {
					if (reg != nullptr) if (reg->gcFlag) delete reg;
					return Result::RES_ERR;
				}
				if (reg != nullptr) if (reg->gcFlag) delete reg;
			} else {
				if (funcList["sub"](&stateReg, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
			}
			return Result::RES_OK;
		};

		funcList["test"] = $OP_{
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->type == RegType::REG_CODE) {
				Register* reg = eval(dst);
				if (reg == nullptr) return Result::RES_ERR;
				if (funcList["mov"](&stateReg, reg, nullptr) == Result::RES_ERR) {
					if (reg != nullptr) if (reg->gcFlag) delete reg;
					return Result::RES_ERR;
				}
				if (reg != nullptr) if (reg->gcFlag) delete reg;
			} else {
				if (funcList["mov"](&stateReg, dst, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
			}
			Register zero;
			zero.type = RegType::REG_INT; zero.readOnly = false;
			zero.n.i = 0; zero.gcFlag = true;
			if (funcList["sub"](&stateReg, &zero, nullptr) == Result::RES_ERR)
				return Result::RES_ERR;
			return Result::RES_OK;
		};
		

		funcList["use"] = $OP_{
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (dst->gcFlag) return Result::RES_ERR;
			if (dst->type != RegType::REG_MAP) return Result::RES_ERR;
			useReg = dst;
			return Result::RES_OK;
		};

		funcList["put"] = $OP_{
			if (ext != nullptr) {
				if (funcList["use"](dst, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["put"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (useReg == nullptr) return Result::RES_ERR;
			if (useReg->type != RegType::REG_MAP) return Result::RES_ERR;
			// 映射不能当键：Register::operator< 对所有 map 都判「相等」，
			// 允许的话会静默覆盖前一个键，属于丢数据，这里直接拒绝。
			// 只拒绝 dst（键）：映射作为「值」是合法的（嵌套映射）。
			if (dst->type == RegType::REG_MAP) return Result::RES_ERR;
			if (dst->type == RegType::REG_CODE) {
				Register* reg = eval(dst);
				if (reg == nullptr) return Result::RES_ERR;
				useReg->m[*reg] = *src;
				if (reg != nullptr) if (reg->gcFlag) delete reg;
			} else useReg->m[*dst] = *src;

			return Result::RES_OK;
		};

		funcList["get"] = $OP_{
			if (ext != nullptr) {
				if (funcList["use"](dst, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["get"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (useReg == nullptr) return Result::RES_ERR;
			if (useReg->type != RegType::REG_MAP) return Result::RES_ERR;

			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				if (reg == nullptr) return Result::RES_ERR;
				if (useReg->m.count(*reg) == 0) return Result::RES_ERR;
				Result res = funcList["mov"](dst, &useReg->m[*reg], nullptr);
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else {
				if (useReg->m.count(*src) == 0) return Result::RES_ERR;
				return funcList["mov"](dst, &useReg->m[*src], nullptr);
			}
		};

		funcList["cat"] = $OP_{
			if (ext != nullptr) return triOp("cat", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			switch (dst->type) {
			case RegType::REG_STR:
				if (src->type != RegType::REG_STR)
					return Result::RES_ERR;
				dst->s += src->s;
				break;
			case RegType::REG_MAP:
				if (src->type != RegType::REG_MAP)
					return Result::RES_ERR;
				for (auto it = src->m.begin(); it != src->m.end(); it++)
					dst->m[it->first] = it->second;
				break;
			default:
				return Result::RES_ERR;
			}
			return Result::RES_OK;
		};

		funcList["dog"] = $OP_{
			if (ext != nullptr) return triOp("dog", dst, src, ext);
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			switch (dst->type) {
			case RegType::REG_STR:
				if (src->type != RegType::REG_STR)
					return Result::RES_ERR;
				Util::replace(dst->s, src->s, "");
				break;
			case RegType::REG_MAP:
				if (src->type != RegType::REG_MAP)
					return Result::RES_ERR;
				for (auto it = src->m.begin(); it != src->m.end(); it++)
					if (dst->m.count(it->first) != 0)
						dst->m.erase(it->first);
				break;
			default:
				return Result::RES_ERR;
			}
			return Result::RES_OK;
		};

		funcList["type"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			
			Register reg;
			reg.type = RegType::REG_STR;
			reg.readOnly = true; reg.gcFlag = true;
			switch (src->type) {
			case RegType::REG_INT: reg.s = "int"; break;
			case RegType::REG_CHAR: reg.s = "char"; break;
			case RegType::REG_FLOAT: reg.s = "float"; break;
			case RegType::REG_STR: reg.s = "str"; break;
			case RegType::REG_CODE: reg.s = "code"; break;
			case RegType::REG_MAP: reg.s = "map"; break;
			case RegType::REG_PAR: reg.s = "par"; break;
			case RegType::REG_NUL: reg.s = "nul"; break;
			}
			return funcList["mov"](dst, &reg, nullptr);
		};

		funcList["len"] = $OP_{
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			Register reg;
			reg.type = RegType::REG_INT;
			reg.readOnly = true; reg.gcFlag = true;
			if (src == nullptr) {
				if (useReg == nullptr) return Result::RES_ERR;
				if (useReg->type != RegType::REG_MAP) return Result::RES_ERR;
				reg.n.i = useReg->m.size();
			} else {
				if (src->type != RegType::REG_STR) return Result::RES_ERR;
				reg.n.i = src->s.length();
			}
			return funcList["mov"](dst, &reg, nullptr);
		};

		funcList["ctn"] = $OP_{
			if (dst == nullptr) return Result::RES_ERR;
			Register reg;
			reg.type = RegType::REG_INT;
			reg.readOnly = true; reg.gcFlag = true;
			if (src == nullptr) {
				if (useReg == nullptr) return Result::RES_ERR;
				if (useReg->type != RegType::REG_MAP) return Result::RES_ERR;
				reg.n.i = useReg->m.count(*dst);
			} else {
				if (src->type != RegType::REG_STR) return Result::RES_ERR;
				if (dst->type != RegType::REG_STR) return Result::RES_ERR;
				reg.n.i = dst->s.find(src->s) != dst->s.npos ? 1 : 0;
			}
			return funcList["mov"](&stateReg, &reg, nullptr);
		};

		funcList["equ"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (src->type != RegType::REG_STR) return Result::RES_ERR;
			if (dst->type != RegType::REG_STR) return Result::RES_ERR;
			Register reg;
			reg.type = RegType::REG_INT;
			reg.readOnly = true; reg.gcFlag = true;
			reg.n.i = dst->s == src->s ? 0 : 1;
			return funcList["mov"](&stateReg, &reg, nullptr);
		};
		
	}

}
