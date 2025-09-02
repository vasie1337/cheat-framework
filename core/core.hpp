#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>

#include <core/logger/logger.hpp>
#include <core/rendering/rendering.hpp>
#include <core/access/adapter.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>

enum class TargetKind
{
	Local,
	Remote
};

class Core
{
public:
	Core() = default;
	~Core() = default;

	bool initialize();

	Core &with_target_type(TargetKind target_type)
	{
		m_target_type = target_type;
		return *this;
	}
	Core &with_logger_backend(LoggerBackend backend)
	{
		m_logger_backend_kind = backend;
		return *this;
	}
	Core &with_logger_level(LogLevel level)
	{
		m_logger_level_kind = level;
		return *this;
	}
	Core &with_window_title(const char *title)
	{
		m_window_title = title;
		return *this;
	}
	Core &with_target(const char *title, const char *className, const char *processName)
	{
		m_target_window_title = title;
		m_target_window_class = className;
		m_target_process_name = processName;
		return *this;
	}

	void register_callback(std::function<void(Core*)> callback)
	{
		m_callbacks.emplace_back(callback);
	}

	bool update();

	std::unique_ptr<AccessAdapter> m_access_adapter;
	std::unique_ptr<DX11Renderer> m_renderer;

	// Cheat functions:
	template<typename T>
    bool WorldToScreen(const Vector3<T>& worldPos, Vector2<T>& screenPos, 
                      const Matrix4x4<T>& viewProjMatrix) const
    {
        // Standard 4x4 view-projection matrix (column-major like OpenGL/Vulkan)
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
    
    // DirectX-style (row-major) matrix support
    template<typename T>
    bool WorldToScreenDX(const Vector3<T>& worldPos, Vector2<T>& screenPos,
                        const Matrix4x4<T>& viewProjMatrix) const
    {
        // Row-major multiplication (DirectX style)
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
    
    // Compact 3x4 transform matrix (common in game engines)
    template<typename T>
    bool WorldToScreen3x4(const Vector3<T>& worldPos, Vector2<T>& screenPos,
                         const Matrix3x4<T>& transform,
                         const Matrix4x4<T>& projection) const
    {
        // Transform to view space using 3x4 matrix
        Vector3<T> viewPos;
        viewPos.x = worldPos.x * transform(0,0) + worldPos.y * transform(0,1) + 
                   worldPos.z * transform(0,2) + transform(0,3);
        viewPos.y = worldPos.x * transform(1,0) + worldPos.y * transform(1,1) + 
                   worldPos.z * transform(1,2) + transform(1,3);
        viewPos.z = worldPos.x * transform(2,0) + worldPos.y * transform(2,1) + 
                   worldPos.z * transform(2,2) + transform(2,3);
        
        // Project to clip space
        return WorldToScreen(viewPos, screenPos, projection);
    }
    
    // Unity-style left-handed coordinate system
    template<typename T>
    bool WorldToScreenUnity(const Vector3<T>& worldPos, Vector2<T>& screenPos,
                           const Matrix4x4<T>& mvpMatrix) const
    {
        Vector4<T> clipPos;
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
        
        // Unity uses different NDC to screen conversion
        screenPos.x = (clipPos.x * invW + T{1}) * screenSize.x * T{0.5};
        screenPos.y = (clipPos.y * invW + T{1}) * screenSize.y * T{0.5};
        
        return screenPos.x >= 0 && screenPos.x <= screenSize.x &&
               screenPos.y >= 0 && screenPos.y <= screenSize.y;
    }
	
private:
	TargetKind m_target_type = TargetKind::Local;
	LoggerBackend m_logger_backend_kind = LoggerBackend::Console;
	LogLevel m_logger_level_kind = LogLevel::Debug;

	std::vector<std::function<void(Core*)>> m_callbacks;

	const char *m_window_title = nullptr;
	const char *m_target_window_title = nullptr;
	const char *m_target_window_class = nullptr;
	const char *m_target_process_name = nullptr;

	bool m_show_widgets = false;
	bool m_did_shutdown = false;
};