/*
 * measuring.c
 *
 *  Created on: 09.09.2012
 *      Author: lutz
 */


#include <flawless/init/systemInitializer.h>
#include <flawless/rpc/RPC.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <flawless/core/msg_msgPump.h>

#include <flawless/config/msgIDs.h>

#include <flawless/timer/swTimer.h>

#include "battery.h"

typedef enum tag_measuringState
{
	MEASURING_BAT_VOLTAGE,
	MEASURING_BAT_LOAD
}measuringState_t;

static volatile measuringState_t g_measuringState;

MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(pwrMeasurementVolt, batteryLevel_t, 2, MSG_ID_BATTERY_VOLTAGE)
MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(pwrMeasurementLoad, batteryLevel_t, 2, MSG_ID_BATTERY_LOAD)

static uint32_t g_batteryVoltageLevelAccmulated;
static uint32_t g_batteryLoadLevelAccmulated;

static uint16_t g_batteryVoltageLevel;
static uint16_t g_batteryLoadLevel;

#define EXTERNAL_RESOSTOR_DIVIDER_VOLTAGE 4UL
#define EXTERNAL_RESOSTOR_DIVIDER_LOAD 1UL

#define MEASURING_CONVERSION_FACTOR_VOLTAGE 5UL * (EXTERNAL_RESOSTOR_DIVIDER_VOLTAGE) / 10UL
#define MEASURING_CONVERSION_FACTOR_LOAD    5UL * (EXTERNAL_RESOSTOR_DIVIDER_LOAD) / 10UL

#define MEASUREMENTS_FOR_SMOOTHING 32U
static uint16_t g_measureCnt = 0U;

static void init_measuring(void);
MODULE_INIT_FUNCTION(measuring, 8, init_measuring)
static void init_measuring(void)
{
	g_measuringState = MEASURING_BAT_VOLTAGE;
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS2);
	/*
	 *  Start the AD conversion
	 */
	g_batteryVoltageLevelAccmulated = 0;
	g_batteryLoadLevelAccmulated    = 0;

	g_measureCnt = 0U;

	ADCSRA |= (1 << ADSC);
}

ISR(ADC_vect)
{
	/*
	 *  Read the AD conversion result (16 bit)
	 */
	const uint16_t curMeasurement = (batteryLevel_t)ADCW;
	if (MEASURING_BAT_VOLTAGE == g_measuringState)
	{
		g_batteryVoltageLevelAccmulated += curMeasurement;
		++g_measureCnt;
		if (MEASUREMENTS_FOR_SMOOTHING <= g_measureCnt)
		{
			g_batteryVoltageLevelAccmulated = g_batteryVoltageLevelAccmulated / MEASUREMENTS_FOR_SMOOTHING;
			g_batteryVoltageLevel = g_batteryVoltageLevelAccmulated * MEASURING_CONVERSION_FACTOR_VOLTAGE;

			msgPump_postMessage(MSG_ID_BATTERY_VOLTAGE, &g_batteryVoltageLevel);
			g_measureCnt = 0U;
			g_batteryVoltageLevelAccmulated = 0;
			g_measuringState = MEASURING_BAT_LOAD;
			ADMUX |= (1 << MUX0);
		}
	} else
	{
		g_batteryLoadLevelAccmulated += curMeasurement;
		++g_measureCnt;
		if (MEASUREMENTS_FOR_SMOOTHING <= g_measureCnt)
		{
			g_batteryLoadLevelAccmulated = g_batteryLoadLevelAccmulated / MEASUREMENTS_FOR_SMOOTHING;
			g_batteryLoadLevel = g_batteryLoadLevelAccmulated * MEASURING_CONVERSION_FACTOR_LOAD;

			msgPump_postMessage(MSG_ID_BATTERY_LOAD, &g_batteryLoadLevel);
			g_measureCnt = 0U;
			g_batteryLoadLevelAccmulated = 0;
			g_measuringState = MEASURING_BAT_VOLTAGE;
			ADMUX &= ~(1 << MUX0);
		}
	}

	/*
	 * start new conversion
	 */
	ADCSRA |= (1 << ADSC);
}

