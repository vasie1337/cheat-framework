#pragma once

#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>
#include <core/rendering/rendering.hpp>

class ProjectionUtils
{
public:
    explicit ProjectionUtils(const DX11Renderer* renderer);
    ~ProjectionUtils() = default;

    template<typename T>
    bool WorldToScreen(const vec3_t<T>& worldPos, vec2_t<T>& screenPos, 
                      const matrix4x4_t<T>& viewProjMatrix) const
    {
        const T clipX = worldPos.x * viewProjMatrix(0,0) + 
                       worldPos.y * viewProjMatrix(0,1) + 
                       worldPos.z * viewProjMatrix(0,2) + 
                       viewProjMatrix(0,3);
        
        const T clipY = worldPos.x * viewProjMatrix(1,0) + 
                       worldPos.y * viewProjMatrix(1,1) + 
                       worldPos.z * viewProjMatrix(1,2) + 
                       viewProjMatrix(1,3);
        
        const T clipW = worldPos.x * viewProjMatrix(3,0) + 
                       worldPos.y * viewProjMatrix(3,1) + 
                       worldPos.z * viewProjMatrix(3,2) + 
                       viewProjMatrix(3,3);
        
        if (clipW < T{0.01})
            return false;
        
        const T invW = T{1} / clipW;
        const auto screenSize = m_renderer->getSize();
        
        screenPos.x = (clipX * invW + T{1}) * screenSize.x * T{0.5};
        screenPos.y = (T{1} - clipY * invW) * screenSize.y * T{0.5};
        
        return screenPos.x >= 0 && screenPos.x <= screenSize.x &&
               screenPos.y >= 0 && screenPos.y <= screenSize.y;
    }
    
    template<typename T>
    bool WorldToScreenDX(const vec3_t<T>& worldPos, vec2_t<T>& screenPos,
                        const matrix4x4_t<T>& viewProjMatrix) const
    {
        const T clipX = worldPos.x * viewProjMatrix(0,0) + 
                       worldPos.y * viewProjMatrix(1,0) + 
                       worldPos.z * viewProjMatrix(2,0) + 
                       viewProjMatrix(3,0);
        
        const T clipY = worldPos.x * viewProjMatrix(0,1) + 
                       worldPos.y * viewProjMatrix(1,1) + 
                       worldPos.z * viewProjMatrix(2,1) + 
                       viewProjMatrix(3,1);
        
        const T clipW = worldPos.x * viewProjMatrix(0,3) + 
                       worldPos.y * viewProjMatrix(1,3) + 
                       worldPos.z * viewProjMatrix(2,3) + 
                       viewProjMatrix(3,3);
        
        if (clipW < T{0.01})
            return false;
        
        const T invW = T{1} / clipW;
        const auto screenSize = m_renderer->getSize();
        
        screenPos.x = (clipX * invW + T{1}) * screenSize.x * T{0.5};
        screenPos.y = (T{1} - clipY * invW) * screenSize.y * T{0.5};
        
        return screenPos.x >= 0 && screenPos.x <= screenSize.x &&
               screenPos.y >= 0 && screenPos.y <= screenSize.y;
    }
    
    template<typename T>
    bool WorldToScreen3x4(const vec3_t<T>& worldPos, vec2_t<T>& screenPos,
                         const matrix3x4_t<T>& transform,
                         const matrix4x4_t<T>& projection) const
    {
        vec3_t<T> viewPos;
        viewPos.x = worldPos.x * transform(0,0) + worldPos.y * transform(0,1) + 
                   worldPos.z * transform(0,2) + transform(0,3);
        viewPos.y = worldPos.x * transform(1,0) + worldPos.y * transform(1,1) + 
                   worldPos.z * transform(1,2) + transform(1,3);
        viewPos.z = worldPos.x * transform(2,0) + worldPos.y * transform(2,1) + 
                   worldPos.z * transform(2,2) + transform(2,3);
        
        return WorldToScreen(viewPos, screenPos, projection);
    }
    
    template<typename T>
    bool WorldToScreenUnity(const vec3_t<T>& worldPos, vec2_t<T>& screenPos,
                           const matrix4x4_t<T>& mvpMatrix) const
    {
        vec4_t<T> clipPos;
        clipPos.x = worldPos.x * mvpMatrix(0,0) + worldPos.y * mvpMatrix(0,1) + 
                   worldPos.z * mvpMatrix(0,2) + mvpMatrix(0,3);
        clipPos.y = worldPos.x * mvpMatrix(1,0) + worldPos.y * mvpMatrix(1,1) + 
                   worldPos.z * mvpMatrix(1,2) + mvpMatrix(1,3);
        clipPos.z = worldPos.x * mvpMatrix(2,0) + worldPos.y * mvpMatrix(2,1) + 
                   worldPos.z * mvpMatrix(2,2) + mvpMatrix(2,3);
        clipPos.w = worldPos.x * mvpMatrix(3,0) + worldPos.y * mvpMatrix(3,1) + 
                   worldPos.z * mvpMatrix(3,2) + mvpMatrix(3,3);
        
        if (clipPos.w < T{0.01})
            return false;
        
        const T invW = T{1} / clipPos.w;
        const auto screenSize = m_renderer->getSize();
        
        screenPos.x = (clipPos.x * invW + T{1}) * screenSize.x * T{0.5};
        screenPos.y = (clipPos.y * invW + T{1}) * screenSize.y * T{0.5};
        
        return screenPos.x >= 0 && screenPos.x <= screenSize.x &&
               screenPos.y >= 0 && screenPos.y <= screenSize.y;
    }

private:
    const DX11Renderer* m_renderer;
};
