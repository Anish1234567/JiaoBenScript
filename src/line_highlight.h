#ifndef JIAOBENSCRIPT_LINE_HIGHLIGHT_H
#define JIAOBENSCRIPT_LINE_HIGHLIGHT_H

#include <vector>

#include "sourcepos.h"
#include "unicode.h"


void line_lighlight(const std::vector<ustring> &lines, const SourcePos &start, const SourcePos &end);


#endif //JIAOBENSCRIPT_LINE_HIGHLIGHT_H
