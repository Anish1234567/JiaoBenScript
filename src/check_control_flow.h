#ifndef JIAOBENSCRIPT_CHECK_CONTROL_FLOW_H
#define JIAOBENSCRIPT_CHECK_CONTROL_FLOW_H

#include <string>

#include "exceptions.h"
#include "node.h"


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


void check_control_flow(S_Block &block);


#endif //JIAOBENSCRIPT_CHECK_CONTROL_FLOW_H
