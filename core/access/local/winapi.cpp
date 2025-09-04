#include "winapi.hpp"

#include <core/logger/logger.hpp>

bool WinApiAccessAdapter::attach(const std::string &process_name)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    PROCESSENTRY32 pe32{};
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(snapshot, &pe32))
    {
        do
        {
            if (strcmp((char *)pe32.szExeFile, process_name.c_str()) == 0)
            {
                m_processId = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &pe32));
    }

    CloseHandle(snapshot);

    if (m_processId == 0)
    {
        return false;
    }

    m_process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_processId);
    if (m_process_handle == NULL)
    {
        return false;
    }

    return true;
}

void WinApiAccessAdapter::detach()
{
    log_debug("Detaching WinApiAccessAdapter");
    cleanup_scatter_handle();
    if (m_process_handle != NULL)
    {
        CloseHandle(m_process_handle);
        m_process_handle = NULL;
        m_processId = 0;
    }
}

bool WinApiAccessAdapter::get_modules(std::vector<ProcessModule> &modules)
{
    log_debug("Getting modules for WinApiAccessAdapter");
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_processId);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    MODULEENTRY32 me32{};
    me32.dwSize = sizeof(MODULEENTRY32);
    if (Module32First(snapshot, &me32))
    {
        do
        {
            ProcessModule module;
            module.name = std::string((char *)me32.szModule);
            module.base = (uintptr_t)me32.modBaseAddr;
            module.size = me32.modBaseSize;
            modules.push_back(module);
        } while (Module32Next(snapshot, &me32));
    }

    CloseHandle(snapshot);
    return true;
}

ScatterHandle WinApiAccessAdapter::create_scatter_handle()
{
    return new WinApiScatterHandle();
}

void WinApiAccessAdapter::add_scatter_read(uintptr_t address, void *buffer, size_t size)
{
    initialize_scatter_handle();
    if (m_scatter_handle == nullptr)
        return;
        
    WinApiScatterHandle* scatterHandle = static_cast<WinApiScatterHandle*>(m_scatter_handle);
    ScatterReadEntry entry;
    entry.address = address;
    entry.buffer = buffer;
    entry.size = size;
    scatterHandle->entries.push_back(entry);
}

bool WinApiAccessAdapter::execute_scatter_read()
{
    if (m_scatter_handle == nullptr)
        return false;
        
    WinApiScatterHandle* scatterHandle = static_cast<WinApiScatterHandle*>(m_scatter_handle);
    bool allSuccess = true;
    
    for (const auto& entry : scatterHandle->entries)
    {
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(m_process_handle, (LPCVOID)entry.address, entry.buffer, entry.size, &bytesRead) || 
            bytesRead != entry.size)
        {
            allSuccess = false;
        }
    }
    
    scatterHandle->entries.clear();
    
    return allSuccess;
}

void WinApiAccessAdapter::destroy_scatter_handle(ScatterHandle handle)
{
    if (handle != nullptr)
    {
        WinApiScatterHandle* scatterHandle = static_cast<WinApiScatterHandle*>(handle);
        delete scatterHandle;
    }
}

bool WinApiAccessAdapter::set_mouse_position(const vec2_t<int> &position)
{
    mouse_event(MOUSEEVENTF_MOVE, position.x, position.y, 0, 0);
    return true;
}

bool WinApiAccessAdapter::set_left_mouse_button(bool state)
{
    if (state)
    {
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    }
    else
    {
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    }
    return true;
}

bool WinApiAccessAdapter::get_key_state(int key)
{
    return GetAsyncKeyState(key) & 1;
}