#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

#include <core/math/vector.hpp>

class ProcessModule
{
public:
	ProcessModule() : name(""),
		base(0),
		size(0)
	{
	}
	std::string name;
	uintptr_t base;
	size_t size;
};

typedef void* ScatterHandle;

class AccessAdapter
{
public:
	AccessAdapter() {}
	virtual ~AccessAdapter() = default;

	virtual bool attach(const std::string& process_name) = 0;
	virtual void detach() = 0;

	virtual bool get_modules(std::vector<ProcessModule>& modules) = 0;

	virtual void add_scatter_read(uintptr_t address, void* buffer, size_t size) = 0;
	virtual bool execute_scatter_read() = 0;

protected:
	ScatterHandle m_scatter_handle = nullptr;

	virtual ScatterHandle create_scatter_handle() = 0;
	virtual void destroy_scatter_handle(ScatterHandle handle) = 0;

	virtual void initialize_scatter_handle() {
		if (m_scatter_handle == nullptr) {
			m_scatter_handle = create_scatter_handle();
		}
	}
	virtual void cleanup_scatter_handle() {
		if (m_scatter_handle != nullptr) {
			destroy_scatter_handle(m_scatter_handle);
			m_scatter_handle = nullptr;
		}
	}

public:
	virtual bool set_mouse_position(const vec2_t<int>& position) = 0;
	virtual bool set_left_mouse_button(bool state) = 0;
	virtual bool get_key_state(int key) = 0;

	std::unique_ptr<ProcessModule> get_module(const std::string& module_name)
	{
		std::vector<ProcessModule> modules;
		if (!get_modules(modules))
		{
			return nullptr;
		}
		for (const auto& module : modules)
		{
			if (module.name == module_name)
			{
				return std::make_unique<ProcessModule>(module);
			}
		}
		return nullptr;
	}

	void add_scatter(uintptr_t address, void* buffer, size_t size) {
		add_scatter_read(address, buffer, size);
	}

	bool execute_scatter() {
		return execute_scatter_read();
	}
};