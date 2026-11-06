// 指令表依赖的数值运算与「文本 → 立即数」解析。
//
// calc() 这组重载是初版就有的，add/sub/mul/div/mod/and/or/xor/not/shl/shr 都用它。
// 与初版的两点差异：
//   * 整数除零 / 取模零返回 RES_ERR（原来直接触发 SIGFPE 崩掉进程）
//   * getRegister() 现在走词法+表达式解析，所以 in 指令读进来的 "1 + 2" 也能算

#include "NSASM.h"
#include "Util.h"

namespace NSASM {

	Register* NSASM::getRegister(const string& var) {
		Diag d;
		ExprP e = parseExpression(var, d);
		if (!e) return nullptr;
		Operand o = resolve(e);
		if (o.reg == nullptr || !o.error.empty()) {
			release(o);
			return nullptr;
		}
		// 调用方（in 指令）用 delete 释放，所以这里交出一个堆上的拷贝
		if (o.owned) return o.reg;
		return new Register(*o.reg);
	}

	Result NSASM::calc(int* dst, int src, char type) {
		switch (type) {
		case '+': *dst += src; break;
		case '-': *dst -= src; break;
		case '*': *dst *= src; break;
		case '/': if (src == 0) return RES_ERR; *dst /= src; break;
		case '%': if (src == 0) return RES_ERR; *dst %= src; break;
		case '&': *dst &= src; break;
		case '|': *dst |= src; break;
		case '~': *dst = ~(*dst); break;
		case '^': *dst ^= src; break;
		case '<': *dst = *dst << src; break;
		case '>': *dst = *dst >> src; break;
		default: return RES_ERR;
		}
		return RES_OK;
	}

	Result NSASM::calc(char* dst, char src, char type) {
		switch (type) {
		case '+': *dst = static_cast<char>(*dst + src); break;
		case '-': *dst = static_cast<char>(*dst - src); break;
		case '*': *dst = static_cast<char>(*dst * src); break;
		case '/': if (src == 0) return RES_ERR; *dst = static_cast<char>(*dst / src); break;
		case '%': if (src == 0) return RES_ERR; *dst = static_cast<char>(*dst % src); break;
		case '&': *dst = static_cast<char>(*dst & src); break;
		case '|': *dst = static_cast<char>(*dst | src); break;
		case '~': *dst = static_cast<char>(~(*dst)); break;
		case '^': *dst = static_cast<char>(*dst ^ src); break;
		case '<': *dst = static_cast<char>(*dst << src); break;
		case '>': *dst = static_cast<char>(*dst >> src); break;
		default: return RES_ERR;
		}
		return RES_OK;
	}

	Result NSASM::calc(float* dst, float src, char type) {
		switch (type) {
		case '+': *dst += src; break;
		case '-': *dst -= src; break;
		case '*': *dst *= src; break;
		case '/': if (src == 0.0f) return RES_ERR; *dst /= src; break;
		default: return RES_ERR;
		}
		return RES_OK;
	}

	// 字符串寄存器上的加减：移动读游标
	Result NSASM::calc(Register* dst, int src, char type) {
		switch (type) {
		case '+': dst->sp += src; break;
		case '-': dst->sp -= src; break;
		default: return RES_ERR;
		}
		if (dst->sp < 0) dst->sp = 0;
		const int len = static_cast<int>(dst->s.size());
		if (dst->sp > len) dst->sp = len;
		return RES_OK;
	}

	Result NSASM::calc(Register* dst, Register* src, char type) {
		// 一元运算：not 会传 src == nullptr（原来直接解引用 src->type，必然崩溃）
		if (src == nullptr) {
			switch (dst->type) {
			case REG_INT: return calc(&dst->n.i, 0, type);
			case REG_CHAR: return calc(&dst->n.c, static_cast<char>(0), type);
			default: return RES_ERR;
			}
		}
		switch (dst->type) {
		case REG_CHAR:
			switch (src->type) {
			case REG_CHAR: return calc(&dst->n.c, src->n.c, type);
			case REG_FLOAT: return calc(&dst->n.c, static_cast<char>(src->n.f), type);
			case REG_INT: return calc(&dst->n.c, static_cast<char>(src->n.i), type);
			default: return RES_ERR;
			}
		case REG_FLOAT:
			switch (src->type) {
			case REG_CHAR: return calc(&dst->n.f, static_cast<float>(src->n.c), type);
			case REG_FLOAT: return calc(&dst->n.f, src->n.f, type);
			case REG_INT: return calc(&dst->n.f, static_cast<float>(src->n.i), type);
			default: return RES_ERR;
			}
		case REG_INT:
			switch (src->type) {
			case REG_CHAR: return calc(&dst->n.i, static_cast<int>(src->n.c), type);
			case REG_FLOAT: return calc(&dst->n.i, static_cast<int>(src->n.f), type);
			case REG_INT: return calc(&dst->n.i, src->n.i, type);
			default: return RES_ERR;
			}
		case REG_STR:
			switch (src->type) {
			case REG_CHAR: return calc(dst, static_cast<int>(src->n.c), type);
			case REG_INT: return calc(dst, src->n.i, type);
			default: return RES_ERR;
			}
		default:
			return RES_ERR;
		}
	}

}  // namespace NSASM
