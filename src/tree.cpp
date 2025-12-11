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
#include <array>
#include <cassert>
#include <exception>
#include <iostream>
#include <iterator>
#include <future>
#include <numeric>
#include <random>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <vector>

#include "matrix.h"
#include "tree.h"
#include "util.h"

/** Node structure for binary tree created by neighbor joining. */
struct BNode;

/** Convert nucleotides in "ACGT" to a bitset in the low nibble. */
uint8_t
bit_encode(char nt);

/** Take a low nibble with one or more 1 bits and return one of the set bits at random. */
uint8_t
random_bit(uint8_t mask);

/** Convert a binary/bitset nucleotide back to char.
 * If mutliple fields in the bitset are set, one will be chosen at random.
 */
char
bit_decode(uint8_t b);

/** Convert a std::string of ACGT into binary/bitset representation. */
std::vector<uint8_t>
to_bdna(std::string_view s);

/** Convert a binary/bitset dna vector string into a std::string. */
std::string
to_dna(std::span<const uint8_t> b);

/** Run Fitch algorithm starting at leaves and working up. Precondition is that
 for root and all internal nodes, bseq is empty and, for all leaves, bseq is initialized.
*/
void
fitch_label_up(BNode &root);

/** Run modified Fitch algorithm from root and working down. Precondition is that for
 * all nodes, bseq is initialized.
 */
void
fitch_label_down(BNode &root);

/** Hamming distance. */
uint32_t
hamming_distance(std::string_view a, std::string_view b);

/** Distance matrix entries are always uint32_t but the 2 most significant bytes
* hold d(child, parent) and the least significant bytes hold d(parent, root).
* Note: this means the distance matrix returned from this function is not symetric.
*/
Matrix<uint32_t>
make_distance_matrix(const std::vector<std::string> &sequences);

/**
* For debugging purposes. Make a distance matrix of the appropriate size to hold
* sequences x sequences entries but fill each with a random, unique integer.
* @param sequences a vector of sequences - sequence data is ignored but sequences.size() is used
* but sequences.size() will be used to determine the number of rows and columns of the distance
* matrix.
* @return a Matrix<uint32_t> where each element is unique integer  in [0..sequences.size() * sequences.size())
*/
Matrix<uint32_t>
make_unique_random_distance_matrix(const std::vector<std::string> &sequences);

/** Create a neighbor joining tree using min-linkaged based on the
* supplied distance matrix.
*/
std::vector<uint32_t>
construct_nj_tree(const Matrix<uint32_t> &dism);

/** Build a standard binary phylogenetic tree using neighbor joining
 * and min-linkage, label the common ancestor with the known ancestor and infer
 * intermediates.
 */
std::vector<std::string>
infer_ancestors(const std::vector<std::string_view> &seqs, const Matrix<uint32_t> &dism, std::string_view known_ancestor={});

/* Construct a minimum spanning tree over the input sequences. If infer_ancestors is true
* then an nj tree will first be created and used for phylogenetic inference. Inferred 
* ancestor sequences will be used to construct a draft of the mst but removed (i.e.,
* children of inferred ancestors will be parented to their most recent 'real' ancestor) 
* in the final version.
*/
std::vector<uint32_t>
build_mst(const std::vector<std::string_view> &input,
              const Matrix<uint32_t> &dism,
              bool shuffle_sequences,
              bool do_infer_ancestors);

struct BNode {
    uint32_t id = 0;
    BNode *p = nullptr, *l = nullptr, *r = nullptr; //parent and child pointers
    std::vector<uint8_t> bseq; //sequence data in binary/bitset format

    /** Add a child to left first, or right if left child already exists
    * and update parent pointers.
    */
    void add_child(BNode &n);

    /** Reroot tree on this node and return this node's single child.
    * Must be called on a leaf node.
    */
    BNode *set_as_root();

    bool is_root() const { return nullptr == p; }
    bool is_leaf() const { return !l && !r; }
};

BNode *
BNode::set_as_root() {
    assert(is_leaf());
    //walk up the root path always making parent the left child
    for (BNode *n = this; n->p; n = n->p) {
        if (n->l) n->r = n->l;
        if (n->p->l == n)
            n->p->l = nullptr;
        else
            n->p->r = nullptr;
        n->l = n->p;
    }
    //this is now the root, zero out parent
    this->p = nullptr;
    //now work downward to fix the parent pointers
    for (BNode *n = this; n;) {
        if (n->l) {
            n->l->p = n;
            n = n->l;
        } else if (n->r) {
            n->r->p = n;
            n = n->r;
        } else {
            n = nullptr;
        }
    }
    return l;
}

void
BNode::add_child(BNode &n) {
    n.p = this;
    if (nullptr == l) {
        l = &n;
    } else if (nullptr == r) {
        r = &n;
    } else {
        assert(false);
    }
}

uint32_t
hamming_distance(std::string_view a, std::string_view b) {
    uint32_t d=0;
    for (auto aa=a.begin(), bb=b.begin(); aa != a.end(); ++aa, ++bb) d += (*aa != *bb);
    return d;
}

uint32_t
levenstein_distance(std::string_view a, std::string_view b) {
    thread_local std::vector<uint32_t> upper, lower;

    if (a.empty() || b.empty())
        return std::max(uint32_t(a.size()), uint32_t(b.size()));

    const size_t rows = a.size(), cols = b.size();

    upper.clear(); upper.resize(cols + 1, 0);
    lower.clear(); lower.resize(cols + 1, 0);

    for (size_t i=0; i < rows; ++i) {
        std::swap(lower, upper);
        for (size_t j=1; j <= cols; ++j)
            upper[j] = lower[j - 1] + (a[i] == b[j - 1]);
    }

    return upper[cols];
}

Matrix<uint32_t>
make_distance_matrix(const std::vector<std::string> &sequences) {
    Matrix<uint32_t> dism(sequences.size(), sequences.size(), 0);
    for (size_t i=0; i < sequences.size(); ++i) {
        const std::string &a = sequences[i];
        for (size_t j=0; j < i; ++j) {
            const std::string &b = sequences[j];
            dism[{i, j}] = dism[{j, i}] = hamming_distance(a, b);
        }
    }

    //break symetry here when we include the root distances
    std::vector<uint32_t> root_distance(dism[0].begin(), dism[0].end());
    for (size_t i=0; i < dism.rows(); ++i) {
        for (size_t j=0; j < dism.cols(); ++j) {
            uint32_t &d = dism[{i, j}];
            d <<= 16;
            d |= root_distance[j];
        }
    }

    return dism;
}

Matrix<uint32_t>
make_unique_random_distance_matrix(const std::vector<std::string> &sequences) {
    Matrix<uint32_t> dism(sequences.size(), sequences.size(), 0);
    std::vector<uint32_t> numbers(sequences.size() * sequences.size(), 0);
    for (uint32_t i = 0; i < numbers.size(); ++i) numbers[i] = i;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(numbers.begin(), numbers.end(), g);
    for (size_t i = 0, k = 0; i < dism.rows(); ++i)
    for (size_t j = 0; j < dism.cols(); ++j, ++k) {
        dism[{i, j}] = numbers[k];
    }
    return dism;
}

std::vector<uint32_t>
construct_nj_tree(const Matrix<uint32_t> &dism) {
    assert(dism.rows() == dism.cols());

    //represents a joining of nodes a and b 
    //(where a != b and a and b correspond to sequences[a] and sequences[b])
    struct Join {
        uint32_t a = 0; //node id a
        uint32_t b = 0; //node id b
        uint32_t d = 0; //distance from sequences[a] to sequences[b]
    };

    //make a sorted list of all possible inter-sequence distances
    std::vector<Join> q;
    q.reserve(dism.rows() * (dism.rows() - 1) / 2);
    for (uint32_t i = 1; i < dism.rows(); ++i)
        for (uint32_t j = 0; j != i; ++j) {
            q.push_back({.a = i, .b = j, .d = dism[{j, i}]});
        }

    std::sort(q.begin(), q.end(), [](const Join &a, const Join &b)->bool{ return a.d > b.d; });

    //tree holds the tree structure
    //the parent of node(id=i) is parents[i]
    //there are sequences.size() nodes corresponding to observed sequences / leaves
    //and sequences.size() - 1 nodes whose sequences will be inferred / internal nodes
    //so there are 2*sequences.size() - 1 elements in parents
    //for neighbor joining the initial tree topology is a star so the inital parent of
    //all nodes is root, whose id is parents.size() - 1 (aka 2*dism.rows() - 2)
    std::vector<uint32_t> tree(2*dism.rows() - 1, static_cast<uint32_t>(2*dism.rows() - 2));

    uint32_t u = static_cast<uint32_t>(dism.rows());
    const uint32_t root_id = static_cast<uint32_t>(tree.size() - 1);
    while (u != root_id + 1) { //the final step joins two nodes to root so stop on the step after u==root
        const Join jn = q.back(); //q.back() contains the smallest distance
        q.pop_back();

        uint32_t pa = jn.a;
        uint32_t pb = jn.b;

        //find the id of the most distant ancestors of ja.a and jn.b that aren't root
        while (tree[pa] != root_id) pa = tree[pa];
        while (tree[pb] != root_id) pb = tree[pb];

        //if jn.a and jn.b already share a common non-root ancestor we ignore this join
        if (pa == pb) continue;

        //otherwise create a new, more distant common ancestor for jn.a and jn.b
        tree[pa] = u;
        tree[pb] = u;

        ++u;
    }

    return tree;
}

/** Run Fitch algorithm starting at leaves and working up. Precondition is that
 for root and all internal nodes, bseq is empty and, for all leaves, bseq is initialized.
*/
void
fitch_label_up(BNode &root) {
    BNode *n = &root;
    assert(&root != root.p);
    while (n != root.p) {
        for (;;) { //descend leftward into tree
            while (n->l && n->l->bseq.empty()) n = n->l;
            if (n->r && n->r->bseq.empty())
                n = n->r;
            else
                break;
        }
        if (n->l && n->r) { //if we have an l and and r, label ourselves with a combination of their labels
            const std::vector<uint8_t> &label_l = n->l->bseq;
            const std::vector<uint8_t> &label_r = n->r->bseq;

            assert(!label_l.empty() && (label_l.size() == label_r.size()));

            std::vector<uint8_t> &label_n = n->bseq;
            label_n.resize(label_l.size(), 0x00);

            for (size_t i = 0; i < label_n.size(); ++i) {
                label_n[i] = (label_l[i] & label_r[i]) ? (label_l[i] & label_r[i]) : (label_l[i] | label_r[i]);
                assert(label_n[i]);
            }
        } else if (n->l && !n->r) { //if we just have an l, copy its label (this happens because a re-rooted tree is no longer binary)
            n->bseq = n->l->bseq;
        } else if (!n->l && n->r) { //same with only an r
            n->bseq = n->r->bseq;
        }
        n = n->p;
    }
}

/** Run modified Fitch algorithm from root and working down. Precondition is that for
 * all nodes bseq is initialized.
 */
void
fitch_label_down(BNode &root) {
    //explore the tree labeling nodes as we go down
    BNode *n = &root, *from = root.p;
    while (n != root.p) { //stop when we've come all the way back up

        //if we just descended from our parent, set our label
        //(unless we're a leaf - leaves are already labeled with known input)
        if (from == n->p && from && n->l && n->r) {
            std::vector<uint8_t> &label_n = n->bseq;
            std::vector<uint8_t> &label_p = n->p->bseq;
            
            for (size_t i = 0; i < label_n.size(); ++i) {
                label_n[i] = (label_n[i] & label_p[i]) ? random_bit(label_n[i] & label_p[i]) : random_bit(label_n[i]);
            }
        }

        if (from == n->p) {        //if we came from parent, try to descend
            if (n->l)              //try left first    
                n = n->l;
            else if (n->r)         //then right
                n = n->r;
            else                   //otherwise give up and head back
                n = n->p;
        } else if (from == n->l) { //if we came from our left child
            if (n->r)              //try going right
                n = n->r;
            else                   //if not, then head back up
                n = n->p;
        } else {                   //if we came from our right child
            n = n->p;              //then we've explored this subtree, head back up
        }
        from = n;
    }
}

std::vector<std::string>
infer_ancestors(const std::vector<std::string_view> &seqs, const Matrix<uint32_t> &dism, std::string_view common_ancestor) {
    assert(seqs.size() <= dism.rows() && dism.rows() == dism.cols());
    std::vector<uint32_t> tree = construct_nj_tree(dism);

    size_t n_internal = seqs.size() - 1;
    std::vector<BNode> nodes(seqs.size() + n_internal);
    for (uint32_t i = 0; i < nodes.size(); ++i) nodes[i].id = i;
    for (uint32_t i = 0; i < tree.size(); ++i) {
        if (i != tree[i]) //i == tree[i] is true for the root of the nj tree
            nodes[tree[i]].add_child(nodes[i]);
    }

    BNode &root = nodes.back();
    assert(root.id == nodes.size() - 1);

    if (root.p) throw std::runtime_error("root has a parent...");

    auto count = std::count_if(nodes.begin(), nodes.end(), [](BNode &n)->bool {return !n.p; });
    if (count != 1) throw std::runtime_error("too many roots! (" + std::to_string(count) + ")");

    for (size_t i = 0; i < seqs.size(); ++i) nodes[i].bseq = to_bdna(seqs[i]);

    fitch_label_up(root);

    //resolve, to the extent possible, ambiguities in our common ancestor sequence
    if (common_ancestor.empty()) common_ancestor = seqs[0];
    auto true_root = to_bdna(common_ancestor);
    for (size_t i = 0; i != true_root.size(); ++i) {
        root.bseq[i] = (true_root[i] & root.bseq[i]) ? true_root[i] : random_bit(root.bseq[i]);
    }

    //root.bseq = to_bdna(seqs[0]);
    fitch_label_down(root);

    std::vector<std::string> inferred(nodes.size() - seqs.size());
    std::unordered_set<std::string_view> unique;

    for (size_t i = seqs.size(); i != nodes.size(); ++i) {
        inferred.push_back(to_dna(nodes[i].bseq));
        unique.insert(inferred.back());
    }

    for (const std::string_view &s : seqs) unique.erase(s);

    return std::vector<std::string>(unique.begin(), unique.end());
}

/** Construct a minumum spanning tree from a set of sequences (and optionally inferred
* ancestral sequences) and a pre-calculated distance matrix.
* @param input the nucleotide sequences; the tree will be rooted on input[0]
* @param shuffle_sequences if true, edges for addition to the tree will be considred in random order
* @param do_infer_ancestors if true, the tree will be constructed with inferred ancestral sequences
* @return an vector<uint32t> 'tree' where tree[i] is the the index in input of the parent of node i
*/
std::vector<uint32_t>
build_mst(const std::vector<std::string_view> &input,
          const Matrix<uint32_t> &dism,
          bool shuffle_sequences,
          bool do_infer_ancestors) {
    constexpr uint32_t MAX_D = std::numeric_limits<uint32_t>::max();

    std::vector<std::string_view> sequences(input.begin(), input.end());
    std::vector<std::string> inferred;

    if (do_infer_ancestors) {
        inferred = infer_ancestors(input, dism);
        sequences.insert(sequences.end(), inferred.begin(), inferred.end());
    }

    const size_t dim = std::max(sequences.size(), dism.rows());

    struct Join {
        uint32_t p = 0;
        uint32_t c = 0;
        uint32_t d = MAX_D;
    };

    std::vector<Join> joins;
    joins.reserve(dim);
    for (uint32_t i = 0; i < dim; ++i) joins.push_back(Join{.p = 0, .c = i, .d = MAX_D});

    if (shuffle_sequences) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(joins.begin() + 1, joins.end(), g);
    }

    for (size_t pivot = 1; pivot < joins.size(); ++pivot) {
        const Join &last_added = joins[pivot - 1];
        size_t min_i = pivot;
        uint32_t min_d = joins[pivot].d;
        for (size_t i = pivot; i < joins.size(); ++i) {
            uint32_t c = joins[i].c, p = last_added.c;
            uint32_t d;
            if (c < dism.rows() && p < dism.cols()) {
                d = dism[{c,p}];
            } else {
                uint32_t d1 = hamming_distance(sequences[c], sequences[p]);
                uint32_t d2 = hamming_distance(sequences[p], sequences[0]);
                d = (d1 << 16) | (d2 & 0xFFFF);
            }

            if (d < joins[i].d) {
                joins[i].d = d;
                joins[i].p = last_added.c;
            }

            if (joins[i].d < min_d) {
                min_d = joins[i].d;
                min_i = i;
            }
        }

        std::swap(joins[pivot], joins[min_i]);
    }

    std::vector<uint32_t> tree(joins.size(), 0);
    for (const Join &j : joins) tree[j.c] = j.p;
    return tree;
}

/** Convert nucleotides in "ACGT" to a bit flag in a nibble. */
uint8_t
bit_encode(char nt) {
    static const std::string nts("ACGT-");
    size_t i = nts.find(nt);
    assert (i != std::string::npos);
    return 1 << i;
}

/** Take a nibble with one or more 1 bits and return a nucleotide corresponding to one of those bits at random. */
uint8_t
random_bit(uint8_t mask) {
    thread_local std::random_device rd;
    thread_local std::mt19937 g(rd());

    thread_local std::vector<uint8_t> choices;
    choices.clear();

    for (uint8_t i = 0; i < 5; ++i) if (1 & (mask >> i)) choices.push_back(i);
    size_t choice = std::uniform_int_distribution<size_t>(0, choices.size() - 1)(g);
    return 1 << choices[choice];
}

/** Convert a nibble back into a char in "ACGT". */
char
bit_decode(uint8_t b) {
    static const std::string lut("-AC-G---T--------");
    assert (0b0001 <= b && b <= 0b11111);
    return lut[random_bit(b)];
}

std::vector<uint8_t>
to_bdna(std::string_view s) {
    std::vector<uint8_t> b(s.size());
    std::transform(s.begin(), s.end(), b.begin(), ::bit_encode);
    return b;
}

std::string
to_dna(std::span<const uint8_t> b) {
    std::string s(b.size(), 0);
    std::transform(b.begin(), b.end(), s.begin(), ::bit_decode);
    return s;
}

uint32_t
calculate_parsimony_score(const std::vector<uint32_t> &tree, Matrix<uint32_t> &dism) {
    assert(dism.rows() == dism.cols());

    uint32_t score = 0;
    for (uint32_t c = 1; c < dism.rows(); ++c) {
        uint32_t p = tree[c];
        while (dism.rows() <= p) p = tree[p];
        score += dism[{c, p}] >> 16;
    }
    return score;
}

std::vector<Edge>
build_consensus_mst(const std::vector<std::string> &input, uint32_t n_samples, bool do_infer_ancestors) {
    std::vector<std::string_view> sequences(input.begin(), input.end());

    Matrix<uint32_t> dism = make_distance_matrix(input);

    constexpr uint32_t PCT_MAX = std::numeric_limits<uint32_t>::max();
    Matrix<uint32_t> pct(input.size(), input.size(), PCT_MAX);

    const uint32_t n_threads = std::max<uint32_t>(1, std::thread::hardware_concurrency());
    std::vector<std::future<std::vector<uint32_t>>> futures;
    uint32_t remaining = n_samples;

    {
        std::vector<uint32_t> max_p_tree = build_mst(sequences, dism, false, false);
        uint32_t parsimony_score = calculate_parsimony_score(max_p_tree, dism);
        std::cout << "Best possible parsimony score is " << parsimony_score << std::endl;
    }
    std::cout << "Infer ancestors? " << std::boolalpha << do_infer_ancestors << std::endl;

    while (remaining) {
        for (size_t i = 0; i < n_threads && remaining; ++i, --remaining) {
            futures.push_back(std::async([&]()->auto {
                return build_mst(sequences, dism, true, do_infer_ancestors);
                              }));
        }

        for (size_t i = 0; i < futures.size(); ++i) {
            uint32_t parsimony_score = 0;
            std::vector<uint32_t> tree = futures[i].get();
            for (uint32_t c = 1; c < input.size(); ++c) {
                uint32_t p = tree[c];
                while (input.size() <= p) p = tree[p];
                parsimony_score += dism[{c, p}] >> 16;
                pct[{c, p}] -= 1;
            }
            std::cout << "parsimony score of sample " << i << " = " << parsimony_score << std::endl;
        }

        futures.clear();
    }

    std::vector<uint32_t> tree = build_mst({}, pct, false, false);

    std::vector<Edge> edges;
    edges.reserve(input.size() - 1);
    uint32_t parsimony_score = 0;
    for (uint32_t c = 1; c < input.size(); ++c) {
        uint32_t p = tree[c];
        uint32_t distance = dism[{c, p}] >> 16;
        parsimony_score += distance;
        float weight = (PCT_MAX - (pct[{c, p}] & PCT_MAX)) / static_cast<float>(n_samples);
        edges.push_back(Edge{.parent = p, .child = c, .distance = distance, .weight = weight});
    }
    std::cout << "parsimony score of consensus = " << parsimony_score << std::endl; 
    return edges;
}

Matrix<double>
infer_markov_model(const std::vector<std::string> &sequences, const std::vector<Edge> &adj_list) {
    Matrix<double> m(4, 4);
    std::vector<double> csums(4, 0.0);
    size_t n = 0;

    std::array<size_t, 256> lut = {0};
    lut['A'] = 0;
    lut['C'] = 1;
    lut['G'] = 2;
    lut['T'] = 3;

    for (const Edge &e : adj_list) {
        std::string_view pnts = sequences[e.parent];
        std::string_view cnts = sequences[e.child];

        for (size_t i = 0; i != pnts.size(); ++i) {
            const size_t c = lut[static_cast<uint8_t>(pnts[i])];
            const size_t r = lut[static_cast<uint8_t>(cnts[i])];

            if (c == std::string::npos || r == std::string::npos)
                throw std::domain_error("infer_markov_model not supported for gapped sequences.");

            m[{r, c}] += 1.0;
            csums[c] += 1.0;
        }
    }

    for (size_t c = 0; c < m.cols(); ++c) {
        if (csums[c] == 0.0) {
            m[{c, c}] = 1.0;
        } else {
            for (size_t r = 0; r < m.rows(); ++r) m[{r, c}] /= csums[c];
        }
    }

    return m;
}