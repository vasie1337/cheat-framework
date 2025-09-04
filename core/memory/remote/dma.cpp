#include "dma.hpp"

bool DMAAccessAdapter::attach(const std::string &process_name)
{
    return false;
}

void DMAAccessAdapter::detach()
{
}

bool DMAAccessAdapter::get_modules(std::vector<ProcessModule> &modules)
{
    return false;
}

void DMAAccessAdapter::add_scatter_read(uintptr_t address, void *buffer, size_t size)
{
}

bool DMAAccessAdapter::execute_scatter_read()
{
    return false;
}

ScatterHandle DMAAccessAdapter::create_scatter_handle()
{
    return nullptr;
}

void DMAAccessAdapter::destroy_scatter_handle(ScatterHandle handle)
{
}

bool DMAAccessAdapter::set_mouse_position(const vec2_t<int> &position)
{
    return false;
}

bool DMAAccessAdapter::set_left_mouse_button(bool state)
{
    return false;
}

bool DMAAccessAdapter::get_key_state(int key)
{
    return false;
}