/*************************************************************************
  Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010
  Part of HeadsetSDK-Stereo R110.0

	FILE : 
				battery.c

	CONTAINS:
				The Battery Monitor Library for BlueLab3

	DESCRIPTION:
				Allows readings of the test pins and supply voltage to be taken
				either once, or at regular intervals.
				The readings are returned to a specified Task.

**************************************************************************/



/* INCLUDES */
#include "battery.h"
#include "battery_private.h"

#include <stdlib.h>                         /*For malloc and NULL*/

#include <adc.h>                            /*For Adc functions*/
#include <app/message/system_message.h>	    /*For Message types*/
#include <message.h>                        /*For Message functions*/



/* FUNCTION IMPLEMENTATIONS */

/*
   Map the battery source to read from to an ADC source.
   This code depends on the battery_reading_source type and the
   vm_adc_source_type defining AIO0, AIO1, VDD/VREF, AIO2 in 
   the same order:
       if(AIO0 == VM_ADC_SRC_AIO0) => if(0==0) etc...
   Compiler can optimise out the if else statement for each case, 
   so each case becomes:
       add_src = (vm_adc_source_type) bat_src;
   Compiler can then optimise out the whole switch.
*/
static vm_adc_source_type battSrcToAdcSrc(battery_reading_source bat_src)
{
    vm_adc_source_type adc_src = 0;
    
    switch(bat_src)
    {
        case AIO0:
            if(AIO0 == adcsel_aio0)
                adc_src = (vm_adc_source_type) bat_src;
            else
                adc_src = adcsel_aio0;
        break;
        case AIO1:
            if(AIO1 == adcsel_aio1)
                adc_src = (vm_adc_source_type) bat_src;
            else
                adc_src = adcsel_aio1;
        break;
        case VDD:
            if(VDD == adcsel_vref)
                adc_src = (vm_adc_source_type) bat_src;
            else
                adc_src = adcsel_vref;
        break;
        case AIO2:
            if(AIO2 == adcsel_aio2)
                adc_src = (vm_adc_source_type) bat_src;
            else
                adc_src = adcsel_aio2;
        break;
        case AIO3:
            if(AIO3 == adcsel_aio3)
                adc_src = (vm_adc_source_type) bat_src;
            else
                adc_src = adcsel_aio3;
        break;
        case BATTERY_INTERNAL:
            if(BATTERY_INTERNAL == adcsel_vdd_bat)
                adc_src = (vm_adc_source_type) bat_src;
            else
                adc_src = adcsel_vdd_bat;
        break;
    }
    
    return adc_src;
}

/*
  If any readings need to be adjusted (for example due to an
  external divider network) the adjustments should be made here.
*/
#define battGetReading(readingMessage) readingMessage->reading

/* Vdd = (Vref / reading)*255, vref is always 1250mV => Vdd = 318750/reading */
#define VREF_MULT_255 318750
static uint16 battCalcVdd(uint16 reading)
{
    return (uint32)(VREF_MULT_255/reading);
}

/*
  This function sends a retry message to the task passed with the source passed
  Used to reattempt readings from the Adc.
*/
static void battAttemptReading(Task task, vm_adc_source_type source)
{
    if(!AdcRequest(task, source))
    {
        vm_adc_source_type* reading_to_repeat = malloc(sizeof(vm_adc_source_type));
        *reading_to_repeat = source;
        MessageSend(task, BATTERY_INTERNAL_RETRY_MESSAGE, reading_to_repeat);
    }
}

/*
  This function is used to return mV readings to the task passed.
  Readings will not exceed uint16 range, but we don't do pMalloc blocks
  of 1 word and many existing apps rely on a uint32 return value.
*/
static void battSendReading(Task task, uint16 reading)
{
    uint32* mV = malloc(sizeof(uint32));
    *mV = reading;
    MessageSend(task, BATTERY_READING_MESSAGE, mV);
}

/*
  This function is used in calculating the average reading, 
  and will either update the running total or send the 
  resultant average to the application if enough readings
  have been gathered.
*/
static void battCalcAvg(BatteryState* batt_state, uint16 reading)
{
    /* They want an averaged reading, so store this one */
    batt_state->mv_running_total += reading;
    batt_state->readings_taken ++;
    
    /* If we have reached the number of readings we wish to average over */
    if(batt_state->readings_taken == AVERAGE_OVER)
    {
        /* Get averaged reading and send to client. Reset count and running total */
        uint16 avg = batt_state->mv_running_total / AVERAGE_OVER;
        uint16 result = (batt_state->source == VDD) ? battCalcVdd(avg) : avg;
        
        battSendReading(batt_state->client, result);
        
        batt_state->readings_taken = 0;
        batt_state->mv_running_total = 0;
    }
    
    /* Now send a timed message to self to read Vref again */
    MessageSendLater(&batt_state->task, BATTERY_INTERNAL_TIMED_MESSAGE, 0, batt_state->period);
}

/*
  This function is sent:
   MESSAGE_ADC_RESULT messages from the firmware.
   BATTERY_INTERNAL_TIMED_MESSAGE messages from itself.	
   BATTERY_INTERNAL_RETRY_MESSAGE messages when an attempted to read the adc failed.
*/
static void batteryHandler(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case BATTERY_INTERNAL_TIMED_MESSAGE:
        {
            /* We are resampling Vref to get Vdd */
            battAttemptReading(task, adcsel_vref);
        }
        break;
		
        case BATTERY_INTERNAL_RETRY_MESSAGE:
        {
            vm_adc_source_type* repeat_src = (vm_adc_source_type*)message;
            battAttemptReading(task, *repeat_src);
        }	
        break;

        case MESSAGE_ADC_RESULT:
        {
            BatteryState* batt_state = (BatteryState*)task;
            MessageAdcResult* result = (MessageAdcResult*)message;
            uint16 vdd;
            uint16 reading = battGetReading(result);
            vdd = battCalcVdd(reading);
            
            
            /* Was the clients original request a reading from Vdd */
            if(batt_state->source == VDD)
            {
                /* Yes it was, check that we have just received a reading of Vref from the adc which we can use to calculate Vdd */
                if(result->adc_source == adcsel_vref)
                {
                    /* Work out if we're averaging or doing single reading */
                    if(batt_state->period == 0)
                        battSendReading(batt_state->client, vdd);
                    else
                        battCalcAvg(batt_state, reading);
                }
            }
            else
            {
                /* Client requested something other than Vdd, what do we have */
                if(result->adc_source == adcsel_vref)
                {
                    batt_state->vdd_reading = vdd;
                    
                    /* Now get the clients original request from the Adc */
                    battAttemptReading(task, battSrcToAdcSrc(batt_state->source));			

                }
                else
                {
                    uint16 cur_reading = ((uint32)batt_state->vdd_reading * reading) / 255;
				
                    if(batt_state->period ==0)
                        battSendReading(batt_state->client, cur_reading);
                    else
                        battCalcAvg(batt_state, cur_reading);
                }
            }
        }
        break;
    }
}


/*
    This is called by the client. They will have created a BatteryState struct.
    This struct has it's first data element as a TaskData item. This is a bit
    like sub classing. As long as the first element in the state is a function pointer
    we can always deliver the message to the correct place.
*/
void BatteryInit(BatteryState* state, Task client, battery_reading_source source, uint16 period)
{
    /* Set handler for messages from Adc */
    state->task.handler = batteryHandler;

    /* Set task we send messages back to (readings in mV) */
    state->client = client;

    /* Set source to read from */
    state->source = source;

    /* Set how frequently we make readings - zero means do only one reading */
    state->period = period/AVERAGE_OVER;
	
    state->readings_taken = 0;
    state->mv_running_total = 0;

    /* as this function is used to stop and restart the battery voltage readings
       ensure there are no pending reading messages otherwise it is possible to
       have numerous concurrent timed readings in progress */
    if(!period)
        MessageCancelAll(&state->task,BATTERY_INTERNAL_TIMED_MESSAGE);
    
    /* Now make the first request - task to deliver msg to and src to read */
    battAttemptReading(&state->task, adcsel_vref);
}





