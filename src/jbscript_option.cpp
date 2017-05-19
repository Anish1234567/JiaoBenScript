#include <cstdlib>   // atol
#include <cstring>   // strlen
#include <string>    // to_string
#include "jbscript_option.h"

// WARNING: Automatically generated code by arggen.py. Do not edit.


bool JBScriptOption::operator==(const JBScriptOption &rhs) const {
    return std::tie(this->file) \
        == std::tie(rhs.file);
}
bool JBScriptOption::operator!=(const JBScriptOption &rhs) const {
    return !(*this == rhs);
}


std::string JBScriptOption::to_string() const {
    std::string ans = "<JBScriptOption";
    ans += " file=";
    ans += '"' + this->file + '"';
    return ans + ">";
}


JBScriptOption JBScriptOption::parse_args(const std::vector<std::string> &args) {
    JBScriptOption ans {};   // initialized
    int position_count = 0;
    // required options
    for (size_t i = 0; i < args.size(); i++) {
        const std::string &piece = args[i];
        if (piece.size() > 2 && piece[0] == '-' && piece[1] == '-') {
            // long options
            throw ArgError("Unknown option: " + piece);
        } else if (piece.size() >= 2 && piece[0] == '-') {
            // short options
            for (auto it = piece.begin() + 1; it != piece.end(); ++it) {
                throw ArgError("Unknown flag: " + std::string(1, *it));
            }
        } else {
            // positional args
            if (position_count == 0) {
                ans.file = piece.data();
            } else {
                throw ArgError("too many args: " + piece);
            }
            position_count++;
        }
    }

    // check required options
    // check positional args
    if (position_count < 0) {
        throw ArgError("expect more argument");
    }
    return ans;
}


JBScriptOption JBScriptOption::parse_argv(int argc, const char *const argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.emplace_back(argv[i]);
    }
    return JBScriptOption::parse_args(args);
}
