/*
 * system.c
 *
 *  Created on: Sep 6, 2012
 *      Author: lutz
 */

#include <flawless/stdtypes.h>
#include <flawless/init/systemInitializer.h>
#include <flawless/platform/system.h>
#include <avr/interrupt.h>

static volatile int8_t g_suspendCount = 0;

void system_mutex_lock(void)
{
	cli();
	++g_suspendCount;
}

void system_mutex_unlock(void)
{
	--g_suspendCount;
	if (0 >= g_suspendCount)
	{
		g_suspendCount = 0;
		sei();
	}
}

void system_init(void);
MODULE_INIT_FUNCTION(system, 1, system_init)
void system_init(void)
{
	g_suspendCount = 0;
	sei();
}

