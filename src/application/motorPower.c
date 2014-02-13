/*
 * motorPower.c
 *
 *  Created on: Mar 2, 2013
 *      Author: lutz
 */


#include <flawless/init/systemInitializer.h>
#include <flawless/protocol/genericFlawLessProtocolApplication.h>
#include <flawless/core/msg_msgPump.h>
#include <flawless/config/msgIDs.h>
#include <flawless/protocol/msgProxy.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define MOTORPOWER_PORT      (PORTC)
#define MOTORPOWER_DIRECTION (DDRC)
#define MOTORPOWER_SWITCH    (PC4)

#define MOTORPOWER_SENSE_DIR  (DDRD)
#define MOTORPOWER_SENSE_PORT (PORTD)
#define MOTORPOWER_SENSE      (PD2)
#define MOTORPOWER_SENSE_IN   (PIND)

MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(motorPwrState, uint8_t, 10, MSG_ID_USER_MOTOR_SWITCH)


typedef enum tag_motorUserSwitchState
{
	MOTOR_USER_SWITCH_STATE_OFF = 0,
	MOTOR_USER_SWITCH_STATE_ON = 1
} motorUserSwitchState_t;

static motorUserSwitchState_t g_switchState = MOTOR_USER_SWITCH_STATE_OFF;


static void motorpower_init();
MODULE_INIT_FUNCTION(motorPower, 9, motorpower_init)
static void motorpower_init()
{
	/* setup switch pin */
	MOTORPOWER_DIRECTION |= 1 << MOTORPOWER_SWITCH;
	MOTORPOWER_PORT |= (1 << MOTORPOWER_SWITCH);

	/* setup sense pin */
	MOTORPOWER_SENSE_DIR &= ~(1 << MOTORPOWER_SENSE);
	MOTORPOWER_SENSE_PORT |= (1 << MOTORPOWER_SENSE);

	EICRA = (EICRA & ~(0x3)) | ISC01;
	EIMSK |= (1 << PCIE0);

	/* enable timer0 for debouncing */
	TCNT0 = 0;
	TIMSK0 |= (1 << TOIE0);
	g_switchState = (0 != (MOTORPOWER_SENSE_IN & (1 << MOTORPOWER_SENSE)))? MOTOR_USER_SWITCH_STATE_ON : MOTOR_USER_SWITCH_STATE_OFF;

	msgProxy_addMsgForBroadcast(MSG_ID_USER_MOTOR_SWITCH);
}

ISR(TIMER0_OVF_vect)
{
	system_mutex_lock();
	/* disable timer */
	TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));


	motorUserSwitchState_t newState = (0 != (MOTORPOWER_SENSE_IN & (1 << MOTORPOWER_SENSE)))? MOTOR_USER_SWITCH_STATE_ON : MOTOR_USER_SWITCH_STATE_OFF;
	if (newState != g_switchState)
	{
		g_switchState = newState;
		msgPump_postMessage(MSG_ID_USER_MOTOR_SWITCH, &g_switchState);
	}

	/* re-enable pin changed interrupt */
	EIMSK |= (1 << PCIE0);
	system_mutex_unlock();
}

ISR(INT0_vect)
{
	system_mutex_lock();
	motorUserSwitchState_t newState = (0 != (MOTORPOWER_SENSE_IN & (1 << MOTORPOWER_SENSE)))? MOTOR_USER_SWITCH_STATE_ON : MOTOR_USER_SWITCH_STATE_OFF;
	if (newState != g_switchState)
	{
		/* disable pin changed interrupt and setup timer0 for debouncing */
		EIMSK &= ~(1 << PCIE0);

		TCCR0B |= (1 << CS02) | (1 << CS00); /* set psc to 1024 for 256 * 1024 ticks = 30ms */
		TIMSK0 |= (1 << TOIE0);
		TCNT0 = 0;
	}
	system_mutex_unlock();
}

void setMotorPower(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor);
GENERIC_PROTOCOL_ENDPOINT(setMotorPower, 105)
void setMotorPower(const void *ipacket, uint16_t i_packetLen, flawLessInterfaceDescriptor_t i_interfaceDescriptor)
{
	if (0 < i_packetLen)
	{
		const uint8_t value = *((const uint8_t*)ipacket);
		if (0 == value)
		{
			MOTORPOWER_PORT &= ~(1 << MOTORPOWER_SWITCH);
		} else
		{
			MOTORPOWER_PORT |= (1 << MOTORPOWER_SWITCH);
		}
	}
}
