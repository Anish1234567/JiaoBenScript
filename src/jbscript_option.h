#ifndef ARGGEN_JBSCRIPT_OPTION_H
#define ARGGEN_JBSCRIPT_OPTION_H

// WARNING: Automatically generated code by arggen.py. Do not edit.

#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>


class ArgError : public std::runtime_error {
public:
    ArgError(const std::string &msg) : std::runtime_error(msg) {}
};


struct JBScriptOption {
    // options: ('file',), arg_type: ArgType.ONE
    std::string file = "-";

    std::string to_string() const;
    bool operator==(const JBScriptOption &rhs) const;
    bool operator!=(const JBScriptOption &rhs) const;
    static JBScriptOption parse_args(const std::vector<std::string> &args);
    static JBScriptOption parse_argv(int argc, const char *const argv[]);
};

#endif // ARGGEN_JBSCRIPT_OPTION_H
