/*
 * battery.h
 *
 *  Created on: Sep 10, 2012
 *      Author: lutz
 */

#ifndef BATTERY_H_
#define BATTERY_H_

#include <flawless/stdtypes.h>

typedef enum tag_measuringState {
	MEASURING_BAT_VOLTAGE,
	MEASURING_5V_VOLTAGE,
	MEASURING_MOTOR_VOLTAGE,
	MEASURING_5V_CURRENT,
	MEASURING_MOTOR_CURRENT,
	MEASURING_CHANNEL_COUNT = 5
} measuringState_t;

/*
 * this type is distributed via the msgPump and over the proxy
 * unit is centi Volts
 *
 */
typedef uint32_t batteryVoltageLevel_t;
typedef uint32_t motorVoltageLevel_t;
typedef uint32_t systemVoltageLevel_t;

/*
 * this type is distributed via the msgPump and over the proxy
 * unit is milliampere
 */
typedef int32_t motorCurrentLevel_t;
typedef int32_t systemCurrentLevel_t;

typedef struct {
	batteryVoltageLevel_t batteryVoltcV;   /* battery voltage in centivolts */
	motorVoltageLevel_t   motorVoltcV;     /* motor voltage in centivolts */
	systemVoltageLevel_t  systemVoltcV;    /* 5V domain voltage in centivolts */
	motorCurrentLevel_t   motorCurrentcA;  /* current on the motor domain in milliAmps*/
	systemCurrentLevel_t  systemCurrentcA; /* current on the motor 5V domain in milliAmps*/
} systemPowerState_t;

/*
 * a number indicating the relative voltage level of battery
 * 0 == completely empty (power shutdown in progress)
 * 0xff = completely full or above
 */
typedef uint8_t relativeBatteryLevel_t;


typedef uint16_t voltageMeasurementsRaw_t[MEASURING_CHANNEL_COUNT * 20]; // 10 measurements of each channel


#endif /* BATTERY_H_ */
