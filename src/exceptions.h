#ifndef JIAOBENSCRIPT_EXCEPTIONS_H
#define JIAOBENSCRIPT_EXCEPTIONS_H


#include <memory>
#include <stdexcept>
#include <string>

#include "sourcepos.h"
#include "node.h"


class BaseException : public std::runtime_error {
public:
    explicit BaseException(
        const std::string &msg,
        const SourcePos &pos_start = SourcePos(), const SourcePos &pos_end = SourcePos()
    )
        : std::runtime_error(msg), pos_start(pos_start), pos_end(pos_end)
    {}

    SourcePos pos_start;
    SourcePos pos_end;
};


class TokenizerError : public BaseException {
public:
    using BaseException::BaseException;
};


class ParserError : public BaseException {
public:
    using BaseException::BaseException;
};


class CompileError : public BaseException {
public:
    using BaseException::BaseException;
};

template<class NodeType>
class _BadStmt : public CompileError {
public:
    _BadStmt<NodeType>(const NodeType &node, const std::string &msg)
        : CompileError(msg, node.pos_start, node.pos_end), node(node)
    {}
    const NodeType &node;
};


class BadReturn : public _BadStmt<S_Return> {
public:
    BadReturn(const S_Return &node) : _BadStmt<S_Return>(node, "Bad return") {}
};


class BadBreak : public _BadStmt<S_Break> {
public:
    BadBreak(const S_Break &node) : _BadStmt<S_Break>(node, "Bad break") {}
};


class BadContinue : public _BadStmt<S_Continue> {
public:
    BadContinue(const S_Continue &node) : _BadStmt<S_Continue>(node, "Bad return") {}
};


class JBError : public BaseException {
public:
    using BaseException::BaseException;
};


#endif //JIAOBENSCRIPT_EXCEPTIONS_H
