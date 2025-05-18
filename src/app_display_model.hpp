
#pragma once

struct display_model
{

    enum display_page
    {
        run_info_page,
        sleep_mode,
    };

    #define oled_display_model_max_page 1

    int page = 0;

    int Voltage = 120; // 12.0
    int Iload =0;// 0.0A
    int Ibat = 0;
    int Ibat_setting = 0;
    enum work_mode_t {
        CV,
        CC_charge,    
        DISCHARGE,
    } work_mode = CV;
};
