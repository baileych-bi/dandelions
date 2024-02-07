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

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <thread>

#include "network.h"

using std::numbers::pi;

const int Node::NA = -1;

size_t
Node::nts(std::string_view seq) {
    size_t filtered = 0;
    std::tie(nts_, filtered) = make_valid_dna(seq);
    aas_ = translate(nts_);
    return filtered;
}

void
Node::add_child(Node *c) {
    c->parent_ = this;
    children_.insert(c);
}

void
Node::remove_child(Node *c) {
    c->parent_ = nullptr;
    children_.erase(c);    
}

void
Node::merge_child(Node *c) {
    assert(this == c->parent_);

    children_.erase(c);

    for (Node *gc : c->children_) {
        gc->parent_ = this;
        children_.insert(gc);
    }

    c->parent_ = nullptr;
    c->children_.clear();

    total += c->total;
    inferred += c->inferred;
}

void
Node::merge_sibling(Node *s) {
    assert(s->parent_ == parent_);

    parent_->children_.erase(s);

    for (Node *c : s->children_) {
        c->parent_ = this;
        children_.insert(c);
    }

    s->parent_ = nullptr;
    s->children_.clear();

    total += s->total;
    inferred += s->inferred;
}

Network::Network() {
    params_['G'] = Constant(-0.1f,   0.0f, -1.0f);
    params_['C'] = Constant(0.001f,  0.0f, 0.01f);
    params_['B'] = Constant(1.0f,    0.0f, 2.0f);
    params_['K'] = Constant(0.25f,   0.0f, 2.0f);
    params_['E'] = Constant(1.0f,    0.5f, 2.0f);
    params_['V'] = Constant(0.2f,   10.0f, 0.1f);
    params_['T'] = Constant(1.0f,    0.1f, 4.0f);
}

Node &
Network::add_node(size_t id) {
    auto [ii, ins] = nodes_.insert({id, Node(id)});
    if (!ins) throw std::runtime_error("Network already contains Node with id=" + std::to_string(id));
    return ii->second;
}

void
Network::add_edge(size_t p, size_t c, float weight, float confidence) {
    try {
        nodes_.at(p).add_child(&nodes_.at(c));
        nodes_.at(c).length = weight;
        nodes_.at(c).confidence = confidence;
    } catch (std::out_of_range &ex) {
        (void)ex;
        throw std::out_of_range("Network missing either Node " + 
            std::to_string(p) + " or Node " + std::to_string(c));
    }
}

void
Network::label_centroids() {
    std::sort(centroids_.begin(),
              centroids_.end(),
              [](Node *a, Node *b)->bool {
                return a->children().size() + a->total > b->children().size() + b->total;
              });
    for (size_t i = 0; i < centroids_.size(); ++i) centroids_[i]->centroid_id = i;
}

void
Network::identify_centroids(const std::vector<size_t> &centroid_ids) {
    clear_centroids();
    for (size_t id : centroid_ids) if (!nodes_.at(id).is_root()) centroids_.push_back(&nodes_.at(id));
    label_centroids();
}

void
Network::clear_centroids() {
    centroids_.clear();
    for (auto &[_, n] : nodes_) n.centroid_id = Node::NA;
}

void
Network::init_simulation() {
    iteration_ = 0;
    max_velocity_ = 0.0f;

    ptrs_.clear();
    for (auto &[id, node] : nodes_) ptrs_.push_back(&node);
    std::sort(
        ptrs_.begin(),
        ptrs_.end(), 
        [](Node *a, Node *b)->bool{return a->style.z < b->style.z;}
    );

    pins_.clear();
    pins_.resize(ptrs_.size(), 1);

    x_.clear();
    x_.resize(ptrs_.size());
    for (float &x : x_) x = std::cos(rand() / static_cast<float>(RAND_MAX) * 2 * pi);

    y_.clear();
    y_.resize(ptrs_.size());
    for (float &y : y_) y = std::sin(rand() / static_cast<float>(RAND_MAX) * 2 * pi);

    m_.clear();
    for (Node *n : ptrs_) {
        float mass = n->mass + n->children_.size();
        m_.push_back(mass);
    }

    //l and k are lower triangular without diagonal
    //0-based row and col indices i, j can be obtained
    //from linear index k by ltri_ij(k) function
    l_.clear();
    s_.clear();

    for (size_t i = 1; i < ptrs_.size(); ++i) {
        for (size_t j = 0; j != i; ++j) {
            if (ptrs_[i]->parent() == ptrs_[j]) {
                auto [mi, mj] = ltri_ij(l_.size());
                assert (mi == i && mj == j);
                l_.push_back(ptrs_[i]->length + ptrs_[i]->r + ptrs_[j]->r);
                s_.push_back(1.0f);
            } else if (ptrs_[j]->parent() == ptrs_[i]) {
                auto [mi, mj] = ltri_ij(l_.size());
                assert(mi == i && mj == j);
                l_.push_back(ptrs_[j]->length + ptrs_[i]->r + ptrs_[j]->r);
                s_.push_back(1.0f);
            } else {
                l_.push_back(0.0f);
                s_.push_back(0.0f);
            }
        }
    }

    vx_.clear();
    vx_.resize(ptrs_.size(), 0.0f);

    vy_.clear();
    vy_.resize(ptrs_.size(), 0.0f);

    d_vx_.clear();
    d_vx_.resize(ptrs_.size(), 0.0f);

    d_vy_.clear();
    d_vy_.resize(ptrs_.size(), 0.0f);

    for (size_t i = 0; i < ptrs_.size(); ++i) {
        Node &n = *ptrs_[i];
        n.pos.x = x_[i];
        n.pos.y = y_[i];
    }

    n_workers_ = std::max(2U, std::thread::hardware_concurrency()) - 1;
    fxs_.clear();
    fxs_.resize(n_workers_, std::vector<float>(ptrs_.size(), 0.0f));
    fys_.clear();
    fys_.resize(n_workers_, std::vector<float>(ptrs_.size(), 0.0f));
}

void
simulate_step_worker(
    std::vector<float> &fx_out,
    std::vector<float> &fy_out,
    size_t lo,
    size_t hi,
    const std::vector<float> &x,
    const std::vector<float> &y,
    const std::vector<float> &s,
    const std::vector<float> &L,
    const std::vector<float> &m,
    float E,
    float G,
    float K,
    float EPSILON) {

    std::fill(fx_out.begin(), fx_out.end(), 0.0f);
    std::fill(fy_out.begin(), fy_out.end(), 0.0f);

    auto [i, j] = ltri_ij(lo);
    for (size_t li=lo; li != hi; ++li) {
        const float dx = x[j] - x[i];
        const float dy = y[j] - y[i];
        float r_sq = dx * dx + dy * dy;
        float r = sqrtf(r_sq);

        r_sq = std::max(r_sq, EPSILON);
        r = std::max(r, EPSILON);

        float fg = G * m[i] * m[j];
        fg /= r_sq;

        float k = K * s[li];
        float l = E * L[li];

        float fs = r - l;
        fs *= k;

        float fx = fg + fs;
        fx *= dx;
        fx /= r;

        float fy = fg + fs;
        fy *= dy;
        fy /= r;

        fx_out[i] += fx;
        fy_out[i] += fy;
        fx_out[j] -= fx;
        fy_out[j] -= fy;
  
        if (++j == i) {
            ++i;
            j = 0;
        }
    }
}

size_t
Network::simulate_step() {
    const float B    = params_['B'].value();
    const float C    = params_['C'].value();
    const float E    = params_['E'].value();
    const float G    = params_['G'].value();
    const float K    = params_['K'].value();
    const float Vmax = params_['V'].value();
    const float dT   = params_['T'].value();

    std::fill(d_vx_.begin(), d_vx_.end(), 0.0f);
    std::fill(d_vy_.begin(), d_vy_.end(), 0.0f);

    std::vector<std::thread> threads;

    size_t chunk = (ptrs_.size() * (ptrs_.size() - 1)) / 2 / n_workers_;
    for (size_t i=0; i<n_workers_; ++i) {
        threads.push_back(
            std::thread(
                simulate_step_worker,
                std::ref(fxs_[i]),
                std::ref(fys_[i]),
                i * chunk,
                (i == ptrs_.size() - 1) ? ptrs_.size() : (i + 1) * chunk,
                std::cref(x_),
                std::cref(y_),
                std::cref(s_),
                std::cref(l_),
                std::cref(m_),
                E,
                G,
                K,
                EPSILON_
            )
        );
    }

    threads[0].join();
    std::vector<float> &fx = fxs_[0];
    std::vector<float> &fy = fys_[0];

    for (size_t i=1; i<threads.size(); ++i) {
        threads[i].join();
        for (size_t j=0; j<fx.size(); ++j) {
            fx[j] += fxs_[i][j];
            fy[j] += fys_[i][j];
        }
    }

    for (size_t i = 0; i < fx.size(); ++i) {
        d_vx_[i] = (fx[i] - B * vx_[i] - C * x_[i]) / m_[i];
    }

    for (size_t i = 0; i < fy.size(); ++i) {
        d_vy_[i] = (fy[i] - B * vy_[i] - C * y_[i]) / m_[i];
    }

    for (size_t i = 0; i < vx_.size(); ++i) vx_[i] += d_vx_[i];
    for (size_t i = 0; i < vy_.size(); ++i) vy_[i] += d_vy_[i];

    for (size_t i = 0; i < vx_.size(); ++i) {
        float vx_sq = vx_[i] * vx_[i];
        float vy_sq = vy_[i] * vy_[i];
        float s = std::sqrt(vx_sq + vy_sq);
        float s_max = std::min(Vmax, s);
        if (s <= EPSILON_) continue;
        vx_[i] *= s_max / s * pins_[i];
        vy_[i] *= s_max / s * pins_[i];
    }

    for (size_t i = 0; i < x_.size(); ++i) x_[i] += dT * vx_[i];
    for (size_t i = 0; i < y_.size(); ++i) y_[i] += dT * vy_[i];

    for (size_t i = 0; i < ptrs_.size(); ++i) {
        Node &n = *ptrs_[i];
        n.pos.x = x_[i];
        n.pos.y = y_[i];
    }

    return ++iteration_;
}

float
Network::max_velocity() const {
    return max_velocity_;
}

void
Network::pin_node(size_t id) { 
    for (size_t i=0; i<ptrs_.size(); ++i) {
        if (ptrs_[i]->id() == id) {
            vx_[i] = 0.0f;
            vy_[i] = 0.0f;
            pins_[i] = 0;
            break;
        }
    }
}

void
Network::unpin_node(size_t id) {
    for (size_t i=0; i < ptrs_.size(); ++i) {
        if (ptrs_[i]->id() == id) {
            pins_[i] = 1;
            break;
        }
    }
}

void Network::translate_node(size_t id, double dx, double dy) {
    Node *n = nullptr;
    for (size_t i = 0; i < ptrs_.size(); ++i) {
        if (ptrs_[i]->id() == id) {
            n = ptrs_[i];
            x_[i] += dx;
            y_[i] += dy;
            n->pos.x += dx;
            n->pos.y += dy;
            break;
        }
    }
}

Node *
Network::pick(Vec2 p) {
    for (auto ri = ptrs_.rbegin(); ri != ptrs_.rend(); ++ri) {
        Node *n = *ri;
        if (p.dist(n->pos) < n->r) return n; 
    }
    return nullptr;
}

size_t
Network::remove_inferred_leaves() {
    size_t initial_count = nodes_.size();
    for (;;) {
        int erased = false;
        for (auto ii = nodes_.begin(); ii != nodes_.end(); ) {
            Node &n = ii->second;
            if (n.is_leaf() && n.inferred == n.total) {
                erased = true;
                if (!n.is_root()) n.parent()->remove_child(&n);
                ii = nodes_.erase(ii);
            } else {
                ++ii;
            }
        }
        if (!erased) break;
    }
    return initial_count - nodes_.size();
}

Constant::Constant(float value, float minimum, float maximum) 
    : v_(value)
    , default_(value)
    , minv_(minimum)
    , maxv_(maximum) {
    if (minimum <= maximum) 
        assert (minimum <= value && value <= maximum);
    else
        assert (maximum <= value && value <= minimum);
}

void
Constant::set_fraction(float f) { 
    assert (0.0f <= f && f <= 1.0f);
    v_ = minv_ + f*(maxv_ - minv_);
}
