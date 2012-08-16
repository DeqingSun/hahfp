/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010

FILE NAME
    headset_LEDmanager.c
    
DESCRIPTION
    Module responsible for managing the PIO outputs including LEDs
    
*/

#ifdef ROM_LEDS
/****************************************************************************
INCLUDES
*/
#include "headset_led_manager.h"
#include "headset_private.h"


#include "headset_configmanager.h"
#include "headset_statemanager.h"
#include "headset_leds.h"
#include "headset_leddata.h"
#include "headset_pio.h"
#include "headset_powermanager.h"

#include <stddef.h>
#include <pio.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
DEFINITIONS
*/


#ifdef DEBUG_LM
#define LM_DEBUG(x) {printf x;}
#else
#define LM_DEBUG(x) 
#endif


/****************************************************************************
LOCAL FUNCTION PROTOTYPES
*/


 /*methods to allocate/ initialise the space for the patterns and mappings*/
static void LEDManagerInitStatePatterns   ( void ) ;
static void LEDManagerInitEventPatterns   ( void ) ;
static void LEDManagerInitActiveLEDS      ( void ) ;
static void LEDManagerCreateFilterPatterns( void ) ;

/****************************************************************************
NAME	
	ledManagerMemoryInit

DESCRIPTION
	Initialise memory for led manager, this has moved from theHeadset as ran 
    out of globals space.

RETURNS
	void
    
*/
void LedManagerMemoryInit(void) 
{
	/* Allocate memory to hold the led manager states */
	theHeadset.theLEDTask = mallocPanic(sizeof(LedTaskData));
    theHeadset.theLEDTask->gActiveLEDS = (LEDActivity_t *)mallocPanic(sizeof(LEDActivity_t) * HEADSET_NUM_LEDS);
    memset(theHeadset.theLEDTask->gActiveLEDS, 0, (sizeof(LEDActivity_t) * HEADSET_NUM_LEDS));
}

/****************************************************************************
NAME 
 PIOManagerInit

DESCRIPTION
 Initialises LED manager

RETURNS
 void
    
*/
void LEDManagerInit ( void ) 
{
        
    LM_DEBUG(("LM Init :\n")) ;
   
    /* reset the number of allocated pattern positions */
    theHeadset.theLEDTask->gPatternsAllocated = 0;
                
    LM_DEBUG(("LM : p[%x][%x][%x]\n" ,  (int)theHeadset.theLEDTask->gStatePatterns ,
                                        (int)theHeadset.theLEDTask->gEventPatterns ,
                                        (int)theHeadset.theLEDTask->gActiveLEDS    
            )) ;
    
    /*create the patterns we want to use*/
    LEDManagerInitStatePatterns ( ) ;
    LEDManagerInitActiveLEDS( ) ;
    LEDManagerInitEventPatterns( ) ;
    
    theHeadset.theLEDTask->Queue.Event1 = 0 ;
    theHeadset.theLEDTask->Queue.Event2 = 0 ;
    theHeadset.theLEDTask->Queue.Event3 = 0 ;
    theHeadset.theLEDTask->Queue.Event4 = 0 ;
    
    /*the filter information*/
    LEDManagerCreateFilterPatterns( ) ;
    
    LedsInit ( ) ;

}
/****************************************************************************
NAME 
 LEDManagerInitActiveLEDS

DESCRIPTION
 Creates the active LED space for the number of leds the system supports

RETURNS
 void
    
*/
static void LEDManagerInitActiveLEDS ( void ) 
{
    uint16 lIndex = 0; 
 
    for ( lIndex= 0 ; lIndex < HEADSET_NUM_LEDS ; lIndex ++ )
    {
        LedsSetLedActivity ( &theHeadset.theLEDTask->gActiveLEDS [ lIndex ] , IT_Undefined , 0 , 0 ) ;    
    }
}
/****************************************************************************
NAME 
 LEDManagerInitStatePatterns

DESCRIPTION
 Creates the state patterns space for the system states

RETURNS
 void
    
*/
static void LEDManagerInitStatePatterns ( void ) 
{
    uint16 lIndex = 0; 
 
    for ( lIndex= 0 ; lIndex < HEADSET_NUM_STATES ; lIndex ++ )
    {
        LEDPattern_t *lStatePattern = &(theHeadset.theLEDTask->gStatePatterns[lIndex]);
        
        memset(lStatePattern, 0, sizeof(LEDPattern_t));
        lStatePattern->Colour     = LED_COL_LED_A ;  
    }
     
}
/****************************************************************************
NAME 
 LEDManagerInitEventPatterns

DESCRIPTION
 inits the Event pattern pointers

RETURNS
 void
    
*/
static void LEDManagerInitEventPatterns ( void )
{
    uint16 lIndex = 0; 
 
    for ( lIndex= 0 ; lIndex < LM_MAX_NUM_PATTERNS ; lIndex ++ )
    {
        LEDPattern_t *lEventPattern = &(theHeadset.theLEDTask->gEventPatterns[lIndex]);
        
        memset(lEventPattern, 0, sizeof(LEDPattern_t));
        lEventPattern->Colour     = LED_COL_LED_A ;  
    } 
}
/****************************************************************************
NAME 
 LEDManagerCreateFilterPatterns

DESCRIPTION
 Creates the Filter patterns space 

RETURNS
 void
    
*/
static void LEDManagerCreateFilterPatterns ( void )
{
    uint16 lIndex = 0 ;
    

    for (lIndex = 0 ; lIndex < LM_NUM_FILTER_EVENTS ; lIndex++ )
    {
        LEDFilter_t *lEventFilter = &(theHeadset.theLEDTask->gEventFilters [ lIndex ]);
        
        memset(lEventFilter, 0, sizeof(LEDFilter_t));
    }
    
    theHeadset.theLEDTask->gLMNumFiltersUsed = 0 ;

    theHeadset.theLEDTask->gTheActiveFilters = 0x0000 ;
}
 

#ifdef DEBUG_LM
/****************************************************************************
NAME 
 LMPrintPattern

DESCRIPTION
    debug fn to output a LED pattern
    
RETURNS
 void
*/

void LMPrintPattern ( LEDPattern_t * pLED ) 
{
#ifdef DEBUG_PRINT_ENABLED
    const char * const lColStrings [ 5 ] =   {"LED_E ","LED_A","LED_B","ALT","Both"} ;
    if(pLED)
    {
        LM_DEBUG(("[%d][%d] [%d][%d][%d] ", pLED->LED_A , pLED->LED_B, pLED->OnTime ,pLED->OffTime ,pLED->RepeatTime)) ;  
        LM_DEBUG(("[%d] [%d] [%s]\n",       pLED->NumFlashes, pLED->TimeOut, lColStrings[pLED->Colour])) ;    
        LM_DEBUG(("[%d]\n",       pLED->OverideDisable)) ;    
    }
    else
    {
        LM_DEBUG(("LMPrintPattern = NULL \n")) ;  
    }
#endif

}
#endif

/****************************************************************************
NAME 
 LEDManagerIndicateEvent

DESCRIPTION
 displays event notification
    This function also enables / disables the event filter actions - if a normal event indication is not
    associated with the event, it checks to see if a filer is set up for the event 

RETURNS
 void
    
*/

void LEDManagerIndicateEvent ( MessageId pEvent ) 
{
	uint8 i,lPatternIndex;
    uint16 lEventIndex = pEvent - EVENTS_MESSAGE_BASE ;
    LEDPattern_t * lPattern = NULL;
    
    lPatternIndex = NO_STATE_OR_EVENT;
    LM_DEBUG(("LM IndicateEvent [%x]\n", lEventIndex)) ;   
    
    /* search for a matching event */
    for(i=0;i<theHeadset.theLEDTask->gEventPatternsAllocated;i++)
    {
        if(theHeadset.theLEDTask->gEventPatterns[i].StateOrEvent == lEventIndex)
        {
            lPatternIndex = i;
            lPattern      = &theHeadset.theLEDTask->gEventPatterns[i];
            break;
        }
    }
        
    /*if there is an event configured*/
    if ( lPatternIndex != NO_STATE_OR_EVENT )
    {    
        /*only indicate if LEDs are enabled*/
        if ((theHeadset.theLEDTask->gLEDSEnabled ) ||
            (lPattern->OverideDisable) ||
            LedActiveFiltersCanOverideDisable( ))
        {
    
            LM_DEBUG(("LM : IE[%x]\n",pEvent )) ;
 
            /*only update if wer are not currently indicating an event*/
            if ( ! theHeadset.theLEDTask->gCurrentlyIndicatingEvent )
            {
                ledsIndicateLedsPattern(lPattern, lPatternIndex, IT_EventIndication);
            }    
            else
            {
                if (theHeadset.features.QueueLEDEvents )
                {
                    /*try and add it to the queue*/
                    LM_DEBUG(("LM: Queue LED Event [%x]\n" , pEvent )) ;
					
					if ( theHeadset.theLEDTask->Queue.Event1 == 0)
                    {
                        theHeadset.theLEDTask->Queue.Event1 = ( pEvent - EVENTS_MESSAGE_BASE) ;
                    }
                    else if ( theHeadset.theLEDTask->Queue.Event2 == 0)
                    {
                        theHeadset.theLEDTask->Queue.Event2 = ( pEvent - EVENTS_MESSAGE_BASE) ;
                    }
                    else if ( theHeadset.theLEDTask->Queue.Event3 == 0)
                    {
                        theHeadset.theLEDTask->Queue.Event3 = ( pEvent - EVENTS_MESSAGE_BASE) ;
                    }
                    else if ( theHeadset.theLEDTask->Queue.Event4 == 0)
                    {
                        theHeadset.theLEDTask->Queue.Event4 = ( pEvent - EVENTS_MESSAGE_BASE) ;
                    }    
                    else
                    {
                        LM_DEBUG(("LM: Err Queue Full!!\n")) ;
                    }
                }    
            }
        }
        else
        {
            LM_DEBUG(("LM : No IE disabled\n")) ;
        }  
    }
    else
    {
        LM_DEBUG(("LM: NoEvPatCfg %x\n",pEvent )) ;
    }
    
    /*indicate a filter if there is one present*/
    LedsCheckForFilter ( pEvent ) ;
	
}
/****************************************************************************
NAME	
	LEDManagerIndicateState

DESCRIPTION
	displays state indication information

RETURNS
	void
    
*/

void LEDManagerIndicateState ( headsetState pState )  
{   
    uint8 i,lPatternIndex;   
    LEDPattern_t * lPattern = NULL;
     
    lPatternIndex = NO_STATE_OR_EVENT;
   
#ifdef MONO_A2DP_STREAMING
	pState = stateManagerGetCombinedLEDState(pState);
#endif
	
    /* search for a matching state */
	for(i=0;i<theHeadset.theLEDTask->gStatePatternsAllocated;i++)
    {
        if(theHeadset.theLEDTask->gStatePatterns[i].StateOrEvent == pState)
        {
            /* force indicated state to that of Low Battery configured pattern */
            lPatternIndex = i;
            lPattern = &theHeadset.theLEDTask->gStatePatterns[i];
            break;
        }
    }       
    
    /* check for low battery warning and determine if a pattern exists for the low battery
       state, if no pattern exists then do nothing otherwise show low battery pattern without
       actually changing headset state */
    if((BatteryIsBatteryLow()) && (lPatternIndex != NO_STATE_OR_EVENT))
    {
        /* check for pattern match on low battery state */
        for(i=0;i<theHeadset.theLEDTask->gStatePatternsAllocated;i++)
        {
            /* if low batt pattern exists then change headset state, otherwise leave as is */
            if(theHeadset.theLEDTask->gStatePatterns[i].StateOrEvent == headsetLowBattery)
            {
                /* force indicated state to that of Low Battery configured pattern */
                pState   = headsetLowBattery;   
                lPattern = &theHeadset.theLEDTask->gStatePatterns[i];
                break;
            }
        }      
    }
    
    if(lPatternIndex != NO_STATE_OR_EVENT)
    {
        /*if there is a pattern associated with the state and not disabled, indicate it*/
        theHeadset.theLEDTask->gStateCanOverideDisable = lPattern->OverideDisable;
            
        /* only indicate if LEDs are enabled*/
        if ((theHeadset.theLEDTask->gLEDSEnabled ) ||
            (lPattern->OverideDisable)             ||
            LedActiveFiltersCanOverideDisable( ))
        {
            LM_DEBUG(("LM : IS[%x]\n", pState)) ;

            if (    ( theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_A].Type != IT_EventIndication  )
                 && ( theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_B].Type != IT_EventIndication  ) )
            {            
                /*Indicate the LED Pattern of Event/State*/
                ledsIndicateLedsPattern(lPattern, lPatternIndex, IT_StateIndication);
            }
        }
        else
        {           
            LM_DEBUG(("LM: NoStCfg[%x]\n",pState)) ;
            LedsIndicateNoState ( ) ; 
        }
    }
    else
    {
        LM_DEBUG(("LM : DIS NoStCfg[%x]\n", pState)) ;
        LedsIndicateNoState ( );
    }
	
}

/****************************************************************************
NAME	
	LedManagerDisableLEDS

DESCRIPTION
    Disable LED indications
RETURNS
	void
    
*/
void LedManagerDisableLEDS ( void )
{
    LM_DEBUG(("LM Disable LEDS\n")) ;

    /*turn off all current LED Indications if not overidden by state or filter */
    if (!theHeadset.theLEDTask->gStateCanOverideDisable && !LedActiveFiltersCanOverideDisable())
    {
        LedsIndicateNoState ( ) ;
    }    
    
    theHeadset.theLEDTask->gLEDSEnabled = FALSE ;
}

/****************************************************************************
NAME	
	LedManagerEnableLEDS

DESCRIPTION
    Enable LED indications
RETURNS
	void
    
*/
void LedManagerEnableLEDS ( void )
{
    LM_DEBUG(("LM Enable LEDS\n")) ;
    
    theHeadset.theLEDTask->gLEDSEnabled = TRUE ;
         
    LEDManagerIndicateState ( stateManagerGetState() ) ;    
}


/****************************************************************************
NAME	
	LedManagerToggleLEDS

DESCRIPTION
    Toggle Enable / Disable LED indications
RETURNS
	void
    
*/
void LedManagerToggleLEDS ( void ) 
{
    if ( theHeadset.theLEDTask->gLEDSEnabled )
    {
   		MessageSend (&theHeadset.task , EventDisableLEDS , 0) ;
    }
    else
    {
   		MessageSend (&theHeadset.task , EventEnableLEDS , 0) ;
    }
}

/****************************************************************************
NAME	
	LedManagerResetLEDIndications

DESCRIPTION
    Resets the LED Indications and reverts to state indications
	Sets the Flag to allow the Next Event to interrupt the current LED Indication
    Used if you have a permanent LED event indication that you now want to interrupt
RETURNS
	void
    
*/
void LedManagerResetLEDIndications ( void )
{    
    LedsResetAllLeds ( ) ;
    
    theHeadset.theLEDTask->gCurrentlyIndicatingEvent = FALSE ;
    
    LEDManagerIndicateState (stateManagerGetState() ) ;
}

/****************************************************************************
NAME	
	LEDManagerResetStateIndNumRepeatsComplete

DESCRIPTION
    Resets the LED Number of Repeats complete for the current state indication
       This allows the time of the led indication to be reset every time an event 
       occurs.
RETURNS
	void
    
*/
void LEDManagerResetStateIndNumRepeatsComplete  ( void ) 
{
    uint8 i,lPatternIndex;   
    LEDPattern_t * lPattern = NULL;
    
    /*get state*/
    headsetState lState = stateManagerGetState() ;
    
    /*get pattern*/   
    lPatternIndex = NO_STATE_OR_EVENT;
        
    /* search for a matching state */
    for(i=0;i<theHeadset.theLEDTask->gStatePatternsAllocated;i++)
    {
        if(theHeadset.theLEDTask->gStatePatterns[i].StateOrEvent == lState)
        {
            /* force indicated state to that of Low Battery configured pattern */
            lPattern = &theHeadset.theLEDTask->gStatePatterns[i] ;
            lPatternIndex = i;
            break;
        }
    }       
       
    /* does pattern exist for this state */
    if (lPattern)
    {
        LEDActivity_t * lLED   = &theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_A] ;
        if (lLED)
        {
            /*reset num repeats complete to 0*/
            lLED->NumRepeatsComplete = 0 ;
        }    
    }
}

/****************************************************************************
NAME	
	LEDManagerCheckTimeoutState

DESCRIPTION
    checks the led timeout state and resets it if required, this function is called from
    an event or volume button press to re-enable led indications as and when required
    to do so 
RETURNS
	void
    
*/
void LEDManagerCheckTimeoutState( void )
{
    /*handles the LED event timeouts - restarts state indications if we have had a user generated event only*/
    if (theHeadset.theLEDTask->gLEDSStateTimeout)
    {   
        /* send message that can be used to show an led pattern when led's are re-enabled following a timeout */
        MessageSend( &theHeadset.task, EventResetLEDTimeout, 0);
    }
    else
    {
        /*reset the current number of repeats complete - i.e restart the timer so that the leds will disable after
          the correct time*/
        LEDManagerResetStateIndNumRepeatsComplete  ( ) ;
    }
}

#else

static const int temp ;

#endif
