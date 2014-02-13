/*
 * transport.h
 *
 *  Created on: Jun 8, 2012
 *      Author: lutz
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <flawless/stdtypes.h>

/**
 * mutex functions
 * The mutex functions must be safe for recursive usage!
 */
void system_mutex_lock();
void system_mutex_unlock();

uint32_t system_getTime_ms(void);

#endif /* SYSTEM_H_ */
