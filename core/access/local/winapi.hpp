#pragma once
#include <core/access/adapter.hpp>
#include <windows.h>
#include <tlhelp32.h>

class WinApiAccessAdapter : public AccessAdapter {
public:
    WinApiAccessAdapter() : 
        m_processHandle(NULL),
        m_processId(0)
    {
    }
    ~WinApiAccessAdapter() {
        detach();
    }

    bool attach(const std::string& processName) override;
    bool detach() override;

    bool getModules(std::vector<ProcessModule>& modules) override;

    bool read(uintptr_t address, void* buffer, size_t size) override;
    bool write(uintptr_t address, const void* buffer, size_t size) override;

    bool setMousePosition(const Vector2<int>& position) override;
    bool setLeftMouseButton(bool state) override;
    bool getKeyState(int key) override;

private:
    HANDLE m_processHandle;
    DWORD m_processId;
};