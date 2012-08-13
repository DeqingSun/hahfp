/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    power_battery_monitoring.c

DESCRIPTION
    This file contains the battery monitoring specific functionality.  This includes
	battery voltage and temperature monitoring

NOTES
	Certain assumptions have been made on the assignment of the analog input
	signals:
	
	
	VBAT = AIO_0 *   Ra
				   -------
				   Ra + Rb
				   
	Where the divisor ratio of Ra and Rb is configured from persistent store
				   
	The AIOs that are connected to measure the voltage and temperature are configurable 
    in (power.battery_config)->Batt_Volt_AIO and (power.battery_config)->Batt_Temp_AIO;
    
    If (power.battery_config)->Batt_Volt_AIO or (power.battery_config)->Batt_Temp_AIO is 
    configured to POWER_SOURCE_NONE, no monitoring is running for voltage or temperature.
*/


/****************************************************************************
    Header files
*/
#include "power.h"
#include "power_private.h"

#include <app/message/system_message.h>	    /*For Message types*/
#include <vm.h>    
#include <stdio.h>

/* Static function declarations */
static void batteryLevelInd(power_battery_level pLevel, uint16 voltage);
static void batteryHandleVoltageReading(uint16 reading);
static void batteryHandleTemperatureReading(uint16 reading);
static void BatteryInit(Task task);
static void battery_Handler(Task task, MessageId id, Message message);

const PowerTaskdata batt_voltage_plugin     = {{ battery_Handler }, POWER_BATT_VOLT};
const PowerTaskdata batt_temperature_plugin = {{ battery_Handler }, POWER_BATT_TEMP};

/* Define a global variable for power library */
power_type *power;

/****************************************************************************
NAME    
    repeatReading
    
DESCRIPTION
    This function sends a retry message to the task passed with the source passed
    Used to reattempt readings from the Adc.
*/
static void repeatReading(Task task, vm_adc_source_type source)
{
	vm_adc_source_type* reading_to_repeat = malloc(sizeof(vm_adc_source_type));
	*reading_to_repeat = source;
	MessageSend(task, BATTERY_INTERNAL_RETRY_MESSAGE, reading_to_repeat);
}


/****************************************************************************
NAME    
    batteryLevelInd
    
DESCRIPTION
    Call to send the battery voltage and its level to App.
*/
static void batteryLevelInd(power_battery_level pLevel, uint16 reading )
{
    /* Send a message to applications to indicate the battery level and voltage */
    MAKE_BATT_MESSAGE(POWER_BATTERY_LEVEL_IND);
    message->pLevel  = pLevel;
    message->voltage = reading;
  
    MessageSend(power->clientTask, POWER_BATTERY_LEVEL_IND, message);
}


/****************************************************************************
NAME    
    batteryDecideLevel
    
DESCRIPTION
  	This function is called to decide the battery level based on the reading 
    voltage value.
    
RETURNS
    power_battery_level
*/
static power_battery_level batteryDecideLevel(uint16 vb)
{
    power_config_type *battery = &(power->battery_config); 
    
    power_battery_level pLevel = POWER_BATT_LEVEL0;
    
    uint16 l3_threshold       = battery->level_3_threshold  * 20;
    uint16 l2_threshold       = battery->level_2_threshold  * 20;
    uint16 l1_threshold       = battery->level_1_threshold  * 20;
    uint16 l0_threshold       = battery->level_0_threshold  * 20;
    uint16 shutdown_threshold = battery->shutdown_threshold * 20;
     
 	/* Check current voltage level against configured thresholds */
	if(vb >= l3_threshold)
	{
        pLevel = POWER_BATT_LEVEL3;
	}
	else if(vb >= l2_threshold)
	{
        pLevel = POWER_BATT_LEVEL2;
	}
	else if(vb >= l1_threshold)
	{
        pLevel = POWER_BATT_LEVEL1;
	}
    else if(vb >= l0_threshold)
	{
        pLevel = POWER_BATT_LEVEL0;
	}
	else if(vb < shutdown_threshold)
	{
        pLevel = POWER_BATT_CRITICAL;
	}
	else if(vb < l0_threshold)
	{
        pLevel = POWER_BATT_LOW;
	}
       
    return(pLevel);
}

/****************************************************************************
NAME    
    batteryHandleVoltageReading
    
DESCRIPTION
  	Calculate current battery voltage and check to determine the battery level.
    Both battery voltage and level are sent to App and the App decides the 
    further operation base on the reported results.
*/
static void batteryHandleVoltageReading(uint16 reading)
{
    power_battery_level pLevel_old = batteryDecideLevel(power->voltage);
	power_battery_level pLevel_new;
    charger_status state = ChargerStatus();
    
    if(!power->initial_reading_complete)
    {
        MAKE_BATT_MESSAGE(POWER_INIT_CFM);
        message->status = TRUE;
        MessageSend(power->clientTask, POWER_INIT_CFM, message);  
        
        power->voltage = reading;
        power->initial_reading_complete = TRUE; 
    }
    else
    {
	    /* Based on the last reading and charging state, decide on the current reading value */
        if( state != NO_POWER )
        {
            /*when the headset starts, the voltage is set 0*/
            if( power->voltage < reading )
                power->voltage = reading;
        }
        else
        {
            /*when the headset starts, the voltage is set 0*/
            if( power->voltage > reading )
                power->voltage = reading;
        }
    }
    
	pLevel_new = batteryDecideLevel(power->voltage);
  
	/* Send the power battery level to App if the battery level changed or battery low and no charger connected */
    if( pLevel_new != pLevel_old      || 
       ((pLevel_new == POWER_BATT_LOW) && ((state == NO_POWER) || !power->charger_connect_flag) ) ) 
    {
        batteryLevelInd(pLevel_new, power->voltage);
        power->charger_connect_flag = (state != NO_POWER) ? 1 : 0;
    }
}


/****************************************************************************
NAME    
    batteryHandleTemperatureReading
    
DESCRIPTION
    Calculate the current battery temperature
*/
static void batteryHandleTemperatureReading(uint16 reading)
{
    int16 tb;
    power_config_type *battery = &(power->battery_config);
    
	/* Calculate the current battery temperature in 'C - tb ('C) = (th_m * (AIO/1000)) + th_c + Td */      
    tb = ((((int32)battery->th_m * (int32)reading) + ((int32)battery->th_c * 1000)) / (int32)1000) + (int32)battery->td;

    /* Store current temperature reading */
	power->temperature = tb;
}


/****************************************************************************
NAME    
    battery_Handler
    
DESCRIPTION
  This function is sent:
   MESSAGE_ADC_RESULT messages from the firmware.
   BATTERY_INTERNAL_TIMED_MESSAGE messages from itself.	
   BATTERY_INTERNAL_RETRY_MESSAGE messages when an attempted to read the adc.
*/
static void battery_Handler(Task task, MessageId id, Message message)
{
    switch(id)
    {		
        case BATTERY_INTERNAL_TIMED_MESSAGE:
        {
            /* We are resampling Vref to get Vdd */
            if(!AdcRequest(task, adcsel_vref))
                repeatReading(task, adcsel_vref);
        }		
        break;
		
        case BATTERY_INTERNAL_RETRY_MESSAGE:
        {
            vm_adc_source_type* repeat_src = (vm_adc_source_type*)message;
			
            if(!AdcRequest(task, *repeat_src))
                repeatReading(task, *repeat_src);
        }	
        break;

        case MESSAGE_ADC_RESULT:
        {
            PowerTaskdata  *state    = (PowerTaskdata   *)task;
            MessageAdcResult* result = (MessageAdcResult*)message;
            uint16 reading           = result->reading;
            
            power_config_type *battery_config = &((power->battery_config));
                    
            /* 
             * Vdd = (Vref/reading)*255  =  (Vref*255)/reading 
             */
            uint32 vref_mult_255 = (uint32)VmReadVrefConstant() * 255;
            
            if(result->adc_source == adcsel_vref)
            {
                /*
                    We have Vref, so use this to calculate Vdd
                    Vdd = (Vref / reading)*255  = (Vref*255)/reading  
                */
                power->vdd_reading = vref_mult_255 / reading;
                    
                /*
                    Now get the clients original request from the Adc 
                */
                if(state->aio_source == POWER_BATT_VOLT)
                {
					vm_adc_source_type adc_src = battery_config->Batt_Volt_AIO;
				    
					if(!AdcRequest(task, adc_src))
                        repeatReading(task, adc_src);			
                }
                
                if(state->aio_source == POWER_BATT_TEMP)
                {
					vm_adc_source_type adc_src = battery_config->Batt_Temp_AIO;

				    if(!AdcRequest(task, adc_src))
                        repeatReading(task, adc_src);			
                }
            }
            else
            {
                /*
                   We have a reading from the source the client requested and a reading of Vdd. 
                */
                uint16 cur_reading = ((uint32)(power->vdd_reading)*(reading))/255;
                
				/* Processing the reading results */  
                if(state->aio_source == POWER_BATT_VOLT)
                {
				    batteryHandleVoltageReading(cur_reading);                    
                }
                else if(state->aio_source == POWER_BATT_TEMP)
                {
                    batteryHandleTemperatureReading(cur_reading);
                }

                /* Work out if we're just doing single reading */
                if(battery_config->report_period)
                {
                    /* Now send a timed message to self to read Vref again */
                    MessageSendLater(task, BATTERY_INTERNAL_TIMED_MESSAGE, 0, D_SEC(battery_config->report_period));
                }
            }
        }
        
        break;
    }
}

/****************************************************************************
NAME    
    BatteryInit
    
DESCRIPTION
    Start the power library to monitor the voltage or temperature
*/

static void BatteryInit(Task task)
{
    /* as this function is used to stop and restart the battery voltage/temperature readings
       ensure there are no pending reading messages otherwise it is possible to
       have numerous concurrent timed readings in progress */
    
    if(!((power->battery_config).report_period))
        MessageCancelAll(task, BATTERY_INTERNAL_TIMED_MESSAGE);
    
    /* Now make the first request - task to deliver msg to and src to read */
    if(!AdcRequest(task, adcsel_vref))
        repeatReading(task, adcsel_vref);
}


/****************************************************************************
NAME    
    PowerInit
    
DESCRIPTION
    This function will initialise the battery and its charging sub-system.  
    The sub-systems manages the reading and calulation of the battery voltage 
    and temperature, and the monitoring the charger status. 
*/
void PowerInit(Task clientTask, const power_type *power_config)
{
    power = (power_type *)power_config;
    
    if(power)
    {
        power_config_type *battery_config = &((power->battery_config));
        
        /* copy the configuration data to library */
        power->clientTask     = clientTask;
  
        /* Set default voltage and temperature*/
        power->voltage        = 0;
        power->temperature    = battery_config->min_temp;
 
        power->initial_reading_complete = FALSE;
        power->charger_connect_flag     = (ChargerStatus() != NO_POWER) ? 1 : 0;
        
        /* Initial battery voltage monitoring */
        if(battery_config->enable_volt_monitoring)
            BatteryInit((TaskData *)&batt_voltage_plugin);
        
        /* Initial battery temperature monitoring */
        if(battery_config->enable_temp_monitoring)
            BatteryInit((TaskData *)&batt_temperature_plugin);
        
    	/* Assume charger disconnected at boot time */
	    power->charger_state  = NO_POWER;
    }
    else
    {
        MAKE_BATT_MESSAGE(POWER_INIT_CFM);
        message->status = FALSE;
        MessageSend(power->clientTask, POWER_INIT_CFM, message);
    }
}

/****************************************************************************
NAME    
    PowerGetVoltage
    
DESCRIPTION
    Call this function to get the current battery voltage and its level
    
RETURN
    void
*/
void PowerGetVoltage( POWER_BATTERY_LEVEL_IND_T * Volt_Level )
{
    /* Headset will panic if trying to get the voltage value before the first reading completes */
    if(power->initial_reading_complete)
    {
        Volt_Level->voltage = (uint16)(power->voltage);
        Volt_Level->pLevel  = batteryDecideLevel(Volt_Level->voltage);
    }
    else
    {
        Panic();
    }
}

/****************************************************************************
NAME    
    PowerGetTemperature
    
DESCRIPTION
    Call this function to get the current battery temperature in 'C
    
RETURN
    BOOL
*/
bool PowerGetTemperature( int16 *current_reading )
{
    power_config_type *battery_config = &((power->battery_config));
    *current_reading = power->temperature;
        
    if(power->temperature >= battery_config->min_temp && 
       power->temperature <= battery_config->max_temp )
	    return TRUE;
    else
        return FALSE;
}

