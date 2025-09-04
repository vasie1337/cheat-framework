#pragma once

#include <core/math/vector.hpp>
#include <core/math/matrix.hpp>
#include <core/graphics/rendering.hpp>

class ProjectionUtils
{
public:
	ProjectionUtils(const DX11Renderer* renderer) : m_renderer(renderer) {}
    ~ProjectionUtils() = default;

    template<typename T>
    bool WorldToScreen(const vec3_t<T>& world_pos, vec2_t<T>& screen_pos, 
                      const matrix4x4_t<T>& view_matrix) const
    {
        const T clipX = world_pos.x * view_matrix(0,0) + 
                       world_pos.y * view_matrix(0,1) + 
                       world_pos.z * view_matrix(0,2) + 
                       view_matrix(0,3);
        
        const T clipY = world_pos.x * view_matrix(1,0) + 
                       world_pos.y * view_matrix(1,1) + 
                       world_pos.z * view_matrix(1,2) + 
                       view_matrix(1,3);
        
        const T clipW = world_pos.x * view_matrix(3,0) + 
                       world_pos.y * view_matrix(3,1) + 
                       world_pos.z * view_matrix(3,2) + 
                       view_matrix(3,3);
        
        if (clipW < T{0.01})
            return false;
        
        const T invW = T{1} / clipW;
        const auto screen_size = m_renderer->get_size();
        
        screen_pos.x = (clipX * invW + T{1}) * screen_size.x * T{0.5};
        screen_pos.y = (T{1} - clipY * invW) * screen_size.y * T{0.5};
        
        return screen_pos.x >= 0 && screen_pos.x <= screen_size.x &&
               screen_pos.y >= 0 && screen_pos.y <= screen_size.y;
    }
    
    template<typename T>
    bool WorldToScreenDX(const vec3_t<T>& world_pos, vec2_t<T>& screen_pos,
                        const matrix4x4_t<T>& view_matrix) const
    {
        const T clipX = world_pos.x * view_matrix(0,0) + 
                       world_pos.y * view_matrix(1,0) + 
                       world_pos.z * view_matrix(2,0) + 
                       view_matrix(3,0);
        
        const T clipY = world_pos.x * view_matrix(0,1) + 
                       world_pos.y * view_matrix(1,1) + 
                       world_pos.z * view_matrix(2,1) + 
                       view_matrix(3,1);
        
        const T clipW = world_pos.x * view_matrix(0,3) + 
                       world_pos.y * view_matrix(1,3) + 
                       world_pos.z * view_matrix(2,3) + 
                       view_matrix(3,3);
        
        if (clipW < T{0.01})
            return false;
        
        const T invW = T{1} / clipW;
        const auto screen_size = m_renderer->get_size();
        
        screen_pos.x = (clipX * invW + T{1}) * screen_size.x * T{0.5};
        screen_pos.y = (T{1} - clipY * invW) * screen_size.y * T{0.5};
        
        return screen_pos.x >= 0 && screen_pos.x <= screen_size.x &&
               screen_pos.y >= 0 && screen_pos.y <= screen_size.y;
    }
    
    template<typename T>
    bool WorldToScreen3x4(const vec3_t<T>& world_pos, vec2_t<T>& screen_pos,
                         const matrix3x4_t<T>& transform,
                         const matrix4x4_t<T>& projection) const
    {
        vec3_t<T> viewPos;
        viewPos.x = world_pos.x * transform(0,0) + world_pos.y * transform(0,1) + 
                   world_pos.z * transform(0,2) + transform(0,3);
        viewPos.y = world_pos.x * transform(1,0) + world_pos.y * transform(1,1) + 
                   world_pos.z * transform(1,2) + transform(1,3);
        viewPos.z = world_pos.x * transform(2,0) + world_pos.y * transform(2,1) + 
                   world_pos.z * transform(2,2) + transform(2,3);
        
        return WorldToScreen(viewPos, screen_pos, projection);
    }
    
    template<typename T>
    bool WorldToScreenUnity(const vec3_t<T>& world_pos, vec2_t<T>& screen_pos,
                           const matrix4x4_t<T>& mvpMatrix) const
    {
        vec4_t<T> clipPos;
        clipPos.x = world_pos.x * mvpMatrix(0,0) + world_pos.y * mvpMatrix(0,1) + 
                   world_pos.z * mvpMatrix(0,2) + mvpMatrix(0,3);
        clipPos.y = world_pos.x * mvpMatrix(1,0) + world_pos.y * mvpMatrix(1,1) + 
                   world_pos.z * mvpMatrix(1,2) + mvpMatrix(1,3);
        clipPos.z = world_pos.x * mvpMatrix(2,0) + world_pos.y * mvpMatrix(2,1) + 
                   world_pos.z * mvpMatrix(2,2) + mvpMatrix(2,3);
        clipPos.w = world_pos.x * mvpMatrix(3,0) + world_pos.y * mvpMatrix(3,1) + 
                   world_pos.z * mvpMatrix(3,2) + mvpMatrix(3,3);
        
        if (clipPos.w < T{0.01})
            return false;
        
        const T invW = T{1} / clipPos.w;
        const auto screen_size = m_renderer->get_size();
        
        screen_pos.x = (clipPos.x * invW + T{1}) * screen_size.x * T{0.5};
        screen_pos.y = (clipPos.y * invW + T{1}) * screen_size.y * T{0.5};
        
        return screen_pos.x >= 0 && screen_pos.x <= screen_size.x &&
               screen_pos.y >= 0 && screen_pos.y <= screen_size.y;
    }

private:
    const DX11Renderer* m_renderer;
};
