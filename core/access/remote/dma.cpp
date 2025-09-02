#include "dma.hpp"

bool DMAAccessAdapter::attach(const std::string &processName)
{
    return false;
}

void DMAAccessAdapter::detach()
{
}

bool DMAAccessAdapter::getModules(std::vector<ProcessModule> &modules)
{
    return false;
}

bool DMAAccessAdapter::read(uintptr_t address, void *buffer, size_t size)
{
    return false;
}

bool DMAAccessAdapter::write(uintptr_t address, const void *buffer, size_t size)
{
    return false;
}

void DMAAccessAdapter::addScatterRead(uintptr_t address, void *buffer, size_t size)
{
}

bool DMAAccessAdapter::executeScatterRead()
{
    return false;
}

ScatterHandle DMAAccessAdapter::createScatterHandle()
{
    return nullptr;
}

void DMAAccessAdapter::destroyScatterHandle(ScatterHandle handle)
{
}

bool DMAAccessAdapter::setMousePosition(const Vector2<int> &position)
{
    return false;
}

bool DMAAccessAdapter::setLeftMouseButton(bool state)
{
    return false;
}

bool DMAAccessAdapter::getKeyState(int key)
{
    return false;
}