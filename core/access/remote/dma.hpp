#pragma once
#include <core/access/adapter.hpp>
#include <windows.h>
#include <tlhelp32.h>

class DMAAccessAdapter : public AccessAdapter
{
public:
    DMAAccessAdapter()
    {
    }
    ~DMAAccessAdapter()
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

    bool setMousePosition(const Vector2<int> &position) override;
    bool setLeftMouseButton(bool state) override;
    bool getKeyState(int key) override;

protected:
    ScatterHandle createScatterHandle() override;
    void destroyScatterHandle(ScatterHandle handle) override;

private:
};