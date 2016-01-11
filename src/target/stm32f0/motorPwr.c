/*
 * motorPwr.c
 *
 *  Created on: 06.01.2015
 *      Author: lutz
 */


#include <flawless/init/systemInitializer.h>
#include <flawless/protocol/genericFlawLessProtocolApplication.h>
#include <flawless/timer/swTimer.h>
#include <flawless/core/msg_msgPump.h>
#include <flawless/config/msgIDs.h>
#include <flawless/protocol/msgProxy.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "gpio_interrupts.h"

#define MOTOR_POWER_PORT GPIOB
#define MOTOR_POWER_SWITCH_PIN GPIO2

#define DEBOUNCE_TIME_MS 10

MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(motorPwrState, uint32_t, 2, MSG_ID_USER_MOTOR_SWITCH)

void setMotorPower(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor);
GENERIC_PROTOCOL_ENDPOINT(setMotorPower, 105)
void setMotorPower(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor)
{
	UNUSED(i_interfaceDescriptor);
	if (0 < i_packetLen)
	{
		const uint8_t value = *((const uint8_t*)ipacket);
		if (0 == value)
		{
			gpio_set(MOTOR_POWER_PORT, MOTOR_POWER_SWITCH_PIN);
		} else
		{
			gpio_clear(MOTOR_POWER_PORT, MOTOR_POWER_SWITCH_PIN);
		}
	}
}

static void onDebounceTimer()
{
	uint32_t motorPwrState = gpio_get(MOTOR_POWER_PORT, MOTOR_POWER_SWITCH_PIN);
	motorPwrState = !!motorPwrState;
	msgPump_postMessage(MSG_ID_USER_MOTOR_SWITCH, &motorPwrState);
	gpio_enable_interrupt(MOTOR_POWER_PORT, MOTOR_POWER_SWITCH_PIN);
}

static void onMotorPwrChange(void *info)
{
	UNUSED(info);
	swTimer_registerOnTimer(&onDebounceTimer, DEBOUNCE_TIME_MS, true);
	gpio_disable_interrupt(MOTOR_POWER_PORT, MOTOR_POWER_SWITCH_PIN);
}

static void init_motorPWR(void);
MODULE_INIT_FUNCTION(motorPWR, 9, init_motorPWR)
static void init_motorPWR(void)
{
	RCC_AHBENR |= RCC_AHBENR_GPIOBEN;

	gpio_registerFor_interrupt(&onMotorPwrChange, MOTOR_POWER_PORT, MOTOR_POWER_SWITCH_PIN, GPIO_TRIGGER_LEVEL_RISING | GPIO_TRIGGER_LEVEL_FALLING, NULL);
	gpio_set_output_options(MOTOR_POWER_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, MOTOR_POWER_SWITCH_PIN);
	gpio_mode_setup(MOTOR_POWER_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MOTOR_POWER_SWITCH_PIN);

	gpio_set(MOTOR_POWER_PORT, MOTOR_POWER_SWITCH_PIN);

	gpio_enable_interrupt(MOTOR_POWER_PORT, MOTOR_POWER_SWITCH_PIN);
	msgProxy_addMsgForBroadcast(MSG_ID_USER_MOTOR_SWITCH);
}
