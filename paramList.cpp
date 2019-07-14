#include "NSASM.h"

#include "Util.h"

#include <random>
std::default_random_engine e;
std::uniform_real_distribution<float> u(0, 1);

namespace NSASM {

    void NSASM::loadParamList() {

        paramList["null"] = $PA_{
            Register* res = new Register();
            res->type = RegType::REG_STR;
            res->gcFlag = true;
            res->s = "null";
            return res;
        };

        paramList["rand"] = $PA_{
            if (reg == nullptr) {
                Register* res = new Register();
                res->type = RegType::REG_FLOAT;
                res->readOnly = true;
                res->gcFlag = true;
                res->n.f = u(e);
                return res;
            }
            return reg;
        };

        paramList["cinc"] = $PA_{
            if (reg == nullptr) {
                Register* res = new Register();
                res->type = RegType::REG_CHAR;
                if (funcList["in"](res, nullptr, nullptr) != Result::RES_OK)
                    return nullptr;
                res->readOnly = true;
                res->gcFlag = true;
                return res;
            }
            return reg;
        };

        paramList["cini"] = $PA_{
            if (reg == nullptr) {
                Register* res = new Register();
                res->type = RegType::REG_INT;
                if (funcList["in"](res, nullptr, nullptr) != Result::RES_OK)
                    return nullptr;
                res->readOnly = true;
                res->gcFlag = true;
                return res;
            }
            return reg;
        };

        paramList["cinf"] = $PA_{
            if (reg == nullptr) {
                Register* res = new Register();
                res->type = RegType::REG_FLOAT;
                if (funcList["in"](res, nullptr, nullptr) != Result::RES_OK)
                    return nullptr;
                res->readOnly = true;
                res->gcFlag = true;
                return res;
            }
            return reg;
        };

        paramList["cins"] = $PA_{
            if (reg == nullptr) {
                Register* res = new Register();
                res->type = RegType::REG_STR;
                if (funcList["in"](res, nullptr, nullptr) != Result::RES_OK)
                    return nullptr;
                res->readOnly = true;
                res->gcFlag = true;
                return res;
            }
            return reg;
        };

        paramList["cin"] = $PA_{
            if (reg == nullptr) {
                Register* res = new Register();
                res->type = RegType::REG_STR;
                if (funcList["in"](res, nullptr, nullptr) != Result::RES_OK)
                    return nullptr;
                res->readOnly = true;
                res->gcFlag = true;
                return res;
            }
            return reg;
        };

        paramList["cout"] = $PA_{
            if (reg == nullptr) {
                Register* res = new Register();
                res->gcFlag = true;
                return res;
            }
            funcList["out"](reg, nullptr, nullptr);
            return reg;
        };

        paramList["cprt"] = $PA_{
            if (reg == nullptr) {
                Register* res = new Register();
                res->gcFlag = true;
                return res;
            }
            funcList["prt"](reg, nullptr, nullptr);
            return reg;
        };

    }

}