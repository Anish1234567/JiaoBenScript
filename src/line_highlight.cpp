#include <algorithm>
#include <cassert>
#include "iosfwd"

#include "line_highlight.h"


static void print_single_line_highlight(const ustring &line, size_t start, size_t end) {
    assert(!line.empty() && line.back() == '\n');
    assert(start <= end && end < line.size());
    std::cerr << u8_encode(line);
    // FIXME: use wcwith
    std::cerr << std::string(start, ' ') << std::string(end - start + 1, '~') << std::endl;
}


void line_lighlight(const std::vector<ustring> &lines, const SourcePos &start, const SourcePos &end)
{
    assert(start.is_valid() && end.is_valid());
    assert((start.lineno) <= end.lineno && (size_t)(end.lineno) < lines.size());

    for (int lineno = start.lineno; lineno <= end.lineno; ++lineno) {
        const ustring &line = lines[lineno];
        int start_idx = lineno == start.lineno ? start.rowno : 0;
        int end_idx = lineno == end.lineno ? end.rowno : std::max(0, (int)line.size() - 2);
        print_single_line_highlight(line, (size_t)start_idx, (size_t)end_idx);
    }
}
