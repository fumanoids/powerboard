/*
 * led.c
 *
 *  Created on: 07.01.2015
 *      Author: lutz
 */

#include <flawless/init/systemInitializer.h>
#include <flawless/timer/swTimer.h>
#include <flawless/platform/system.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>

#define RGB_TIMER TIM3

#define LED_PORT GPIOB
#define LED_R_PIN GPIO5
#define LED_G_PIN GPIO4
#define LED_B_PIN GPIO3

#define RGB_BLINK_DELAY_MS 100

static bool g_isRedBlinking = false;
static uint32_t g_blinkCount = 0;


void setLEDColor(uint8_t red, uint8_t green, uint8_t blue)
{
	if (false == g_isRedBlinking)
	{
		int cnt = TIM_CNT(RGB_TIMER);
		TIM_CCR1(RGB_TIMER) = TIM_ARR(RGB_TIMER) - green;
		TIM_CCR2(RGB_TIMER) = TIM_ARR(RGB_TIMER) - red;
		int tim_ccr1 = TIM_CCR1(RGB_TIMER);
		int tim_ccr2 = TIM_CCR2(RGB_TIMER);
		UNUSED(tim_ccr1);
		UNUSED(tim_ccr2);
		UNUSED(cnt);

		if (0 == blue) {
			gpio_set(LED_PORT, LED_B_PIN);
		} else {
			gpio_clear(LED_PORT, LED_B_PIN);
		}
	}
}

void setupPWM()
{
	gpio_mode_setup(LED_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, LED_R_PIN | LED_G_PIN);
	gpio_set_output_options(LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, LED_R_PIN | LED_G_PIN);
	gpio_set_af(LED_PORT, GPIO_AF1, LED_R_PIN | LED_G_PIN);

	TIM_CCMR1(RGB_TIMER) = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC2M_PWM1;
	TIM_CCER(RGB_TIMER) = TIM_CCER_CC1E | TIM_CCER_CC2E;

	TIM_PSC(RGB_TIMER) = 1000;
	TIM_ARR(RGB_TIMER) = 0xff;
	TIM_CR1(RGB_TIMER) = TIM_CR1_CEN;


	g_isRedBlinking = false;
	setLEDColor(127, 127, 0);
}

void onBlinkTimer()
{
	if (g_blinkCount > 0)
	{
		gpio_toggle(LED_PORT, LED_R_PIN);
		--g_blinkCount;
	} else
	{
		swTimer_unRegisterFromTimer(onBlinkTimer);
		g_isRedBlinking = false;
		setupPWM();
	}
}

void blinkRed(uint32_t count)
{
	g_isRedBlinking = true;
	TIM_CR1(RGB_TIMER) = 0; // stop timer

	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_R_PIN);
	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_G_PIN);

	gpio_clear(LED_PORT, LED_R_PIN);
	gpio_clear(LED_PORT, LED_G_PIN);
	gpio_set(LED_PORT, LED_B_PIN);

	system_mutex_lock();
	g_blinkCount = count;
	swTimer_registerOnTimer(onBlinkTimer, RGB_BLINK_DELAY_MS, false);
	system_mutex_unlock();
}

static void onBlinkBlueTimerOff()
{
//	gpio_set(LED_PORT, LED_B_PIN);
}

static void onBlinkBlueTimerOn()
{
//	gpio_clear(LED_PORT, LED_B_PIN);
	swTimer_registerOnTimer(&onBlinkBlueTimerOff, 100, true);
}

static void led_init();
MODULE_INIT_FUNCTION(led, 5, led_init)
static void led_init()
{
	RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;
	RCC_AHBENR |= RCC_AHBENR_GPIOBEN;

	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_R_PIN | LED_G_PIN | LED_B_PIN);
	gpio_set_output_options(LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, LED_R_PIN | LED_G_PIN | LED_B_PIN);

	setupPWM();
}

static void led_late_init();
MODULE_INIT_FUNCTION(led_late, 9, led_late_init)
static void led_late_init()
{
	swTimer_registerOnTimer(&onBlinkBlueTimerOn, 1000, false);
}
