/*
 * powerCheck.c
 *
 *  Created on: 08.09.2012
 *      Author: lutz
 */
#include <flawless/stdtypes.h>
#include <flawless/init/systemInitializer.h>
#include <flawless/protocol/genericFlawLessProtocolApplication.h>
#include <flawless/protocol/genericFlawLessProtocol_Data.h>

#include <flawless/core/msg_msgPump.h>
#include <flawless/config/msgIDs.h>

#include "battery.h"
#include "eeprom_addrs.h"

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#define PWR_ENABLE_PORT      (PORTC)
#define PWR_ENABLE_DIRECTION (DDRC)
#define PWR_ENABLE_PIN       (PC2)


#define DEFAULT_LOW_THRESHOLD_VAL_V 1300U

/*
 * this can be considered FULL
 */
#define DEFAULT_HIGH_THRESHOLD_VAL_V 1600U


static batteryLevel_t g_lowThresh;
static batteryLevel_t g_highThresh;

MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(relativeBatteryVoltage, relativeBatteryLevel_t, 1U, MSG_ID_RELATIVE_BATTERY_VOLTAGE)

relativeBatteryLevel_t g_level = 0xff;

static void onNewPwrMeasurement(msgPump_MsgID_t i_id, const void* buf);
static void onNewPwrMeasurement(msgPump_MsgID_t i_id, const void* buf)
{
	/* if the current measurement is below the lowThresh well cut the power */
	const batteryLevel_t curLevel = *((const batteryLevel_t*) buf);
	if (g_lowThresh >= curLevel)
	{
		/* this will cut off the battery */
		PWR_ENABLE_PORT = 0;
	}
	{
		if (curLevel < g_lowThresh)
		{
			g_level = 0x00;
		} else if (curLevel > g_highThresh)
		{
			g_level = 0xff;
		} else
		{
			const batteryLevel_t relative = ((uint32_t)(255UL * (curLevel - g_lowThresh))) / (g_highThresh - g_lowThresh);
			g_level = (relative);
		}

		msgPump_postMessage(MSG_ID_RELATIVE_BATTERY_VOLTAGE, &g_level);
	}
}

static void pwrCheck_init0(void);
MODULE_INIT_FUNCTION(pwrCheck0, 0, pwrCheck_init0)
static void pwrCheck_init0(void)
{
	/* enable pwr suply for the whole board and hold it */
	PWR_ENABLE_DIRECTION |= 1 << PWR_ENABLE_PIN;
	PWR_ENABLE_PORT |= (1<<PWR_ENABLE_PIN);

	/* FIXME: ugly hack to circumvent the eeprom usage
	g_lowThresh  = eeprom_read_word((batteryLevel_t*)LOW_THRESH_EEPROM_ADDR);
	g_highThresh = eeprom_read_word((batteryLevel_t*)HIGH_THRESH_EEPROM_ADDR);

	if ((0xffff == g_lowThresh) || (0x0000 == g_lowThresh))
	{
		g_lowThresh = DEFAULT_LOW_THRESHOLD_VAL_V;
	}
	if ((0xffff == g_highThresh) || (0x0000 == g_highThresh))
	{
		g_highThresh = DEFAULT_HIGH_THRESHOLD_VAL_V;
	}
	*/

	g_lowThresh = DEFAULT_LOW_THRESHOLD_VAL_V;
	g_highThresh = DEFAULT_HIGH_THRESHOLD_VAL_V;
}

void setUpperVoltageEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor);
GENERIC_PROTOCOL_ENDPOINT(setUpperVoltageEndoint, 102)
void setUpperVoltageEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor)
{
	if (i_packetLen == sizeof(g_highThresh))
	{
		g_highThresh = *((batteryLevel_t*) ipacket);
		eeprom_write_word((batteryLevel_t*)HIGH_THRESH_EEPROM_ADDR, g_highThresh);
	}
}

void setLowerVoltageEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor);
GENERIC_PROTOCOL_ENDPOINT(setLowerVoltageEndoint, 103)
void setLowerVoltageEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor)
{
	if (i_packetLen == sizeof(g_lowThresh))
	{
		g_lowThresh = *((batteryLevel_t*) ipacket);
		eeprom_write_word((batteryLevel_t*)LOW_THRESH_EEPROM_ADDR, g_lowThresh);
	}
}

void getLimitsEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor);
GENERIC_PROTOCOL_ENDPOINT(getLimitsEndoint, 101)
void getLimitsEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor)
{
	uint16_t reply[2];
	reply[0] = g_lowThresh;
	reply[1] = g_highThresh;
	genericProtocol_sendMessage(0, 100, sizeof(reply), &reply);
}


void masterOff(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor);
GENERIC_PROTOCOL_ENDPOINT(masterOff, 100)
void masterOff(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor)
{
	if (i_packetLen > 0U)
	{
		const uint8_t val = *((const uint8_t*)ipacket);
		if (0 != val)
		{
			/* switch off */
			PWR_ENABLE_PORT &= ~(1<<PWR_ENABLE_PIN);
			while (1);
		}
	}
}


static void pwrCheck_init1(void);
MODULE_INIT_FUNCTION(pwrCheck1, 7, pwrCheck_init1)
static void pwrCheck_init1(void)
{
	msgPump_registerOnMessage(MSG_ID_BATTERY_VOLTAGE, &onNewPwrMeasurement);
}
