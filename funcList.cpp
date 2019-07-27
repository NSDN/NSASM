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
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!verifyWord(dst->s, WordType::WD_VAR)) return Result::RES_ERR;
			if (heapManager.count(dst->s) != 0) return Result::RES_ERR;
			if (src->type != RegType::REG_CODE) return Result::RES_ERR;
			heapManager[dst->s] = *src;
			heapManager[dst->s].readOnly = false;
			return Result::RES_OK;
		};

		funcList["map"] = $OP_{
			if (src == nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (!verifyWord(dst->s, WordType::WD_VAR)) return Result::RES_ERR;
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
					Register* reg = eval(src);
					if (reg == nullptr) return Result::RES_ERR;
					*reg >> s; if (reg->gcFlag) delete reg;
				} else *dst >> s;
				Util::print(s + '\n');
			}
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
			if (src == nullptr) {
				if (dst != nullptr) prevDstReg = *dst;
				else prevDstReg = regGroup[0];
				return Result::RES_ETC;
			}
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
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->type != RegType::REG_STR) return Result::RES_ERR;
			if (!verifyWord(dst->s, WordType::WD_SEG)) return Result::RES_ERR;
			for (int seg = 0; seg < segs.size(); seg++) {
				if (segs[seg] == dst->s) {
					tmpSeg = seg; tmpCnt = 0;
					backupReg.push(progSeg);
					backupReg.push(progCnt);
					return Result::RES_OK;
				}
			}
			return Result::RES_OK;
		};

		funcList["ld"] = $OP_{
			if (src != nullptr) return Result::RES_ERR;
			if (dst == nullptr) return Result::RES_ERR;
			if (dst->type != RegType::REG_STR && dst->type != RegType::REG_CODE)
				return Result::RES_ERR;

			string path;
			if (dst->type == RegType::REG_CODE) {
				Register* reg = eval(dst);
				if (reg == nullptr) return Result::RES_ERR;
				if (reg->type != RegType::REG_STR) {
					if (reg->gcFlag) delete reg;
					return Result::RES_ERR;
				}
				path = reg->s;
				if (reg->gcFlag) delete reg;
			} else path = dst->s;

			string code = Util::read(path);
			if (code == nulstr) return Result::RES_ERR;
			auto segs = Util::getSegments(code);
			if (appendCode(segs) == Result::RES_ERR) {
				Util::print("At file: " + path + "\n");
				return Result::RES_ERR;
			}
			return Result::RES_OK;
		};

		funcList["eval"] = $OP_{
			if (dst == nullptr) return Result::RES_ERR;

			Register* reg;
			if (src == nullptr) reg = eval(dst);
			else {
				if (dst->readOnly) return Result::RES_ERR;
				reg = eval(src);
				*dst = *reg;
			}
			if (reg != nullptr) if (reg->gcFlag) delete reg;

			return Result::RES_OK;
		};

	#ifdef USE_MULTITHREAD
		funcList["par"] = $OP_{
			if (dst == nullptr) return Result::RES_ERR;
			if (src == nullptr) return Result::RES_ERR;
			if (ext == nullptr) return Result::RES_ERR;

			if (dst->readOnly) return Result::RES_ERR;
			if (src->type != RegType::REG_CODE) return Result::RES_ERR;
			if (ext->type != RegType::REG_MAP) return Result::RES_ERR;

			auto map = ext->m;
			if (map.size() != 0) {
				int cnt = map.size();
				auto code = Util::getSegments(src->s);
				vector<Register> keys;
				for (auto it = map.begin(); it != map.end(); it++)
					keys.push_back(it->first);

				vector<thread> threads;
				SafePool<NSASM*> runnerPool;
				SafePool<Register> outputPool;
				for (int i = 0; i < cnt; i++) {
					NSASM* core = instance(*this, code);
					core->setArgument(&map[keys[i]]);
					runnerPool.add(core);
				}
				for (int i = 0; i < cnt; i++)
					threads.push_back(
						thread([&](int index) -> void {
							NSASM* core = runnerPool[index];
							Register reg(*(core->run()));
							outputPool.insert(index, reg);
						}, i)
					);

				for (int i = 0; i < cnt; i++)
					threads[i].join();
				while (outputPool.count() < cnt)
					funcList["nop"](nullptr, nullptr, nullptr);

				for (int i = 0; i < cnt; i++)
					delete runnerPool[i];

				dst->type = RegType::REG_MAP;
				dst->readOnly = false;
				dst->m.clear();
				for (int i = 0; i < cnt; i++)
					dst->m[keys[i]] = outputPool[i];
			}

			return Result::RES_OK;
		};
	#endif

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
