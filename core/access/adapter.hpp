#pragma once

#include <string>
#include <vector>
#include <memory>

#include <core/types/vector.hpp>

class ProcessModule
{
public:
	ProcessModule() : name(""),
					  baseAddress(0),
					  size(0)
	{
	}
	std::string name;
	uintptr_t baseAddress;
	size_t size;
};

typedef void* ScatterHandle;

class AccessAdapter
{
public:
	AccessAdapter() {}
	virtual ~AccessAdapter() = default;

	// Process
	virtual bool attach(const std::string &processName) = 0;
	virtual void detach() = 0;

	// Modules
	virtual bool getModules(std::vector<ProcessModule> &modules) = 0;

	// Memory
	virtual bool read(uintptr_t address, void *buffer, size_t size) = 0;
	virtual bool write(uintptr_t address, const void *buffer, size_t size) = 0;

    // Scatter reading operations
	virtual void addScatterRead(uintptr_t address, void *buffer, size_t size) = 0;
	virtual bool executeScatterRead() = 0;

protected:
	// Internal scatter handle management
	ScatterHandle m_scatter_handle = nullptr;
	virtual ScatterHandle createScatterHandle() = 0;
	virtual void destroyScatterHandle(ScatterHandle handle) = 0;
	
	virtual void initializeScatterHandle() {
		if (m_scatter_handle == nullptr) {
			m_scatter_handle = createScatterHandle();
		}
	}
	virtual void cleanupScatterHandle() {
		if (m_scatter_handle != nullptr) {
			destroyScatterHandle(m_scatter_handle);
			m_scatter_handle = nullptr;
		}
	}
	
public:
	// I/O
	virtual bool setMousePosition(const Vector2<int> &position) = 0;
	virtual bool setLeftMouseButton(bool state) = 0;
	virtual bool getKeyState(int key) = 0;

	// Implementation
	std::unique_ptr<ProcessModule> getModule(const std::string &moduleName)
	{
		std::vector<ProcessModule> modules;
		if (!getModules(modules))
		{
			return nullptr;
		}
		for (const auto &module : modules)
		{
			if (module.name == moduleName)
			{
				return std::make_unique<ProcessModule>(module);
			}
		}
		return nullptr;
	}

	template <typename T>
	T read(uintptr_t address)
	{
		T value{};
		read(address, &value, sizeof(T));
		return value;
	}

	template <typename T>
	void write(uintptr_t address, const T &value)
	{
		write(address, &value, sizeof(T));
	}
};