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

#ifndef CCB_UTIL_H_
#define CCB_UTIL_H_

#include <string>
#include <string_view>
#include <vector>

/** An RGB color. */
struct RGB {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    uint8_t lightness() const;

    /** Initialize from 6-digit hex string, format: "#123456". */
    RGB &operator=(const std::string &hex_color);
};

/** Contrasting colors based on Sasha Trubetskoy's palette. */
extern const
std::vector<std::string> PALETTE;

/** Casefold sv and return a string of ACGT only chars, and the number of chars filtered from sv. */
std::pair<std::string, size_t>
make_valid_dna(std::string_view sv);

/** Translate string of nucleotides to amino acids
* @param uppercase string of only ACGT
* @return string containing the translation
* @throw std::out_of_range if invalid characters are included in nts
*/
std::string
translate(std::string_view);

/** Return mean and standard devation of v. */
std::pair<float, float>
exp_dist_mean_and_sdev(const std::vector<size_t> &v);

/** Calculates i, j for linear index k into packed lower triangular 
* matrix (diagonal excluded) as shown below:<pre>
*   j=0 1 2 3
* i=0 - - - -
* i=1 0 - - -
* i=2 1 2 - -
* i=3 3 4 5 -
* 
* k=0 => i,j=(1, 0)
* k=1 => i,j=(2, 0)
* k=2 => i,j=(2, 1)
* ...
* </pre>
* @param k the 0-based linear index into packed lower triangle
* @return pair i, j of 0-based row and column indices
*/
std::pair<size_t, size_t>
ltri_ij(size_t k);

/** Split string on delim into tokens. */ 
std::vector<std::string_view>
split(std::string_view s, std::string_view delim);

/** Join some collection of strings/string_views with delim. */
template<typename Range>
std::string
join(const Range &r, std::string_view delim);

/** Join strings/string_views in [lo,hi) with delim. */
template<typename InputIter>
std::string
join(InputIter lo, InputIter hi, std::string_view delim);

/** Remove trailing whitespace from sv. */
std::string_view
rstrip(std::string_view sv);

//

template<typename Range>
std::string
join(const Range &r, std::string_view delim) {
    return join(r.begin(), r.end(), delim);
}

template<typename InputIter>
std::string
join(InputIter lo, InputIter hi, std::string_view delim) {
    std::string s;
    if (lo != hi) {
        s.insert(s.cend(), lo->begin(), lo->end());
        while (++lo != hi) {
            s.insert(s.cend(), delim.begin(), delim.end());
            s.insert(s.cend(), lo->begin(), lo->end());
        }
    }
    return s;
}

std::vector<std::string_view>
wrap(std::string_view s, size_t max_len=80, char delim=0);

#endif
