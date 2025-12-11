/*
Copyright 2024, The Broad Institute of MIT and Harvard

Original Author: Charles C Bailey

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <iomanip>
#include <sstream>

#include "muttable.h"

const char *HTML_TOP =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<meta charset=\"utf-8\" />\n"
"<style type=\"text/css\">\n"
"    table.mutations {\n"
"        font-family: Arial, Helvetica, sans-serif;\n"
"        font-weight: bold;\n"
"        font-size: 9pt;\n"
"        margin:1px;\n"
"        padding:0;\n"
"        background-color:#ffffff;\n"
"    }\n"
"\n"
"    table.mutations th {\n"
"        text-align: center;\n"
"        vertical-align: bottom;\n"
"        display: table-cell;\n"
"        width:1em;\n"
"        padding:0;\n"
"    }\n"
"\n"
"    table.mutations th span {\n"
"        writing-mode: vertical-rl;\n"
"        transform: scale(-1);\n"
"    }\n"
"\n"
"    table.mutations td {\n"
"        text-align:center;\n"
"        padding:0;\n"
"    }\n"
"</style>\n"
"</head>\n"
"<body>\n";

const char *HTML_BOTTOM =
"</body>"
"</html>";

MutTable::MutTable(const std::vector<std::string> &seqs) {
    if (seqs.size() < 2) return;

    const std::string &anc = seqs[0];
    ancestor_ = anc;
    for (size_t i=0; i<anc.size(); ++i) {
        for (size_t j=1; j<seqs.size(); ++j) {
            assert (seqs[j].size() == anc.size());
            if (anc[i] != seqs[j][i]) {
                loc_.push_back(i);
                break;
            }
        }
    }

    res_.resize(seqs.size(), std::string(loc_.size(), '.'));

    for (size_t i=0; i<loc_.size(); ++i) {
        size_t p=loc_[i];
        res_[0][i] = anc[p];
        for (size_t j=1; j<seqs.size(); ++j) {
            if (anc[p] != seqs[j][p]) res_[j][i] = seqs[j][p]; 
        }
    }
}

uint32_t
to_int(wxColor c) {
    uint32_t i = 0;
    uint32_t k = c.GetRGB();
    i |= (k & 0x0000FF) << 16;
    i |= (k & 0x00FF00) <<  0;
    i |= (k & 0xFF0000) >> 16;
    return i;
}

std::string
MutTable::to_html(const std::vector<wxColor> &colors) const {
    assert (colors.size() == res_.size());

    std::stringstream ss;
    ss << HTML_TOP;
    ss << "<div>" << ancestor_ << "</div>\n";
    ss << "<table class=\"mutations\">\n";

    //position header row
    ss << "<tr><th></th>";
    for (size_t i : loc_) ss << "<th><span>" << (i+1) << "</span></th>";
    ss << "</tr>\n";

    //residue rows
    for (size_t j=0; j<res_.size(); ++j) {
        ss << "<tr style=\"color:#" << std::setfill('0') << std::setw(6) << std::hex << std::right << to_int(colors[j]) << "\"><th>"; //std::format("{:06X}", to_int(colors[j])) << "\"><th>";
        ss << std::dec;
        if (j != 0) ss << j;
        ss << "</th>";
        for (char c : res_[j]) ss << "<td>" << c << "</td>";
        ss << "</tr>\n";
    }

    ss << "</table>\n";
    ss << HTML_BOTTOM;
    return ss.str();
}
