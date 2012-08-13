/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010

FILE NAME
    power_charger_monitoring.c

DESCRIPTION
    This file contains the monitoring of battery charging functionality.
    
    Once the battery charger is connected, charger is monitored and the
    charging state is repeatly reported to App. 
    
    If the battery temperature is out of the configured range, the charging 
    or boost charging should be disabled.
	
NOTES
	
**************************************************************************/


/****************************************************************************
    Header files
*/
#include "power.h"
#include "power_private.h"

#include <charger.h>

/* Private messages for controlling battery charging process */
#define CHARGER_MESSAGE_BASE	(0)
enum
{
	CHARGER_UPDATE = CHARGER_MESSAGE_BASE,
	CHARGER_MONITOR
};

/* Static function declarations */
static bool chargerBatteryOverMaxTemp( void );
static bool chargerBatteryUnderMinTemp( void );

static void chargerUpdateState(charger_status state);
static void charger_Handler(Task task, MessageId id, Message message);

const TaskData charger_monitoring_plugin = { charger_Handler };


/****************************************************************************
NAME    
    chargerBatteryOverMaxTemp
    
DESCRIPTION
  	This function is called by the charge handler. it uses the battery 
    temperature read by thermister and checks higher values, if within theses 
    limits false is returned however is out of range then true is returned;
    
RETURNS
    false = within limits, true = out of limits, must disable charger
    
****************************************************************************/
static bool chargerBatteryOverMaxTemp( void )
{
    int16 upper_limit = 0;
    int16 tb = power->temperature;
 
    /* Decide what our upper limit is based on boost charge */    
    if((power->battery_config).boost_charge_enable)
    {
        upper_limit = (power->battery_config).max_boost_temp;
    }
    else
    {
        upper_limit = (power->battery_config).max_temp;
    }

    /* check the battery temperature by use of the external thermister and compare it against upper and lower limits */
    if((tb > upper_limit))
    {
        /* temperature is outside of limits, return TRUE to disable charger */
        return TRUE;
    }
    
    /* default case, charger is within limits so don't disable */
    return FALSE;
}


/****************************************************************************
NAME    
    chargerBatteryUnderMinTemp
    
DESCRIPTION
  	This function is called by the charge handler. it uses the battery 
    temperature read by thermister and checks lower values, if within theses 
    limits false is returned however is out of range then true is returned;
    
RETURNS
    false = within limits, true = out of limits, must disable charger
    
****************************************************************************/
static bool chargerBatteryUnderMinTemp( void )
{
    int16 tb = power->temperature;

    if(tb < (power->battery_config).min_temp)
    {
        /* temperature is outside of limits, return TRUE to disable charger */
        return TRUE;
    }
    
    /* default case, charger is within limits so don't disable */
    return FALSE;
}


/****************************************************************************
NAME    
    chargerUpdateState
    
DESCRIPTION
  	change the charging state indications if state is different to the last 
    indicated state, and if changed, charging state is sent to App.
    
RETURNS
    void
*/
static void chargerUpdateState( charger_status state )
{
    /* only update is current charging state is different to that last indicated */
    if(power->charger_state != state) 
    {
        MAKE_BATT_MESSAGE(POWER_CHARGER_STATE_IND);
        message->old_state = power->charger_state;
        message->new_state = state;       
        MessageSend(power->clientTask, POWER_CHARGER_STATE_IND, message);
        
        /* If we're leaving boost charge state be sure to turn it off (done already if charge error) */
        if( ((power->battery_config).boost_charge_enable) && 
             (power->boost_charge_started)         &&
             (!power->boost_charge_disabled)       &&
             (power->charger_state == FAST_CHARGE) && 
             (state != DISABLED_ERROR)     )
        { 
            ChargerConfigure(CHARGER_ENABLE_BOOST, FALSE);
            power->boost_charge_disabled = TRUE;
            
            /* Reset the charger */
            ChargerConfigure(CHARGER_ENABLE, FALSE);
            ChargerConfigure(CHARGER_ENABLE, TRUE);
        }

        power->charger_state = state;
    }
}

/****************************************************************************
NAME    
    chargerHandler
    
DESCRIPTION
  	Messages for the charger control task arrive here
    
RETURNS
    void
*/
static void charger_Handler(Task task, MessageId id, Message message)
{
	/* Get the charger state */
	charger_status lcharger_status = ChargerStatus();
    
	/* Process message */
	if(id == CHARGER_MONITOR)
    {
        chargerUpdateState(lcharger_status);

        /* need to check the battery temperature by use of external thermister, 
           if the temperature is above or below max and min limits repsectively then the charger must
           be disabled, when the temperature returns to within these limits it must be re-enabled */
        if(chargerBatteryOverMaxTemp())
        {
            /* Turn off boost charge if boost charger has enabled and in progress */
            if((power->battery_config).boost_charge_enable && (power->boost_charge_started) )
            {
                ChargerConfigure(CHARGER_ENABLE_BOOST, FALSE);
                power->boost_charge_disabled = TRUE;
            }
            
            /* temperature is outside of limits, disable the charger */
            ChargerConfigure(CHARGER_ENABLE, FALSE);
        }
        else if(chargerBatteryUnderMinTemp())
        {
            /* temperature is outside of limits, disable the charger */
            ChargerConfigure(CHARGER_ENABLE, FALSE);
        }
        /* temperature is within limits, ensure charger is enabled */
        else if(lcharger_status == DISABLED_ERROR) 
        {
            /* temperature is back within limits and is disabled, re-enable the charger */
            ChargerConfigure(CHARGER_ENABLE, TRUE);
        }
        else if((lcharger_status == FAST_CHARGE) && !(power->boost_charge_started)) 
        {
            /* If charger just connected and within temp limits, kick off boost charge */
            ChargerConfigure(CHARGER_ENABLE_BOOST, TRUE);
            
            /*Boost Charging has been started and don't start again */
            power->boost_charge_started = TRUE;
            power->boost_charge_disabled = FALSE;
        }
        
        /* Monitoring period of 1s whilst charger is plugged in */				
		MessageSendLater(task, CHARGER_MONITOR, 0, D_SEC(1));
    }
}


/****************************************************************************
NAME    
    PowerCharger
    
DESCRIPTION
  	This function is called by the application when the charger has been 
	plugged or disconnectd with the headset
    
RETURNS
    void
*/
void PowerCharger( bool enable )
{
    if(enable)       /* Charger is being connected */
    {
        power->boost_charge_started = FALSE;
        
        /* Start monitoring onchip LiON battery charger status */
        MessageSend((TaskData*) &charger_monitoring_plugin, CHARGER_MONITOR, 0);
    }
    else
    {
        /* Cancel any pending LiON battery charger status monitoring messages */
		MessageCancelAll((TaskData*) &charger_monitoring_plugin, CHARGER_MONITOR);
 
        power->charger_state =  NO_POWER;
            
        if((power->battery_config).boost_charge_enable && (power->boost_charge_started))      
        {
            /* Disable boost charge */
            ChargerConfigure(CHARGER_ENABLE_BOOST, FALSE);
            
            power->boost_charge_started = FALSE;
	    }
    }
}


