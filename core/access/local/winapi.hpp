#pragma once
#include <core/access/adapter.hpp>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>

class WinApiAccessAdapter : public AccessAdapter
{
public:
    WinApiAccessAdapter() : m_processHandle(NULL),
                            m_processId(0)
    {
    }
    ~WinApiAccessAdapter()
    {
        detach();
    }

    bool attach(const std::string &processName) override;
    void detach() override;

    bool getModules(std::vector<ProcessModule> &modules) override;

    bool read(uintptr_t address, void *buffer, size_t size) override;
    bool write(uintptr_t address, const void *buffer, size_t size) override;

    void addScatterRead(uintptr_t address, void *buffer, size_t size) override;
    bool executeScatterRead() override;

    bool setMousePosition(const vec2_t<int> &position) override;
    bool setLeftMouseButton(bool state) override;
    bool getKeyState(int key) override;

protected:
    ScatterHandle createScatterHandle() override;
    void destroyScatterHandle(ScatterHandle handle) override;

private:
    struct ScatterReadEntry
    {
        uintptr_t address;
        void* buffer;
        size_t size;
    };
    
    struct WinApiScatterHandle
    {
        std::vector<ScatterReadEntry> entries;
    };

    HANDLE m_processHandle;
    DWORD m_processId;
};