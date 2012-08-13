/*************************************************************************
 Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010
 Part of HeadsetSDK-Stereo R110.0

	FILE : 
				battery_private.h

	CONTAINS:
				Internal information for the Battery Power Monitoring and 
                and charging management 

**************************************************************************/


#ifndef POWER_PRIVATE_H_
#define POWER_PRIVATE_H_

#include <adc.h>                            /*For Adc functions*/
#include <message_.h> 
#include <power.h>

#include <stdlib.h>

/* Macros for creating messages */
#include <panic.h>

#define MAKE_BATT_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/* Internal messages */
enum
{
    BATTERY_INTERNAL_TIMED_MESSAGE, 
    BATTERY_INTERNAL_RETRY_MESSAGE
};

/*!
  @brief define the instance of the power library for monitoring battery.
*/
typedef enum
{
	POWER_BATT_VOLT	= 1,
	POWER_BATT_TEMP	= 2
}Power_Monitoring_Source;

/* This structure is used to define the battery task of each instance */
typedef struct
{
	TaskData	              task;
	Power_Monitoring_Source   aio_source ;	    
}PowerTaskdata;


extern const PowerTaskdata batt_voltage_plugin;
extern const PowerTaskdata batt_temperature_plugin;


/*the power data structure - visible to all (so don't pass it between functions!)*/
extern power_type *power;


#endif /* POWER_PRIVATE_H_ */


