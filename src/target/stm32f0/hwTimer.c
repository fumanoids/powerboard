/*
 * hwTimer.c
 *
 *  Created on: 02.01.2015
 *      Author: lutz
 */


#include <flawless/init/systemInitializer.h>
#include <flawless/platform/hwTimer.h>
#include <flawless/timer/swTimer.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/f0/nvic.h>
#include <libopencm3/stm32/timer.h>

#include "clock.h"

#define HW_TIMER TIM2

#define USECONDS_PER_SECOND 1000000ULL

void tim2_isr()
{
	TIM_CR1(HW_TIMER) &= ~TIM_CR1_CEN; /* disable timer */
//	TIM_SR(HW_TIMER) = 0;
	swTimer_trigger();
}


hw_timerTicks hw_timerGetTicksForInterval_us(timer_TimeInterval_us i_interval)
{
	hw_timerTicks ret = i_interval * (CLOCK_APB_TIMER_CLK / USECONDS_PER_SECOND);
	return ret;
}

hw_timerTicks hw_timerGetTicksElapsed()
{
	const uint32_t psc = TIM_PSC(HW_TIMER);
	const uint32_t cnt = TIM_CNT(HW_TIMER);
	return (psc + 1) * cnt;
}

void hw_timerSetupTimer(hw_timerTicks i_ticks)
{
	const uint32_t psc = ((i_ticks & 0xffff0000) >> 16U);
	uint32_t arr = (i_ticks / (psc + 1U));
	arr = MIN(arr, 0xffff);

	TIM_CR1(HW_TIMER) &= ~TIM_CR1_CEN; /* disable timer */

	TIM_ARR(HW_TIMER) = 0xffff;
	TIM_PSC(HW_TIMER) = psc;
	TIM_CNT(HW_TIMER) = 0U;

	/* disable interrupt */

	/* generate an update event to update the contents of PSC and ARR */
	TIM_SR(HW_TIMER) = 0;
	TIM_DIER(HW_TIMER) = 0;
	TIM_CCR1(HW_TIMER) = arr;
	TIM_EGR(HW_TIMER) = TIM_EGR_UG;
	TIM_SR(HW_TIMER) = 0;

	/* re-enable interrupt */

	/* test if this improves things! */
	TIM_CR1(HW_TIMER) |= TIM_CR1_OPM;
	TIM_DIER(HW_TIMER) |= TIM_DIER_CC1IE;

	/* enable timer */
	TIM_CR1(HW_TIMER) |= TIM_CR1_CEN;
}

static void init(void);
MODULE_INIT_FUNCTION(timer, 2, init)
static void init(void)
{
	/* Enable TIM clock. */
	RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;
	/* Enable TIM interrupt. */
	nvic_enable_irq(NVIC_TIM2_IRQ);

	TIM_PSC(HW_TIMER) = 0U;
	TIM_CNT(HW_TIMER) = 0U;
	TIM_DIER(HW_TIMER) = 0U;
}
