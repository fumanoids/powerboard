/*
 * dataDistribution.c
 *
 *  Created on: Mar 6, 2013
 *      Author: lutz
 */

#include <flawless/init/systemInitializer.h>
#include <flawless/core/msg_msgPump.h>
#include <flawless/protocol/msgProxy.h>

#include "battery.h"

#define DISTRIBUTION_INTERVAL 255UL

MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(voltOut, batteryLevel_t, 2, MSG_ID_BATTERY_VOLTAGE_OUT)
MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(voltRelOut, uint8_t, 2, MSG_ID_RELATIVE_BATTERY_VOLTAGE_OUT)

static void onMeasurement(msgPump_MsgID_t msgId, const void *dataPtr);
static void onMeasurement(msgPump_MsgID_t msgId, const void *dataPtr)
{
	static uint16_t voltCounter = 0U;
	static uint16_t voltRelCounter = 0U;
	void *ptr = (void*) dataPtr;
	switch (msgId)
	{
	case MSG_ID_BATTERY_VOLTAGE:
		++voltCounter;
		if (DISTRIBUTION_INTERVAL <= voltCounter)
		{
			msgPump_postMessage(MSG_ID_BATTERY_VOLTAGE_OUT, ptr);
			voltCounter = 0U;
		}
		break;

	case MSG_ID_RELATIVE_BATTERY_VOLTAGE:
		++voltRelCounter;
		if (DISTRIBUTION_INTERVAL <= voltRelCounter)
		{
			msgPump_postMessage(MSG_ID_RELATIVE_BATTERY_VOLTAGE_OUT, ptr);
			voltRelCounter = 0U;
		}
		break;

	default:
		break;
	}
}

static void initDataDistribution(void);
MODULE_INIT_FUNCTION(dataDistribution, 9, initDataDistribution)
static void initDataDistribution(void)
{
	msgPump_registerOnMessage(MSG_ID_BATTERY_VOLTAGE, &onMeasurement);
	msgPump_registerOnMessage(MSG_ID_RELATIVE_BATTERY_VOLTAGE, &onMeasurement);

	msgProxy_addMsgForBroadcast(MSG_ID_BATTERY_VOLTAGE_OUT);
	msgProxy_addMsgForBroadcast(MSG_ID_RELATIVE_BATTERY_VOLTAGE_OUT);
}

