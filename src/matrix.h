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

#ifndef CCB_MATRIX_H_
#define CCB_MATRIX_H_

#include <span>
#include <vector>

/** Simple wrapper class for std::vector that adds a matrix interface. */
template<typename T>
struct Matrix {
    Matrix(size_t rows, size_t cols, T default_t = T{})
        : rows_(rows)
        , cols_(cols)
        , buf_(rows * cols, default_t) { }

    Matrix(size_t rows, size_t cols, std::vector<T> &&v)
        : rows_(rows)
        , cols_(cols)
        , buf_(std::move(v)) {
        assert(rows * cols == v.size());
    }

    auto begin() { return buf_.begin(); }
    auto end()   { return buf_.end();   }

    auto cbegin() const { return buf_.cbegin(); }
    auto cend()   const { return buf_.cend();   }

    inline static Matrix<T> eye(size_t n);

    std::span<T> flatten() { return std::span<T>(buf_.data(), buf_.size()); }
    std::span<T> operator[](size_t i) { return std::span<T>(buf_.data() + i * cols_, cols_); }
    std::span<const T> operator[](size_t i) const { return std::span<const T>(buf_.data() + i * cols_, cols_); }

    T &operator[](std::pair<size_t, size_t> ij) { return buf_[ij.first * cols_ + ij.second]; }
    const T &operator[](std::pair<size_t, size_t> ij) const { return buf_[ij.first * cols_ + ij.second]; }

    std::span<const T> row(size_t i) const { return std::span<const T>(&buf_[i * cols_], cols_); }

    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }

    void expand(size_t rows, size_t cols, T default_t = T{});

    inline Matrix<T> transpose() const;
    inline Matrix<T> operator*(const Matrix<T> &rhs) const;
    inline Matrix<T> inverse() const;
private:
    size_t rows_ = 0, cols_ = 0;
    std::vector<T> buf_;
};

template<typename T>
void
Matrix<T>::expand(size_t rows, size_t cols, T t) {
    Matrix<T> m(rows_ + rows, cols_ + cols, t);
    for (size_t i = 0; i != rows_; ++i)
    for (size_t j = 0; j != cols_; ++j) {
        m[{i, j}] = (*this)[{i, j}];
    }
    std::swap(m.buf_, buf_);
    rows_ += rows;
    cols_ += cols;
}

template<typename T>
Matrix<T>
Matrix<T>::eye(size_t n) {
    Matrix<T> I(n, n, static_cast<T>(0));
    for (size_t i = 0; i != n; ++i) I[{i,i}] = static_cast<T>(1);
    return I;
}

template<typename T>
Matrix<T>
Matrix<T>::transpose() const {
    Matrix<T> M(cols(), rows());
    for (size_t i = 0; i != rows(); ++i)
    for (size_t j = 0; j != cols(); ++j) {
        M[{j, i}] = (*this)[{i, j}];
    }
    return M;
}

template<typename T>
Matrix<T>
Matrix<T>::operator*(const Matrix<T> &rhs) const {
    assert(cols() == rhs.rows());
    Matrix<T> X(rows(), rhs.cols());
    for (size_t i = 0; i != X.rows(); ++i)
    for (size_t j = 0; j != X.cols(); ++j) {
        T dp = 0;
        for (size_t k = 0; k != cols(); ++k) dp += (*this)[{i, k}] * rhs[{k, j}];
        X[{i, j}] = dp;
    }
    return X;
}

template<typename T>
Matrix<T>
Matrix<T>::inverse() const {
    if (rows() != 2 || cols() != 2) throw std::domain_error("Matrix inverse only implemented for 2x2 matrices.");
    double detA = (*this)[{0, 0}] * (*this)[{1, 1}] - (*this)[{0, 1}] * (*this)[{1, 0}];
    if (std::abs(detA) < 0.0000001) throw std::domain_error("2x2 matrix has determinant too close to zero.");  
    Matrix<T> Inv(2, 2);
    Inv[{0, 0}] =  (*this)[{1, 1}] / detA;
    Inv[{0, 1}] = -(*this)[{0, 1}] / detA;
    Inv[{1, 0}] = -(*this)[{1, 0}] / detA;
    Inv[{1, 1}] =  (*this)[{0, 0}] / detA;

    return Inv;
}

#endif
