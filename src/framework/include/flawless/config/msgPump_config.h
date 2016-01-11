/*
 * msgPump_config.h
 *
 *  Created on: Jun 8, 2012
 *      Author: lutz
 */

#ifndef MSGPUMP_CONFIG_H_
#define MSGPUMP_CONFIG_H_


/**
 * a maximum of 64 messages can be posted at a time
 */
#define MSG_PUMP_MSG_QUEUE_MAX_SIZE 16

/*
 * the maximum amount of message recievers per message
 */
#define MAX_AMOUNT_OF_MSG_RECERIVER_PER_MESSAGE 4

/*
 * some hex number should be mostly unique and must fit into an integer
 */
#define MSG_PUMP_MAGIC 0xDEADBEEF

/*
 * use magic number in header or not
 * (if defined the header magic will be used)
 */
#define MSG_PUMP_USE_HEADER_MAGIC

#endif /* MSGPUMP_CONFIG_H_ */
