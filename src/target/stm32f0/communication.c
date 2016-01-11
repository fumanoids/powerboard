/*
 * data_comm.c
 *
 *  Created on: Nov 1, 2012
 *      Author: lutz
 */


/*
 * this is the communication interface to the power board
 * this interface is numbered as logic 0
 */

#define COM_2_PWR_BOARD_LOGIC_INTERFACE_NR 0


#include <flawless/init/systemInitializer.h>
#include <flawless/protocol/genericFlawLessProtocol_Data.h>
#include <flawless/platform/system.h>

#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/f0/nvic.h>

#define TX_FIFO_SIZE 128U
#define RX_FIFO_SIZE 128U

#include "clock.h"

#define GEN_FIFO_USE_ATOMIC 1
#define GEN_FIFO_CLI_FUNCTION system_mutex_lock();
#define GEN_FIFO_SEI_FUNCTION system_mutex_unlock();
#include <flawless/misc/fifo/genericFiFo.h>

CREATE_GENERIC_FIFO(uint8_t, data, TX_FIFO_SIZE);

#define COM_USART_INTERFACE USART1
#define COM_USART_INTERFACE_SPEED 115200ULL

#define COM_USART_PORT GPIOA
#define COM_USART_TX_PIN  GPIO9
#define COM_USART_RX_PIN  GPIO10

#define COM_2_PWR_TX_DMA_CHANNEL 2U
#define COM_2_PWR_RX_DMA_CHANNEL 3U

static data_fifoHandle_t g_txFifo;

static flawLessTransportSymbol_t g_rxFifoBuf[RX_FIFO_SIZE];
static uint32_t g_rxFifoReadIndex = 0U;
static uint32_t g_currentTXTransactionSize = 0U;


static void trigger_transmision(void);
static void trigger_transmision(void)
{

	system_mutex_lock();
	uint16_t bytesToSendCurrently = MIN(TX_FIFO_SIZE - g_txFifo.start, g_txFifo.count);

	if ((bytesToSendCurrently > 0U) &&
		(0 == g_currentTXTransactionSize))
	{
		while (0 != (DMA1_CCR(COM_2_PWR_TX_DMA_CHANNEL) & DMA_CCR_EN))
		{
			DMA1_CCR(COM_2_PWR_TX_DMA_CHANNEL) &= ~DMA_CCR_EN;
		}
		/* set txBuffer as source */
		DMA1_CMAR(COM_2_PWR_TX_DMA_CHANNEL) = (uint32_t) &(g_txFifo.data[g_txFifo.start]);
		DMA1_CNDTR(COM_2_PWR_TX_DMA_CHANNEL) = bytesToSendCurrently;
		g_currentTXTransactionSize = bytesToSendCurrently;

		DMA1_CCR(COM_2_PWR_TX_DMA_CHANNEL) |= DMA_CCR_TCIE;
		DMA1_CCR(COM_2_PWR_TX_DMA_CHANNEL) |= DMA_CCR_EN;
	}
	system_mutex_unlock();
}

/*
 * send function called from framework
 */
void phySendFunction0(const flawLessTransportSymbol_t *i_data, uint16_t i_packetLen)
{
	uint16_t packetBuffered = 0;
	while (packetBuffered < i_packetLen)
	{
		system_mutex_lock();
		/* how much of that packet can we send at once */
		uint16_t spaceFree = TX_FIFO_SIZE - g_txFifo.count;
		uint16_t spaceFreeTillWrap = MIN(TX_FIFO_SIZE - g_txFifo.end, spaceFree);
		uint16_t elementsToStore = MIN(i_packetLen - packetBuffered, spaceFreeTillWrap);
		memcpy(&(g_txFifo.data[g_txFifo.end]), &(i_data[packetBuffered]), elementsToStore);
		g_txFifo.end = (g_txFifo.end + elementsToStore) % TX_FIFO_SIZE;
		g_txFifo.count += elementsToStore;
		packetBuffered += elementsToStore;
		system_mutex_unlock();

		/* check if dma is allready running if not enable dma */
		trigger_transmision();
	}
}

/*
 * ISR for USART6 here we receive Data
 */
extern volatile uint32_t g_lockCounter;
void usart1_isr(void)
{
	/* did we receive something? */
	while (g_rxFifoReadIndex != (RX_FIFO_SIZE - DMA1_CNDTR(COM_2_PWR_RX_DMA_CHANNEL)))
	{
		uint8_t received = g_rxFifoBuf[g_rxFifoReadIndex];
		g_rxFifoReadIndex = (g_rxFifoReadIndex + 1) % RX_FIFO_SIZE;
		flawLess_ReceiveIndication(COM_2_PWR_BOARD_LOGIC_INTERFACE_NR, &received, sizeof(received));
	}
	if ((g_rxFifoReadIndex == (RX_FIFO_SIZE - DMA1_CNDTR(COM_2_PWR_RX_DMA_CHANNEL))) &&
		(0 != (USART_ISR(COM_USART_INTERFACE) & USART_ISR_ORE)))
	{
		/* dummy read on usart interface */
		(void)USART_RDR(COM_USART_INTERFACE);
	}
	UNUSED(g_lockCounter);
}


void dma1_ch2_3_isr(void)
{
	DMA1_CCR(COM_2_PWR_TX_DMA_CHANNEL) &= (~DMA_CCR_EN);
	while (DMA1_CCR(COM_2_PWR_TX_DMA_CHANNEL) & DMA_CCR_EN); // wait for DMA to be done


	system_mutex_lock();

	// clear interrupt bits
	DMA1_IFCR = 0xf << 4;

	g_txFifo.start = (g_txFifo.start + g_currentTXTransactionSize) % TX_FIFO_SIZE;
	g_txFifo.count = g_txFifo.count - g_currentTXTransactionSize;
	g_currentTXTransactionSize = 0;
	system_mutex_unlock();
	trigger_transmision();
}


static void comm2pwrBrd_init(void);
MODULE_INIT_FUNCTION(comm2pwrBrd, 9, comm2pwrBrd_init);
static void comm2pwrBrd_init(void)
{
	data_init(&g_txFifo);
	data_peek(&g_txFifo, 0U, NULL); /* dummy call to suppress compiler warnings */
	data_getCount(&g_txFifo);       /* dummy call to suppress compiler warnings */
	data_put(NULL, NULL);       /* dummy call to suppress compiler warnings */
	data_get(NULL, NULL);       /* dummy call to suppress compiler warnings */

	memset(&g_rxFifoBuf, 0, sizeof(g_rxFifoBuf));
	g_rxFifoReadIndex = 0U;

	/* setup USART for communication */

	RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
	RCC_AHBENR |= RCC_AHBENR_GPIOAEN;
	RCC_AHBENR |= RCC_AHBENR_DMAEN;

	gpio_mode_setup(COM_USART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, COM_USART_TX_PIN);
	gpio_set_output_options(COM_USART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, COM_USART_TX_PIN);
	gpio_set_af(COM_USART_PORT, GPIO_AF1, COM_USART_TX_PIN);

	gpio_mode_setup(COM_USART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, COM_USART_RX_PIN);
	gpio_set_af(COM_USART_PORT, GPIO_AF1, COM_USART_RX_PIN);

	nvic_enable_irq(NVIC_USART1_IRQ);
	nvic_enable_irq(NVIC_DMA1_CHANNEL2_3_IRQ);

	USART_BRR(COM_USART_INTERFACE) = CLOCK_APB2_CLK / COM_USART_INTERFACE_SPEED;
	USART_CR1(COM_USART_INTERFACE) |= USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
	USART_CR3(COM_USART_INTERFACE) |= USART_CR3_DMAT | USART_CR3_DMAR;

	/********* TX DMA **********/
	/* set channel */
	DMA1_CCR(COM_2_PWR_TX_DMA_CHANNEL) =  DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_TCIE;
	/* write to usart_dr */
	DMA1_CPAR(COM_2_PWR_TX_DMA_CHANNEL) = (uint32_t)&(USART_TDR(COM_USART_INTERFACE));

	/********* RX DMA **********/
	DMA1_CCR(COM_2_PWR_RX_DMA_CHANNEL)  = DMA_CCR_MINC | DMA_CCR_CIRC;
	/* read from usart_dr */
	DMA1_CPAR(COM_2_PWR_RX_DMA_CHANNEL)  = (uint32_t) &USART_RDR(COM_USART_INTERFACE);

	DMA1_CMAR(COM_2_PWR_RX_DMA_CHANNEL) = (uint32_t) &g_rxFifoBuf;
	DMA1_CNDTR(COM_2_PWR_RX_DMA_CHANNEL) = RX_FIFO_SIZE;

	DMA1_CCR(COM_2_PWR_RX_DMA_CHANNEL) |= DMA_CCR_EN;

	USART_CR1(COM_USART_INTERFACE) |= USART_CR1_UE;

	int blubb = DMA1_CCR(COM_2_PWR_TX_DMA_CHANNEL);
	++blubb;
}

