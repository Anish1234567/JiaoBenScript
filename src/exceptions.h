#ifndef JIAOBENSCRIPT_EXCEPTIONS_H
#define JIAOBENSCRIPT_EXCEPTIONS_H


#include <memory>
#include <stdexcept>
#include <string>

#include "sourcepos.h"


class BaseException : public std::runtime_error {
public:
    explicit BaseException(
        const std::string &msg,
        const SourcePos &start = SourcePos(), const SourcePos &end = SourcePos()
    )
        : std::runtime_error(msg), start(start), end(end)
    {}

    SourcePos start;
    SourcePos end;
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


class JBError : public BaseException {
public:
    using BaseException::BaseException;
};


#endif //JIAOBENSCRIPT_EXCEPTIONS_H
