/*------------------------includes---------------------------------*/
#include <flawless/stdtypes.h>
#include <flawless/init/systemInitializer.h>
#include <flawless/protocol/genericFlawLessProtocolApplication.h>
#include <flawless/protocol/genericFlawLessProtocol_Data.h>

#include <flawless/core/msg_msgPump.h>
#include <flawless/config/msgIDs.h>

#include <interfaces/battery.h>
#include <interfaces/configuration.h>
#include <interfaces/led.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

/*-----------------------------------------------------------------*/


/*--------------------------defines--------------------------------*/
/*bottom range of the battery*/
static const batteryVoltageLevel_t DEFAULT_LOW_THRESHOLD_VAL_V = 970U;

/*this can be considered FULL*/
static const batteryVoltageLevel_t  DEFAULT_HIGH_THRESHOLD_VAL_V = 1200U;

#define PWR_ENABLE_PORT (GPIOA)
#define PWR_ENABLE_PIN (GPIO3)
/*-----------------------------------------------------------------*/


/*--------------------------variables------------------------------*/
CONFIG_VARIABLE(batteryVoltageLevel_t, g_lowThresh, &DEFAULT_LOW_THRESHOLD_VAL_V);
CONFIG_VARIABLE(batteryVoltageLevel_t, g_highThresh, &DEFAULT_HIGH_THRESHOLD_VAL_V);

/*-----------------------------------------------------------------*/

MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(relativeBatteryVoltageBuf, batteryVoltageLevel_t, 2, MSG_ID_RELATIVE_BATTERY_VOLTAGE)


void switchAllOff()
{
	gpio_clear(PWR_ENABLE_PORT, PWR_ENABLE_PIN);
	while(1);
}

/*This function should work now with the f0*/
static void onNewPwrMeasurement(msgPump_MsgID_t i_id, const void* buf);
static void onNewPwrMeasurement(msgPump_MsgID_t i_id, const void* buf)
{
	UNUSED(i_id);
	/* if the current measurement is below the lowThresh well cut the power */
	const systemPowerState_t systemPwr = *((const systemPowerState_t*) buf);
	batteryVoltageLevel_t curLevel = systemPwr.batteryVoltcV;
	if (g_lowThresh >= curLevel)
	{
		switchAllOff();
	}

	relativeBatteryLevel_t level;
	if (curLevel < g_lowThresh)
	{
		level = 0x00;
	} else if (curLevel > g_highThresh)
	{
		level = 0xff;
	} else {
		level = ((uint32_t)(255UL * (curLevel - g_lowThresh))) / (g_highThresh - g_lowThresh);
	}

	uint8_t green = level;
	uint8_t red = 0xff - level;

	if (level >= 128)
	{
		green = 0xff;
		red   = 2 * (255 - level);
	} else
	{
		red = 0xff;
		green   = 2 * level;
	}

	setLEDColor(red, green, 0);


	msgPump_postMessage(MSG_ID_RELATIVE_BATTERY_VOLTAGE, &level);
}

void setLowerVoltageEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor);
GENERIC_PROTOCOL_ENDPOINT(setLowerVoltageEndoint, 103)
void setLowerVoltageEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor)
{
	UNUSED(i_interfaceDescriptor);
	if (i_packetLen == sizeof(uint16_t))
	{
		g_lowThresh = *((uint16_t*) ipacket);
		config_updateToFlash();
	}
}

void setUpperVoltageEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor);
GENERIC_PROTOCOL_ENDPOINT(setUpperVoltageEndoint, 102)
void setUpperVoltageEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor)
{
	UNUSED(i_interfaceDescriptor);
	if (i_packetLen == sizeof(uint16_t))
	{
		g_highThresh = *((uint16_t*) ipacket);
		config_updateToFlash();
	}
}

void getLimitsEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor);
GENERIC_PROTOCOL_ENDPOINT(getLimitsEndoint, 101)
void getLimitsEndoint(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor)
{
	UNUSED(i_packetLen);
	UNUSED(ipacket);
	uint16_t reply[2];
	reply[0] = g_lowThresh;
	reply[1] = g_highThresh;
	genericProtocol_sendMessage(i_interfaceDescriptor, 100, sizeof(reply), &reply);
}

/*This seems to be the software power off function*/
/*This function should work now with the f0*/
void masterOff(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor);
GENERIC_PROTOCOL_ENDPOINT(masterOff, 100)
void masterOff(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor)
{
	UNUSED(i_interfaceDescriptor);
	if (i_packetLen > 0U)
	{
		const uint8_t val = *((const uint8_t*)ipacket);
		if (0 != val)
		{
			switchAllOff();
		}
	}
}

/*This seems like a second init function*/
static void pwrCheck_init1(void);
MODULE_INIT_FUNCTION(pwrCheck1, 5, pwrCheck_init1)
static void pwrCheck_init1(void)
{
	RCC_AHBENR |= RCC_AHBENR_GPIOAEN;
	msgPump_registerOnMessage(MSG_ID_SYSTEM_POWER_STATE, &onNewPwrMeasurement);

	// turn on
	gpio_mode_setup(PWR_ENABLE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PWR_ENABLE_PIN);
	gpio_set_output_options(PWR_ENABLE_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, PWR_ENABLE_PIN);
	gpio_set(PWR_ENABLE_PORT, PWR_ENABLE_PIN);
}


/*-----------------------------------------------------------------*/



