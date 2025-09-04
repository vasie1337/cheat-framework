#pragma once
#include <array>

template<typename T, size_t Rows, size_t Cols = Rows>
class matrix_t
{
public:
    std::array<T, Rows * Cols> data;
    static constexpr size_t rows = Rows;
    static constexpr size_t cols = Cols;
    static constexpr size_t size = Rows * Cols;

    matrix_t() {
        data.fill(T{0});
    }
    
    matrix_t(const matrix_t& other) : data(other.data) {}
    
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

template<typename T> using matrix2x2_t = matrix_t<T, 2, 2>;
template<typename T> using matrix3x3_t = matrix_t<T, 3, 3>;
template<typename T> using matrix4x4_t = matrix_t<T, 4, 4>;
template<typename T> using matrix3x4_t = matrix_t<T, 3, 4>;

using mat4f = matrix4x4_t<float>;
using mat4d = matrix4x4_t<double>;
using mat3x4f = matrix3x4_t<float>;