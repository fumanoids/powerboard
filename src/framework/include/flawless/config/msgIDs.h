/*
 * msgIDs.h
 *
 *  Created on: 27.09.2011
 *      Author: lutz
 */

#ifndef MSGIDS_H_
#define MSGIDS_H_

typedef enum tag_msgID
{
	/* user */
	MSG_ID_RELATIVE_BATTERY_VOLTAGE,
	MSG_ID_BATTERY_VOLTAGE,
	MSG_ID_BATTERY_LOAD,
	MSG_ID_USER_MOTOR_SWITCH,

	/* for data distribution */
	MSG_ID_BATTERY_VOLTAGE_OUT,
	MSG_ID_RELATIVE_BATTERY_VOLTAGE_OUT,
	/* system */
	MSG_LAST_ID
}msgID_t;


#endif /* MSGIDS_H_ */
