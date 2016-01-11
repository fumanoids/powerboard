

#include <string.h>

#include <flawless/stdtypes.h>
#include <flawless/platform/system.h>

#include <flawless/logging/logging.h>
#include <flawless/init/systemInitializer.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/wwdg.h>

volatile uint32_t g_blaguard = 0;
volatile uint32_t g_lockCounter = 0U;
volatile uint32_t g_blaguard2 = 0;

static void system_init(void);
MODULE_INIT_FUNCTION(system, 2, system_init)
static void system_init(void)
{
	g_blaguard = 0xaffeaffe;
	g_lockCounter = 0U;
	g_blaguard2 = 0xdeadbeef;
}


void system_mutex_lock(void)
{
	__asm("CPSID I"); /* Disable Interrupts */
	++g_lockCounter;
}


void system_mutex_unlock(void)
{
	if ( g_lockCounter > 0U)
	{
		--g_lockCounter;
	} else
	{
		LOG_INFO_0("resumeAllInterrupts too often called g_lockCounter == 0U");
	}
	if (0 == g_lockCounter)
	{
		__asm("CPSIE I"); /* enable Interrupts */
	}
}



void system_reset(void)
{
	RCC_APB1ENR |= RCC_APB1ENR_WWDGEN;

	WWDG_CR = WWDG_CR_WDGA;
}
