#pragma once
#include <array>

template<size_t Size>
class Matrix
{
public:
    std::array<float, Size> matrix;
    static constexpr size_t size = Size;

    Matrix() {
        matrix.fill(0.0f);
    }
    
    Matrix(const Matrix &other) : matrix(other.matrix) {}
    
    float& operator[](size_t index) { return matrix[index]; }
    const float& operator[](size_t index) const { return matrix[index]; }
    
    float* data() { return matrix.data(); }
    const float* data() const { return matrix.data(); }
};