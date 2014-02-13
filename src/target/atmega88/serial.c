
#include <flawless/stdtypes.h>

#include <config.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include <flawless/init/systemInitializer.h>
#include <flawless/protocol/genericFlawLessProtocol_Data.h>

#include <flawless/platform/system.h>


/*************************************************************************************************/
#define GEN_FIFO_USE_ATOMIC 1
#define GEN_FIFO_CLI_FUNCTION system_mutex_lock()
#define GEN_FIFO_SEI_FUNCTION system_mutex_unlock()

#include <flawless/misc/fifo/genericFiFo.h>

CREATE_GENERIC_FIFO(uint8_t, serial, 32);

serial_fifoHandle_t g_txHandle;


/*************************************************************************************************/
#define BAUD UART_BAUDRATE
#include <util/setbaud.h>

typedef enum tag_txMode
{
	txMode_idle,
	txMode_transmitting
} txMode_t;

static txMode_t g_txMode = txMode_idle;

/**
 ** Initialize the uart (rs 485)
 */

void init_serial();
MODULE_INIT_FUNCTION(serial, 5, init_serial)
void init_serial()
{
	serial_init(&g_txHandle);

	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;

	#if USE_2X
		/* U2X-Modus erforderlich */
		UCSR0A |= (1 << U2X0);
	#else
		/* U2X-Modus nicht erforderlich */
		UCSR0A &= ~(1 << U2X0);
	#endif

	g_txMode = txMode_idle;

	UCSR0B |= (1<<TXEN0);
	UCSR0C = (1 << UCSZ00)|(1 << UCSZ01); // Asynchron 8N1

	UCSR0B |= (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);  // UART RX, TX und RX Interrupt einschalten
}

static void triggerTransmitter(void)
{
	uint8_t symbol = 0U;
	system_mutex_lock();
	if (txMode_idle == g_txMode)
	{
		serial_ErrorType_t error = serial_get(&g_txHandle, &symbol);
		if (serial_FIFO_OKAY == error)
		{
			g_txMode = txMode_transmitting;
			UCSR0B |= (1 << TXCIE0);
			UDR0 = symbol;
		}
	}
	system_mutex_unlock();
}



/********* INTERFACE 0 *************/
void phySendFunction0(const flawLessTransportSymbol_t *i_data, uint16_t i_packetLen)
{
	/* send via UART */
	uint16_t i = 0U;
	for ( i = 0U; i < i_packetLen; ++i)
	{
		/* this is a busy wait for filling the fifo */
		while (serial_FIFO_OKAY != serial_put(i_data[i], &g_txHandle));
		triggerTransmitter();
	}
}

/*************************************************************************************************/

/**
 **
 */

ISR(USART_TX_vect)
{
	// send next byte or switch back to receive
	const uint8_t txCnt = serial_getCount(&g_txHandle);
	if (txCnt > 0U) {
		uint8_t nextByte = 0xff;
		serial_ErrorType_t error = serial_get(&g_txHandle, &nextByte);
		if (serial_FIFO_OKAY == error)
		{
			g_txMode = txMode_transmitting;
			UDR0 = nextByte;
		}
	} else
	{
		g_txMode = txMode_idle;
	}
}


/*************************************************************************************************/

/**
 ** Interrupt service routine for receiving bytes over uart
 */

ISR(USART_RX_vect) {
	uint8_t incomingByte = UDR0;
	flawLess_ReceiveIndication(0U, &incomingByte, sizeof(incomingByte));
}
