#include <cstdio>
#include <iosfwd>
#include <fstream>
#include "unistd.h"

#include "script.h"
#include "interactive.h"
#include "jbscript_option.h"


int main(int argc, char *argv[]) {
    JBScriptOption option = JBScriptOption::parse_argv(argc, argv);
    if (option.file == "-" && isatty(fileno(stdin))) {
        InteractiveRepl repl;
        repl.start();
        return 0;
    } else if (option.file == "-") {
        return run_script_main(std::cin);
    } else {
        std::ifstream fs(option.file);
        return run_script_main(fs);
    }
}
