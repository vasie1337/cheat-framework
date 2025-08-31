#include <core/core.hpp>
#include <imgui.h>

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
		
	if (!m_renderer->initializeOverlay(m_target_window_title, m_target_window_class))
	{
		log_error("Failed to initialize overlay renderer");
		return false;
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

	if (!m_show_widgets) {
		m_renderer->endFrame();
		return true;
	}
	
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
	ImGui::Begin("Overlay Menu", nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Target Window: %s", m_target_window_title ? m_target_window_title : "None");
	ImGui::Separator();

	if (ImGui::CollapsingHeader("ESP Settings"))
	{
		static bool enable_esp = true;
		ImGui::Checkbox("Enable ESP", &enable_esp);

		static float esp_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
		ImGui::ColorEdit4("ESP Color", esp_color);
	}

	if (ImGui::CollapsingHeader("Aimbot Settings"))
	{
		static bool enable_aimbot = false;
		ImGui::Checkbox("Enable Aimbot", &enable_aimbot);

		static float fov = 5.0f;
		ImGui::SliderFloat("FOV", &fov, 1.0f, 30.0f);
	}

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