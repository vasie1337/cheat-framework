#pragma once
#include <array>

template<typename T, size_t Rows, size_t Cols = Rows>
class Matrix
{
public:
    std::array<T, Rows * Cols> data;
    static constexpr size_t rows = Rows;
    static constexpr size_t cols = Cols;
    static constexpr size_t size = Rows * Cols;

    Matrix() {
        data.fill(T{0});
    }
    
    Matrix(const Matrix& other) : data(other.data) {}
    
    T& operator()(size_t row, size_t col) { 
        return data[row * Cols + col]; 
    }
    const T& operator()(size_t row, size_t col) const { 
        return data[row * Cols + col]; 
    }
    
    T& operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }
    
    T* ptr() { return data.data(); }
    const T* ptr() const { return data.data(); }
};

template<typename T> using Matrix2x2 = Matrix<T, 2, 2>;
template<typename T> using Matrix3x3 = Matrix<T, 3, 3>;
template<typename T> using Matrix4x4 = Matrix<T, 4, 4>;
template<typename T> using Matrix3x4 = Matrix<T, 3, 4>;

using Mat4f = Matrix4x4<float>;
using Mat4d = Matrix4x4<double>;
using Mat3x4f = Matrix3x4<float>;