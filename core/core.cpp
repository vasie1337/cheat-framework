#include <core/core.hpp>

#include <iostream>

bool Core::initialize() {
	std::cout << "Initializing core" << std::endl;
	std::cout << "Access adapter: " << static_cast<int>(m_access_adapter) << std::endl;
	std::cout << "Render backend: " << static_cast<int>(m_render_backend) << std::endl;
	std::cout << "Cheat name: " << m_cheat_name << std::endl;
	return true;
}

void Core::update() {
	std::cout << "Updating core" << std::endl;
}

void Core::shutdown() {
	std::cout << "Shutting down core" << std::endl;
}