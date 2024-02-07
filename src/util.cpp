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

#include <cmath>
#include <stdexcept>
#include <unordered_map>

#include "util.h"

std::pair<std::string, size_t>
make_valid_dna(std::string_view sv) {
    static const std::string_view ACGT = "ACGT-";
    std::string dna;
    size_t filtered = 0;
    for (char c : sv) {
        c = std::toupper(c);
        if (ACGT.find(c) != std::string::npos) {
            dna.push_back(c);
        } else {
            ++filtered;
        } 
    }
    return std::make_pair(dna, filtered);
}

std::string
translate(std::string_view nts) {
    static const std::unordered_map<std::string, char> translation = {
        {"AAA", 'K'}, {"AAC", 'N'}, {"AAG", 'K'}, {"AAT", 'N'},
        {"ACA", 'T'}, {"ACC", 'T'}, {"ACG", 'T'}, {"ACT", 'T'},
        {"AGA", 'R'}, {"AGC", 'S'}, {"AGG", 'R'}, {"AGT", 'S'},
        {"ATA", 'I'}, {"ATC", 'I'}, {"ATG", 'M'}, {"ATT", 'I'},
        {"CAA", 'Q'}, {"CAC", 'H'}, {"CAG", 'Q'}, {"CAT", 'H'},
        {"CCA", 'P'}, {"CCC", 'P'}, {"CCG", 'P'}, {"CCT", 'P'},
        {"CGA", 'R'}, {"CGC", 'R'}, {"CGG", 'R'}, {"CGT", 'R'},
        {"CTA", 'L'}, {"CTC", 'L'}, {"CTG", 'L'}, {"CTT", 'L'},
        {"GAA", 'E'}, {"GAC", 'D'}, {"GAG", 'E'}, {"GAT", 'D'},
        {"GCA", 'A'}, {"GCC", 'A'}, {"GCG", 'A'}, {"GCT", 'A'},
        {"GGA", 'G'}, {"GGC", 'G'}, {"GGG", 'G'}, {"GGT", 'G'},
        {"GTA", 'V'}, {"GTC", 'V'}, {"GTG", 'V'}, {"GTT", 'V'},
        {"TAA", '*'}, {"TAC", 'Y'}, {"TAG", '*'}, {"TAT", 'Y'},
        {"TCA", 'S'}, {"TCC", 'S'}, {"TCG", 'S'}, {"TCT", 'S'},
        {"TGA", '*'}, {"TGC", 'C'}, {"TGG", 'W'}, {"TGT", 'C'},
        {"TTA", 'L'}, {"TTC", 'F'}, {"TTG", 'L'}, {"TTT", 'F'}
    };
    std::string aas;
    std::string cdn;

    for (size_t i=0; i != nts.size(); ++i) {
        if ('-' != nts[i]) cdn.push_back(nts[i]);
        if (3 == cdn.size()) {
            aas.push_back(translation.at(cdn));
            cdn.clear();
        }
    }
    
    return aas;
}

std::pair<float, float>
exp_dist_mean_and_sdev(const std::vector<size_t> &v) {
    double sum = 0.0;
    for (size_t k : v) sum += static_cast<double>(k);
    return {static_cast<float>(sum/v.size()), static_cast<float>(sum/v.size())};
}

const std::vector<std::string>
PALETTE = {
    "#e6194b",
    "#3cb44b",
    "#4363d8",
    "#f58231",
    "#911eb4",
    "#f032e6",
    "#bcf60c",
    "#fabebe",
    "#008080",
    "#e6beff",
    "#9a6324",
    "#fffac8",
    "#800000",
    "#aaffc3",
    "#808000",
    "#ffd8b1",
    "#000075",
    "#808080",
    "#46f0f0",
    "#ffe119"
};

uint8_t RGB::lightness() const {
    uint8_t lo=0xFF, hi=0x00;
    for (size_t i=0; i<3; ++i) {
        if ((&r)[i] < lo) lo = (&r)[i];
        if ((&r)[i] > hi) hi = (&r)[i];
    }
    return lo / 2 + hi / 2;
}

RGB &
RGB::operator=(const std::string &hex_color) {
    RGB c;
    c.r = std::stoi(hex_color.substr(1, 2), nullptr, 16);
    c.g = std::stoi(hex_color.substr(3, 2), nullptr, 16);
    c.b = std::stoi(hex_color.substr(5, 2), nullptr, 16);
    *this = c;
    return *this;
}

std::pair<size_t, size_t>
ltri_ij(size_t k) {
    ++k;
    //note: std::sqrt is rounded correctly for perfect squares so this
    //formula works for 'reasonably' sized values of k
    double r = (std::sqrt(1.0 + 8*static_cast<double>(k))-1.0)/2.0;
    size_t i = static_cast<size_t>(r);
    if (static_cast<double>(i) == r) {
        return std::make_pair(i, i-1);
    } else {
        return std::make_pair(i+1, k-i*(i+1)/2-1);
    }
}

std::vector<std::string_view>
split(std::string_view s, std::string_view delim) {
    std::vector<std::string_view> v;
    size_t lo = 0, hi;
    do {
        hi = s.find(delim, lo);
        v.push_back(s.substr(lo, hi - lo));
        lo = hi + delim.size();
    } while (hi != std::string_view::npos);
    return v;
}

std::string_view
rstrip(std::string_view s) {
    while (std::isspace(s.back())) s.remove_suffix(1);
    return s;
}

std::vector<std::string_view>
wrap(std::string_view s, size_t max_len, char delim) {
    std::vector<std::string_view> lines;
    size_t pos = 0;
    if (delim) {
        while (pos < s.size()) {
            std::string_view ss = s.substr(pos, max_len);
            if (max_len < ss.size()) {
                size_t i = ss.find_last_of(delim);
                if (i != std::string::npos) {
                    ss = ss.substr(0, i + 1);
                }
            }
            lines.push_back(ss);
            pos += ss.size();
        }
    } else {
        while (pos < s.size()) {
            lines.push_back(s.substr(pos, max_len));
            pos += max_len;
        }
    }
    return lines;
}