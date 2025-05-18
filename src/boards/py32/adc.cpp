
#ifdef PY32F0xx

#include <Arduino.h>

#include <algorithm>

#include "py32f0xx_ll_adc.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_system.h"
#include <py32f0xx_hal_adc.h>
#include <py32f0xx_hal_dma.h>
#include <py32f0xx_ll_dma.h>
#include "py32f0xx_ll_tim.h"

#include "app.hpp"

static __IO uint32_t ADCxConvertedDatas[4];
extern APP* app;

/**
 * @brief  ADC calibration function
 * @param  None
 * @retval None
 */
static void APP_AdcCalibrate(void)
{
	if (LL_ADC_IsEnabled(ADC1) == 0)
	{
		LL_ADC_Reset(ADC1);
		/* Enable calibration */
		LL_ADC_StartCalibration(ADC1);

		while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0)
			;

		/* Delay between ADC calibration end and ADC enable: minimum 4 ADC Clock cycles */
		delay(10);
	}
}

/**
 * @brief  DMA configuration function
 * @param  None
 * @retval None
 */
static void APP_DmaConfig()
{
	/* ADC corresponds to channel LL_DMA_CHANNEL_1 */
	LL_SYSCFG_SetDMARemap_CH1(LL_SYSCFG_DMA_MAP_ADC);

	LL_DMA_ConfigTransfer(DMA1, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_PERIPH_TO_MEMORY |
		LL_DMA_MODE_CIRCULAR                   |
		LL_DMA_PERIPH_NOINCREMENT  |
		LL_DMA_MEMORY_INCREMENT  |
		LL_DMA_PDATAALIGN_WORD |
		LL_DMA_MDATAALIGN_WORD |
		LL_DMA_PRIORITY_HIGH);

	/* Configure DMA data length as 4 */
	LL_DMA_SetDataLength(
		DMA1, LL_DMA_CHANNEL_1, std::distance(std::begin(ADCxConvertedDatas), std::end(ADCxConvertedDatas)));

	/* Configure DMA peripheral and memory addresses */
	LL_DMA_ConfigAddresses(DMA1,
		LL_DMA_CHANNEL_1,
		(uint32_t)&ADC1->DR,
		(uint32_t)ADCxConvertedDatas,
		LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1));

	/* Enable DMA transfer complete interrupt */
	// LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);

	// /* DMA interrupt configuration */
	// NVIC_SetPriority(DMA1_Channel1_IRQn, 0);
	// NVIC_EnableIRQ(DMA1_Channel1_IRQn);

	/* Enable DMA */
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

	NVIC_EnableIRQ(DMA1_Channel1_IRQn);
	NVIC_SetPriority(DMA1_Channel1_IRQn, 2);

	LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);
}

static int get_volt()
{
	// 0 - 4095 map to 0 - 18.3v
	return ADCxConvertedDatas[3] * 183 / 4095;
}

static int get_Iload()
{
	return ADCxConvertedDatas[1] * 66 / 4095;
}

static int get_Ibat()
{
	return ADCxConvertedDatas[2] * 66 / 4095;
}

static int get_Ibat_setting()
{
	return ADCxConvertedDatas[0] * 66 / 4095;
}

extern "C" void DMA1_Channel1_IRQHandler(void)
{
	LL_DMA_ClearFlag_TC1(DMA1);
	// void APP::update_ADC(int V, int Iload, int Ibat, int Ibat_setting)//
	// ADCxConvertedDatas[0]; // setting
	// ADCxConvertedDatas[1]; // iload
	// ADCxConvertedDatas[2]; // ibat
	// ADCxConvertedDatas[3]; // Volt
	app->update_ADC(get_Iload(), get_Ibat_setting());
}

/**
 * @brief  ADC configuration function
 * @param  None
 * @retval None
 */
static void APP_AdcConfig(void)
{
	/* Enable GPIOA clock */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

	LL_TIM_InitTypeDef tim3_init{

		.Prescaler = 0,
		.CounterMode = TIM_COUNTERMODE_UP,
		.Autoreload = SystemCoreClock/ 1000 -1,
		.ClockDivision = TIM_CLOCKDIVISION_DIV1,
		.RepetitionCounter = 0,
	};

	LL_TIM_Init(TIM3,  &tim3_init);
	LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_UPDATE);
	LL_TIM_SetClockSource(TIM3, LL_TIM_CLOCKSOURCE_INTERNAL);
	LL_TIM_EnableCounter(TIM3);


	/* Configure pins PA4/5/6/7 as analog inputs */
	LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_1 | LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7| LL_GPIO_PIN_15, LL_GPIO_MODE_ANALOG);

	/* ADC channel and clock source should be configured when ADEN=0, others should be configured when ADSTART=0 */
	/* Set ADC clock */
	LL_ADC_SetClock(ADC1, LL_ADC_CLOCK_SYNC_PCLK_DIV4);

	/* Set 12-bit resolution */
	LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_12B);

	/* Set data alignment to right */
	LL_ADC_SetDataAlignment(ADC1, LL_ADC_DATA_ALIGN_RIGHT);

	/* Set low power mode to none */
	LL_ADC_SetLowPowerMode(ADC1, LL_ADC_LP_MODE_NONE);

	/* Set channel conversion time */
	// 15us 转换一个 通道
	LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_28CYCLES_5);

	/* Set trigger source as software */
	LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_EXT_TIM3_TRGO);

	/* Set conversion mode to single */
	LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_SINGLE);

	/* Set DMA mode to unlimited */
	LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);

	/* Set overrun management mode to data overwritten */
	LL_ADC_REG_SetOverrun(ADC1, LL_ADC_REG_OVR_DATA_OVERWRITTEN);

	/* Set scan direction to forward */
	LL_ADC_REG_SetSequencerScanDirection(ADC1, LL_ADC_REG_SEQ_SCAN_DIR_FORWARD);

	/* Configure internal conversion channel */
	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_NONE);

	/* Wait for ADC TempSensor stability */
	int wait_loop_index = ((LL_ADC_DELAY_TEMPSENSOR_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
	while (wait_loop_index != 0)
	{
		wait_loop_index--;
	}

	/* Set discontinuous mode to disabled */
	LL_ADC_REG_SetSequencerDiscont(ADC1, LL_ADC_REG_SEQ_DISCONT_DISABLE);

	/* Set channels 3/4/5/6/7 as conversion channels */
	LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_4 | LL_ADC_CHANNEL_5 | LL_ADC_CHANNEL_6 | LL_ADC_CHANNEL_7);
}

void init_adc()
{
	LL_ADC_Reset(ADC1);

	APP_AdcCalibrate();

	APP_DmaConfig();

	APP_AdcConfig();

	LL_ADC_Enable(ADC1);

	LL_ADC_REG_StartConversion(ADC1);
}
#endif
