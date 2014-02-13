

#include "genericFlawLessProtocol.h"

#include <flawless/core/msg_msgPump.h>
#include "../core/systemMsgIDs.h"
#include <flawless/config/flawLessProtocol_config.h>

#include <flawless/stdtypes.h>
#include <flawless/misc/Assert.h>

#include <flawless/init/systemInitializer.h>
#include <flawless/protocol/genericFlawLessProtocol_Data.h>
#include <flawless/platform/genericFlawLessProtocol_phy.h>

#include <flawless/misc/communication.h>

/******************* internals **************************/


typedef enum tag_genericProtocolReceiveModus
{
	GENERIC_PROTOCOL_RECEIVING_UNKOWN,
	GENERIC_PROTOCOL_RECEIVED_ESCAPE_CHAR,
	GENERIC_PROTOCOL_RECEIVING_PAYLOAD,
	GENERIC_PROTOCOL_RECEIVING_PACKET_END
} genericProtocolReceiveModus_t;

typedef struct tag_genericProtocolReceiveStatus
{
	genericProtocoll_Packet_t *buffer;
	uint16_t receivedPayloadBytes;
	genericProtocolReceiveModus_t mode;
	genericProtocol_Checksum_t checkSum;
} genericProtocolReceiveState_t;

typedef struct tag_genericProtocolSendStatus
{
	uint16_t payloadLen;
	uint16_t bytesTransmitted;
	genericProtocol_Checksum_t checksum;
	bool isSending;
} genericProtocol_SendState_t;


/********************* Globals ***********************************/
static genericProtocolReceiveState_t g_receiveState[FLAWLESS_PROTOCOL_PHY_INTERFACES_COUNT];
static genericProtocol_SendState_t g_sendState[FLAWLESS_PROTOCOL_PHY_INTERFACES_COUNT];

/******************* the physical interfaces ********************/
#define WEAK __attribute__ ((weak))

#if FLAWLESS_PROTOCOL_PHY_INTERFACES_COUNT != 1
#error "The amount of physical interfaces supported by the framework must match the size of the interface vector"
#endif

void WEAK phySendFunction0(const flawLessTransportSymbol_t *i_data, uint16_t packetLen);

const phySendFunction_t g_sendFunctions[FLAWLESS_PROTOCOL_PHY_INTERFACES_COUNT] =
{
		&phySendFunction0
};

/***************** Message related stuff ************************/

#define GENERIC_PROTCOL_BUFFER_COUNT  2
MSG_PUMP_DECLARE_MESSAGE_BUFFER_POOL(protocollIGEP_IGEPBuffer, genericProtocoll_Packet_t, GENERIC_PROTCOL_BUFFER_COUNT, MSG_ID_GENERIC_PROTOCOL_PACKET_READY)

/***************** Functions **********************/
/***************** private functions **********************/

static genericProtocol_Checksum_t genericProtocol_send(flawLessInterfaceDescriptor_t i_interface, const uint8_t i_data, bool i_escape);
static genericProtocol_Checksum_t genericProtocol_sendChecksum(flawLessInterfaceDescriptor_t i_interface, uint8_t i_checksum);

static void genericProtocol_receiveByte(flawLessInterfaceDescriptor_t i_interface, const uint8_t received);

static bool genericProtocoll_resetBuffer(flawLessInterfaceDescriptor_t i_interface)
{
	bool ret = false;

	void *bufPtr = NULL;
	const bool acquireSuccess = msgPump_getFreeBuffer(MSG_ID_GENERIC_PROTOCOL_PACKET_READY, &bufPtr);
	if (false != acquireSuccess)
	{
		/* reset the bufferStatus */
		g_receiveState[i_interface].buffer = bufPtr;
		g_receiveState[i_interface].buffer->interface = i_interface;
		g_receiveState[i_interface].buffer->payloadLen = 0U;
		g_receiveState[i_interface].buffer->subProtocolID = 0;
	} else
	{
		/* in this situation we cannot do anything usefull...
		 * anyway, this is a very bad situation here!!!
		 */
	}
	g_receiveState[i_interface].mode                    = GENERIC_PROTOCOL_RECEIVING_UNKOWN;
	g_receiveState[i_interface].receivedPayloadBytes    = 0;
	g_receiveState[i_interface].checkSum                = 0U;

	return ret;
}

static void genericProtocol_receiveByte(flawLessInterfaceDescriptor_t i_interface, const uint8_t i_received)
{
	/* check if we have to acquire a buffer */
	genericProtocolReceiveState_t *receiveState = &g_receiveState[i_interface];
	if (NULL == receiveState->buffer)
	{
		const bool resetSuccess = genericProtocoll_resetBuffer(i_interface);
		ASSERT(true == resetSuccess);
	}

	if (NULL != receiveState->buffer)
	{
		/* maybe we have just received the beginning of a new message because of a lost symbol on the bus */
		{
			if (GENERIC_PROTOCOL_RECEIVED_ESCAPE_CHAR != receiveState->mode)
			{
				if (FLAWLESS_PROTOCOL_PACKET_CHAR_BEGINNING == i_received)
				{
					/* this must be the beginning of a new packet! Or we are completely out of synch... whatever */
					receiveState->mode = GENERIC_PROTOCOL_RECEIVING_PAYLOAD;
					receiveState->checkSum = 0U;
					receiveState->receivedPayloadBytes = 0U;
					return;
				} else if (FLAWLESS_PROTOCOL_PACKET_CHAR_END == i_received)
				{
					receiveState->mode = GENERIC_PROTOCOL_RECEIVING_PACKET_END;
				}
			}
		}

		if (FLAWLESS_PROTOCOL_MAX_PACKET_LEN <= receiveState->receivedPayloadBytes)
		{
			/* we cannot process that message so try to synch up with the next packet */
		}

		receiveState->checkSum += i_received;
		/* store the received byte */
		switch (receiveState->mode)
		{
			case(GENERIC_PROTOCOL_RECEIVING_PAYLOAD):
				{
					if (FLAWLESS_PROTOCOL_PACKET_CHAR_ESCAPE == i_received)
					{
						/* dont store what was escaped */
						receiveState->mode = GENERIC_PROTOCOL_RECEIVED_ESCAPE_CHAR;
						break;
					}
					if (0U == receiveState->receivedPayloadBytes)
					{
						/* this is the target endpoint */
						receiveState->buffer->subProtocolID = i_received;
						receiveState->receivedPayloadBytes = 1U;
						break;
					}
					receiveState->buffer->packet[receiveState->receivedPayloadBytes - 1U] = i_received;
					++(receiveState->receivedPayloadBytes);
					break;
				}
			case (GENERIC_PROTOCOL_RECEIVED_ESCAPE_CHAR):
				{
					receiveState->mode = GENERIC_PROTOCOL_RECEIVING_PAYLOAD;
					if (0U == receiveState->receivedPayloadBytes)
					{
						/* this is the target endpoint */
						receiveState->buffer->subProtocolID = i_received;
						receiveState->receivedPayloadBytes = 1U;
						break;
					}
					/* store what was escaped as payload */
					receiveState->buffer->packet[receiveState->receivedPayloadBytes - 1U] = i_received;
					++(receiveState->receivedPayloadBytes);
					break;
				}
			case (GENERIC_PROTOCOL_RECEIVING_PACKET_END):
				{
					receiveState->checkSum -= FLAWLESS_PROTOCOL_PACKET_CHAR_END;
					if (0U == receiveState->checkSum)
					{
						receiveState->buffer->interface = i_interface;
						receiveState->buffer->payloadLen = receiveState->receivedPayloadBytes - sizeof(genericProtocol_Checksum_t) - sizeof(genericProtocol_subProtocolIdentifier_t);
						{
							/* post it */
							const bool postSuccess = msgPump_postMessage(MSG_ID_GENERIC_PROTOCOL_PACKET_READY, receiveState->buffer);
							if (false != postSuccess)
							{
								/* everything is fine */
								receiveState->buffer = NULL;
							}
							genericProtocoll_resetBuffer(i_interface);
						}
					}
					receiveState->checkSum = 0U;
					receiveState->receivedPayloadBytes = 0U;
					break;
				}
			case (GENERIC_PROTOCOL_RECEIVING_UNKOWN):/* nothing to do since we have no clue what's happening */
			default:
				break;
		}
	}
}

void flawLess_ReceiveIndication(flawLessInterfaceDescriptor_t i_interface, const flawLessTransportSymbol_t *i_data, uint16_t i_packetLen)
{
	uint16_t i = 0U;
	for (i = 0U; i < i_packetLen; ++i)
	{
		genericProtocol_receiveByte(i_interface, i_data[i]);
	}
}

static genericProtocol_Checksum_t genericProtocol_send(flawLessInterfaceDescriptor_t i_interface, const uint8_t i_data, bool i_escape)
{
	// pick the send function
	genericProtocol_Checksum_t ret = 0U;
	const phySendFunction_t sendFunction = g_sendFunctions[i_interface];
	static const uint8_t escapeChar = FLAWLESS_PROTOCOL_PACKET_CHAR_ESCAPE;
	if (false != i_escape)
	{
		switch (i_data)
		{
		case FLAWLESS_PROTOCOL_PACKET_CHAR_BEGINNING:
		case FLAWLESS_PROTOCOL_PACKET_CHAR_END:
		case FLAWLESS_PROTOCOL_PACKET_CHAR_ESCAPE:
			/* transmitt escape char */
			(void)(sendFunction)(&escapeChar, sizeof(escapeChar));
			ret += escapeChar;
			break;
		default:
			break;
		}
	}
	ret += i_data;
	(void)(sendFunction)(&i_data, sizeof(i_data));

	return ret;
}


static genericProtocol_Checksum_t genericProtocol_sendChecksum(flawLessInterfaceDescriptor_t i_interface, uint8_t i_checksum)
{
	// pick the send function
	genericProtocol_Checksum_t ret = 0U;
	const phySendFunction_t sendFunction = g_sendFunctions[i_interface];
	static const uint8_t escapeChar = FLAWLESS_PROTOCOL_PACKET_CHAR_ESCAPE;

	switch (i_checksum)
	{
	case FLAWLESS_PROTOCOL_PACKET_CHAR_BEGINNING:
	case FLAWLESS_PROTOCOL_PACKET_CHAR_END:
	case FLAWLESS_PROTOCOL_PACKET_CHAR_ESCAPE:
		/* transmitt escape char */
		(void)(sendFunction)(&escapeChar, sizeof(escapeChar));
		i_checksum += escapeChar;
		break;
	default:
		break;
	}
	i_checksum = (~i_checksum) + 1;
	(void)(sendFunction)(&i_checksum, sizeof(i_checksum));

	return ret;
}

void genericProtocol_sendMessage(flawLessInterfaceDescriptor_t i_interface, genericProtocol_subProtocolIdentifier_t i_subProtocolID, uint16_t i_size, const void *i_buffer)
{
	const uint8_t *data = (const uint8_t*)i_buffer;
	if (i_interface < FLAWLESS_PROTOCOL_PHY_INTERFACES_COUNT)
	{
		genericProtocol_SendState_t *sendState = &g_sendState[i_interface];
		if (false == sendState->isSending)
		{
			uint16_t i = 0U;
			genericProtocol_Checksum_t checksum = 0U;
			static const uint8_t header = FLAWLESS_PROTOCOL_PACKET_CHAR_BEGINNING;
			static const uint8_t tail = FLAWLESS_PROTOCOL_PACKET_CHAR_END;

			genericProtocol_send(i_interface, header, false);
			checksum += genericProtocol_send(i_interface, i_subProtocolID, true);

			for (i = 0U; i < i_size; ++i)
			{
				checksum += genericProtocol_send(i_interface, data[i], true);
			}

			genericProtocol_sendChecksum(i_interface, checksum);
			genericProtocol_send(i_interface, tail, false);
		}
	}
}

void genericProtocol_BeginTransmittingFrame(flawLessInterfaceDescriptor_t i_interface, const uint16_t payloadLen, genericProtocol_subProtocolIdentifier_t i_subProtocolID)
{
	if (i_interface < FLAWLESS_PROTOCOL_PHY_INTERFACES_COUNT)
	{
		genericProtocol_SendState_t *sendState = &g_sendState[i_interface];
		if (false == sendState->isSending)
		{
			const uint8_t header = FLAWLESS_PROTOCOL_PACKET_CHAR_BEGINNING;
			sendState->checksum = 0U;
			sendState->bytesTransmitted = 0U;
			sendState->payloadLen = payloadLen;
			sendState->isSending = true;

			/* send header */
			/* the payloadlen in the header includes the checksum */
			genericProtocol_send(i_interface, header, false);
			sendState->checksum += genericProtocol_send(i_interface, i_subProtocolID, true);
		}
	}
}

void genericProtocol_SendInsideFrame(flawLessInterfaceDescriptor_t i_interface, uint16_t i_size, const void *buffer)
{
	if (i_interface < FLAWLESS_PROTOCOL_PHY_INTERFACES_COUNT)
	{
		genericProtocol_SendState_t *sendState = &g_sendState[i_interface];
		/* send payload */
		const uint8_t *buf = (uint8_t*)buffer;
		uint16_t i = 0U;
		for (i = 0U; i < i_size; ++i)
		{
			sendState->checksum += genericProtocol_send(i_interface, buf[i], true);
		}
		sendState->bytesTransmitted += i_size;
	}

}

void genericProtocol_EndTransmittingFrame(flawLessInterfaceDescriptor_t i_interface)
{
	if (i_interface < FLAWLESS_PROTOCOL_PHY_INTERFACES_COUNT)
	{
		static const uint8_t tail = FLAWLESS_PROTOCOL_PACKET_CHAR_END;
		genericProtocol_SendState_t *sendState = &g_sendState[i_interface];

		genericProtocol_sendChecksum(i_interface, sendState->checksum);
		genericProtocol_send(i_interface, tail, false);

		sendState->checksum = 0U;
		sendState->bytesTransmitted = 0U;
		sendState->isSending = false;
		sendState->payloadLen = 0U;
	}
}

static void genericProtocol_init(void);
MODULE_INIT_FUNCTION(genericProtocol, 5, genericProtocol_init)
static void genericProtocol_init(void)
{
	{
		flawLessInterfaceDescriptor_t i = 0U;
		for (i = 0U; i < FLAWLESS_PROTOCOL_PHY_INTERFACES_COUNT; ++i)
		{
			g_sendState[i].bytesTransmitted = 0U;
			g_sendState[i].checksum = 0U;
			g_sendState[i].isSending = false;
			g_sendState[i].payloadLen = 0U;
			(void)genericProtocoll_resetBuffer(i);
		}
	}
}
