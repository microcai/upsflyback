
#pragma once

#include "app_display_model.hpp"
#include "coroutine.hpp"
#include "mcu_coro.hpp"
#include "SSD1306Ascii.h"
#include "app.hpp"

#include "awaitable.hpp"

static int dec_to_str(char* buf, int num)
{
	if (num == 0)
	{
		*buf = '0';
		return 1;
	}
	int len = 1;
	int num_ = num;
	while (num_ /= 10)
		len ++;
	buf += len-1;
	num_ = num;
	while(num_ > 0)
	{
		*buf = num_ % 10 + '0';
		buf --;
		num_ /=10;
	}

	return len;
}

static const char * to_str(int value_premul_by_10)
{
	static char buffer[32];
	memset(buffer, 0, 32);
	buffer[0] = (value_premul_by_10 >= 0) ? ' ' : '-';

	// 接着格式化 整数部分
	int abs_value = value_premul_by_10 >0 ? value_premul_by_10 : -value_premul_by_10;

	int part1 = abs_value / 10;
	int part2 = abs_value % 10;

	int len = dec_to_str(buffer +1, part1);

	if (part2 !=0)
	{
		buffer[1+len] = '.';
		dec_to_str(buffer + len + 2, part2);
	}

	return buffer;
}

static inline mcucoro::awaitable<void> async_oled_display_normal(APP* app, SSD1306Ascii* oled)
{
	auto model = app->get_display_model();
	if (model->page == display_model::run_info_page)
	{
		oled->setFont(TimesNewRoman16);
		oled->setCursor(0, 0);
		oled->print("Out:");
		oled->print(to_str(model->Voltage));
		oled->print("V");
		oled->clear(oled->col(), 69, 0, 0);
		oled->setCursor(70, 0);
		oled->print("Set:");
		oled->print(to_str(model->Ibat_setting));
		oled->print("A");
		oled->clearToEOL();
		oled->setCursor(0, 2);
		oled->print("Load: ");
		oled->print(to_str(model->Iload));
		oled->print("A");
		oled->clearToEOL();
		oled->setCursor(0, 4);
		oled->print("Bat: ");
		oled->print(to_str(model->Ibat));
		oled->print("A");
		oled->clear(oled->col(), 71, 4, 5);
		oled->setCursor(72, 4);
		oled->print("M: ");
		switch (model->work_mode)
		{
		case display_model::CC_charge:
			oled->print("CC");
			/* code */
			break;
		case display_model::CV:
			oled->print("CV");
			/* code */
			break;
		case display_model::DISCHARGE:
			oled->print("DISCHARGE");
			/* code */
			break;
		default:
			break;
		}
		oled->clearToEOL();
		oled->setCursor(0, 6);
		oled->print("Total: ");
		if (model->Ibat <0 )
			oled->print(to_str(model->Iload - model->Ibat));
		else
			oled->print("0");
		oled->print("A");

		oled->clearToEOL();
	}
	co_return ;
}

template <typename DisplayIniter>
mcucoro::awaitable<void> oled_update(APP* app, SSD1306Ascii* oled, DisplayIniter init_oled)
{
	int last_page = 999;


	for (;;)
	{
		auto model = app->get_display_model();
		if (last_page!=model->page)
		{
			last_page = model->page;
			oled->clear();
			co_await coro_delay_ms(10);
		}

		if (model->page == display_model::run_info_page)
			co_await async_oled_display_normal(app, oled);
		else if (model->page == display_model::sleep_mode)
		{
			co_await coro_delay_ms(500);
		}

		if (oled->getWriteError() )
		{
			oled->clearWriteError();
			init_oled();
		}

		co_await coro_delay_ms(10);
	}

};
