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

#include <algorithm>
#include <fstream>
#include <iterator>
#include <unordered_set>

#include "parsers.h"
#include "util.h"

std::vector<std::string>
parse_dsa(std::istream &ifs) {
    std::unordered_set<std::string> sequences;
    std::string ancestor;
    std::string line;

    size_t line_no = 1;
    for (; std::getline(ifs, line); ++line_no) {
        if (line.starts_with("#dna template sequence")) {
            auto tokens = split(line, "\t");
            ancestor = rstrip(tokens[1]);
            std::transform(ancestor.begin(), ancestor.end(), ancestor.begin(), ::toupper);
            break;
        }
    }

    if (ancestor.empty())
        throw std::runtime_error("Could not locate '#dna template sequence' column");
    if (ancestor.find_first_not_of("ACGT") != std::string::npos)
        throw std::runtime_error("Dna template seqeunce contained invalid (i.e., non-ACGT) characters");

    bool found_alignments = false;
    for (; std::getline(ifs, line); ++line_no) {
        if (line.starts_with("#Alignments#")) {
            found_alignments = true;
            std::getline(ifs, line); //skip headers
            ++line_no;
            break;
        }
    }

    if (!found_alignments)
        throw std::runtime_error("#Alignments section could not be identified");

    size_t filtered=0;
    for (; std::getline(ifs, line); ++line_no) {
        if (line.starts_with("#")) break;
        std::getline(ifs, line); ++line_no; //skip amino acids
        auto tokens = split(line, "\t");
        if (tokens.size() < 4)
            throw std::runtime_error("Invalid sequence data in line " + std::to_string(line_no));
        std::string_view stripped = rstrip(tokens[3]);
        std::tie(line, filtered) = make_valid_dna(stripped);
        if (filtered)
            throw std::runtime_error("Sequence on line " + std::to_string(line_no) + " contained invalid (i.e., non-ACGT) characters");
        if (line.size() != ancestor.size()) continue;
        sequences.insert(std::move(line));
    }

    sequences.erase(ancestor);
    std::vector<std::string> output;
    output.push_back(std::move(ancestor));
    output.insert(output.cend(),
                  std::make_move_iterator(sequences.begin()),
                  std::make_move_iterator(sequences.end()));

    if (output.empty()) throw std::runtime_error("File contained no usable data.");
    return output;
}

std::vector<std::string>
parse_fasta(std::istream &ifs) {
    std::vector<std::string> seqs;
    size_t line_no = 1;
    std::string line;
    bool start_new = true;
    size_t filtered = 0;
    for (; std::getline(ifs, line); ++line_no) {
        std::string_view stripped = rstrip(line);
        if (stripped.empty()) {
            break;
        } else if ('>' == stripped.front()) {
            start_new = true;
        } else if (start_new) {
            start_new = false;
            std::tie(line, filtered) = make_valid_dna(stripped);
            seqs.push_back(std::move(line));
            line.clear();
        } else {
            std::tie(line, filtered) = make_valid_dna(stripped);
            seqs.back() += line;
        }
    }

    //get vector of unique sequences of same length keeping seqs[0] in place
    if (seqs.empty()) throw std::runtime_error("File contained no usable data.");

    std::unordered_set<std::string> uniq;
    std::copy_if(
        std::make_move_iterator(seqs.begin()+1),
        std::make_move_iterator(seqs.end()    ),
        std::inserter(uniq, uniq.end()),
        [&seqs](const auto &s)->bool{ return s.size()==seqs.front().size(); }
    );
    uniq.erase(seqs.front());
    seqs.resize(1);
    std::copy(
        std::make_move_iterator(uniq.begin()),
        std::make_move_iterator(uniq.end()  ),
        std::back_inserter(seqs)
    );

    return seqs;
}

std::vector<std::string>
parse_text(std::istream &ifs) {
    std::string ancestor;
    std::unordered_set<std::string> seqs;

    size_t filtered=0, line_no=1;
    for (std::string line; std::getline(ifs, line); ++line_no) {
        std::tie(line, filtered) = make_valid_dna(line);
        if (seqs.empty()) {
            ancestor = std::move(line);
        } else {
            seqs.insert(std::move(line));
        }
        line.clear();
    }

    if (seqs.empty()) throw std::runtime_error("File contained no usable data.");

    seqs.erase(ancestor);
    std::vector<std::string> output;
    output.push_back(std::move(ancestor));
    std::copy_if(
        std::make_move_iterator(seqs.begin()),
        std::make_move_iterator(seqs.end()  ),
        std::back_inserter(output),
        [&](auto &s)->bool{return s.size()==output.front().size();}
    );
    return output;
}
