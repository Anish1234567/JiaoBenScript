#ifndef JIAOBENSCRIPT_NAME_RESOLVE_H
#define JIAOBENSCRIPT_NAME_RESOLVE_H

#include "exceptions.h"
#include "node.h"


class DuplicatedLocalName : public CompileError {
    using CompileError::CompileError;
};


class NoSuchName : public CompileError {
    using CompileError::CompileError;
};


void resolve_names_in_block(S_Block &block, Node &node);
void resolve_names(S_Block &block);


#endif //JIAOBENSCRIPT_NAME_RESOLVE_H
