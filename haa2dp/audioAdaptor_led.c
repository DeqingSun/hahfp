/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2006-2007
Part of Audio-Adaptor-SDK Q3-2007.Release

DESCRIPTION
    
*/

#include "audioAdaptor_charger.h"
#include "audioAdaptor_led.h"
#include "audioAdaptor_states.h"
#include "leds.h"


/*! 
    @brief a led state pattern type
*/
typedef struct
{
	unsigned normal:8;  
	unsigned low_battery:8 ;
	unsigned charging:8 ;
	unsigned reserved:8 ;
} ledStatePattern_t ;

static const ledStatePattern_t ledStates [ MaxAppStates ] = 
{
/*AppStateUninitialised*/ 	{LEDS_OFF,              LEDS_OFF,	           	LEDS_CHARGING_RPT,          0 } ,
/*AppStateInitialising*/  	{LEDS_OFF,              LEDS_OFF,               LEDS_CHARGING_RPT,          0 } ,
/*AppStateIdle*/         	{LEDS_WAITING_RPT,    	LEDS_LOW_BATT_RPT,     	LEDS_CHARGING_RPT,			0 } ,
/*AppStateInquiring*/    	{RED_BLUE_ALT_RPT_FAST, RED_BLUE_ALT_RPT_FAST,	RED_BLUE_ALT_RPT_FAST,    	0 } ,
/*AppStateSearching*/    	{RED_BLUE_ALT_RPT_FAST, RED_BLUE_ALT_RPT_FAST,	RED_BLUE_ALT_RPT_FAST,    	0 } ,
/*AppStateConnecting*/   	{RED_BLUE_ALT_RPT_FAST, RED_BLUE_ALT_RPT_FAST,	RED_BLUE_ALT_RPT_FAST,      0 } ,
/*AppStateStreaming*/    	{LEDS_TRANSMITTING_RPT, LEDS_LOW_BATT_RPT,  	LEDS_CHARGING_RPT,      	0 } ,
/*AppStateEnteringDfu*/   {RED_BLUE_BOTH_RPT_FAST,RED_BLUE_BOTH_RPT_FAST, RED_BLUE_BOTH_RPT_FAST,     0 } ,
/*AppStateLowBattery*/   	{LEDS_LOW_BATT_RPT,     LEDS_LOW_BATT_RPT,      LEDS_CHARGING_RPT,          0 } ,
/*AppStatePoweredOff*/   	{LEDS_OFF,              LEDS_OFF,               LEDS_CHARGING_RPT,          0 } 
} ;


void ledPlayPattern(mvdAppState state)
{
    if (state == AppStatePoweredOff)
    {
        if (chargerIsConnected())
        {
            ledsPlay(ledStates[ state ].charging);
        }
        else 
        {
            ledsPlay(ledStates[ state ].normal);
        }
    }
    else if (state == AppStateLowBattery)
    {
        if (the_app->audioAdaptorPoweredOn)
        {
            ledsPlay(ledStates[ the_app->app_state ].low_battery);
        }
    }
    else
    {
        if (the_app->audioAdaptorPoweredOn)
        {
            if (chargerIsConnected())
            {
                ledsPlay(ledStates[ the_app->app_state ].charging);
            }
            else 
            {
                ledsPlay(ledStates[ the_app->app_state ].normal);
            }
        }
    }
}
