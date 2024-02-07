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

#ifndef CCB_PARSERS_H_
#define CCB_PARSERS_H_

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

/** Parse a .csv output from dsa (made with --template_dna=... and --show_codons=horizontal)
  * and return a vector of the unique DNA sequences whose length is the same as the template.
  */
std::vector<std::string>
parse_dsa(std::istream &ifs);

/** Parse a .fasta file and return the nucleotide sequences. The first sequence in the file
  * is assumed to be the common ancestor.
  */
std::vector<std::string>
parse_fasta(std::istream &ifs);

/** Parse a plain text file with one DNA sequence per line. Return the sequences. The first
* sequence in the file is assumed to be the common ancestor.
*/
std::vector<std::string>
parse_text(std::istream &ifs);

#endif