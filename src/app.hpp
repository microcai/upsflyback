
#pragma once

#include <array>
#include "coroutine.hpp"
#include "app_display_model.hpp"
#include <type_traits>

enum class op_mode
{
    mode_VFD,
    mode_1pharse,
    mode_INV,
};

struct app_status
{
    float cur_volt;
    float setIBat;
    float ILoad;
    float IBat;
};

class APP
{
public:
    APP();

    void loop();

    const app_status* get_status() const;
    const display_model* get_display_model() const;
    void update_ADC(int Iload, int Ibat_setting);
    void update_INA226(float V, float Ibat);

    void enter_sleep();
    void leave_sleep();

private:
    display_model _app_model;
    app_status m_status;
};
