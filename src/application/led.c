#include "led.h"
#include <flawless/init/systemInitializer.h>
#include <flawless/core/msg_msgPump.h>
#include <flawless/config/msgIDs.h>
#include <flawless/timer/swTimer.h>

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define LED_PORT      (PORTB)
#define LED_DIRECTION (DDRB)
#define LED_BLUE      (PB0)
#define LED_GREEN     (PB1)
#define LED_RED       (PB2)



static void onBatteryVoltageReady(msgPump_MsgID_t, const void *);


void led_init();
MODULE_INIT_FUNCTION(led, 0, led_init)
void led_init()
{
	// set pins to output
	LED_DIRECTION |= 1 << LED_RED;
	LED_DIRECTION |= 1 << LED_GREEN;
	LED_DIRECTION |= 1 << LED_BLUE;
	LED_PORT |= (1 << LED_RED);
	LED_PORT |= (1 << LED_GREEN);
	LED_PORT |= (1 << LED_BLUE);

	TCCR1A = 0U;
	TCCR1B = 0U;

	ICR1L = 0xff;
	ICR1H = 0x00;

	/* set OC1A/B on compare match (fast PWM-mode with ICR1 as TOP) no prescaler */
	TCCR1A |= (1 << COM1A1) | (1 << COM1A0) | (1 << COM1B1) | (1 << COM1B0);
	TCCR1A |=  (1 << WGM11);
	TCCR1B |=  (1 << WGM12) | (1 << WGM13) | (1 << CS10);
}

void led_init2();
MODULE_INIT_FUNCTION(led2, 9, led_init2)
void led_init2()
{
	led_setColor(64, 64);
	msgPump_registerOnMessage(MSG_ID_RELATIVE_BATTERY_VOLTAGE, &onBatteryVoltageReady);
}


static void onBatteryVoltageReady(msgPump_MsgID_t i_msgID, const void *i_data)
{
	const uint8_t value = *((const uint8_t*) i_data);

	uint8_t green;
	uint8_t red;

	if (value >= 128)
	{
		green = 0xff;
		red   = 2 * (255 - value);
	} else
	{
		red = 0xff;
		green   = 2 * value;
	}

	led_setColor(red, green);
}

void led_setColor(uint8_t red, uint8_t green)
{
	OCR1BL = (red & 0x00ff);
	OCR1AL = (green & 0x00ff);
}
