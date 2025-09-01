#include "dma.hpp"

DMAAccessAdapter::DMAAccessAdapter() {
}

DMAAccessAdapter::~DMAAccessAdapter() {
}

bool DMAAccessAdapter::attach(const std::string& processName) {
    return false;
}

bool DMAAccessAdapter::detach() {
    return false;
}

bool DMAAccessAdapter::getModules(std::vector<ProcessModule>& modules) {
    return false;
}

bool DMAAccessAdapter::read(uintptr_t address, void* buffer, size_t size) {
    return false;
}

bool DMAAccessAdapter::write(uintptr_t address, const void* buffer, size_t size) {
    return false;
}

bool DMAAccessAdapter::setMousePosition(const Vector2<int>& position) {
    return false;
}

bool DMAAccessAdapter::setLeftMouseButton(bool state) {
    return false;
}

bool DMAAccessAdapter::getKeyState(int key) {
    return false;
}