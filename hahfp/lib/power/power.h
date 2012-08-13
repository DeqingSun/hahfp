/*************************************************************************
 Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010
 Part of HeadsetSDK-Stereo R110.0

	FILE : 
				power.h

	CONTAINS:
				API for the Battery power monitoring and charger management

**************************************************************************/


#ifndef POWER_H_
#define POWER_H_

#include <csrtypes.h>
#include <message.h>
#include <adc.h>
#include <charger.h>

/*!
    @brief Define the types for the upstream messages sent from the Power
    library to the application.
*/

/*!
    @brief Configuration parameters passed into the power library in
    order for battery and charging monitoring instance to be created and initialised.
*/
typedef struct
{
    unsigned shutdown_threshold:8 ;      /* < (shutdown_threshold*20 (mV)), headset should be powered off */
    unsigned level_0_threshold:8 ;       /* level 0 threshold, < (level_0_threshold*20 (mV)) means battery low */
                                         /* > (level_0_threshold*20 (mV)) means battery LEVEL_0 */
    unsigned level_1_threshold:8 ;       /* > (level_1_threshold*20 (mV)) means battery LEVEL_1 */
    unsigned level_2_threshold:8 ;       /* > (level_2_threshold*20 (mV)) means battery LEVEL_2 */
    unsigned level_3_threshold:8 ;       /* > (level_3_threshold*20 (mV)) means battery LEVEL_3 */

    /* Interval (in second) at which the battery voltage is read */
    unsigned report_period:8;
    
    /*which AIOs to use for battery and temp monitoring */
    unsigned reserved:7 ;                       /* reserved for future feature */
    unsigned boost_charge_enable:1;             /* enable boost charging */
    
    unsigned enable_volt_monitoring:1;          /* enable voltage monitoring */
    vm_adc_source_type  Batt_Volt_AIO:3 ;      /* AIO0, AIO1, AIO2, AIO3, VDD, BATTERY_INTERNAL */
     
    unsigned enable_temp_monitoring:1;          /* enable temperature monitoring */
    vm_adc_source_type  Batt_Temp_AIO:3 ;      /* AIO0, AIO1, AIO2, AIO3, VDD, BATTERY_INTERNAL */
             
    /* thermister settings for lithium ion battery charging;     
       The following three parameters are used to calculate the 
       the current battery temperature in 'C                    
       The formula for calculating the thermister values is as follows: 
       (((th_m x A2D in mV)+(th_c x 1000)) / 1000) + td 
       the thermister values can be calculated from the manufacturers data sheets 
       and is likely to be different for every different type of thermister used. 
    */

    int16    th_m ;
    int16    th_c ;
    unsigned td:8 ;
    
    /* temperature limits (in 'C) for lithium ion charger, outside of these limits the charger is switched off */
    unsigned max_temp:8 ;
    unsigned min_temp:8 ;
    signed max_boost_temp:8;
    
}power_config_type;

/* Battery Level */
typedef enum
{
    POWER_BATT_CRITICAL,           /* when voltage (mV) < shutdown_threshold * 20 */
    POWER_BATT_LOW,                /* when voltage (mV) < level_0_threshold * 20 */
    POWER_BATT_LEVEL0,             /* when voltage (mV) >= level_0_threshold * 20 */
	POWER_BATT_LEVEL1,             /* when voltage (mV) >= level_1_threshold * 20 */
	POWER_BATT_LEVEL2,             /* when voltage (mV) >= level_2_threshold * 20 */
	POWER_BATT_LEVEL3              /* when voltage (mV) >= level_3_threshold * 20 */
}power_battery_level;


/* Definition of the power type, a global structure used in library */
typedef struct
{
	Task                clientTask;                     /* The Application/Higher level task */
       
    /* Configuration */
	power_config_type	battery_config;                 
	  
    /* Charger state */
    charger_status      charger_state:3;                /* Store the last charger state, it is used for charger state report */
    unsigned            voltage:13;

    unsigned            charger_connect_flag:1;     
    unsigned            initial_reading_complete:1;
    unsigned            boost_charge_started:1;          /* TRUE:  boost charge has been started since charger was connected; 
                                                            FALSE: boost charge has not been started since charger was connected */
    unsigned            vdd_reading:13;                  /* It is used to calculate the battery voltage and temperature */
    
    int16               temperature;
    
    unsigned            boost_charge_disabled:1;
    unsigned            unused:15;
}power_type;


/* Define the BATTERY MESSAGES */

#define POWER_MESSAGE_BASE    0x6800       /* This is API message*/

typedef enum
{
/* 0x6800 */    POWER_INIT_CFM = POWER_MESSAGE_BASE ,
/* 0x6801 */    POWER_BATTERY_LEVEL_IND, 
/* 0x6802 */    POWER_CHARGER_STATE_IND, 
                
                POWER_MESSAGE_TOP
}PowerMessageId ;


/*!
    @brief This message is returned when the battery and charger monitoring 
     subsystem has been initialised.
*/
typedef struct
{
  bool status;
} POWER_INIT_CFM_T;

/*!
    @brief This message is returned to App to indicate the battery voltage level and its value.

    This message will be returned based on the configured monitoring_period, or when function 
    PowerGetVoltage() has been called;
    
    The application can use this level or voltage and make corresponding operations, 
    for example, send them to AG.
*/
typedef struct
{
  power_battery_level pLevel;
  uint16              voltage;                    /* voltage in mV */
} POWER_BATTERY_LEVEL_IND_T;


/*!
    @brief This message is returned to App to indicate the charging state.

    The application, for example, can use charging state change to change LED pattern.
*/
typedef struct
{
  charger_status   old_state;
  charger_status   new_state;
} POWER_CHARGER_STATE_IND_T;


/****************************************************************************
NAME    
    PowerInit
    
DESCRIPTION
  	This function will initialise the battery and battery charging sub-system.  
    The sub-system manages the reading and calulation of the battery voltage and 
    temperature
    
    The application will receive a POWER_INIT_CFM message from the library 
    indicating the initialisation status.
    
RETURNS
    void
*/
void PowerInit(Task clientTask, const power_type *config);


/****************************************************************************
NAME    
    PowerGetVoltage
    
DESCRIPTION
  	Call this function to get the current battery voltage (in mV) and the level;
 
RETURNS
    void
*/
void PowerGetVoltage(POWER_BATTERY_LEVEL_IND_T * Volt_Level);


/****************************************************************************
NAME    
    PowerGetTemperature
    
DESCRIPTION
  	Call this function to get the current battery temperature (in 'c)
    
RETURNS
    bool
*/
bool PowerGetTemperature( int16 *temperature );


/****************************************************************************
NAME    
    PowerCharger
    
DESCRIPTION
  	This function is called by the application when the charger has been 
	plugged or disconnectd with the headset
    
RETURNS
    void
*/
void PowerCharger( bool enable );

#endif     /* POWER_H_ */
