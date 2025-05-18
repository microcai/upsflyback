
#include "app.hpp"
#include <algorithm>
#include <numeric>

#include "app_display_model.hpp"
#include "mcu_coro.hpp"

template <typename T>
T clamp(T input, T min, T max)
{
	if (input >= max)
		return max;
	else if (input <= min)
		return min;
	return input;
}

APP::APP()
{
}

const app_status* APP::get_status() const
{
	return & m_status;
}

const display_model* APP::get_display_model() const
{
	return & _app_model;
}

void APP::loop()
{}

void APP::update_ADC(int Iload, int Ibat_setting)
{
	_app_model.Iload = _app_model.Iload/2 + Iload /2;
	_app_model.Ibat_setting = Ibat_setting;
}

void APP::update_INA226(float V, float Ibat)
{
	_app_model.Voltage = V*10.0;
	_app_model.Ibat = Ibat*10.0;

	if (Ibat > 0)
		_app_model.work_mode = display_model::DISCHARGE;
	else
	{
		if (Ibat > -0.02)
		{
			_app_model.work_mode = display_model::CV;
		}
		else
		{
			_app_model.work_mode = display_model::CC_charge;
		}
	}
}

void APP::enter_sleep()
{
	_app_model.page = display_model::sleep_mode;
}

void APP::leave_sleep()
{
	_app_model.page = display_model::run_info_page;
}
