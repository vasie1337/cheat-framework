#include <core/core.hpp>

bool Core::initialize() {
	Logger::instance().initialize(m_logger_backend, m_logger_level, "core.log");
	return true;
}

bool Core::update() {
	return true;
}

void Core::shutdown() {

}