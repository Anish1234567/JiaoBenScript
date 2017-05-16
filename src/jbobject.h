#ifndef JIAOBENSCRIPT_JBOBJECT_H
#define JIAOBENSCRIPT_JBOBJECT_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "node.h"
#include "unicode.h"


class JBObject {
public:
    virtual ~JBObject() {}
    virtual void each_ref(std::function<void (JBObject &)>) {}
};


class JBValue : public JBObject {
public:
    virtual bool eq(const JBValue &rhs) const;
    virtual bool is_truthy() const;
    virtual std::string repr() const = 0;

    virtual bool operator==(const JBValue &rhs) const = 0;
    bool operator!=(const JBValue &rhs) const {
        return !(*this == rhs);
    }
};


template<class T>
class _JBSimpeValue : public JBValue {
public:
    explicit _JBSimpeValue<T>(T value) : value(value) {}
    virtual bool eq(const JBValue &rhs) const override;
    virtual bool is_truthy() const override {
        return this->value != 0;
    }
    virtual std::string repr() const override;

    virtual bool operator==(const JBValue &rhs) const override {
        const _JBSimpeValue<T> *other = dynamic_cast<const _JBSimpeValue<T> *>(&rhs);
        return other != nullptr && this->value == other->value;
    }

    const T value;
};


typedef _JBSimpeValue<int64_t> JBInt;
typedef _JBSimpeValue<double> JBFloat;
typedef _JBSimpeValue<bool> JBBool;


class JBString : public JBValue {
public:
    explicit JBString(const ustring &value) : value(value) {}

    virtual bool is_truthy() const override;
    virtual std::string repr() const override;
    virtual bool operator==(const JBValue &rhs) const override;

    const ustring value;
};


class JBNull : public JBValue {
public:
    virtual bool is_truthy() const override {
        return false;
    }
    virtual std::string repr() const override;
    virtual bool operator==(const JBValue &rhs) const override {
        return dynamic_cast<const JBNull *>(&rhs) != nullptr;
    }
};


class JBList : public JBValue {
public:
    virtual void each_ref(std::function<void (JBObject &)> callback);

    virtual bool is_truthy() const override;
    virtual std::string repr() const override;
    virtual bool operator==(const JBValue &rhs) const override;

    std::vector<JBValue *> value;
};


class Frame;


class JBFunc : public JBValue {
public:
    JBFunc(Frame *frame, const E_Func &code)
        : parent_frame(frame), code(code)
    {}
    virtual void each_ref(std::function<void (JBObject &)> callback);
    virtual std::string repr() const override;
    virtual bool operator==(const JBValue &rhs) const override;

    Frame *parent_frame;    // optional if function do not have closure
                            // (func.block and its children do not have non-local variable)
    const E_Func &code;
    // TODO: function name
};


class JBBuiltinFunc : public JBValue {
public:
    typedef std::function<JBValue &(const std::vector<JBValue *> &)> Func;

    JBBuiltinFunc(Func func) : func(func) {}

    virtual std::string repr() const override;
    virtual bool operator==(const JBValue &rhs) const override;

    Func func;
};


#endif //JIAOBENSCRIPT_JBOBJECT_H
