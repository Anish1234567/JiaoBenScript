#include <algorithm>
#include <cmath>
#include <functional>
#include <iosfwd>
#include <string>

#include "builtins.h"
#include "exceptions.h"
#include "unicode.h"


// TODO: merge this module into eval_ast for better error handling


JBValue &Builtins::builtin_pos(JBValue &lhs) {
    if (dynamic_cast<JBInt *>(&lhs)) {
        return lhs;
    } else if (dynamic_cast<JBFloat *>(&lhs)) {
        return lhs;
    } else {
        throw JBError("Type error: expect number");
    }
}


JBValue &Builtins::builtin_neg(JBValue &lhs) {
    if (JBInt *int_obj = dynamic_cast<JBInt *>(&lhs)) {
        return this->create<JBInt>(-int_obj->value);
    } else if (JBFloat *float_obj = dynamic_cast<JBFloat *>(&lhs)) {
        return this->create<JBFloat>(-float_obj->value);
    } else {
        throw JBError("Type error: expect number");
    }
}


static double jb_value_to_double(JBValue &lhs) {
    if (JBInt *int_obj = dynamic_cast<JBInt *>(&lhs)) {
        return int_obj->value;
    } else if (JBFloat *float_obj = dynamic_cast<JBFloat *>(&lhs)) {
        return float_obj->value;
    } else {
        throw JBError("Type error: expect number");
    }
}


template<class IntOp, class FloatOp>
JBValue &do_simple_binop(Allocator &allocator, JBValue &lhs, JBValue &rhs) {
    if (dynamic_cast<JBInt *>(&lhs) && dynamic_cast<JBInt *>(&rhs)) {
        return *allocator.construct<JBInt>(IntOp()(
            static_cast<JBInt &>(lhs).value,
            static_cast<JBInt &>(rhs).value
        ));
    } else {
        return *allocator.construct<JBFloat>(FloatOp()(
            jb_value_to_double(lhs),
            jb_value_to_double(rhs)
        ));
    }
}

#define SIMPLE_BINOP(op_name) \
do_simple_binop<std::op_name<int64_t>, std::op_name<double>>(this->allocator, lhs, rhs)


JBValue &Builtins::builtin_add(JBValue &lhs, JBValue &rhs) {
    if (JBString *str = dynamic_cast<JBString *>(&lhs)) {
        return this->builtin_str_cat(*str, rhs);
    } else if (JBList *list = dynamic_cast<JBList *>(&lhs)) {
        return this->builtin_list_cat(*list, rhs);
    } else {
        return SIMPLE_BINOP(plus);
    }
}


JBValue& Builtins::builtin_str_cat(JBString &lhs, JBValue &rhs) {
    if (JBString *rstr = dynamic_cast<JBString *>(&rhs)) {
        return this->create<JBString>(lhs.value + rstr->value);
    } else {
        throw JBError("Type error: expect string");
    }
}


JBValue& Builtins::builtin_list_cat(JBList &lhs, JBValue &rhs) {
    if (JBList *rlist = dynamic_cast<JBList *>(&rhs)) {
        JBList &ret = this->create<JBList>();
        ret.value = lhs.value;
        ret.value.reserve(lhs.value.size() + rlist->value.size());
        ret.value.insert(ret.value.end(), rlist->value.begin(), rlist->value.end());
        return ret;
    } else {
        throw JBError("Type error: expect list");
    }
}


JBValue &Builtins::builtin_sub(JBValue &lhs, JBValue &rhs) {
    return SIMPLE_BINOP(minus);
}


JBValue &Builtins::builtin_mul(JBValue &lhs, JBValue &rhs) {
    if (JBList *llist = dynamic_cast<JBList *>(&lhs)) {
        return this->builtin_list_dup(*llist, rhs);
    } else if (JBList *rlist = dynamic_cast<JBList *>(&rhs)) {
        return this->builtin_list_dup(*rlist, lhs);
    } else {
        return SIMPLE_BINOP(multiplies);
    }
}


JBValue &Builtins::builtin_div(JBValue &lhs, JBValue &rhs) {
    if (dynamic_cast<JBInt *>(&lhs) && dynamic_cast<JBInt *>(&rhs)) {
        if (static_cast<JBInt &>(rhs).value == 0) {
            throw JBError("Arithmetic error: zero division");
        } else {
            return *allocator.construct<JBInt>(
                static_cast<JBInt &>(lhs).value / static_cast<JBInt &>(rhs).value
            );
        }
    } else {
        return *allocator.construct<JBFloat>(
            jb_value_to_double(lhs) / jb_value_to_double(rhs)
        );
    }
}


JBValue &Builtins::builtin_mod(JBValue &lhs, JBValue &rhs) {
    if (dynamic_cast<JBInt *>(&lhs) && dynamic_cast<JBInt *>(&rhs)) {
        if (static_cast<JBInt &>(rhs).value == 0) {
            throw JBError("Arithmetic error: zero remainder");
        } else {
            return *allocator.construct<JBInt>(
                static_cast<JBInt &>(lhs).value % static_cast<JBInt &>(rhs).value
            );
        }
    } else {
        return *allocator.construct<JBFloat>(
            std::remainder(jb_value_to_double(lhs), jb_value_to_double(rhs))
        );
    }
}


#define CMP_IMPL(name, OP) \
JBValue &Builtins::builtin_##name(JBValue &lhs, JBValue &rhs) { \
    return *this->allocator.construct<JBBool>( \
        jb_value_to_double(lhs) OP jb_value_to_double(rhs)); \
}

// TODO: string comparison
CMP_IMPL(lt, <)
CMP_IMPL(le, <=)
CMP_IMPL(gt, >)
CMP_IMPL(ge, >=)

#undef CMP_IMPL


JBValue &Builtins::builtin_eq(JBValue &lhs, JBValue &rhs) {
    return *this->allocator.construct<JBBool>(lhs.eq(rhs));
}


JBValue &Builtins::builtin_ne(JBValue &lhs, JBValue &rhs) {
    return *this->allocator.construct<JBBool>(!lhs.eq(rhs));
}


JBBool &Builtins::builtin_truth(JBValue &value) {
    return *this->allocator.construct<JBBool>(this->is_truthy(value));
}


JBBool &Builtins::builtin_not(JBValue &value) {
    return *this->allocator.construct<JBBool>(!this->is_truthy(value));;
}


bool Builtins::is_truthy(JBValue &value) {
    return value.is_truthy();
}


static JBValue **getitem(JBValue &base, JBValue &offset) {
    if (JBList *list = dynamic_cast<JBList *>(&base)) {
        if (JBInt *int_obj = dynamic_cast<JBInt *>(&offset)) {
            if (0 <= int_obj->value && static_cast<size_t>(int_obj->value) < list->value.size()) {
                return &list->value[int_obj->value];
            } else {
                throw JBError(string_fmt(
                    "Index error: length=%zu, index=%d", list->value.size(), int_obj->value
                ));
            }
        } else {
            throw JBError("Type error: indices must be int");
        }
    } else {
        throw JBError("Type error: expect list");
    }
}


// TODO: string
JBValue &Builtins::builtin_setitem(JBValue &base, JBValue &offset, JBValue &value) {
    *getitem(base, offset) = &value;
    return value;
}


JBValue &Builtins::builtin_getitem(JBValue &base, JBValue &offset) {
    return **getitem(base, offset);
}


JBValue &Builtins::builtin_list_dup(JBList &lhs, JBValue &n) {
    if (JBInt *int_obj = dynamic_cast<JBInt *>(&n)) {
        int64_t num = std::max(decltype(int_obj->value)(0), int_obj->value);
        JBList &ret = this->create<JBList>();
        ret.value.reserve(int_obj->value * lhs.value.size());
        for (int i = 0; i < num; ++i) {
            ret.value.insert(ret.value.end(), lhs.value.begin(), lhs.value.end());
        }
        return ret;
    } else {
        throw JBError("Type error: int expected");
    }
}


#define ARG_NUM(num) \
    if (args.size() != num) { \
        throw JBError("Type error: expect " #num " args"); \
    }


JBValue &Builtins::builtin_func_list_size(const std::vector<JBValue *> &args) {
    ARG_NUM(1);

    if (JBList *list = dynamic_cast<JBList *>(args[0])) {
        return this->create<JBInt>(list->value.size());
    } else {
        throw JBError("Type error: expect list");
    }
}


JBValue &Builtins::builtin_func_list_append(const std::vector<JBValue *> &args) {
    ARG_NUM(2);

    if (JBList *list = dynamic_cast<JBList *>(args[0])) {
        list->value.push_back(args[1]);
        return *list;
    } else {
        throw JBError("Type error: expect list");
    }
}


JBValue &Builtins::builtin_func_print(const std::vector<JBValue *> &args) {
    const char *space = "";
    for (JBValue *item : args) {
        if (JBString *str = dynamic_cast<JBString *>(item)) {
            std::cout << u8_encode(str->value);
        } else {
            std::cout << space << item->repr();
        }
        space = " ";
    }
    std::cout << std::endl;
    return this->create<JBNull>();
}


#undef ARG_NUM
