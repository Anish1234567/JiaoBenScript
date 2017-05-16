#ifndef JIAOBENSCRIPT_BUILTINS_H
#define JIAOBENSCRIPT_BUILTINS_H

#include <vector>

#include "jbobject.h"
#include "allocator.h"


class Builtins {
public:
    Builtins(Allocator &allocator) : allocator(allocator) {}

    JBValue &builtin_pos(JBValue &lhs);
    JBValue &builtin_neg(JBValue &lhs);
    JBValue &builtin_add(JBValue &lhs, JBValue &rhs);
    JBValue &builtin_sub(JBValue &lhs, JBValue &rhs);
    JBValue &builtin_mul(JBValue &lhs, JBValue &rhs);
    JBValue &builtin_div(JBValue &lhs, JBValue &rhs);
    JBValue &builtin_mod(JBValue &lhs, JBValue &rhs);
    JBValue &builtin_lt(JBValue &lhs, JBValue &rhs);
    JBValue &builtin_le(JBValue &lhs, JBValue &rhs);
    JBValue &builtin_gt(JBValue &lhs, JBValue &rhs);
    JBValue &builtin_ge(JBValue &lhs, JBValue &rhs);
    JBValue &builtin_eq(JBValue &lhs, JBValue &rhs);
    JBValue &builtin_ne(JBValue &lhs, JBValue &rhs);
    JBBool  &builtin_truth(JBValue &value);
    JBBool  &builtin_not(JBValue &value);
    bool is_truthy(JBValue &value);
    JBValue &builtin_setitem(JBValue &base, JBValue &offset, JBValue &value);
    JBValue &builtin_getitem(JBValue &base, JBValue &offset);
    JBValue &builtin_print(const std::vector<JBValue *> &args);

private:
    // FIXME: duplicated code
    template<class T, class ...Args>
    T &create(Args &&...args) {
        return *this->allocator.construct<T>(std::forward<Args>(args)...);
    }

    Allocator &allocator;
};


#endif //JIAOBENSCRIPT_BUILTINS_H
