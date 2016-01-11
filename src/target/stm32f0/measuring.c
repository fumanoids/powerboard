/*------------------------includes---------------------------------*/
#include <flawless/init/systemInitializer.h>
#include <flawless/core/msg_msgPump.h>
#include <flawless/config/msgIDs.h>
#include <flawless/platform/system.h>
#include <flawless/timer/swTimer.h>

/*new includes:*/
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/f0/nvic.h>

#include <interfaces/battery.h>
/*-----------------------------------------------------------------*/

/*--------------------------defines--------------------------------*/

#define RESISTANCE_FACTOR_BATTERY 11
#define RESISTANCE_FACTOR_5V 11
#define RESISTANCE_FACTOR_SERVO 11

#define MEASURING_INTERVAL_US 500
/*-----------------------------------------------------------------*/

/*--------------------------variables------------------------------*/

MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(systemPowerState, systemPowerState_t, 2, MSG_ID_SYSTEM_POWER_STATE)

MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(pwrMeasurementRaw, voltageMeasurementsRaw_t, 2, MSG_ID_VOLTAGE_RAW)
MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(pwrMeasurementCalib, voltageMeasurementsRaw_t, 1, MSG_ID_VOLTAGE_CALIBRATION)

//the measured values
static voltageMeasurementsRaw_t* g_curBuffer;
static bool g_calibrationMeasurementDone;

/*-----------------------------------------------------------------*/

/*---------------------------methods-------------------------------*/


int32_t g_offsetVoltages[MEASURING_CHANNEL_COUNT];
static void onRawCalibMeasurement(msgPump_MsgID_t msgId, const void *dataPtr);
static void onRawCalibMeasurement(msgPump_MsgID_t msgId, const void *dataPtr)
{
	UNUSED(msgId);
	voltageMeasurementsRaw_t* rawData = (voltageMeasurementsRaw_t*) dataPtr;
	for (uint32_t i = 0; i < MEASURING_CHANNEL_COUNT; ++i) {
		g_offsetVoltages[i] = 0;
	}
	// smooth the data
	for (uint32_t i = 0; i < (sizeof(*rawData) / sizeof(**rawData)); ++i) {
		g_offsetVoltages[i % MEASURING_CHANNEL_COUNT] += (*rawData)[i];
	}

	for (uint32_t i = 0; i < MEASURING_CHANNEL_COUNT; ++i) {
		g_offsetVoltages[i] = g_offsetVoltages[i] / ((sizeof(*rawData) / sizeof(**rawData)) / MEASURING_CHANNEL_COUNT);
	}
}

static void onRawMeasurement(msgPump_MsgID_t msgId, const void *dataPtr);
static void onRawMeasurement(msgPump_MsgID_t msgId, const void *dataPtr)
{
	UNUSED(msgId);
	systemPowerState_t* powerStatePtr = NULL;
	msgPump_getFreeBuffer(MSG_ID_SYSTEM_POWER_STATE, (void**)&powerStatePtr);
	if (NULL != powerStatePtr) {
		voltageMeasurementsRaw_t* rawData = (voltageMeasurementsRaw_t*) dataPtr;

		int32_t voltages[MEASURING_CHANNEL_COUNT];

		for (uint32_t i = 0; i < MEASURING_CHANNEL_COUNT; ++i)
		{
			voltages[i] = 0;
		}
		// smooth the data
		for (uint32_t i = 0; i < (sizeof(*rawData) / sizeof(**rawData)); ++i)
		{
			voltages[i % MEASURING_CHANNEL_COUNT] += (*rawData)[i];
		}

		for (uint32_t i = 0; i < MEASURING_CHANNEL_COUNT; ++i)
		{
			voltages[i] = voltages[i] / ((sizeof(*rawData) / sizeof(**rawData)) / MEASURING_CHANNEL_COUNT);
		}

		voltages[MEASURING_BAT_VOLTAGE]   = voltages[MEASURING_BAT_VOLTAGE] * 330 * RESISTANCE_FACTOR_BATTERY / 4096;
		voltages[MEASURING_5V_VOLTAGE]    = voltages[MEASURING_5V_VOLTAGE] * 330 * RESISTANCE_FACTOR_5V / 4096;
		voltages[MEASURING_MOTOR_VOLTAGE] = voltages[MEASURING_MOTOR_VOLTAGE] * 330 * RESISTANCE_FACTOR_SERVO / 4096;

		voltages[MEASURING_5V_CURRENT]    = (g_offsetVoltages[MEASURING_5V_CURRENT] - voltages[MEASURING_5V_CURRENT]) * 330000 / 9 / 4096;
		voltages[MEASURING_MOTOR_CURRENT] = (g_offsetVoltages[MEASURING_MOTOR_CURRENT] - voltages[MEASURING_MOTOR_CURRENT]) * 330000 / 9 / 4096;

		powerStatePtr->batteryVoltcV   = voltages[MEASURING_BAT_VOLTAGE];
		powerStatePtr->motorVoltcV     = voltages[MEASURING_MOTOR_VOLTAGE];
		powerStatePtr->systemVoltcV    = voltages[MEASURING_5V_VOLTAGE];
		powerStatePtr->motorCurrentcA  = voltages[MEASURING_MOTOR_CURRENT];
		powerStatePtr->systemCurrentcA = voltages[MEASURING_5V_CURRENT];

		msgPump_postMessage(MSG_ID_SYSTEM_POWER_STATE, powerStatePtr);
	}
}


static void setupADC()
{
	ADC1_CR |= ADC_CR_ADEN;
	while (0 == (ADC1_ISR & ADC_ISR_ADRDY));

	ADC1_SMPR = 3;
	ADC1_CHSELR = (BIT0 | BIT1 | BIT2 | BIT4 | BIT7);

	ADC1_CFGR1 = ADC_CFGR1_WAIT | ADC_CFGR1_CONT | ADC_CFGR1_DMAEN;
}

static void triggerMeasurement()
{
	system_mutex_lock();
	if (g_curBuffer == NULL) {
		msgPump_getFreeBuffer(MSG_ID_VOLTAGE_RAW, (void*)(&g_curBuffer));
		if (g_curBuffer != NULL) {
			/* set memory adress */
			DMA1_CMAR1 = (uint32_t) (g_curBuffer);
			/* set the number of transmits */
			uint16_t size = (sizeof(*g_curBuffer) / sizeof(**g_curBuffer));
			DMA1_CNDTR1 = size;

			/* enable channel1 */
			DMA1_CCR1 |= DMA_CCR_EN;

			/* start ADC */
			ADC1_CR |= ADC_CR_ADSTART;
		}
	}
	system_mutex_unlock();
}

static void triggerCalibration()
{
	system_mutex_lock();
	if (g_curBuffer == NULL) {
		msgPump_getFreeBuffer(MSG_ID_VOLTAGE_CALIBRATION, (void*)(&g_curBuffer));
		if (g_curBuffer != NULL) {
			/* set memory adress */
			DMA1_CMAR1 = (uint32_t) (g_curBuffer);
			/* set the number of transmits */
			uint16_t size = (sizeof(*g_curBuffer) / sizeof(**g_curBuffer));
			DMA1_CNDTR1 = size;

			/* enable channel1 */
			DMA1_CCR1 |= DMA_CCR_EN;

			/* start ADC */
			ADC1_CR |= ADC_CR_ADSTART;
		}
	}
	system_mutex_unlock();
	swTimer_registerOnTimerUS(triggerMeasurement, MEASURING_INTERVAL_US, false);
}

void dma1_ch1_isr(void) {
	/*disable channel1*/
	DMA1_CCR1 &= (~DMA_CCR_EN);
	while (DMA1_CCR1 & DMA_CCR_EN); // wait for DMA to be done

	system_mutex_lock();
	// clear interrupt bits
	DMA1_IFCR = 0xf;

	if (g_calibrationMeasurementDone) {
		msgPump_postMessage(MSG_ID_VOLTAGE_RAW, g_curBuffer);
	} else {
		msgPump_postMessage(MSG_ID_VOLTAGE_CALIBRATION, g_curBuffer);
	}
	g_calibrationMeasurementDone = true;

	g_curBuffer = NULL;
	system_mutex_unlock();
}

static void onLateInit() {

	while (0 != (ADC1_CR & ADC_CR_ADEN)) {
		ADC1_CR &= ~ADC_CR_ADEN;
	}
	ADC1_CR = ADC_CR_ADCAL;
	while (ADC1_CR & ADC_CR_ADCAL);

	setupADC();
	swTimer_registerOnTimerUS(&triggerCalibration, MEASURING_INTERVAL_US, true);
}

static void init_measuring(void);
MODULE_INIT_FUNCTION(measuring, 9, init_measuring)
static void init_measuring(void)
{
	/*enable energy for adc*/
	RCC_APB2ENR |= RCC_APB2ENR_ADCEN;
	RCC_AHBENR  |= RCC_AHBENR_GPIOAEN;
	RCC_AHBENR |= RCC_AHBENR_DMAEN;

	/*-measure on ... */
	/*...	PA0 which is connected to ADC_CH0*/
	/*...	PA1 which is connected to ADC_CH1*/
	/*...	PA2 which is connected to ADC_CH2*/

	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO2);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO7);

	msgPump_registerOnMessage(MSG_ID_VOLTAGE_RAW, &onRawMeasurement);
	msgPump_registerOnMessage(MSG_ID_VOLTAGE_CALIBRATION, &onRawCalibMeasurement);

	/* read from ADC */
	DMA1_CPAR1 = (uint32_t) (&(ADC1_DR));

	/*set priority to high to prevent overrun*/
	DMA1_CCR1 = DMA_CCR_PL_MEDIUM | DMA_CCR_PSIZE_16BIT | DMA_CCR_MSIZE_16BIT | DMA_CCR_MINC | DMA_CCR_TCIE;

	/*enable dma interrupt*/
	nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);

	g_curBuffer = NULL;
	g_calibrationMeasurementDone = false;

	swTimer_registerOnTimer(&onLateInit, 100, true);
}
