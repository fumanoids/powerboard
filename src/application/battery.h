/*
 * battery.h
 *
 *  Created on: Sep 10, 2012
 *      Author: lutz
 */

#ifndef BATTERY_H_
#define BATTERY_H_

#include <flawless/stdtypes.h>

/*
 * this type is distributed via the msgPump and over the proxy
 * unit is Volts
 *
 */
typedef uint16_t batteryLevel_t;

/*
 * a number indicating the relative voltage level of battery
 * 0 == completely empty (power shutdown in progress)
 * 0xff = completely full or above
 */
typedef uint8_t relativeBatteryLevel_t;



#endif /* BATTERY_H_ */
