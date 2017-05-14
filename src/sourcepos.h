#ifndef JIAOBENSCRIPT_SOURCEPOS_H
#define JIAOBENSCRIPT_SOURCEPOS_H


#include <string>

#include "repr.hpp"


struct SourcePos {
    int lineno;
    int rowno;

    SourcePos(int lineno, int rowno)
        : lineno(lineno), rowno(rowno)
    {}

    // invalid
    SourcePos() : lineno(-1), rowno(-1) {}

    bool operator==(const SourcePos &other) const;
    bool operator!=(const SourcePos &other) const;
    bool is_valid() const;
};


class LineTracer {
public:
    LineTracer(SourcePos &pos) : pos(pos) {}

    void add_char(unsigned int ch);

private:
    SourcePos &pos;
    bool last_newline = true;
};


REPR(SourcePos) {
    return "<Pos " + std::to_string(value.lineno) + ":" + std::to_string(value.rowno) + ">";
}


#endif //JIAOBENSCRIPT_SOURCEPOS_H
