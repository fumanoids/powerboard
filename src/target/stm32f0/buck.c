/*------------------------includes---------------------------------*/
#include <flawless/stdtypes.h>
#include <flawless/init/systemInitializer.h>
#include <flawless/timer/swTimer.h>
#include <flawless/core/msg_msgPump.h>
#include <flawless/config/msgIDs.h>
#include <flawless/platform/system.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "gpio_interrupts.h"

#include <interfaces/battery.h>
#include <interfaces/led.h>
/*-----------------------------------------------------------------*/


#define BUCK_CONTROL_PORT GPIOB
#define BUCK_ENABLE_SWITCHER_PIN GPIO7

static bool g_switcherStarted = false;

static void onRelativeBatteryVoltage(msgPump_MsgID_t i_id, const void* buf)
{
	UNUSED(i_id);

	relativeBatteryLevel_t level = *((const relativeBatteryLevel_t*)buf);

	system_mutex_lock();
	if (level > 0) {
		gpio_set(BUCK_CONTROL_PORT, BUCK_ENABLE_SWITCHER_PIN);
		g_switcherStarted = true;
	}
	system_mutex_unlock();
}

static void onLateInitTimer() {
	msgPump_registerOnMessage(MSG_ID_RELATIVE_BATTERY_VOLTAGE, &onRelativeBatteryVoltage);
}

/*--------------------------methods--------------------------------*/
static void init_buck(void);
MODULE_INIT_FUNCTION(buck, 8, init_buck)
static void init_buck(void)
{
	RCC_AHBENR |= RCC_AHBENR_GPIOBEN;
	RCC_AHBENR |= RCC_AHBENR_GPIOAEN;

	gpio_mode_setup(BUCK_CONTROL_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BUCK_ENABLE_SWITCHER_PIN);
	gpio_set_output_options(BUCK_CONTROL_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, BUCK_ENABLE_SWITCHER_PIN);
	gpio_clear(BUCK_CONTROL_PORT, BUCK_ENABLE_SWITCHER_PIN);


	blinkRed(4);

	swTimer_registerOnTimer(&onLateInitTimer, 100, true);
}
/*-----------------------------------------------------------------*/

