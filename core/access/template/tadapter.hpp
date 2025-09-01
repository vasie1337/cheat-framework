#pragma once

#include <string>

// Includes memory reading and writing
// process attaching
// modules enumeration
// set mous pos and buttons
// get mous pos and buttons
// get keyboard state

class TAccessAdapter {
public:
	TAccessAdapter() = default;
	~TAccessAdapter() = default;

	// Process
	virtual bool attach(const std::string& processName) = 0;
	virtual bool detach() = 0;
	
	// Memory
	virtual bool read(uintptr_t address, void* buffer, size_t size) = 0;
	virtual bool write(uintptr_t address, const void* buffer, size_t size) = 0;

};