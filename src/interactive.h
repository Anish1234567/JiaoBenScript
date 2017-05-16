#ifndef JIAOBENSCRIPT_INTERACTIVE_H
#define JIAOBENSCRIPT_INTERACTIVE_H

#include <string>
#include <vector>

#include "tokenizer.h"
#include "parser.h"
#include "eval_ast.h"
#include "node.h"


class InteractiveRepl {
public:
    InteractiveRepl();

    void start();
private:
    void feed(const std::string &line);
    void print_start_info();
    std::string get_input_prompt();
    void print_result(JBValue &value);
    std::string read_line(const std::string &prompt);
    bool is_ready() const;

    bool eof = false;
    int count = 0;
    Tokenizer tokenizer;
    Parser parser;
    AstInterpreter interp;
    std::vector<Node::Ptr> nodes;
};


#endif //JIAOBENSCRIPT_INTERACTIVE_H
