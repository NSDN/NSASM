#include "NSASM.h"

#include "Util.h"

#ifdef USE_MULTITHREAD
#include <thread>
#endif

namespace NSASM {

	void NSASM::loadFuncList() {

		funcList["rem"] = $OP_{
			return Result::RES_OK;
		};

		funcList["var"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!verifyWord(dst->s, WordType::WD_VAR)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			if (heapManager[dst->s].type != RegType::REG_STR)
				heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["int"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!verifyWord(dst->s, WordType::WD_VAR)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_INT) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["char"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!verifyWord(dst->s, WordType::WD_VAR)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_CHAR) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["float"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!verifyWord(dst->s, WordType::WD_VAR)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_FLOAT) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["str"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!verifyWord(dst->s, WordType::WD_VAR)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_STR) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = true;
			return Result::RES_OK;
		};

		funcList["code"] = $OP_{
			return Result::RES_ERR;
		};

		funcList["map"] = $OP_{
			return Result::RES_ERR;
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
					
					break;
				case 0xFF:
					
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
				*dst >> s;
				Util::print(s);
			} else {
				if (dst->type != RegType::REG_INT) return Result::RES_ERR;
				switch (dst->n.i) {
				case 0x00:
					
					break;
				case 0xFF:
					
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
			*dst >> s;
			Util::print(s + '\n');
			return Result::RES_OK;
		};

		funcList["add"] = $OP_{
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["add"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
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
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["sub"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
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
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mul"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				Result res = calc(dst, reg, '*');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '*');
		};

		funcList["div"] = $OP_{
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["div"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				Result res = calc(dst, reg, '/');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '/');
		};

		funcList["mod"] = $OP_{
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mod"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				Result res = calc(dst, reg, '%');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '%');
		};

		funcList["and"] = $OP_{
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["and"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				Result res = calc(dst, reg, '&');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '&');
		};

		funcList["or"] = $OP_{
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["or"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				Result res = calc(dst, reg, '|');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '|');
		};

		funcList["xor"] = $OP_{
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["xor"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
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
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["shl"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
				Result res = calc(dst, reg, '<');
				if (reg != nullptr) if (reg->gcFlag) delete reg;
				return res;
			} else return calc(dst, src, '<');
		};

		funcList["shr"] = $OP_{
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["shr"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type == RegType::REG_CODE) {
				Register* reg = eval(src);
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
		
		funcList["jmp"] = $OP_{
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->type != RegType::REG_STR) return Result::RES_ERR;
			if (!verifyWord(dst->s, WordType::WD_TAG)) return Result::RES_ERR;
			for (int seg = 0; seg < segs.size(); seg++) {
				if (code[segs[seg]].empty()) continue;
				for (int line = 0; line < code[segs[seg]].size(); line++) {
					if (code[segs[seg]][line] == dst->s) {
						tmpSeg = seg; tmpCnt = line;
						return Result::RES_OK;
					}
				}
			}
			return Result::RES_ERR;
		};

		funcList["jz"] = $OP_{
			stringstream ss; ss.clear();
			string s; stateReg >> s;
			ss << s; float f = 0.0F; ss >> f;
			if (f == 0.0F) {
				return funcList["jmp"](dst, src, nullptr);
			}
			return Result::RES_OK;
		};

		funcList["jnz"] = $OP_{
			stringstream ss; ss.clear();
			string s; stateReg >> s;
			ss << s; float f = 0.0F; ss >> f;
			if (f != 0.0F) {
				return funcList["jmp"](dst, src, nullptr);
			}
			return Result::RES_OK;
		};

		funcList["jg"] = $OP_{
			stringstream ss; ss.clear();
			string s; stateReg >> s;
			ss << s; float f = 0.0F; ss >> f;
			if (f > 0.0F) {
				return funcList["jmp"](dst, src, nullptr);
			}
			return Result::RES_OK;
		};

		funcList["jl"] = $OP_{
			stringstream ss; ss.clear();
			string s; stateReg >> s;
			ss << s; float f = 0.0F; ss >> f;
			if (f < 0.0F) {
				return funcList["jmp"](dst, src, nullptr);
			}
			return Result::RES_OK;
		};

		funcList["loop"] = $OP_{
			if (dst == nullptr) return Result::RES_ERR;
			if (src == nullptr) return Result::RES_ERR;
			if (ext == nullptr) return Result::RES_ERR;

			if (dst->type != RegType::REG_INT) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			if (src->type != RegType::REG_INT) return Result::RES_ERR;
			if (ext->type != RegType::REG_STR) return Result::RES_ERR;
			if (!verifyWord(ext->s, WordType::WD_TAG)) return Result::RES_ERR;

			if (src->n.i > 0) {
				if (funcList["inc"](dst, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
			} else {
				if (funcList["dec"](dst, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
			}
			if (funcList["cmp"](dst, src, nullptr) == Result::RES_ERR)
				return Result::RES_ERR;
			if (funcList["jnz"](ext, nullptr, nullptr) == Result::RES_ERR)
				return Result::RES_ERR;
			
			return Result::RES_OK;
		};

		funcList["end"] = $OP_{
			if (dst == nullptr && src == nullptr)
				return Result::RES_ETC;
			return Result::RES_ERR;
		};

		funcList["ret"] = $OP_{
			return Result::RES_ERR;
		};

		funcList["nop"] = $OP_{
			if (dst == nullptr && src == nullptr)
				return Result::RES_OK;
			return Result::RES_ERR;
		};

		funcList["rst"] = $OP_{
			if (dst == nullptr && src == nullptr) {
				tmpSeg = 0; tmpCnt = 0;
				return Result::RES_OK;
			}
			return Result::RES_ERR;
		};

		funcList["run"] = $OP_{
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->type != RegType::REG_STR) return Result::RES_ERR;
			if (!verifyWord(dst->s, WordType::WD_SEG)) return Result::RES_ERR;
			for (int seg = 0; seg < segs.size(); seg++) {
				if (segs[seg] == dst->s) {
					tmpSeg = seg; tmpCnt = 0;
					return Result::RES_OK;
				}
			}
			return Result::RES_ERR;
		};

		funcList["call"] = $OP_{
			
			return Result::RES_ERR;
		};

		funcList["ld"] = $OP_{
			return Result::RES_ERR;
		};

		funcList["eval"] = $OP_{
			return Result::RES_ERR;
		};

	#ifdef USE_MULTITHREAD
		funcList["par"] = $OP_{
			return Result::RES_ERR;
		};
	#endif

		funcList["use"] = $OP_{
			return Result::RES_ERR;
		};

		funcList["put"] = $OP_{
			return Result::RES_ERR;
		};

		funcList["get"] = $OP_{
			return Result::RES_ERR;
		};

		funcList["cat"] = $OP_{
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["cat"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			switch (dst->type) {
			case RegType::REG_STR:
				if (src->type != RegType::REG_STR)
					return Result::RES_ERR;
				dst->s += src->s;
				break;
			default:
				return Result::RES_ERR;
			}
			return Result::RES_OK;
		};

		funcList["dog"] = $OP_{
			if (ext != nullptr) {
				if (funcList["push"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["dog"](src, ext, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["mov"](dst, src, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				if (funcList["pop"](src, nullptr, nullptr) == Result::RES_ERR)
					return Result::RES_ERR;
				return Result::RES_OK;
			}
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			switch (dst->type) {
			case RegType::REG_STR:
				if (src->type != RegType::REG_STR)
					return Result::RES_ERR;
				Util::replace(dst->s, src->s, "");
				break;
			default:
				return Result::RES_ERR;
			}
			return Result::RES_OK;
		};

		funcList["type"] = $OP_{
			return Result::RES_ERR;
		};

		funcList["len"] = $OP_{
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->readOnly) return Result::RES_ERR;
			Register reg;
			reg.type = RegType::REG_INT;
			reg.readOnly = true; reg.gcFlag = true;
			if (src->type != RegType::REG_STR) return Result::RES_ERR;
			reg.n.i = src->s.length();
			return funcList["mov"](dst, &reg, nullptr);
		};

		funcList["ctn"] = $OP_{
			if (dst == nullptr) return Result::RES_ERR;
			Register reg;
			reg.type = RegType::REG_INT;
			reg.readOnly = true; reg.gcFlag = true;
			if (src->type != RegType::REG_STR) return Result::RES_ERR;
			if (dst->type != RegType::REG_STR) return Result::RES_ERR;
			reg.n.i = dst->s.find(src->s) != dst->s.npos ? 1 : 0;
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
