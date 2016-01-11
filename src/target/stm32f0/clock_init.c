/**
 * eQuake 2014 v0.1
 *
 * There is no point in satisfaction.
 *
 * Purpose: Initialize HSI clock using PLL to set speed to 48Mhz
 */

#include <flawless/stdtypes.h>
#include <flawless/init/systemInitializer.h>
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/f0/rcc.h>
#include <libopencm3/stm32/f0/memorymap.h>
#include <libopencm3/stm32/f0/flash.h>
#include <libopencm3/stm32/f0/adc.h>

static void clock_init(void);
MODULE_INIT_FUNCTION(clock_init, 0, clock_init)
static void clock_init(void)
{
	//volatile uint32_t* rccCRPtr = &RCC_CR;
	//int initialCRVal = RCC_CR;
	//initialCRVal &= (~RCC_CR_PLLON);

	// Select HSI as system clock
	RCC_CFGR = (RCC_CFGR_SW_HSI | (RCC_CFGR & (~RCC_CFGR_SW)));

	// Wait until the hardware has set HSI as system clock
	while ((RCC_CFGR & (RCC_CFGR_SWS << 2)) != (RCC_CFGR_SWS_HSI << 2));

	// Disable PLL
	RCC_CR &= (~RCC_CR_PLLON);

	// Wait for PLLRDY bit to be cleared
	while ((RCC_CR & RCC_CR_PLLRDY) != 0);

	// Make flash adjustments
//	FLASH_ACR |= FLASH_ACR_PRFTBE;
	FLASH_ACR = (FLASH_ACR_LATENCY_1WS | (FLASH_ACR & (~(0x07))));

	// Select HSI as PLL source
	RCC_CFGR &= (~RCC_CFGR_PLLSRC);

	// Devide input clock by 2
	RCC_CFGR2 = 0x1;

	// Set multiplicator to 12, since it holds that 12 * 4Mhz = 48Mhz
	RCC_CFGR = ((0xa << 18) | (RCC_CFGR & (~(0xf << 18))));

	// Enable PLL and that would be it.
	RCC_CR |= RCC_CR_PLLON;

	// Wait for PLL to become ready
	while ((RCC_CR & RCC_CR_PLLRDY) == 0);

	// Set APB and AHB prescaler to 1, which effectively is no prescaling
	RCC_CFGR = (RCC_CFGR_PPRE_NODIV | (RCC_CFGR & (~RCC_CFGR_PPRE)));
	RCC_CFGR = (RCC_CFGR_HPRE_NODIV | (RCC_CFGR & (~RCC_CFGR_HPRE)));

	// Select PLL as system clock
	RCC_CFGR = (RCC_CFGR_SW_PLL | (RCC_CFGR & (~RCC_CFGR_SW)));

	// Wait until the hardware has set PLL as system clock
	while ((RCC_CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}
