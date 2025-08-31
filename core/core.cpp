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
	// Initialize logger
	Logger::instance().initialize(m_logger_backend_kind, m_logger_level_kind, "core.log");
	
	log_info("Initializing Core...");
	
	// Initialize renderer
	bool renderer_initialized = false;
	
	if (m_external_hwnd)
	{
		// Use external window
		log_info("Using external window handle: 0x%p", m_external_hwnd);
		renderer_initialized = m_renderer->initialize(m_external_hwnd, m_window_width, m_window_height);
	}
	else
	{
		// Create internal window
		log_info("Creating internal window: %s", m_window_title);
		renderer_initialized = m_renderer->initialize(m_window_title, m_window_width, m_window_height);
	}
	
	if (!renderer_initialized)
	{
		log_error("Failed to initialize DX11 renderer");
		return false;
	}
	
	log_info("Core initialized successfully");
	return true;
}

bool Core::update() {
	if (!m_renderer || !m_renderer->isInitialized())
		return false;
	
	// Process window messages
	if (!m_renderer->processMessages())
		return false;
	
	// Begin frame with a dark blue color
	m_renderer->beginFrame(0.1f, 0.2f, 0.3f, 1.0f);
	
	// TODO: Add rendering code here
	
	// End frame and present
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