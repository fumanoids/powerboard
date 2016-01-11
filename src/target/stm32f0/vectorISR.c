/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2011 Fergus Noble <fergusnoble@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#define WEAK __attribute__ ((weak))

/* Symbols exported by the linker script(s): */
extern unsigned _stack;

void main(void);
void reset_handler(void);
void blocking_handler(void);
void null_handler(void);

extern void WEAK reset_handler(void);
extern void WEAK nmi_handler(void);
extern void WEAK hard_fault_handler(void);
extern void WEAK sv_call_handler(void);
extern void WEAK pend_sv_handler(void);
extern void WEAK sys_tick_handler(void);
extern void WEAK wwdg_isr(void);
extern void WEAK pvd_vddio2_isr(void);
extern void WEAK rtc_isr(void);
extern void WEAK flash_isr(void);
extern void WEAK rcc_crs_isr(void);
extern void WEAK exti0_1_isr(void);
extern void WEAK exti2_3_isr(void);
extern void WEAK exti4_15_isr(void);
extern void WEAK tsc_isr(void);
extern void WEAK dma1_ch1_isr(void);
extern void WEAK dma1_ch2_3_isr(void);
extern void WEAK dma1_ch4_5_6_7_isr(void);
extern void WEAK adc_comp_isr(void);
extern void WEAK tim1_brk_up_trg_com_isr(void);
extern void WEAK tim1_cc_isr(void);
extern void WEAK tim2_isr(void);
extern void WEAK tim3_isr(void);
extern void WEAK tim6_dac_isr(void);
extern void WEAK tim7_isr(void);
extern void WEAK tim14_isr(void);
extern void WEAK tim15_isr(void);
extern void WEAK tim16_isr(void);
extern void WEAK tim17_isr(void);
extern void WEAK i2c1_isr(void);
extern void WEAK i2c2_isr(void);
extern void WEAK spi1_isr(void);
extern void WEAK spi2_isr(void);
extern void WEAK usart1_isr(void);
extern void WEAK usart2_isr(void);
extern void WEAK usart3_4_isr(void);
extern void WEAK cec_can_isr(void);
extern void WEAK usb_isr(void);

__attribute__ ((section(".vectors")))
void (*const vector_table[]) (void) = {
	(void*)&_stack,
	reset_handler,
	nmi_handler,
	hard_fault_handler,
	0, 0, 0, 0, 0, 0, 0,		/* Reserved */
	sv_call_handler,
	0, 0,					/* Reserved */
	pend_sv_handler,
	sys_tick_handler,
	wwdg_isr,
	pvd_vddio2_isr,
	rtc_isr,
	flash_isr,
	rcc_crs_isr,
	exti0_1_isr,
	exti2_3_isr,
	exti4_15_isr,
	tsc_isr,
	dma1_ch1_isr,
	dma1_ch2_3_isr,
	dma1_ch4_5_6_7_isr,
	adc_comp_isr,
	tim1_brk_up_trg_com_isr,
	tim1_cc_isr,
	tim2_isr,
	tim3_isr,
	tim6_dac_isr,
	tim7_isr,
	tim14_isr,
	tim15_isr,
	tim16_isr,
	tim17_isr,
	i2c1_isr,
	i2c2_isr,
	spi1_isr,
	spi2_isr,
	usart1_isr,
	usart2_isr,
	usart3_4_isr,
	cec_can_isr,
	usb_isr,
};

void reset_handler(void)
{
	__asm__("MSR msp, %0" : : "r"(&_stack));

	/* Call the application's entry point. */
	main();
}

void blocking_handler(void)
{
	while (1);
}


void nmi_handler()
{
	while (1);
}

#include <interfaces/power.h>

void hard_fault_handler()
{
	switchAllOff();
	while(1);
}

extern volatile int g_lockCounter;

void null_handler(void)
{
	(void)(&vector_table[25]);
	(void)(&vector_table[16]);
	/* Do nothing. */
	(void)g_lockCounter;
}

//#pragma weak nmi_handler = null_handler
//#pragma weak hard_fault_handler = blocking_handler
//#pragma weak mem_manage_handler = blocking_handler
//#pragma weak bus_fault_handler = blocking_handler
//#pragma weak usage_fault_handler = blocking_handler
#pragma weak sv_call_handler = null_handler
#pragma weak pend_sv_handler = null_handler
#pragma weak sys_tick_handler = null_handler
#pragma weak wwdg_isr = null_handler
#pragma weak pvd_vddio2_isr = null_handler
#pragma weak rtc_isr = null_handler
#pragma weak flash_isr = null_handler
#pragma weak rcc_crs_isr = null_handler
#pragma weak exti0_1_isr = null_handler
#pragma weak exti2_3_isr = null_handler
#pragma weak exti4_15_isr = null_handler
#pragma weak tsc_isr = null_handler
#pragma weak dma1_ch1_isr = null_handler
#pragma weak dma1_ch2_3_isr = null_handler
#pragma weak dma1_ch4_5_6_7_isr = null_handler
#pragma weak adc_comp_isr = null_handler
#pragma weak tim1_brk_up_trg_com_isr = null_handler
#pragma weak tim1_cc_isr = null_handler
#pragma weak tim2_isr = null_handler
#pragma weak tim3_isr = null_handler
#pragma weak tim6_dac_isr = null_handler
#pragma weak tim7_isr = null_handler
#pragma weak tim14_isr = null_handler
#pragma weak tim15_isr = null_handler
#pragma weak tim16_isr = null_handler
#pragma weak tim17_isr = null_handler
#pragma weak i2c1_isr = null_handler
#pragma weak i2c2_isr = null_handler
#pragma weak spi1_isr = null_handler
#pragma weak spi2_isr = null_handler
#pragma weak usart1_isr = null_handler
#pragma weak usart2_isr = null_handler
#pragma weak usart3_4_isr = null_handler
#pragma weak cec_can_isr = null_handler
#pragma weak usb_isr = null_handler
