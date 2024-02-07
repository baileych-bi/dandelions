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

#ifndef CCB_MUT_TABLE_H_
#define CCB_MUT_TABLE_H_

#include <string>
#include <map>
#include <vector>

#include <wx/wx.h>

/** Takes a vector of sequences, the first of which is assumed to be wild type
*   and compiles a table of differences among them.
*/
struct MutTable {
    /** Construct a mutation table.
      * @param seqs a vector of 2 or more strings which must be of equal length
      * The first string is used as the wild type refernce sequence
    */
    MutTable(const std::vector<std::string> &seqs);

    /** Write html output to string.
    * Format a mutation table as an html document.
    * @param colors a vector of colors for each row of the table - must have one color per sequence
    * @return a string containing the html document
    */
    std::string to_html(const std::vector<wxColor> &colors) const;

private:
    std::string ancestor_;
    std::vector<size_t>      loc_;
    std::vector<std::string> res_;
};
#endif