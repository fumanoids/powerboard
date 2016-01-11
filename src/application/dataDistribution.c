/*
 * dataDistribution.c
 *
 *  Created on: Mar 6, 2013
 *      Author: lutz
 */

#include <flawless/init/systemInitializer.h>
#include <flawless/core/msg_msgPump.h>
#include <flawless/protocol/msgProxy.h>

#include "interfaces/battery.h"

#define DISTRIBUTION_INTERVAL 50UL

MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(systemPowerStateOut, systemPowerState_t, 2, MSG_ID_SYSTEM_POWER_STATE_OUT)
MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(relVoltageOut, relativeBatteryLevel_t, 2, MSG_ID_RELATIVE_BATTERY_VOLTAGE_OUT)

static void onSystemMeasurement(msgPump_MsgID_t msgId, const void *dataPtr);
static void onSystemMeasurement(msgPump_MsgID_t msgId, const void *dataPtr)
{
	UNUSED(msgId);
	static uint32_t counter = 0U;
	++counter;
	if (counter == DISTRIBUTION_INTERVAL) {
		msgPump_postMessage(MSG_ID_SYSTEM_POWER_STATE_OUT, dataPtr);
		counter = 0;
	}
}

static void onRelativeMeasurement(msgPump_MsgID_t msgId, const void *dataPtr);
static void onRelativeMeasurement(msgPump_MsgID_t msgId, const void *dataPtr)
{
	UNUSED(msgId);
	static uint32_t counter = 0U;
	++counter;
	if (counter == DISTRIBUTION_INTERVAL) {
		msgPump_postMessage(MSG_ID_RELATIVE_BATTERY_VOLTAGE_OUT, dataPtr);
		counter = 0;
	}
}


static void initDataDistribution(void);
MODULE_INIT_FUNCTION(dataDistribution, 9, initDataDistribution)
static void initDataDistribution(void)
{
	msgPump_registerOnMessage(MSG_ID_SYSTEM_POWER_STATE, &onSystemMeasurement);
	msgPump_registerOnMessage(MSG_ID_RELATIVE_BATTERY_VOLTAGE, &onRelativeMeasurement);

	msgProxy_addMsgForBroadcast(MSG_ID_SYSTEM_POWER_STATE_OUT);
	msgProxy_addMsgForBroadcast(MSG_ID_RELATIVE_BATTERY_VOLTAGE_OUT);
}

