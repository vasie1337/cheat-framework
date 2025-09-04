#pragma once
#include "../access_adapter.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <vector>

#include <core/system/logger/logger.hpp>

class WinApiAccessAdapter : public AccessAdapter
{
public:
    WinApiAccessAdapter() : m_process_handle(NULL),
                            m_processId(0)
    {
    }
    ~WinApiAccessAdapter()
    {
        detach();
    }

    bool attach(const std::string &process_name) override;
    void detach() override;

    bool get_modules(std::vector<ProcessModule> &modules) override;

    void add_scatter_read(uintptr_t address, void *buffer, size_t size) override;
    bool execute_scatter_read() override;

    bool set_mouse_position(const vec2_t<int> &position) override;
    bool set_left_mouse_button(bool state) override;
    bool get_key_state(int key) override;

protected:
    ScatterHandle create_scatter_handle() override;
    void destroy_scatter_handle(ScatterHandle handle) override;

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

    HANDLE m_process_handle;
    DWORD m_processId;
};