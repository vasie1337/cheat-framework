#include <core/core.hpp>
#include <core/memory/local/winapi.hpp>
#include <core/memory/remote/dma.hpp>

#include <imgui.h>

bool Core::initialize()
{
	Logger::instance().initialize(m_logger_backend_kind, m_logger_level_kind);

	m_renderer = std::make_unique<DX11Renderer>();
	if (!m_renderer)
	{
		log_critical("Failed to create renderer");
		return false;
	}

	switch (m_target_type)
	{
	case TargetKind::Local:
		m_access_adapter = std::make_unique<WinApiAccessAdapter>();
		if (!m_access_adapter->attach(m_target_process_name))
		{
			log_error("Failed to attach local access adapter");
			return false;
		}
		log_debug("Local access adapter attached to %s", m_target_process_name.c_str());
		if (!m_renderer->initialize(m_target_window_title, m_target_window_title))
		{
			log_error("Failed to initialize overlay renderer");
			return false;
		}
		break;
	case TargetKind::Remote:
		m_access_adapter = std::make_unique<DMAAccessAdapter>();
		if (!m_access_adapter->attach(m_target_process_name))
		{
			log_error("Failed to attach remote access adapter");
			return false;
		}
		log_debug("Remote access adapter attached to %s", m_target_process_name);
		if (!m_renderer->initialize(m_window_title))
		{
			log_error("Failed to initialize renderer");
			return false;
		}
		break;
	}

	m_projection_utils = std::make_unique<ProjectionUtils>(m_renderer.get());
	m_update_manager = std::make_unique<UpdateManager>();

	return true;
}

bool Core::update()
{
	if (!m_renderer || !m_renderer->is_initialized())
		return false;

	if (!m_renderer->process_messages())
		return false;

	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		m_show_widgets = !m_show_widgets;
		m_renderer->set_overlay_interactive(m_show_widgets);
	}

	if (GetAsyncKeyState(VK_END) & 1)
	{
		return false;
	}

	m_renderer->begin_frame(0.0f, 0.0f, 0.0f, 0.0f);

	this->execute_all_functions();

	if (m_show_widgets)
	{
		ImGui::SetNextWindowPos(ImVec2(300, 300), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
		ImGui::Begin("Overlay Menu", nullptr, ImGuiWindowFlags_NoCollapse);
		{
			auto fps = m_renderer->get_fps();
			ImGui::Text("FPS: %f", fps);
		}
		ImGui::End();
	}

	m_renderer->end_frame();

	return true;
}