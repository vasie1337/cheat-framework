#pragma once
#include "../access_adapter.hpp"

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
};