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

struct ScatterStats {
	double reads_per_tick = 0.0;
	size_t total_reads = 0;
	size_t total_ticks = 0;
};

class ScatterStatsTracker {
private:
	size_t read_count = 0;
	size_t tick_count = 0;
	size_t last_tick_count = 0;
	ScatterStats current_stats;
	static constexpr size_t TICK_WINDOW = 100; // Calculate stats every 100 ticks

public:
	ScatterStatsTracker() = default;

	void record_read() {
		read_count++;
		current_stats.total_reads++;
	}

	void record_tick() {
		tick_count++;
		current_stats.total_ticks++;

		if (tick_count - last_tick_count >= TICK_WINDOW) {
			size_t ticks_elapsed = tick_count - last_tick_count;
			current_stats.reads_per_tick = static_cast<double>(read_count) / ticks_elapsed;

			read_count = 0;
			last_tick_count = tick_count;
		}
	}

	const ScatterStats& get_stats() const {
		return current_stats;
	}
};

class AccessAdapter
{
public:
	AccessAdapter() {}
	virtual ~AccessAdapter() = default;

	// Process
	virtual bool attach(const std::string& process_name) = 0;
	virtual void detach() = 0;

	// Modules
	virtual bool get_modules(std::vector<ProcessModule>& modules) = 0;

	// Scatter reading operations
	virtual void add_scatter_read(uintptr_t address, void* buffer, size_t size) = 0;
	virtual bool execute_scatter_read() = 0;

protected:
	// Internal scatter handle management
	ScatterHandle m_scatter_handle = nullptr;
	ScatterStatsTracker m_scatter_stats;

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
	// I/O
	virtual bool set_mouse_position(const vec2_t<int>& position) = 0;
	virtual bool set_left_mouse_button(bool state) = 0;
	virtual bool get_key_state(int key) = 0;

	// Stats
	const ScatterStats& get_scatter_stats() const {
		return m_scatter_stats.get_stats();
	}

	// Implementation
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

	void add_scatter(uintptr_t address, void* buffer, size_t size)
	{
		add_scatter_read(address, buffer, size);
	}

	bool execute_scatter()
	{
		m_scatter_stats.record_read();
		return execute_scatter_read();
	}

	void record_tick()
	{
		m_scatter_stats.record_tick();
	}
};