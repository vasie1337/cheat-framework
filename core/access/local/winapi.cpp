#include "winapi.hpp"

#include <core/logger/logger.hpp>

bool WinApiAccessAdapter::attach(const std::string &processName)
{
    log_debug("Attaching WinApiAccessAdapter to %s", processName.c_str());
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
            if (strcmp((char *)pe32.szExeFile, processName.c_str()) == 0)
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

    m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_processId);
    if (m_processHandle == NULL)
    {
        return false;
    }

    return true;
}

void WinApiAccessAdapter::detach()
{
    log_debug("Detaching WinApiAccessAdapter");
    if (m_processHandle != NULL)
    {
        CloseHandle(m_processHandle);
        m_processHandle = NULL;
        m_processId = 0;
    }
}

bool WinApiAccessAdapter::getModules(std::vector<ProcessModule> &modules)
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
            module.baseAddress = (uintptr_t)me32.modBaseAddr;
            module.size = me32.modBaseSize;
            modules.push_back(module);
        } while (Module32Next(snapshot, &me32));
    }

    CloseHandle(snapshot);
    return true;
}

bool WinApiAccessAdapter::read(uintptr_t address, void *buffer, size_t size)
{
    if (ReadProcessMemory(m_processHandle, (LPCVOID)address, buffer, size, NULL))
    {
        return true;
    }
    return false;
}

bool WinApiAccessAdapter::write(uintptr_t address, const void *buffer, size_t size)
{
    if (WriteProcessMemory(m_processHandle, (LPVOID)address, buffer, size, NULL))
    {
        return true;
    }
    return false;
}

bool WinApiAccessAdapter::setMousePosition(const Vector2<int> &position)
{
    mouse_event(MOUSEEVENTF_MOVE, position.x, position.y, 0, 0);
    return true;
}

bool WinApiAccessAdapter::setLeftMouseButton(bool state)
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

bool WinApiAccessAdapter::getKeyState(int key)
{
    return GetAsyncKeyState(key) & 1;
}