

#include "py32f030x8.h"
#include "wiring_time.h"
#if defined(BOARD_PY32)

#include "Arduino.h"
#include "wiring_digital.h"
#include "app.hpp"
#include <algorithm>

#include "coroutine.hpp"
#include "mcu_coro.hpp"
#include "pins.hpp"

// #include <Wire.h>

#include "SSD1306AsciiWire.h"
#include "SSD1306Ascii.h"

#include "oled_updater.hpp"

#include "py32f0xx_ll_adc.h"
#include "py32f0xx_ll_tim.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_iwdg.h"
#include "py32f0xx_ll_rcc.h"
#include "py32f0xx_ll_system.h"
#include <py32f0xx_hal_adc.h>
#include <py32f0xx_hal_dma.h>
#include <py32f0xx_ll_dma.h>
#include <py32f0xx_ll_crc.h>
#include <py32f0xx_ll_exti.h>
#include <py32f0xx_ll_pwr.h>
#include <py32f0xx_ll_cortex.h>

#include "INA226.h"

INA226 INA0(0x40);

APP* app = nullptr;

volatile int DC_sense_value = 0;

extern void init_adc();

/**
 * @brief  IWDG configuration
 * @param  None
 * @retval None
 */
void APP_IwdgConfig(void)
{
	/* Enable LSI */
	LL_RCC_LSI_Enable();
	while (LL_RCC_LSI_IsReady() == 0U)
	{
		;
	}

	/* Enable IWDG */
	LL_IWDG_Enable(IWDG);

	/* Enable write access */
	LL_IWDG_EnableWriteAccess(IWDG);

	/* Set IWDG prescaler */
	LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_8);

	/* Set watchdog reload counter */
	LL_IWDG_SetReloadCounter(IWDG, 4095); /* 31ms*/

	/* IWDG initialization */
	while (LL_IWDG_IsReady(IWDG) == 0U)
	{
		;
	}

	/* Feed watchdog */
	LL_IWDG_ReloadCounter(IWDG);
}


SSD1306AsciiWire oled(Wire);


/**
  * @brief  Configure external interrupt
  * @param  None
  * @retval None
  */
static void setup_wakeup_button(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_EXTI_InitTypeDef EXTI_InitStruct = {0};

	/* Enable GPIOA clock */
	LL_IOP_GRP1_EnableClock (LL_IOP_GRP1_PERIPH_GPIOA);

	/* Select PA06 pin */
	GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
	/* Select input mode */
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	/* Select pull-up */
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	/* Initialize GPIOA */
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* Select EXTI6 as external interrupt input */
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE2);

	/* Select EXTI6 */
	EXTI_InitStruct.Line = LL_EXTI_LINE_2;
	/* Enable */
	EXTI_InitStruct.LineCommand = ENABLE;
	/* Select interrupt mode */
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT_EVENT;
	/* Select falling edge trigger */
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
	/* Initialize external interrupt */
	LL_EXTI_Init(&EXTI_InitStruct);
	NVIC_SetPriority(EXTI2_3_IRQn, 2);
	NVIC_EnableIRQ(EXTI2_3_IRQn);
}

uint32_t last_op = 0;

void setup()
{
	delay(1000);

	pinMode(PA0, OUTPUT);
	pinMode(PB2, INPUT_FLOATING);

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_ADC1);
	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

	last_op = millis();

	digitalWrite(PA0, 0);

	app = new APP{};
	init_adc();

	setup_wakeup_button();

	Wire.begin();

	// APP_IwdgConfig();
	// digitalWrite(PA0, 1);
	// digitalWrite(PA0, 0);


	oled.begin(&Adafruit128x64, 0x3c);
	oled.setFont(TimesNewRoman16);
	oled.clear();

	if (!INA0.begin())
	{
		oled.setCursor(2, 2);
		oled.print("INA226 not found!");
		oled.setCursor(16, 4);
		oled.print("Restart in 2 sec.");
		delay(2000);
		// reboot
		NVIC_SystemReset();
	}

	INA0.configure(0.01);
	INA0.setMaxCurrentShunt(5, 0.01);

	oled_update(app,
		&oled,
		[]()
		{
			oled.begin(&Adafruit128x64, 0x3c);
			oled.clear();
		});

	// update ADC per 1ms
}

#define VDDA_APPLI ((uint32_t)3300)

/**
* @brief  Enter STOP mode
* @param  None
* @retval None
*/
static void APP_PwrEnterStopMode(void)
{
	// turn off screen
	app->enter_sleep();
	oled.clear();
	digitalWrite(PA0, 1);
	delay(1);

	LL_TIM_DisableCounter(TIM3);
	LL_SYSTICK_DisableIT();

	/* Enable PWR clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	
	/* Low power STOP voltage 1.0V */
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);

	/* Enable low power mode in STOP mode */
	LL_PWR_EnableLowPowerRunMode();
	
	/* Set SLEEPDEEP bit of Cortex System Control Register */
	LL_LPM_EnableDeepSleep();
	
	/* Request Wait For Event */
	__SEV();
	__WFE();
	__WFE();

	LL_LPM_EnableSleep();
	LL_TIM_EnableCounter(TIM3);
	LL_SYSTICK_EnableIT();
	// turn on screen
	digitalWrite(PA0, 0);
	delay(5);
	oled.begin(&Adafruit128x64, 0x3c);
	app->leave_sleep();
}

void loop()
{
	app->loop();
	mcucoro::executor::system_executor().poll();

	LL_IWDG_ReloadCounter(IWDG);

	if (INA0.isConversionReady())
		app->update_INA226(INA0.getBusVoltage(), -INA0.getCurrent());

	if (digitalRead(PB2) == 1)
	{
		last_op = millis();

		if (digitalRead(PA2) == 0)
		{
			delay(5);
			if (digitalRead(PA2) == 0)
			{
				APP_PwrEnterStopMode();
			}
		}
	}
	else
	{
		// 记录清醒时间，如果超过25s， 就进入睡眠状态.
		if (millis() - last_op >= 25000)
		{
			APP_PwrEnterStopMode();
			last_op = millis();
		}
	}

	// LL_ADC_REG_StartConversion(ADC1);
}

extern "C" void EXTI2_3_IRQHandler()
{
	LL_EXTI_ClearFlag(LL_EXTI_LINE_2);
	last_op = millis();
}

#endif
