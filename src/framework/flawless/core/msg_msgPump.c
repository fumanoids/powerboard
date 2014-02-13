/*
 * msg_msgPump.c
 *
 *  Created on: 26.09.2011
 *      Author: lutz
 */

#include <flawless/stdtypes.h>
#include <flawless/platform/system.h>
#include <string.h>
#include <flawless/config/msgIDs.h>
#include <flawless/core/msg_msgPump.h>

#define LOG_LEVEL 0
#include <flawless/logging/logging.h>
#include <flawless/init/systemInitializer.h>

#include <flawless/config/msgPump_config.h>

#include "systemMsgIDs.h"


#define MSG_PUMP_MAX_FREQUENT_JOBS_COUNT 1

#define MAGIC 0xDEADBEEF

typedef struct st_msgPumpMsgQueueEntry
{
	msgPump_MsgID_t id; /* the ID of the message */
	msgBufBufData_t *buffer; /* the reference to the buffer */
	msgBufDescription_t *bufferDescription; /* a pointer to the structure describing the buffer */
} msgPumpMsgQueueEntry_t;

#if MAX_AMOUNT_OF_MSG_RECERIVER_PER_MESSAGE > 255
#error too many possible recepients of a message!
#endif

#if MSG_PUMP_MSG_QUEUE_MAX_SIZE > 255
#error msg queue is too big!
#endif

/************ globals ****************/

extern msgBufDescription_t _msgPumpBPHandlesBegin;
extern msgBufDescription_t _msgPumpBPHandlesEnd;

/*
 * Here we remember pointers to all the buffer hanldes which register
 */
msgBufDescription_t *g_messageBuffersBuffer[MSG_LAST_ID + MSG_ID_SYSTEM_MSG_COUNT] = {NULL};
msgBufDescription_t **g_messageBuffers = &(g_messageBuffersBuffer[MSG_ID_SYSTEM_MSG_COUNT]);

static volatile unsigned int g_msgQueueFront = 0U, g_msgQueueTail = 0U,
		g_msgQueueSize = 0U; /* indexes in the ring buffer */
static volatile msgPumpMsgQueueEntry_t g_msgQueue[MSG_PUMP_MSG_QUEUE_MAX_SIZE];


/*************** internal function ********************/
static void setBufferFlag(msgBufBufData_t *buffer, msgPump_MessageBufferFlag_t flag)
{
	msgPump_MessageBufferFlag_t *flagPtr = ((msgPump_MessageBufferFlag_t*) buffer) - 1;
	*flagPtr = flag;
}

/*
 * register on a message. Adds a callback function to a message
 * @param id the ID of the message to register on
 * @param callbackFunction the function to call if the message occurred
 * @param info the argument to be passed to the callback function
 * @return success. There can be a maximum amount of MAX_AMOUNT_OF_MSG_RECERIVER_PER_MESSAGE receivers per message
 */bool msgPump_registerOnMessage(const msgPump_MsgID_t id,
		const msgPump_callbackFunction_t i_callbackFunction)
{
	system_mutex_lock();
	bool registerSuccess = false;

	if ((id < MSG_LAST_ID) &&
			(NULL != i_callbackFunction))
	{
		msgBufDescription_t *msgBuffer = g_messageBuffers[id];
		if ((NULL != msgBuffer) && (id == msgBuffer->id))
		{
			/* found the corresponding message buffer! So let's add the callback function */
			int i;
			for (i = 0; i < MAX_AMOUNT_OF_MSG_RECERIVER_PER_MESSAGE; ++i)
			{
				if (NULL == (*msgBuffer->callbackVector)[i] || i_callbackFunction == (*msgBuffer->callbackVector)[i])
				{
					(*msgBuffer->callbackVector)[i] = i_callbackFunction;
					registerSuccess = true;
					break;
				}
			}
		}
	}

	system_mutex_unlock();
	return registerSuccess;
}

/*
 * unregister a function from a message
 * @param id the message's ID to unregister from
 * @param callbackFunction the function to unregister
 */bool msgPump_unregisterFromMessage(const msgPump_MsgID_t id,
		const msgPump_callbackFunction_t callbackFunction)
{
	system_mutex_lock();
	bool unRegisterSuccess = false;


	if ((id < MSG_LAST_ID) &&
		(NULL != callbackFunction))
	{
		msgBufDescription_t *msgBuffer = g_messageBuffers[id];

		if (id == msgBuffer->id)
		{
			int i;
			/* found the corresponding message buffer! So let's remove the callback function... if it is registered */
			for (i = 0; i < MAX_AMOUNT_OF_MSG_RECERIVER_PER_MESSAGE; ++i)
			{
				if (callbackFunction == (*msgBuffer->callbackVector)[i])
				{
					/* found it! */
					(*msgBuffer->callbackVector)[i] = NULL;
					break;
				}
			}
		}
	}

	system_mutex_unlock();
	return unRegisterSuccess;
}

/*
 * post a message into the queue. When doing this the reference count is set to 1
 * @param bufferName the buffer to post the message into
 * @param messageToPost a Pointer to a memory region where the message is. This area has to be equal in size to the messageType which was specified when declaring the buffer.
 * @return success. False if the queue is full.
 */
bool msgPump_postMessage(const msgPump_MsgID_t id, void *messageToPost)
{
	bool postSuccess = false;
	/* find the message buffer */
	if (g_msgQueueFront == g_msgQueueTail && 0 != g_msgQueueSize)
	{
		return postSuccess;
	}
	if ((id < MSG_LAST_ID) &&
		(NULL != messageToPost))
	{
		msgBufDescription_t *msgBuffer = g_messageBuffers[id];

		if (id == msgBuffer->id)
		{
			/* found it*/

			msgBufBufData_t *msgBuffData = msgBuffer->data;
			const msgBufMsgSize_t msgSize = msgBuffer->msgSize;

			/* if the pointer to messageToPost belongs to an already reserved buffer use it */
			/* don't ever use implicit casts! */
			const uint8_t *msgBufPos = (const uint8_t*) messageToPost;
			const uint8_t *basePos = (const uint8_t*) msgBuffData;
			const uint16_t ptrOffset = msgBufPos - basePos;
			const uint16_t bufLen = (msgBuffer->msgAmount
					* (MSG_BUFFER_MSG_OVERHEAD + msgSize));

			if (ptrOffset >= (MSG_BUFFER_MSG_OVERHEAD) && ptrOffset < bufLen)
			{
				/* the caller used an internal buffer! */
				/* but is the pointer directing to a valid element? */
				const uint16_t msgLenWithOverhead = msgSize
						+ MSG_BUFFER_MSG_OVERHEAD;
				if (MSG_BUFFER_MSG_OVERHEAD == ptrOffset % msgLenWithOverhead)
				{
					msgPump_MessageBufferFlag_t *flagPtr =
							((msgPump_MessageBufferFlag_t *) messageToPost) - 1;
					msgBufRefCount_t *refCountPtr =
							((msgBufRefCount_t*) flagPtr) - 1;
					if (1 == *refCountPtr
							&& MsgPump_MsgBufFlagInUse == *flagPtr)
					{
						system_mutex_lock();
						/* no one else is using this buffer element! */
						*flagPtr = MsgPump_MsgBufFlagPostedNotDispatched;
						*refCountPtr = 1U;

						/* insert the message into the queue*/
						g_msgQueue[g_msgQueueTail].buffer = messageToPost;
						g_msgQueue[g_msgQueueTail].bufferDescription =
								msgBuffer;
						g_msgQueue[g_msgQueueTail].id = id;
						g_msgQueueTail = (g_msgQueueTail + 1)
								% MSG_PUMP_MSG_QUEUE_MAX_SIZE;
						++g_msgQueueSize;
						postSuccess = true;
						system_mutex_unlock();
					}
					else
					{
						/* WTF?! this buffer is completely lost!!! I'll mark it as invalid */
//							*flagPtr = MsgPump_MsgBufFlagBufferNotUsable;
						/* ugly fix: magic recovery */
						*flagPtr = MsgPump_MsgBufFlagNothing;
						*refCountPtr = 0U;
						LOG_ERROR_0("Buffer lost!");
					}
				}
				else
				{
					/* BADBADBAD!!! someone is using this message pump wrong! */
				}
			}
			else
			{
				int i;
				/* the caller used an own buffer so we have to copy it's content into a free buffer */
				/* find some free space in the buffer */
				for (i = 0; i < msgBuffer->msgAmount; ++i)
				{
					system_mutex_lock();
					msgBufBufData_t *curElement = &(msgBuffData[i
							* (msgSize + MSG_BUFFER_MSG_OVERHEAD)]);
					msgBufRefCount_t *curRefCount =
							(msgBufRefCount_t*) curElement;
					msgPump_MessageBufferFlag_t *curFlagPtr =
							(msgPump_MessageBufferFlag_t*) (curElement + 1);
					msgBufBufData_t *dataPtr = (msgBufBufData_t*) (curFlagPtr
							+ 1);
					if ((0 == *curRefCount)
							&& (MsgPump_MsgBufFlagNothing == *curFlagPtr))
					{
						/* found a free buffer! */
						bool lockSuccess = msgPump_lockMessage(dataPtr);
						if (true == lockSuccess)
						{
							*curFlagPtr = MsgPump_MsgBufFlagPostedNotDispatched;
							memcpy(dataPtr, messageToPost, msgSize);

							/* insert the message into the queue*/
							g_msgQueue[g_msgQueueTail].buffer = dataPtr;
							g_msgQueue[g_msgQueueTail].bufferDescription =
									msgBuffer;
							g_msgQueue[g_msgQueueTail].id = id;
							g_msgQueueTail = (g_msgQueueTail + 1)
									% MSG_PUMP_MSG_QUEUE_MAX_SIZE;
							++g_msgQueueSize;
							postSuccess = true;
							system_mutex_unlock();
							break;
						} else
						{
							/* cannot lock this buffer (should not happen anyway) */
							continue;
						}
					}
					system_mutex_unlock();
				}
				if (false == postSuccess)
				{
				}
			}
		}
	}
	if (false == postSuccess)
	{
	}
	return postSuccess;
}

/**
 * Add a reference to this message to prevent this buffer to be overwritten when a new message was posted.
 * The MessagePump decrements the retain count after each callback function returned so use this function if you need access to that data later.
 * @return success. Returns false if the message cannot be locked any more because an overflow would happen.
 */
 bool msgPump_lockMessage(void *messageToPost)
{
	bool ret = false;
	msgBufRefCount_t *refCountPtr;
	msgPump_MessageBufferFlag_t *flagPtr;
	msgBufRefCount_t refCount;
	msgPump_MessageBufferFlag_t flag;

	flagPtr = ((msgPump_MessageBufferFlag_t*) messageToPost) - 1;
	refCountPtr = ((msgBufRefCount_t*) flagPtr) - 1;
	system_mutex_lock();
	flag = *flagPtr;
	refCount = *refCountPtr;

	if ((255U > refCount) && (MsgPump_MsgBufFlagBufferNotUsable != flag))
	{
		/* we can increment the reference count */
		*(refCountPtr) += 1;
		ret = true;
	} else
	{
		LOG_WARNING_0("cannot lock buffer");
		ret = false;
	}
	system_mutex_unlock();
	return ret;
}

/*
 * unlock a message to signal that it's memory space can be used for the data producer.
 * The retain count will be decremented after a callback function returned.
 * This function has to be called if the retain count was incremented via msgPump_lockMesasge().
 * A Message is considered to be free if the amount of unlocks is equal to the amount of locks.
 * @return success. If this operation causes the reference count to be less 0 false.
 */
 bool msgPump_unlockMessage(void *messageToPost)
{
	bool ret = false;
	msgBufRefCount_t *refCountPtr;
	msgPump_MessageBufferFlag_t *flagPtr;
	msgBufRefCount_t refCount;
	msgPump_MessageBufferFlag_t flag;

	flagPtr = ((msgPump_MessageBufferFlag_t*) messageToPost) - 1;
	refCountPtr = ((msgBufRefCount_t*) flagPtr) - 1;
	system_mutex_lock();
	flag = *flagPtr;
	refCount = *refCountPtr;

	if ((0U < refCount) && (MsgPump_MsgBufFlagBufferNotUsable != flag))
	{
		/* we can decrement the reference count */
		*(refCountPtr) -= 1;
		ret = true;
	} else
	{
		LOG_WARNING_0("cannot unlock a buffer!");
	}

	system_mutex_unlock();
	return ret;
}

/*
 * if there is a free (not-locked) buffer element in a messageBuffer it can be locked with this function
 * @return success. false if there is no unused buffer element
 */
 bool msgPump_getFreeBuffer(const msgPump_MsgID_t id,
		void **o_destPtr)
{
	bool ret = false;
	int i;
	msgBufDescription_t *bufferDescriptor = g_messageBuffers[id];
	if (NULL != bufferDescriptor)
	{
		system_mutex_lock();
		for (i = 0; i < bufferDescriptor->msgAmount; ++i)
		{
			msgBufRefCount_t *refCountPtr =
					(msgBufRefCount_t *) &(bufferDescriptor->data[i
							* (bufferDescriptor->msgSize + MSG_BUFFER_MSG_OVERHEAD)]);
			msgPump_MessageBufferFlag_t *flagPtr =
					(msgPump_MessageBufferFlag_t*)( refCountPtr + 1);
			msgBufBufData_t *bufData = (msgBufBufData_t*) (flagPtr + 1);
			if (0 == *refCountPtr && MsgPump_MsgBufFlagNothing == *flagPtr)
			{
				/* found a free buffer element */
				*o_destPtr = bufData;
				/* lock it */
				bool lockSuccess = msgPump_lockMessage(*o_destPtr);
				if (true == lockSuccess)
				{
					/* we were able to lock this buffer element so get out of the loop */
					/* this check is necessary since an interrupt can catch the buffer element before we do */
					*flagPtr = MsgPump_MsgBufFlagInUse;
					ret = true;
					break;
				}
			} else
			{
				*o_destPtr = NULL;
			}
		}
		system_mutex_unlock();
	}

	if (false == ret)
	{
		LOG_WARNING_0("Cannot provide free buffer");
		*o_destPtr = NULL;
	}
	return ret;
}

void msgPump_pumpMessage()
{
	while (true)
	{
		if (g_msgQueueSize != 0)
		{
			LOG_VERBOSE_0("pumping");
			volatile msgPumpMsgQueueEntry_t *message =
					&(g_msgQueue[g_msgQueueFront]);
			uint8_t i;

			(void) setBufferFlag(message->buffer, MsgPump_MsgBufFlagInUse);
			for (i = 0; i < MAX_AMOUNT_OF_MSG_RECERIVER_PER_MESSAGE; ++i)
			{
				if ((*message->bufferDescription->callbackVector)[i] != NULL)
				{
					/* call the callback function */
					(void) ((*message->bufferDescription->callbackVector)[i])(
							message->bufferDescription->id,
							message->buffer);
				}
			}
			(void) setBufferFlag(message->buffer, MsgPump_MsgBufFlagNothing);

			msgPump_unlockMessage(message->buffer);
			system_mutex_lock();
			g_msgQueueFront = (g_msgQueueFront + 1)
					% MSG_PUMP_MSG_QUEUE_MAX_SIZE;
			g_msgQueueSize--;
			system_mutex_unlock();
		}
		else
		{
			/*	system_wait_for_interrupt(); */
		}
	}
}


/*
 * initialize the message pump.
 * All messages produced during the initialization process are discarded till now.
 * All messages produced after this init function will be processed
 */
static void msgPump_init(void);
MODULE_INIT_FUNCTION(msgPump, 2, msgPump_init)
static void msgPump_init(void)
{
		msgBufDescription_t *desc = &_msgPumpBPHandlesBegin;
		system_mutex_lock();

		while (desc < &_msgPumpBPHandlesEnd)
		{
			g_messageBuffers[desc->id] = desc;
			memset(desc->data, 0, (desc->msgAmount * (desc->msgSize + MSG_BUFFER_MSG_OVERHEAD)));
			desc += 1;
		}
		memset((void*)g_msgQueue, 0, sizeof(g_msgQueue));
		g_msgQueueFront = 0U;
		g_msgQueueSize = 0U;
		g_msgQueueTail = 0U;
		system_mutex_unlock();
}
