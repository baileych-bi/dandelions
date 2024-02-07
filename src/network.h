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

#ifndef CCB_NETWORK_H_
#define CCB_NETWORK_H_

#include <tuple>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "util.h"
#include "style.h"

/** Represents a vector or point in 2D. */
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() {}
    Vec2(float x, float y) : x(x), y(y) {}

    /** Euclidian between this and v */
    inline float dist(Vec2 v) const;

    /** Scalar multiplication */
    Vec2  operator*(float s) const { return {s * x, s * y}; }

    /** Vector additon */
    Vec2 &operator+=(Vec2 v) { x += v.x; y += v.y; return *this; }
};

float
Vec2::dist(Vec2 v) const {
    const float dx = x - v.x;
    const float dy = y - v.y;
    return sqrtf(dx * dx + dy * dy);
}

/** Represents one of the physical constants in the physics simulation */
struct Constant {
    /** Construct an empty constant */
    Constant() {}

    /** Create a constant constrained by min and max values.
    * Note: maximum is actually allowed to be less than minimum allowing
    * for flexible interpolation (see as_fraction and set_fraction).
    * @param value the initial value
    * @param minimum the minimum allowed value
    * @param maximum the maximum allowed value
    */
    Constant(float value, float minimum, float maximum);

    /** Get current value */
    float value() const { return v_;    }
    /** Get "max" allowed value */
    float min()   const { return minv_; }
    /** Get "min" allowed value */
    float max()   const { return maxv_; }

    /** Get the "default" value, the value set during construction. */
    float get_default() const { return default_; }

    /** Return fraction of the distance of value between the extrema */
    float as_fraction() const { return (v_-minv_)/(maxv_-minv_);}

    /** Set value by interpolation between min and max.
    * @f fraction of distance between min and max, must be in [0.0..1.0]
    */
    void set_fraction(float f);

private:
    float v_    = 0.0f;
    float default_ = 0.0f;
    float minv_ = 0.0f;
    float maxv_ = 1.0f;
};

/** Represents a node in the tree and also the inbound edge from its parent.
* The following must be true at all times:<br/>
* 1) the Node with id 0 is the root
* 2) the id of each node in the network is unique
*/
struct Node {
    friend struct Network;

    /** Construct Node with given id */
    Node(size_t id) : id_(id) { }

    /** Value for centroid_id for nodes that aren't centroids. */
    static const int NA;

    /** The number of Nodes in the un-consolidated tree merged into this one
    * during Network::consolidate()
    */
    int total = 1;

    /** The number of inferred sequences merged into this on. */
    int inferred = 0;

    /** If this Node is declared a "centroid" it gets a unique numeric id */
    int centroid_id = NA;

    /** Location of this Node in physics simulation */
    Vec2 pos;

    /** Node radius in physics simulation */
    double r = 1.0f;

    /** Node mass in physics simulation */
    float mass = 1.0f;

    /** Value associated with edge from parent - usually sequence distance */
    float length = 0.0f;

    /** Confidence value for this edge (in our case % of minimum spanning forest it appears in. */
    float confidence = 1.0f;

    /** Colors, line widths, z-order, etc. for drawing */
    NodeStyle style;

    /** Get the Node id */
    size_t id() const { return id_; }

    /** Set the associated nucleotide sequence. Filters non-ACTG characters.
    * Case insensitive. Also translates the sequence: see aas().
    * @param seq string_view of nucleotides
    * @return the number of invalid characters filtered
    */
    size_t nts(std::string_view seq);

    /** Get the associated nucleotide sequence. */
    const std::string &nts() const { return nts_; }

    /** Get the associated amino acid sequence. */
    const std::string &aas() const { return aas_; }
    
    /** Pointer to parent */
          Node *parent()       { return parent_; }
    const Node *parent() const { return parent_; }

    /** Const access to child pointers */
    const std::unordered_set<Node *> children() const { return children_; }

    /** Non-const access to children */
    auto begin()       { return children_.begin();  }

    /** Const access to children */
    auto begin() const { return children_.cbegin(); }

    /** Non-const access to children */
    auto end()       { return children_.end();  }

    /** Const access to children */
    auto end() const { return children_.cend(); }

    /** Add a child and set pointers */
    void add_child(Node *);

    /** Remove child c and set its parent pointer to null. */
    void remove_child(Node *c);
   
    /** Check if root */
    bool is_root() const { return nullptr == parent_; }

    /** Check if leaf */
    bool is_leaf() const { return children_.empty();  }

    /** Check if node is not connected to any other. */
    bool is_disconnected() const { return !parent_ && children_.empty(); }

private:
    /** Pointer to our parent Node */
    Node *parent_ = nullptr;

    /** Incorporate a child into this, increasing total and setting appropriate pointers. */
    void merge_child(Node *c);

    /** Incorporase a sibling into this, increasing total and setting appropriate pointers. */
    void merge_sibling(Node *s);

    size_t id_ = 0;

    /** Nucleotide sequence associated with this Node */
    std::string nts_;

    /** Translation of nts */
    std::string aas_;

    /** Pointers to children */
    std::unordered_set<Node *> children_;
};

/** Network holds the tree structure and performs the physics simulation with it. */
struct Network {
    Network();

    /** Create/add a node with the given id and return a reference to it.
    * @param id must be unique and the id of root must be 0
    */
    Node &add_node(size_t id);

    /** Create a directed edge from Node i to Node j. Both nodes
     * must have been added to the Network already. Network
     * does not prevent or check for cycles but creating cycles
     * will break things.
     * @param i id of parent Node
     * @param j id of child Node
     * @param weight weight of the edge (usually sequence distance)
     */
    void add_edge(size_t i, size_t j, float weight = 1.0f, float confidence=1.0f);

    /** Get a reference to Node with id=i */
    Node &node(size_t i)       { return nodes_.at(i); }

    /** Non-const access to nodes_ */
    auto begin() { return nodes_.begin(); }

    /** Non-const access to nodes_ */
    auto end()   { return nodes_.end();   }

    /** Const access to nodes_ */
    auto begin() const { return nodes_.begin(); }

   /** Const access to nodes_ */
    auto end()   const { return nodes_.end();   }


    /** Get a const reference to Node with id=i. */
    const Node &node(size_t i) const { return nodes_.at(i); }

    /** Get const access to Nodes sorted by id. */
    const std::map<size_t, Node> &nodes() const { return nodes_; }

    /** Label nodes with given ids (excluding root) as centroids. */
    void identify_centroids(const std::vector<size_t> &centroid_ids);

    /** Clear any centroid ids. */
    void clear_centroids();

    /** Get centroids, ordered by child count, ascending. */
    std::span<Node *> centroids() { return std::span<Node *>(centroids_); }

    /** Get const centroids, ordered by child count, ascending. */
    std::span<const Node * const> centroids() const { return std::span<const Node * const>(centroids_); }

    /** Get a vector of Node pointer sorted by style.z ascending. */
    const std::vector<Node *> z_ordered_nodes() const { return ptrs_; }

    void init_simulation();
    size_t simulate_step();

    float max_velocity() const;

    void pin_node(size_t id);
    void unpin_node(size_t id);
    void translate_node(size_t id, double dx, double dy);

    Node *pick(Vec2 p);

    size_t remove_inferred_leaves();

    template<typename F>
    void consolidate(F f, Node *root = nullptr);

          Constant &constant(char c) { return params_.at(c); }
    const Constant &constant(char c) const { return params_.at(c); }

private:
    void label_centroids();

    std::unordered_map<char, Constant> params_;

    float EPSILON_ = 0.0001f; //prevent div 0

    size_t iteration_ = 0;
    float max_velocity_ = 0.0f;

    std::map<size_t, Node> nodes_;
    std::vector<Node *> centroids_; //centroids sorted by child count, ascending

    size_t n_workers_;

    std::vector<Node *> ptrs_; //nodes by child count
    std::vector<int8_t> pins_; //0 for pinned nodes otherwise 1
    std::vector<float> x_;     //node x position
    std::vector<float> y_;     //node y position
    std::vector<float> m_;     //node mass
    std::vector<float> l_;     //spring lengths
    std::vector<float> s_;     //springs
    std::vector<std::vector<float>> fxs_;
    std::vector<std::vector<float>> fys_;
    std::vector<float> vx_;    //node velocity x
    std::vector<float> vy_;    //node velocity y
    std::vector<float> d_vx_;  //node delta velocity x
    std::vector<float> d_vy_;  //node delta velocity y
};

template<typename F>
void
Network::consolidate(F f, Node *root) {
    if (!root) root = &(nodes_.begin()->second);

    size_t mergers = 0;
    do {
        mergers = 0;
        {
            std::vector<Node *> children(root->children_.begin(), root->children_.end());
            for (Node *c : children) {
                if (f(root, c)) {
                    root->merge_child(c);
                    ++mergers;
                }
            }
        }

        {
            std::vector<Node *> siblings(root->children_.begin(), root->children_.end());
            while (!siblings.empty()) {
                Node *a = siblings.back();
                siblings.pop_back();
                for (size_t i = 0; i < siblings.size(); ) {
                    if (f(a, siblings[i])) {
                        a->merge_sibling(siblings[i]);
                        siblings[i] = siblings.back();
                        siblings.pop_back();
                        ++mergers;
                    } else {
                        ++i;
                    }
                }
            }
        }
    } while (mergers != 0);

    for (Node *c : root->children_) consolidate(f, c);

    if (root->is_root()) {
        for (auto ii = nodes_.begin(); ii != nodes_.end(); ) {
            if (ii->second.is_disconnected()) {
                ii = nodes_.erase(ii);
            } else {
                ++ii;
            }
        }
    }
}

#endif