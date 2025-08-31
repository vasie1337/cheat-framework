#include <core/core.hpp>

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
		
	if (!m_renderer->initialize(m_window_title, m_window_width, m_window_height))
	{
		log_error("Failed to initialize DX11 renderer");
		return false;
	}
	
	return true;
}

bool Core::update() {
	if (!m_renderer || !m_renderer->isInitialized())
		return false;
	
	if (!m_renderer->processMessages())
		return false;
	
	m_renderer->beginFrame(0.1f, 0.2f, 0.3f, 1.0f);
	
	// render
	
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