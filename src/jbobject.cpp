#include <utility>

#include "jbobject.h"
#include "eval_ast.h"
#include "unicode.h"
#include "string_fmt.hpp"


bool JBValue::eq(const JBValue &rhs) const {
    return *this == rhs;
}


bool JBValue::is_truthy() const {
    return true;
}


static std::pair<bool, double> to_double(const JBValue &value) {
    if (const JBInt *obj_int = dynamic_cast<const JBInt *>(&value)) {
        return {true, obj_int->value};
    } else if (const JBFloat *obj_float = dynamic_cast<const JBFloat *>(&value)) {
        return {true, obj_float->value};
    } else {
        return {false, 0};
    }
}


template<>
bool JBInt::eq(const JBValue &rhs) const {
    auto result = to_double(rhs);
    return result.first && this->value == result.second;
}


template<>
std::string JBInt::repr() const {
    return std::to_string(this->value);
}


template<>
bool JBFloat::eq(const JBValue &rhs) const {
    auto result = to_double(rhs);
    return result.first && this->value == result.second;
}


template<>
std::string JBFloat::repr() const {
    return std::to_string(this->value);
}


template<class T>
bool value_eq(const T &lhs, const JBValue &rhs) {
    if (const T *other = dynamic_cast<const T *>(&rhs)) {
        return lhs.value == other->value;
    } else {
        return false;
    }
}


template<>
bool JBBool::eq(const JBValue &rhs) const {
    return value_eq(*this, rhs);
}


template<>
std::string JBBool::repr() const {
    return this->value ? "true" : "false";
}


bool JBString::operator==(const JBValue &rhs) const {
    return value_eq(*this, rhs);
}


static std::string quote_char(unichar ch) {
    if (ch == '"') {
        return "\\\"";
    } else if (ch == '\\') {
        return "\\\\";
    } else if (ch == '/') {
        return "/";     // no escape
    } else if (ch == '\b') {
        return "\\b";
    } else if (ch == '\f') {
        return "\\f";
    } else if (ch == '\n') {
        return "\\n";
    } else if (ch == '\t') {
        return "\\t";
    } else if (ch < 0x20) {
        return string_fmt("\\u%04x", ch);
    } else {
        return u8_encode({ch});
    }
}


std::string JBString::repr() const {
    std::string ans;
    ans.reserve(2 + this->value.size());
    ans += "\"";
    for (unichar ch : this->value) {
        ans += quote_char(ch);
    }
    ans += "\"";
    return ans;
}


bool JBString::is_truthy() const {
    return !this->value.empty();
}


std::string JBNull::repr() const {
    return "null";
}


void JBList::each_ref(std::function<void(JBObject &)> callback) {
    for (JBValue *item : this->value) {
        callback(*item);
    }
}


std::string JBList::repr() const {
    // TODO: break list into multiple line
    std::string ans;
    ans.reserve(this->value.size() * 2 + 1);
    ans += "[";
    const char *comma = "";
    for (const JBValue *item : this->value) {
        ans += comma;
        comma = ", ";
        ans += item->repr();
    }
    ans += "]";
    return ans;
}


bool JBList::is_truthy() const {
    return !this->value.empty();
}


bool JBList::operator==(const JBValue &rhs) const {
    const JBList *other = dynamic_cast<const JBList *>(&rhs);
    if (other == nullptr) {
        return false;
    }
    if (this->value.size() != other->value.size()) {
        return false;
    }
    for (size_t i = 0; i < this->value.size(); ++i) {
        if (*this->value[i] != *other->value[i]) {
            return false;
        }
    }
    return true;
}


void JBFunc::each_ref(std::function<void(JBObject &)> callback) {
    for (Frame *frame = this->parent_frame; frame != nullptr; frame = frame->parent) {
        callback(*frame);
    }
}


std::string JBFunc::repr() const {
    // TODO: more info
    return "<Func>";
}


bool JBFunc::operator==(const JBValue &rhs) const {
    return this == &rhs;
}
