#include <core/core.hpp>
#include <imgui.h>
#include <string>
#include <core/access/local/winapi.hpp>
#include <core/access/remote/dma.hpp>

Core::Core()
	: m_renderer(std::make_unique<DX11Renderer>())
{
}

Core::~Core()
{
	shutdown();
}

bool Core::initialize() {
	Logger::instance().initialize(m_logger_backend_kind, m_logger_level_kind, "core.log");

	switch (m_access_adapter_kind) {
		case AccessAdapterKind::Local:
			m_access_adapter = std::make_unique<WinApiAccessAdapter>();
			if (!m_access_adapter->attach(m_target_process_name))
			{
				log_error("Failed to attach local access adapter");
				return false;
			}
			log_debug("Local access adapter attached to %s", m_target_process_name);
			if (!m_renderer->initializeOverlay(m_target_window_title, m_target_window_class))
			{
				log_error("Failed to initialize overlay renderer");
				return false;
			}
			break;
		case AccessAdapterKind::Remote:
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
	return true;
}

bool Core::update() {
	if (!m_renderer || !m_renderer->isInitialized())
		return false;
	
	if (!m_renderer->processMessages())
		return false;

	if (GetAsyncKeyState(VK_INSERT) & 1) {
		m_show_widgets = !m_show_widgets;
		m_renderer->setOverlayInteractive(m_show_widgets);
	}

	if (GetAsyncKeyState(VK_END) & 1) {
		shutdown();
		return true;
	}

	m_renderer->beginFrame(0.0f, 0.0f, 0.0f, 0.0f);

	float fps = ImGui::GetIO().Framerate;
	std::string fps_string = "FPS: " + std::to_string(fps);
	ImGui::GetBackgroundDrawList()->AddText(ImVec2(50, 50), ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), fps_string.c_str());

	if (!m_show_widgets) {
		m_renderer->endFrame();
		return true;
	}
	
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
	ImGui::Begin("Overlay Menu", nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::End();

	m_renderer->endFrame();
	
	return true;
}

void Core::shutdown() {
	log_info("Shutting down Core...");
	
	if (m_renderer)
	{
		m_renderer->shutdown();
		m_renderer.reset();
	}
	
	log_info("Core shut down successfully");
}