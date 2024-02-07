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

#ifndef CCB_TREE_H_
#define CCB_TREE_H_

#include <compare>
#include <string>
#include <vector>

#include "util.h"
#include "matrix.h"

/** Represents a single edge in the graph of a tree. 
 * Indices refer to the vector of sequences used in tree construction.
 */
struct Edge {
    uint32_t parent; /** index of parent sequence. */
    uint32_t child;  /** Index of child sequence. */
    uint32_t distance; /** Sequence distance from child to parent. */
    float  weight; /** Fraction of sampled trees in which this edge occurred. */

    auto operator<=>(const Edge &) const = default;
};

/** Build consensus of n_samples minimum spanning trees.
* @param sequences non-empty list of unique, valid DNA sequences; tree will be rooted in sequences[0]
* @param n_samples the number of minimum spanning trees to build consensus from
* @param infer_ancestors if true, phylogenetic inference will be performed for each sample
* and the inferred sequences will be used in mst construction
* @return the adjacency list for the consensus tree
*/
std::vector<Edge>
build_consensus_mst(const std::vector<std::string> &seqeunces, uint32_t n_samples, bool infer_ancestors=true);

/** Generate a Markov model of nucleotide mutation rates from a given tree. Does not distinguish between
* coding and silent mutations.
* @param sequences non-empty list of unique valid un-gapped DNA sequences
* @param adj_list an adjacency list as output by build_consensus_mst, parent and child indexes in 
* this list should be the indices of the corresponding DNA sequences in sequences
* @return the Markov model as a 4x4 matrix with rows/columns corresponding to A, C, G, and T in that order;
* note that the COLUMNS sum to 1 rather than the rows, i.e. COLUMN 0 contains the probabilities that an A
* statys an A or mutates to C, G, or T
* @throw std::domain_error if a non-ACGT character (including '-') is encountered in any of the sequences
* */
Matrix<double>
infer_markov_model(const std::vector<std::string> &sequences, const std::vector<Edge> &adj_list);

#endif

