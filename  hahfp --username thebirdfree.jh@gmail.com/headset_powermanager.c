/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010

FILE NAME
    headset_powermanager.c
    
DESCRIPTION
    Module responsible for managing the battery monitoring and battery 
	charging functionaility
************************************************************************/

#include "headset_powermanager.h"
#include "headset_private.h"
#include "headset_statemanager.h"
#include "headset_led_manager.h"
#include "headset_audio.h"
#include "headset_pio.h"

#include <pio.h>

#ifdef DEBUG_POWER
    #define PM_DEBUG(x) {printf x;}             
#else
    #define PM_DEBUG(x) 
#endif

/* Local functions */
static void handleBatteryLow( void );
static void handleBatteryNormal(power_battery_level pLevel);
static void handleBatteryLevelInd(power_battery_level pLevel);

/****************************************************************************
NAME    
    setChargeEnablePio
    
DESCRIPTION
  	This function is called to set/reset the charge enable pio line
    
RETURNS
    void
*/
static void setChargeEnablePio(bool state)
{
    if( theHeadset.PIO->ChargerEnablePIO != 0xF )
	{
		PioSetDir32(1 << theHeadset.PIO->ChargerEnablePIO, 1 << theHeadset.PIO->ChargerEnablePIO);
		if(state)
			PioSet32(1 << theHeadset.PIO->ChargerEnablePIO, ~0);
		else
			PioSet32(1 << theHeadset.PIO->ChargerEnablePIO, 0);
	}
}

/****************************************************************************
NAME    
    powerManagerConfig
    
DESCRIPTION
  	Configure power management
    
RETURNS
    void
*/
bool powerManagerConfig(const power_type* config)
{
	bool success = TRUE;
		
	PM_DEBUG(("PM Config\n"));
	
	if(config)
	{
        theHeadset.low_battery_flag_ag = FALSE;
		/* Initialise the battery and its charging sub-system */
        PowerInit(&theHeadset.task, config);
	}
	else
	{
		success = FALSE;
	}
	
	return success;
}


/****************************************************************************
NAME    
    powerManagerChargerConnected
    
DESCRIPTION
  	This function is called when the charger is plugged into the headset
    
RETURNS
    void
*/
void powerManagerChargerConnected( void )
{
    PM_DEBUG(("PM: Charger has been connected\n"));
    setChargeEnablePio(TRUE);
	PowerCharger(TRUE);
    
    /* notify the audio plugin of thenew power state */
    AudioSetPower(POWER_BATT_LEVEL3);
    
}


/****************************************************************************
NAME    
    powerManagerChargerDisconnected
    
DESCRIPTION
  	This function is called when the charger is unplugged from the headset
    
RETURNS
    void
*/
void powerManagerChargerDisconnected( void )
{
    PM_DEBUG(("PM: Charger has been disconnected\n"));
    setChargeEnablePio(FALSE);
	PowerCharger(FALSE);
    
    /* Cancel current LED indication */
	MessageSend(&theHeadset.task, EventCancelLedIndication, 0);

}

/****************************************************************************
NAME    
    ChargerIsChargerConnected
    
DESCRIPTION
  	This function is called by applications to check whether the charger has been 
	plugged into the headset
    
RETURNS
    void
*/
bool ChargerIsChargerConnected(void)
{
    bool connected = FALSE;

    if(ChargerStatus() != NO_POWER)
        connected = TRUE;
    
    return connected;
}

/****************************************************************************
NAME    
    BatteryUserInitiatedRead
    
DESCRIPTION
  	Call this function to take an immediate battery reading and sent to AG.
    
RETURNS
    void
*/
void BatteryUserInitiatedRead( void )
{
    POWER_BATTERY_LEVEL_IND_T volt_level;
    
    PowerGetVoltage(&volt_level);

    /* based on the pLevel and send different messages */
    handleBatteryLevelInd(volt_level.pLevel);
}

/****************************************************************************
NAME    
    handleBatteryLow
    
DESCRIPTION
  	Called when the battery voltage is detected to be in Battery Low state
*/
static void handleBatteryLow( void )
{
    bool last_state = theHeadset.battery_low_state; 
    
    PM_DEBUG(("PM: Battery Low\n"));
   
    /*we only want low battery reminders if the headset is ON and not charging */
    if(stateManagerGetState() != headsetLimbo)
    {
        MessageSend(&theHeadset.task, EventLowBattery, 0); 
        theHeadset.battery_low_state = TRUE;    
    }
    
    if(ChargerIsChargerConnected() || stateManagerGetState() == headsetLimbo)
    {
      /*  printf("the battery is charging or the headset is in limbo state\n");*/
        theHeadset.battery_low_state = FALSE;
    }

    if(!last_state || !theHeadset.battery_low_state)
    {
#ifdef ROM_LEDS          /* update state indication to indicate low batt state change */
        LEDManagerIndicateState( stateManagerGetState() );
#endif     
     }
}


/****************************************************************************
NAME    
    batteryNormal
    
DESCRIPTION
    Called when the battery voltage is detected to be in a Normal state
*/
static void handleBatteryNormal(power_battery_level pLevel)
{
    uint16 level = (uint16)(pLevel - POWER_BATT_LEVEL0);

    PM_DEBUG(("PM: Battery Normal\n"));
	MessageSend(&theHeadset.task, EventOkBattery, 0);
	
    /* If charger connected send a charger gas gauge message (these don't have any functional use but can be associated with LEDs/tones) */
	if (ChargerIsChargerConnected())
	    MessageSend(&theHeadset.task, (EventChargerGasGauge0+level), 0);
    
    /* Send notification to AG if batt level has changed */
    MessageSend(&theHeadset.task, EventGasGauge0+level, 0);    
    
#ifdef ROM_LEDS
    /* when changing from low battery state to a normal state, refresh the led state pattern
       to replace the low battery pattern should it have been shown */
    if(theHeadset.battery_low_state == TRUE)
    {
        /* kick off a check to see if there is a low battery led state configured */
        LEDManagerIndicateState( stateManagerGetState() );
    }
#endif
    
    /* reset any low battery warning that may be in place */
    theHeadset.battery_low_state   = FALSE;  
    theHeadset.low_battery_flag_ag = FALSE;
}


/****************************************************************************
NAME    
    handleBatteryLevelInd
    
DESCRIPTION
    Called when the battery voltage is detected to be in a Normal state
*/
static void handleBatteryLevelInd(power_battery_level pLevel)
{
    /* based on the pLevel and send different messages */
    switch(pLevel)
    {
        case POWER_BATT_CRITICAL:
            PM_DEBUG(("PM: POWER_BATTERY_SHUTDOWN\n"));
            if ( (!ChargerIsChargerConnected()) && stateManagerGetState() != headsetLimbo )
	        {
    	        MessageSend(&theHeadset.task, EventLowBattery, 0); 
		        MessageSend(&theHeadset.task, EventPowerOff, 0);
	        }
            /* reset any low battery warning that may be in place */
            theHeadset.battery_low_state   = FALSE;  
            theHeadset.low_battery_flag_ag = FALSE;
        break;
        case POWER_BATT_LOW:
            handleBatteryLow();
        break;
        case POWER_BATT_LEVEL0:
	    case POWER_BATT_LEVEL1:
	    case POWER_BATT_LEVEL2:
	    case POWER_BATT_LEVEL3:
            handleBatteryNormal(pLevel);
        break;
        default :
            PM_DEBUG(("PM: Unhandled Battery Level[%x]\n", pLevel));
        break ;           
    }
    
    /* notify the audio plugin of the new power state, 
       if the charger is connected this is fixed at max level */
    AudioSetPower(LBIPMPowerLevel());
}

/*************************************************************************
NAME    
    handleBatteryMessage
    
DESCRIPTION
    handles the Battery/Charger Monitoring Messages

RETURNS
    
*/
void handleBatteryMessage( Task task, MessageId id, Message message )
{
    switch(id)
    {
        case POWER_INIT_CFM:
            PM_DEBUG(("PM: POWER_INIT_CFM\n"));
            if(!((POWER_INIT_CFM_T *)message)->status)
                Panic();
        break;
        case POWER_BATTERY_LEVEL_IND:
            PM_DEBUG(("PM: POWER_BATTERY_LEVEL_IND, level [%x], voltage [%d]\n", (uint16)(((POWER_BATTERY_LEVEL_IND_T *)message)->pLevel), (uint16)(((POWER_BATTERY_LEVEL_IND_T *)message)->voltage)));
            handleBatteryLevelInd((((POWER_BATTERY_LEVEL_IND_T *)message)->pLevel));
        break ;
        case POWER_CHARGER_STATE_IND:
            /* Send event if not using LED overide feature or state is currently not trickle charge */
            PM_DEBUG(("PM: POWER_CHARGER_STATE_IND, Old state: [%x], New state:[%x]\n", ((POWER_CHARGER_STATE_IND_T *)message)->old_state, 
                                                                                        ((POWER_CHARGER_STATE_IND_T *)message)->new_state ));

            if( !theHeadset.features.ChargerTerminationLEDOveride || 
                ((POWER_CHARGER_STATE_IND_T *)message)->old_state != STANDBY ) 
            {
                PM_DEBUG(("PM: POWER_CHARGER_STATE_IND, Send message for LED Indication\n"));
                /* Generate new message based on the reported charger state */
                switch(((POWER_CHARGER_STATE_IND_T *)message)->new_state)
                {
                    case TRICKLE_CHARGE:
                    case FAST_CHARGE:
                        MessageSend(&theHeadset.task, EventFastCharge, 0);
                    break;
                    case DISABLED_ERROR:
                        MessageSend(&theHeadset.task, EventBattTempOutOfRange, 0);
                    break;
                    case STANDBY:
                        MessageSend(&theHeadset.task, EventTrickleCharge, 0);
                    break;
                    case NO_POWER:
                    default:
                        PM_DEBUG(("PM: Unhandled Charger State[%x]\n", (uint16)(((POWER_CHARGER_STATE_IND_T *)message)->new_state)));
                    break;
                }
            }
        break ;
            /*all unhandled battery lib messages end up here*/          
        default :
            PM_DEBUG(("PM: Unhandled Battery msg[%x]\n", id));
        break ;
    }
}

/****************************************************************************
NAME    
    LBIPMPowerLevel
    
DESCRIPTION
  	Returns the Power level to use for Low Battery Intelligent Power Management (LBIPM)
    Note will always return high level if this feature is disabled.
    
RETURNS
    void
*/
power_battery_level LBIPMPowerLevel( void )
{
    POWER_BATTERY_LEVEL_IND_T batt_level;     
    
    if(theHeadset.lbipmEnable && !ChargerIsChargerConnected())
    {
        /* get current battery level */
        PowerGetVoltage(&batt_level);
    }            
    else
    {
        /* LBIPM disabled or charger is connected so use full power level */
        batt_level.pLevel = POWER_BATT_LEVEL3;
    }
    
    return batt_level.pLevel;
}
