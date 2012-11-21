/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010

FILE NAME
    main.c        

DESCRIPTION
    This is main file for the headset application software for BC4-Headset

NOTES

*/

/****************************************************************************
    Header files
*/
#include "headset_private.h"
#include "headset_init.h"
#include "headset_auth.h"
#include "headset_scan.h"
#include "headset_slc.h" 
#include "headset_dut.h" 
#include "headset_pio.h" 
#include "headset_multipoint.h" 
#include "headset_led_manager.h"
#include "headset_buttonmanager.h"
#include "headset_configmanager.h"
#include "headset_events.h"
#include "headset_statemanager.h"
#include "headset_states.h"
#include "headset_powermanager.h"
#include "headset_callmanager.h"
#include "headset_energy_filter.h"
#include "headset_csr_features.h"

#ifdef ENABLE_PBAP
#include "headset_pbap.h"
#endif

#ifdef ENABLE_AVRCP
#include "headset_avrcp.h"
#endif

#include "headset_volume.h"
#include "headset_tones.h"
#include "headset_tts.h" 
#include "headset_energy_filter.h"

#include "headset_audio.h"
#include "vm.h"

#ifdef TEST_HARNESS
#include "test_headset.h"
#include "vm2host_connection.h"
#include "vm2host_hfp.h"
#endif

#include <bdaddr.h>
#include <connection.h>
#include <panic.h>
#include <ps.h>
#include <pio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stream.h>
#include <codec.h>
#include <boot.h>
#include <string.h>
#include <audio.h>
#include <sink.h>
#include <kalimba_standard_messages.h>
#include <audio_plugin_if.h>

#include <spp_common.h>
#include <sppc.h>
#include <spps.h>

#ifdef CVC_PRODTEST
#include <kalimba.h>
#include <file.h>
#include <string.h>
#include "headset_config.h"
#endif

#ifdef DEBUG_MAIN
#define MAIN_DEBUG(x) DEBUG(x)
    #define TRUE_OR_FALSE(x)  ((x) ? 'T':'F')   
#else
    #define MAIN_DEBUG(x) 
#endif

/* Single instance of the Headset state */
hsTaskData theHeadset;

static void handleHFPStatusCFM ( hfp_lib_status pStatus ) ;

#ifdef CVC_PRODTEST
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;
#endif

static void headsetInitCodecTask( void ) ;

/*************************************************************************
NAME    
    handleCLMessage
    
DESCRIPTION
    Function to handle the CL Lib messages - these are independent of state

RETURNS

*/
static void handleCLMessage ( Task task, MessageId id, Message message )
{
    MAIN_DEBUG(("CL = [%x]\n", id)) ;    
 
        /* Handle incoming message identified by its message ID */
    switch(id)
    {        
        case CL_INIT_CFM:
            MAIN_DEBUG(("CL_INIT_CFM [%d]\n" , ((CL_INIT_CFM_T*)message)->status ));
            if(((CL_INIT_CFM_T*)message)->status == success)
            {				
				/* Initialise the codec task */
				headsetInitCodecTask();
			}
            else
            {
                Panic();
            }
		break;
		case CL_DM_WRITE_INQUIRY_MODE_CFM:
			/* Read the local name to put in our EIR data */
			ConnectionReadInquiryTx(&theHeadset.task);
		break;
		case CL_DM_READ_INQUIRY_TX_CFM:
			theHeadset.inquiry_tx = ((CL_DM_READ_INQUIRY_TX_CFM_T*)message)->tx_power;
			ConnectionReadLocalName(&theHeadset.task);
		break;
		case CL_DM_LOCAL_NAME_COMPLETE:
			MAIN_DEBUG(("CL_DM_LOCAL_NAME_COMPLETE\n"));
			/* Write EIR data and initialise the codec task */
			headsetWriteEirData((CL_DM_LOCAL_NAME_COMPLETE_T*)message);
		break;
		case CL_SM_SEC_MODE_CONFIG_CFM:
			MAIN_DEBUG(("CL_SM_SEC_MODE_CONFIG_CFM\n"));
			/* Remember if debug keys are on or off */
			theHeadset.debug_keys_enabled = ((CL_SM_SEC_MODE_CONFIG_CFM_T*)message)->debug_keys;
		break;
  		case CL_SM_PIN_CODE_IND:
            MAIN_DEBUG(("CL_SM_PIN_IND\n"));
            headsetHandlePinCodeInd((CL_SM_PIN_CODE_IND_T*) message);
        break;
		case CL_SM_USER_CONFIRMATION_REQ_IND:
			MAIN_DEBUG(("CL_SM_USER_CONFIRMATION_REQ_IND\n"));
			headsetHandleUserConfirmationInd((CL_SM_USER_CONFIRMATION_REQ_IND_T*) message);
		break;
		case CL_SM_USER_PASSKEY_REQ_IND:
			MAIN_DEBUG(("CL_SM_USER_PASSKEY_REQ_IND\n"));
			headsetHandleUserPasskeyInd((CL_SM_USER_PASSKEY_REQ_IND_T*) message);
		break;
		case CL_SM_USER_PASSKEY_NOTIFICATION_IND:
			MAIN_DEBUG(("CL_SM_USER_PASSKEY_NOTIFICATION_IND\n"));
			headsetHandleUserPasskeyNotificationInd((CL_SM_USER_PASSKEY_NOTIFICATION_IND_T*) message);
		break;
		case CL_SM_KEYPRESS_NOTIFICATION_IND:
		break;
		case CL_SM_REMOTE_IO_CAPABILITY_IND:
			MAIN_DEBUG(("CL_SM_IO_CAPABILITY_IND\n"));
			headsetHandleRemoteIoCapabilityInd((CL_SM_REMOTE_IO_CAPABILITY_IND_T*)message);
		break;
		case CL_SM_IO_CAPABILITY_REQ_IND:
			MAIN_DEBUG(("CL_SM_IO_CAPABILITY_REQ_IND\n"));
			headsetHandleIoCapabilityInd((CL_SM_IO_CAPABILITY_REQ_IND_T*) message);
		break;
        case CL_SM_AUTHORISE_IND:
            MAIN_DEBUG(("CL_SM_AUTHORISE_IND\n"));
            headsetHandleAuthoriseInd((CL_SM_AUTHORISE_IND_T*) message);
        break;            
        case CL_SM_AUTHENTICATE_CFM:
            MAIN_DEBUG(("CL_SM_AUTHENTICATE_CFM\n"));
            headsetHandleAuthenticateCfm((CL_SM_AUTHENTICATE_CFM_T*) message);
        break;           
        case CL_DM_REMOTE_FEATURES_CFM:
            MAIN_DEBUG(("HS : Supported Features\n")) ;
            headsetHandleRemoteSuppFeatures((CL_DM_REMOTE_FEATURES_CFM_T *)(message));
        break ;
        case CL_DM_INQUIRE_RESULT:
            MAIN_DEBUG(("HS : Inquiry Result\n"));
            slcHandleInquiryResult((CL_DM_INQUIRE_RESULT_T*)message);
        break;
        case CL_SM_GET_ATTRIBUTE_CFM:
			MAIN_DEBUG(("HS : CL_SM_GET_ATTRIBUTE_CFM Vol:%d \n",((CL_SM_GET_ATTRIBUTE_CFM_T *)(message))->psdata[0]));
        break;
        case CL_SM_GET_INDEXED_ATTRIBUTE_CFM:
            MAIN_DEBUG(("HS: CL_SM_GET_INDEXED_ATTRIBUTE_CFM[%d]\n" , ((CL_SM_GET_INDEXED_ATTRIBUTE_CFM_T*)message)->status)) ;
        break ;    
		
		case CL_DM_LOCAL_BD_ADDR_CFM:
            DutHandleLocalAddr((CL_DM_LOCAL_BD_ADDR_CFM_T *)message);
		break ;
          
        case CL_DM_ROLE_CFM:
            slcHandleRoleConfirm((CL_DM_ROLE_CFM_T *)message);
            return;
        
            /*all unhandled connection lib messages end up here*/          
        default :
            MAIN_DEBUG(("Headset - Unhandled CL msg[%x]\n", id));
        break ;
    }
   
}

/*************************************************************************
NAME    
    handleUEMessage
    
DESCRIPTION
    handles messages from the User Events

RETURNS
    
*/
static void handleUEMessage  ( Task task, MessageId id, Message message )
{
    /* Event state control is done by the config - we will be in the right state for the message
    therefore messages need only be passed to the relative handlers unless configurable */
    headsetState lState = stateManagerGetState() ;
	
    /*if we do not want the event received to be indicated then set this to FALSE*/
    bool lIndicateEvent = TRUE ;
    	
    /* Deal with user generated Event specific actions*/
    switch ( id )
    {   
            /*these are the events that are not user generated and can occur at any time*/
        case EventOkBattery:
        case EventChargerDisconnected:
		case EventPairingFail:
        case EventLEDEventComplete:
        case EventTrickleCharge:
		case EventFastCharge:	
        case EventLowBattery:
        case EventPowerOff:
        case EventLinkLoss:
		case EventLimboTimeout:
        case EventSLCConnected:
        case EventSLCConnectedAfterPowerOn:
        case EventError:
        case EventChargeError:
        case EventChargeErrorInIdleState:
        case EventCancelLedIndication:
        case EventAutoSwitchOff:
		case EventReconnectFailed:
        case EventCheckForLowBatt:
        case EventChargerGasGauge0:
        case EventChargerGasGauge1:
        case EventChargerGasGauge2:
        case EventChargerGasGauge3:		
		case EventRssiPairReminder:
		case EventRssiPairTimeout:
        case EventConnectableTimeout:
        case EventRefreshEncryption:
        case EventEndOfCall:
        case EventSCOLinkClose:
        case EventMissedCall:
        break ;
        default:
                    /* If we have had an event then reset the timer - if it was the event then we will switch off anyway*/
            if (theHeadset.conf->timeouts.AutoSwitchOffTime_s !=0)
            {
                /*MAIN_DEBUG(("HS: AUTOSent Ev[%x] Time[%d]\n",id , theHeadset.conf->timeouts.AutoSwitchOffTime_s ));*/
                MessageCancelAll( task , EventAutoSwitchOff ) ;
                MessageSendLater( task , EventAutoSwitchOff , 0 , D_SEC(theHeadset.conf->timeouts.AutoSwitchOffTime_s) ) ;                
            }
 
            /*cancel any missed call indicator on a user event (button press)*/
           MessageCancelAll(task , EventMissedCall ) ;
 
            /* check for led timeout/re-enable */
#ifdef ROM_LEDS
			LEDManagerCheckTimeoutState();
#endif
   
        break;
    }
     
/*    MAIN_DEBUG (( "HS : UE[%x]\n", id )); */
	 			
    /* The configurable Events*/
    switch ( id )
    {   
		case (EventToggleDebugKeys):
			MAIN_DEBUG(("HS: Toggle Debug Keys\n"));
			/* If the headset has debug keys enabled then toggle on/off */
			ConnectionSmSecModeConfig(&theHeadset.task, cl_sm_wae_acl_none, !theHeadset.debug_keys_enabled, TRUE);
		break;
		case (EventPowerOn):
            MAIN_DEBUG(("HS: Power On\n" )) ;
			
            /*we have received the power on event- we have fully powered on*/
            stateManagerPowerOn();   
            
            /* set flag to indicate just powered up and may use different led pattern for connectable state
               if configured, flag is cleared on first successful connection */
			theHeadset.powerup_no_connection = TRUE;
            
            if(theHeadset.conf->timeouts.EncryptionRefreshTimeout_m != 0)
				MessageSendLater(&theHeadset.task, EventRefreshEncryption, 0, D_MIN(theHeadset.conf->timeouts.EncryptionRefreshTimeout_m));
			
            if ( theHeadset.features.DisablePowerOffAfterPowerOn )
            {
                theHeadset.PowerOffIsEnabled = FALSE ;
                DEBUG(("DIS[%x]\n" , theHeadset.conf->timeouts.DisablePowerOffAfterPowerOnTime_s  )) ;
                MessageSendLater ( &theHeadset.task , EventEnablePowerOff , 0 , D_SEC ( theHeadset.conf->timeouts.DisablePowerOffAfterPowerOnTime_s ) ) ;
            }
            else
            {
                theHeadset.PowerOffIsEnabled = TRUE ;
            }
			
#ifdef DEBUG_MALLOC
			printf("MAIN: Available SLOTS:[%d]\n" , VmGetAvailableAllocations() ) ;
#endif			
        break ;          
        case (EventPowerOff):    
            MAIN_DEBUG(("HS: PowerOff - En[%c]\n" , ((theHeadset.PowerOffIsEnabled) ? 'T':'F') )) ;
            
            /* don't indicate event if already in limbo state */
            if(lState == headsetLimbo) lIndicateEvent = FALSE ;
                
            if ( theHeadset.PowerOffIsEnabled )
            {
                stateManagerEnterLimboState();

				AuthResetConfirmationFlags();
				
                if (theHeadset.gMuted)
                {
                    VolumeMuteOff();
                }
                headsetClearQueueudEvent();                
    
				if(theHeadset.conf->timeouts.EncryptionRefreshTimeout_m != 0)
    				MessageCancelAll ( &theHeadset.task, EventRefreshEncryption) ;
				
                MessageCancelAll ( &theHeadset.task , EventPairingFail) ;
                MessageCancelAll ( &theHeadset.task, EventCheckForLowBatt) ;
           
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
            
		
        break ;
        case (EventInitateVoiceDial):
            MAIN_DEBUG(("HS: InitVoiceDial [%c] [%d]\n", (theHeadset.conf->supp_features_local & HFP_VOICE_RECOGNITION ) ? 'T':'F' , theHeadset.VoiceRecognitionIsActive )) ;            
                /*Toggle the voice dial behaviour depending on whether we are currently active*/
			if ( theHeadset.PowerOffIsEnabled )
            {
                
                if (theHeadset.VoiceRecognitionIsActive)
                {
				    headsetCancelVoiceDial(hfp_primary_link) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {         
				    headsetInitiateVoiceDial (hfp_primary_link) ;
                }            
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventInitateVoiceDial_AG2):
            MAIN_DEBUG(("HS: InitVoiceDial AG2[%c] [%d]\n", (theHeadset.conf->supp_features_local & HFP_VOICE_RECOGNITION ) ? 'T':'F' , theHeadset.VoiceRecognitionIsActive )) ;            
                /*Toggle the voice dial behaviour depending on whether we are currently active*/
			if ( theHeadset.PowerOffIsEnabled )
            {
                
                if (theHeadset.VoiceRecognitionIsActive)
                {
                    headsetCancelVoiceDial(hfp_secondary_link) ;					
                    lIndicateEvent = FALSE ;
                }
                else
                {                
                    headsetInitiateVoiceDial(hfp_secondary_link) ;
                }            
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventLastNumberRedial):
            MAIN_DEBUG(("HS: LNR\n" )) ;
            
            if ( theHeadset.PowerOffIsEnabled )
            {
                if (theHeadset.features.LNRCancelsVoiceDialIfActive)
                {
                    if ( theHeadset.VoiceRecognitionIsActive )
                    {
                        MessageSend(&theHeadset.task , EventInitateVoiceDial , 0 ) ;
                        lIndicateEvent = FALSE ;
                    }
                    else
                    {
                        /* LNR on AG 1 */
						headsetInitiateLNR(hfp_primary_link) ;
                    }
                }
                else
                {
                   /* LNR on AG 1 */
					headsetInitiateLNR(hfp_primary_link) ;
                }
			}
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;   
        case (EventLastNumberRedial_AG2):
            MAIN_DEBUG(("HS: LNR AG2\n" )) ;
			if ( theHeadset.PowerOffIsEnabled )
            {
                if (theHeadset.features.LNRCancelsVoiceDialIfActive)
                {
                    if ( theHeadset.VoiceRecognitionIsActive )
                    {
                        MessageSend(&theHeadset.task , EventInitateVoiceDial , 0 ) ;
                        lIndicateEvent = FALSE ;
                    }
                    else
                    {
                        /* LNR on AG 2 */
                        headsetInitiateLNR(hfp_secondary_link) ;
                    }
                }
                else
                {
                   /* LNR on AG 2 */
            	   headsetInitiateLNR(hfp_secondary_link) ;
                }
			}
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;  
        case (EventAnswer):
            MAIN_DEBUG(("HS: Answer\n" )) ;
            /* Call the HFP lib function, this will determine the AT cmd to send
               depending on whether the profile instance is HSP or HFP compliant. */ 
            
            /* ensure a profile is connected, if not reschedule another answer attempt */
            if(!theHeadset.conf->no_of_profiles_connected)
            {
                /* not yet finished connecting, reschedule another go in 100mS */
                MessageSendLater (&theHeadset.task , EventAnswer , 0, 100 ) ;   
                /* don't indicate this event */
                lIndicateEvent = FALSE ;
            }
            /* connected so ok to answer */
            else
            {
                /* don't indicate event if not in incoming call state as answer event is used
                   for some of the multipoint three way calling operations which generate unwanted
                   tones */
                if(stateManagerGetState() != headsetIncomingCallEstablish) lIndicateEvent = FALSE ;
                /* answer call */              
                headsetAnswerOrRejectCall( TRUE );
            }
			
        break ;   
        case (EventReject):
			MAIN_DEBUG(("HS: Reject\n" )) ;
            /* Reject incoming call - only valid for instances of HFP */ 
			
            headsetAnswerOrRejectCall( FALSE );
			break ;

        case (EventCancelEnd):
			MAIN_DEBUG(("HS: CancelEnd\n" )) ;
            /* Terminate the current ongoing call process */
            headsetHangUpCall();

        break ;
        case (EventTransferToggle):
            MAIN_DEBUG(("HS: Transfer\n" )) ;
            /* Don't indicate this event if it's an HF to AG transfer (B-48360). */
            lIndicateEvent = headsetTransferToggle(id);
        break ;
        case EventCheckForAudioTransfer :
	        MAIN_DEBUG(("HS: Check Aud Tx\n")) ;    
            headsetCheckForAudioTransfer();
	        break ;
        case (EventToggleMute):
            MAIN_DEBUG(("EventToggleMute\n")) ;
            VolumeToggleMute();
        break ;
#ifdef ENABLE_ENERGY_FILTER
        case (EventEnableIIR):
            MAIN_DEBUG(("EventEnableIIR\n")) ;
            if(!theHeadset.iir_enabled)
                Filter_On();
            theHeadset.iir_enabled = 1;
        break ;   
        case (EventDisableIIR):
            MAIN_DEBUG(("EventDisableIIR\n")) ;
            if(theHeadset.iir_enabled)
                Filter_Off();
            theHeadset.iir_enabled = 0;
        break ;
#endif
        case EventMuteOn :
            MAIN_DEBUG(("EventMuteOn\n")) ;
            VolumeMuteOn();
        break ;
        case EventMuteOff:
            MAIN_DEBUG(("EventMuteOff\n")) ;
            VolumeMuteOff();                      
        break ;
        case (EventVolumeUp):      
            MAIN_DEBUG(("EventVolumeUp\n")) ;
		
    		if ( theHeadset.gVolButtonsInverted )
    		{		/*do not indicate the original event*/
	 			lIndicateEvent = FALSE ;	 			 
	 					/*indicate the opposite event*/
        	   	LEDManagerIndicateEvent ( EventVolumeDown ) ;
                TonesPlayEvent ( EventVolumeDown ) ;
						/*change the volume in the opposite dir*/
		       	VolumeDown();
    		}
    		else
    		{
           		VolumeUp();
        	}	
			
        break ;
        case (EventVolumeDown):    	
            MAIN_DEBUG(("EventVolumeDown\n")) ;
			
	       	if ( theHeadset.gVolButtonsInverted )
    		{		/*do not indicate the original event*/
	 			lIndicateEvent = FALSE ;	 			 
	 				/*indicate the opposite event*/
        	 	LEDManagerIndicateEvent ( EventVolumeUp ) ;
                TonesPlayEvent ( EventVolumeUp ) ;
					/*change the volume in the opposite dir*/
		    	VolumeUp();
    		}
    		else
    		{
            	VolumeDown();
        	}	
						
        break ;                        
        case (EventEnterPairing):
#if 0			
			if(theHeadset.ha_mode_only_enable)
			{
                lIndicateEvent = FALSE ;
				break;
			}
#else			
			/* auto disable ha_mode */
			theHeadset.ha_mode_only_enable = FALSE;
#endif
            MAIN_DEBUG(("HS: EnterPair [%d]\n" , lState )) ;
            /*go into pairing mode*/ 
            if (( lState != headsetLimbo) && (lState != headsetConnDiscoverable ))
            {
                stateManagerEnterConnDiscoverableState();                
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventPairingFail):
            /*we have failed to pair in the alloted time - return to the connectable state*/
            MAIN_DEBUG(("HS: Pairing Fail\n")) ;
            if (lState != headsetTestMode)
			{
				switch (theHeadset.features.PowerDownOnDiscoTimeout)
				{
					case PAIRTIMEOUT_POWER_OFF:
						MessageSend ( task , EventPowerOff , 0) ;
						break;
					case PAIRTIMEOUT_POWER_OFF_IF_NO_PDL:
						/* Power off if no entries in PDL list */
						if (stateManagerGetPdlSize() == 0)
						{
							MessageSend ( task , EventPowerOff , 0) ;
						}
						else
						{
							stateManagerEnterConnectableState(TRUE); 
						}
						break;
					case PAIRTIMEOUT_CONNECTABLE:
					default:
						stateManagerEnterConnectableState(TRUE);          
				}
			}
            /* have attempted to connect following a power on and failed so clear the power up connection flag */                
            theHeadset.powerup_no_connection = FALSE;

        break ;                        
        case ( EventPairingSuccessful):
            MAIN_DEBUG(("HS: Pairing Successful\n")) ;
            if (lState == headsetConnDiscoverable)
                stateManagerEnterConnectableState(FALSE);
        break ;
		case ( EventConfirmationAccept ):
			MAIN_DEBUG(("HS: Pairing Correct Res\n" )) ;
			headsetPairingAcceptRes();
		break;
		case ( EventConfirmationReject ):
			MAIN_DEBUG(("HS: Pairing Reject Res\n" )) ;
			headsetPairingRejectRes();
		break;
        case ( EventEstablishSLC ) :    
			if(theHeadset.ha_mode_only_enable)
			{
				lIndicateEvent = FALSE ;
				break;
			}
            
            /* check we are not already connecting before starting */
            {
                MAIN_DEBUG(("EventEstablishSLC\n")) ;
                
                /* if the scroll PDL for preset number of connection attempts before giving is set
                   retrieve the number of connection attempts to use */
                theHeadset.conf->NoOfReconnectionAttempts = theHeadset.conf->timeouts.ReconnectionAttempts ;
                    
                slcEstablishSLCRequest() ;
                
                /* don't indicate the event at first power up if the use different event at power on
                   feature bit is enabled, this enables the establish slc event to be used for the second manual
                   connection request */
                if(stateManagerGetState() == headsetConnectable)
                {
                    /* send message to do indicate a start of paging process when in connectable state */
                    MessageSend(&theHeadset.task, EventStartPagingInConnState ,0);  
                }   
            }  
        break ;
        case ( EventRssiPair ):
            MAIN_DEBUG(("HS: RSSI Pair\n"));
            theHeadset.rssi_action = rssi_pairing;
            slcStartInquiry();
        break;
        case ( EventRssiPairReminder ):
            MAIN_DEBUG(("HS: RSSI Pair Reminder\n"));
            MessageSendLater(&theHeadset.task, EventRssiPairReminder, 0, D_SEC(INQUIRY_REMINDER_TIMEOUT_SECS));
        break;
        case ( EventRssiPairTimeout ):
            MAIN_DEBUG(("HS: RSSI Pair Timeout\n"));
            slcStopInquiry();
        break;
		case ( EventRefreshEncryption ):
			MAIN_DEBUG(("HS: Refresh Encryption\n"));
			{
				uint8 k;
                Sink sink;
                Sink audioSink;
				/* For each profile */
				for(k=0;k<MAX_PROFILES;k++)
				{
					MAIN_DEBUG(("Profile %d: ",k));
					/* If profile is connected */
					if((HfpLinkGetSlcSink((k + 1), &sink))&&(sink))
					{
						/* If profile has no valid SCO sink associated with it */
                        HfpLinkGetAudioSink((k + hfp_primary_link), &audioSink);
						if(!SinkIsValid(audioSink))
						{
							MAIN_DEBUG(("Key Refreshed\n"));
							/* Refresh the encryption key */
							ConnectionSmEncryptionKeyRefreshSink(sink);
						}
#ifdef DEBUG_MAIN
						else
						{
							MAIN_DEBUG(("Key Not Refreshed, SCO Active\n"));
						}
					}
					else
					{
						MAIN_DEBUG(("Key Not Refreshed, SLC Not Active\n"));
#endif
					}
				}
				MessageSendLater(&theHeadset.task, EventRefreshEncryption, 0, D_MIN(theHeadset.conf->timeouts.EncryptionRefreshTimeout_m));
			}
		break;
        
        /* 60 second timer has triggered to disable connectable mode in multipoint
            connection mode */
        case ( EventConnectableTimeout ) :
            /* only disable connectable mode if at least one hfp instance is connected */
            if(theHeadset.conf->no_of_profiles_connected)
            {
                MAIN_DEBUG(("SM: disable Connectable \n" ));
                /* disable connectability */
                headsetDisableConnectable();
            }
	    break;
        
        case ( EventBatteryLevelRequest ) :
		  MAIN_DEBUG(("EventBatteryLevelRequest\n")) ;
		
          /* take an immediate reading of the battery voltage only, returning to any preset
             timed readings */
		  BatteryUserInitiatedRead();
        
        
		break;
        case ( EventLEDEventComplete ) :
            /*the message is a ptr to the event we have completed*/
            MAIN_DEBUG(("HS : LEDEvCmp[%x]\n" ,  (( LMEndMessage_t *)message)->Event  )) ;
            
            
            switch ( (( LMEndMessage_t *)message)->Event )
            {
                case (EventResetPairedDeviceList) :
                {      /*then the reset has been completed*/
                    MessageSend(&theHeadset.task , EventResetComplete , 0 ) ;
   
                        /*Reboot if required*/
                    if ((theHeadset.features.RebootAfterReset )&&
                        (stateManagerGetState() > headsetLimbo )) 

                    {
                        MAIN_DEBUG(("HS: Reboot After Reset\n")) ;
                        MessageSend ( &theHeadset.task , EventPowerOff , 0 ) ;
                    }
                }
                break ;            
				
				case EventPowerOff:
				{
						/*allows a reset of the device for those designs which keep the chip permanently powered on*/
					if (theHeadset.features.ResetAfterPowerOffComplete)
						Panic() ;
				}
				break ;
                
                default: 
                break ;
            }
            
#ifdef ROM_LEDS          
            if (theHeadset.features.QueueLEDEvents )
            {
                    /*if there is a queueud event*/
                if (theHeadset.theLEDTask->Queue.Event1)
                {
                    MAIN_DEBUG(("HS : Play Q'd Ev [%x]\n", (EVENTS_MESSAGE_BASE + theHeadset.theLEDTask->Queue.Event1)  ));
                    MAIN_DEBUG(("HS : Queue [%x][%x][%x][%x]\n", theHeadset.theLEDTask->Queue.Event1,
                                                              theHeadset.theLEDTask->Queue.Event2,
                                                              theHeadset.theLEDTask->Queue.Event3,
                                                              theHeadset.theLEDTask->Queue.Event4
                                                                    
                                                                ));
                
                    LEDManagerIndicateEvent ( (EVENTS_MESSAGE_BASE + theHeadset.theLEDTask->Queue.Event1) ) ;
    
                        /*shuffle the queue*/
                    theHeadset.theLEDTask->Queue.Event1 = theHeadset.theLEDTask->Queue.Event2 ;
                    theHeadset.theLEDTask->Queue.Event2 = theHeadset.theLEDTask->Queue.Event3 ;
                    theHeadset.theLEDTask->Queue.Event3 = theHeadset.theLEDTask->Queue.Event4 ;
                    theHeadset.theLEDTask->Queue.Event4 = 0x00 ;
                }	
                else
                {
                    /* restart state indication */
                    LEDManagerIndicateState ( stateManagerGetState () ) ;
                }
            }
            else
                LEDManagerIndicateState ( stateManagerGetState () ) ;
#endif
                
        break ;   
        case (EventAutoSwitchOff):
            MAIN_DEBUG(("HS: Auto S Off[%d] sec elapsed\n" , theHeadset.conf->timeouts.AutoSwitchOffTime_s )) ;
            switch ( lState )
            {   
                case headsetLimbo:
                case headsetConnectable:
                case headsetConnDiscoverable:
                 MessageSend ( task , EventPowerOff , 0) ;
                    break;

                case headsetConnected:
                case headsetOutgoingCallEstablish:   
                case headsetIncomingCallEstablish:   
                case headsetActiveCallSCO:            
                case headsetActiveCallNoSCO:             
                case headsetTestMode:
                    break ;
                default:
                    MAIN_DEBUG(("HS : UE ?s [%d]\n", lState));
                    break ;
            }
        break;
        case (EventChargerConnected):
        {
            MAIN_DEBUG(("HS: Charger Connected\n"));
			powerManagerChargerConnected();
            if ( lState == headsetLimbo )
            { 
                stateManagerUpdateLimboState();
            }
        }
        break;
        case (EventChargerDisconnected):
        {
            MAIN_DEBUG(("HS: Charger Disconnected\n"));
            powerManagerChargerDisconnected();
 
            if (lState == headsetLimbo )
            {
                stateManagerUpdateLimboState();
            }
        }
        break;
        case (EventResetPairedDeviceList):
            {
				MAIN_DEBUG(("HS: --Reset PDL--")) ;                
                if ( stateManagerIsConnected () )
                {
                        /*then we have an SLC active*/
                   headsetDisconnectAllSlc();
                }                
                configManagerReset();
            }
        break ;
        case ( EventLimboTimeout ):
            {
                /*we have received a power on timeout - shutdown*/
                MAIN_DEBUG(("HS: EvLimbo TIMEOUT\n")) ;
                if (lState != headsetTestMode)
                {
                    stateManagerUpdateLimboState();
                }
            }    
        break ;
        case EventSLCDisconnected: 
                MAIN_DEBUG(("HS: EvSLCDisconnect\n")) ;
            {
                theHeadset.VoiceRecognitionIsActive = FALSE ;
                MessageCancelAll ( &theHeadset.task , EventNetworkOrServiceNotPresent ) ;
            }
        break ;
        case (EventLinkLoss):
            MAIN_DEBUG(("HS: Link Loss\n")) ;
            {
                /* should the headset have been powered off prior to a linkloss event being
                   generated, this can happen if a link loss has occurred within 5 seconds
                   of power off, ensure the headset does not attempt to reconnet from limbo mode */
                if(stateManagerGetState()== headsetLimbo)
                    lIndicateEvent = FALSE;
            }
        break ;
        case (EventMuteReminder) :        
            MAIN_DEBUG(("HS: Mute Remind\n")) ;
            /*arrange the next mute reminder tone*/
            MessageSendLater( &theHeadset.task , EventMuteReminder , 0 ,D_SEC(theHeadset.conf->timeouts.MuteRemindTime_s ) )  ;            
        break;   
        
        case EventCheckForLowBatt:
           MAIN_DEBUG(("HS: Check for Low Batt\n")) ;
 
        break;       
        
        case EventLowBattery:
            DEBUG(("HS: EvLowBatt\n")) ;
			if (theHeadset.features.EnableCSR2CSRBattLevel && !(theHeadset.low_battery_flag_ag))
			{
				MAIN_DEBUG(("HS: EvLowBatt, output to AG\n")) ;
                theHeadset.low_battery_flag_ag = TRUE;
			}
            
            /* If charging, no tone is play */
            if(ChargerIsChargerConnected())
            {
                lIndicateEvent = FALSE ;
            }

        break; 
        case EventTrickleCharge:  
            MAIN_DEBUG(("HS: EvTrickleChg\n")) ;
        break;
        case EventFastCharge:
        break;
        case EventOkBattery:
        break;   
        case EventEnterDUTState :
        {
            MAIN_DEBUG(("EnterDUTState \n")) ;
            stateManagerEnterTestModeState();                
        }
        break;        
        case EventEnterDutMode :
        {
            MAIN_DEBUG(("Enter DUT Mode \n")) ;            
            if (lState !=headsetTestMode)
            {
                MessageSend( task , EventEnterDUTState, 0 ) ;
            }
            enterDutMode () ;               
        }
        break;        
        case EventEnterTXContTestMode :
        {
            MAIN_DEBUG(("Enter TX Cont \n")) ;        
            if (lState !=headsetTestMode)
            {
                MessageSend( task , EventEnterDUTState , 0 ) ;
            }            
            enterTxContinuousTestMode() ;
        }
        break ;		
        case EventVolumeOrientationNormal:
                theHeadset.gVolButtonsInverted = FALSE ;               
                MAIN_DEBUG(("HS: VOL ORIENT NORMAL [%d]\n", theHeadset.gVolButtonsInverted)) ;
                    /*write this to the PSKEY*/                
                /* also include the led disable state as well as orientation, write this to the PSKEY*/ 
				/* also include the selected tts language  */
                configManagerWriteSessionData () ;                          
        break;
        case EventVolumeOrientationInvert:       
               theHeadset.gVolButtonsInverted = TRUE ;
               MAIN_DEBUG(("HS: VOL ORIENT INVERT[%d]\n", theHeadset.gVolButtonsInverted)) ;               
               /* also include the led disable state as well as orientation, write this to the PSKEY*/                
               /* also include the selected tts language  */
			   configManagerWriteSessionData () ;           
        break;        
        case EventToggleVolume:                
                theHeadset.gVolButtonsInverted ^=1 ;    
                MAIN_DEBUG(("HS: Toggle Volume Orientation[%d]\n", theHeadset.gVolButtonsInverted)) ;                
        break ;        
        case EventNetworkOrServiceNotPresent:
            {       /*only bother to repeat this indication if it is not 0*/
                if ( theHeadset.conf->timeouts.NetworkServiceIndicatorRepeatTime_s )
                {       /*make sure only ever one in the system*/
                    MessageCancelAll( task , EventNetworkOrServiceNotPresent) ;
                    MessageSendLater  ( task , 
                                        EventNetworkOrServiceNotPresent ,
                                        0 , 
                                        D_SEC(theHeadset.conf->timeouts.NetworkServiceIndicatorRepeatTime_s) ) ;
                }                                    
                MAIN_DEBUG(("HS: NO NETWORK [%d]\n", theHeadset.conf->timeouts.NetworkServiceIndicatorRepeatTime_s )) ;
            }                                
        break ;
        case EventNetworkOrServicePresent:
            {
                MessageCancelAll ( task , EventNetworkOrServiceNotPresent ) ;                
                MAIN_DEBUG(("HS: YES NETWORK\n")) ;
            }   
        break ;
        case EventEnableDisableLeds  :   
#ifdef ROM_LEDS            
            MAIN_DEBUG(("HS: Toggle EN_DIS LEDS ")) ;
            MAIN_DEBUG(("HS: Tog Was[%c]\n" , theHeadset.theLEDTask->gLEDSEnabled ? 'T' : 'F')) ;
            
            LedManagerToggleLEDS();
            MAIN_DEBUG(("HS: Tog Now[%c]\n" , theHeadset.theLEDTask->gLEDSEnabled ? 'T' : 'F')) ;            
#endif			
			break ;        
        case EventEnableLEDS:
#ifdef ROM_LEDS            
            MAIN_DEBUG(("HS: Enable LEDS\n")) ;
            LedManagerEnableLEDS ( ) ;
                /* also include the led disable state as well as orientation, write this to the PSKEY*/                
            configManagerWriteSessionData ( ) ;        		
#endif
            break ;
        case EventDisableLEDS:
#ifdef ROM_LEDS            
            MAIN_DEBUG(("HS: Disable LEDS\n")) ;            
            LedManagerDisableLEDS ( ) ;
			
                /* also include the led disable state as well as orientation, write this to the PSKEY*/                
            configManagerWriteSessionData ( ) ;            
#endif
            break ;
		case EventCancelLedIndication:
			MAIN_DEBUG(("HS: Disable LED indication\n")) ;        
#ifdef ROM_LEDS
            LedManagerResetLEDIndications ( ) ;            
#endif
		break ;
		case EventCallAnswered:
			MAIN_DEBUG(("HS: EventCallAnswered\n")) ;
		break;
        case EventSLCConnected:
        case EventSLCConnectedAfterPowerOn:
            
            MAIN_DEBUG(("HS: EventSLCConnected\n")) ;
            /*if there is a queued event - we might want to know*/                
            headsetRecallQueuedEvent();
        break;            
      
        case EventVLongTimer:
        case EventLongTimer:
           if (lState == headsetLimbo)
           {
               lIndicateEvent = FALSE ;
           }
        break ;
        case EventChargeError:
            MAIN_DEBUG(("HS: EventChargerError \n")) ;
            {       /*use the standard event if we are connected otherwise:*/
                if (( lState != headsetActiveCallSCO ) &&
                    ( lState != headsetActiveCallNoSCO ) );
                {
                    /*we are not connected so use the idle charger error event*/
                    lIndicateEvent = FALSE ;
                    MessageSend( task , EventChargeErrorInIdleState , 0 ) ;
                }                     
            }
        break ;  
        /* triggered when charging is ceased due to batt temp being out of range */
        case EventBattTempOutOfRange:
            MAIN_DEBUG(("HS: EventBattTempOutOfRange \n")) ;            
        break;
            /*these events have no required action directly associated with them  */
             /*they are received here so that LED patterns and Tones can be assigned*/
        case EventSCOLinkOpen :        
            MAIN_DEBUG(("EventScoLinkOpen\n")) ;
        break ;
        case EventSCOLinkClose:
            MAIN_DEBUG(("EventScoLinkClose\n")) ;
        break ;
        case EventEndOfCall :        
            MAIN_DEBUG(("EventEndOfCall\n")) ;
        break;    
        case EventResetComplete:        
            MAIN_DEBUG(("EventResetComplete\n")) ;
        break ;
        case EventError:        
            MAIN_DEBUG(("EventError\n")) ;
        break;
		case EventChargeErrorInIdleState:        
            MAIN_DEBUG(("EventChargeErrorInIdle\n")) ;
        break; 
	    case EventReconnectFailed:        
            MAIN_DEBUG(("EventReconnectFailed\n")) ;
        break;
		
#ifdef THREE_WAY_CALLING		
        case EventThreeWayReleaseAllHeld:       
            MAIN_DEBUG(("HS3 : RELEASE ALL [%c]\n", TRUE_OR_FALSE(theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING))) ;          
            /* release the held call */
            MpReleaseAllHeld();
        break;
        case EventThreeWayAcceptWaitingReleaseActive:    
            MAIN_DEBUG(("HS3 : ACCEPT & RELEASE [%c]\n", TRUE_OR_FALSE(theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING))) ;
            MpAcceptWaitingReleaseActive();
    		/* if headset has been muted on, mute off now */
    		if(theHeadset.gMuted)
				VolumeMuteOff();
        break ;
        case EventThreeWayAcceptWaitingHoldActive  :
            MAIN_DEBUG(("HS3 : ACCEPT & HOLD[%c]\n", TRUE_OR_FALSE(theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING))) ;
            /* three way calling not available in multipoint usage */
            MpAcceptWaitingHoldActive();
    		/* if headset has been muted on, mute off now */
    		if(theHeadset.gMuted)
				VolumeMuteOff();
        break ;
        case EventThreeWayAddHeldTo3Way  :
            MAIN_DEBUG(("HS3 : ADD HELD to 3WAY[%c]\n", TRUE_OR_FALSE(theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING))) ;
            /* check to see if a conference call can be created, more than one call must be on the same AG */            
            MpHandleConferenceCall(TRUE);
        break ;
        case EventThreeWayConnect2Disconnect:  
            MAIN_DEBUG(("HS3 : EXPLICIT TRANSFER[%c]\n", TRUE_OR_FALSE(theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING))) ;
            /* check to see if a conference call can be created, more than one call must be on the same AG */            
            MpHandleConferenceCall(FALSE);
        break ;
#endif		
        case (EventEnablePowerOff):
        {
            MAIN_DEBUG(("HS: EventEnablePowerOff \n")) ;
            theHeadset.PowerOffIsEnabled = TRUE ;
        }
        break;        
        case EventPlaceIncomingCallOnHold:
            headsetPlaceIncomingCallOnHold();
        break ;
        
        case EventAcceptHeldIncomingCall:
            headsetAcceptHeldIncomingCall();
        break ;
        case EventRejectHeldIncomingCall:
            headsetRejectHeldIncomingCall();
        break;
        
        case EventGasGauge0 :
        case EventGasGauge1 :
        case EventGasGauge2 :
        case EventGasGauge3 :
            csr2csrHandleAgBatteryRequestRes(id);
        break ;
        
        case EventEnterDFUMode:       
        {
			MAIN_DEBUG(("EventEnterDFUMode\n")) ;
	      	BootSetMode(0);
        }
        break;

		case EventEnterServiceMode:
        {
            MAIN_DEBUG(("Enter Service Mode \n")) ;            

            enterServiceMode();
        }
        break ;
        case EventServiceModeEntered:
        {
            MAIN_DEBUG(("Service Mode!!!\n")) ; 
        }
        break;

		case EventAudioMessage1:
		case EventAudioMessage2:
		case EventAudioMessage3:
		case EventAudioMessage4:
		{
			if (&theHeadset.sco_sink)
			{
				uint16 * lParam = PanicUnlessMalloc ( sizeof(uint16)) ;
				*lParam = (id -  EventAudioMessage1) ; /*0,1,2,3*/
				AudioSetMode ( AUDIO_MODE_CONNECTED , (void *) lParam) ;
			}
		}
		break ;
        case  EventCancelHSPIncomingCall:
            MAIN_DEBUG(("EventCancelHSPIncomingCall\n")) ;     
                /*clear the incoming call flag*/
			theHeadset.HSPIncomingCallInd = FALSE;
			/* if the ring indication has timed out and the state is still incoming 
				call establish, return to connected state else the headset will 
				remain in incoming call state if the call is answered or rejected
 				on the AG */
			if ( lState == headsetIncomingCallEstablish)
			{
				MAIN_DEBUG(("HSP ring with no connect, return to connected\n")) ; 
				stateManagerEnterConnectedState();    
			}
        break ;
		
        case EventUpdateStoredNumber:
            headsetUpdateStoredNumber();
        break;
        
		case EventDialStoredNumber:
			MAIN_DEBUG(("EventDialStoredNumber\n"));
			headsetDialStoredNumber();
		
		break;
		case EventRestoreDefaults:
			MAIN_DEBUG(("EventRestoreDefaults\n"));
			configManagerRestoreDefaults();
				
		break;
		
		case EventTone1:
		case EventTone2:
			MAIN_DEBUG(("HS: EventTone[%d]\n" , (id - EventTone1 + 1) )) ;
		break;
		
		case EventSelectTTSLanguageMode:
			MAIN_DEBUG(("EventSelectTTSLanguageModes\n"));
            TTSSelectTTSLanguageMode();
            MessageCancelAll(&theHeadset.task, EventConfirmTTSLanguage);
            MessageSendLater(&theHeadset.task, EventConfirmTTSLanguage, 0, D_SEC(3));
		break;
		
        case EventConfirmTTSLanguage:
            /* Store TTS language in PS */
            configManagerWriteSessionData () ;
        break;
        		
        /* enable multipoint functionality */
        case EventEnableMultipoint:
            MAIN_DEBUG(("EventEnableMultipoint\n"));
            /* enable multipoint operation */
            /* deny enable if disabled because of OVAL */
            theHeadset.MultipointEnable = 1;
            /* and store in PS for reading at next power up */
			configManagerWriteSessionData () ;
		break;
		
        /* disable multipoint functionality */
        case EventDisableMultipoint:
            MAIN_DEBUG(("EventDisableMultipoint\n"));
            /* disable multipoint operation */
            theHeadset.MultipointEnable = 0;
            /* and store in PS for reading at next power up */
			configManagerWriteSessionData () ;           
        break;
        
        /* disabled leds have been re-enabled by means of a button press or a specific event */
        case EventResetLEDTimeout:
#ifdef ROM_LEDS               
            MAIN_DEBUG(("EventResetLEDTimeout\n"));
            LEDManagerIndicateState ( lState ) ;     
			theHeadset.theLEDTask->gLEDSStateTimeout = FALSE ;               
#endif            
        break;
 
        /* starting paging whilst in connectable state */
        case EventStartPagingInConnState:
            MAIN_DEBUG(("EventStartPagingInConnState\n"));
            /* set bit to indicate paging status */
            theHeadset.paging_in_progress = TRUE;
        break;
        
        /* paging stopped whilst in connectable state */
        case EventStopPagingInConnState:
            MAIN_DEBUG(("EventStartPagingInConnState\n"));
            /* set bit to indicate paging status */
            theHeadset.paging_in_progress = FALSE;
        break;
        
        /* continue the slc connection procedure, will attempt connection
           to next available device */
        case EventContinueSlcConnectRequest:
            MAIN_DEBUG(("EventContinueSlcConnectRequest\n"));
            /* attempt next connection */
   	        slcContinueEstablishSLCRequest();
        break;
        
        /* indication of call waiting when using two AG's in multipoint mode */
        case EventMultipointCallWaiting:
            MAIN_DEBUG(("EventMultipointCallWaiting\n"));
        break;
                   
        /* kick off a check the role of the headset and make changes if appropriate by requesting a role indication */
        case EventCheckRole:
        {
            uint8 hfp_idx;
            Sink * sink_passed = (Sink*)message ;
				/*no specific sink to check, check all available - happens on the back of each hfp connect cfm */
			if (!sink_passed)
            {
	            for(hfp_idx = 0; hfp_idx < theHeadset.conf->no_of_profiles_connected; hfp_idx++)
	            {
	                Sink sink; 
                    HfpLinkGetSlcSink((hfp_idx + 1), &sink);
	                if(SinkIsValid(sink))
	        	    {
					    ConnectionGetRole(&theHeadset.task, sink);
                        MAIN_DEBUG(("GET n role[%x]\n", (int)sink));
                    }
	            }
	        }
            else /*a specific sink has been passed in - happens on a failed attempt to role switch - after 1 second*/
            {
            	if (SinkIsValid(*sink_passed) )
                {
                		/*only attempt to switch the sink that has failed to switch*/
	                ConnectionGetRole(&theHeadset.task , *sink_passed) ;
					MAIN_DEBUG(("GET 1 role[%x]\n", (int)*sink_passed));
                }
            }
		}	
        break;

        case EventMissedCall:
        {
			if(theHeadset.conf->timeouts.MissedCallIndicateTime_s != 0)
			{ 
                MessageCancelAll(task , EventMissedCall ) ;
                     
                theHeadset.MissedCallIndicated -= 1;               
                if(theHeadset.MissedCallIndicated != 0)
 			 	{
                    MessageSendLater( &theHeadset.task , EventMissedCall , 0 , D_SEC(theHeadset.conf->timeouts.MissedCallIndicateTime_s) ) ;
                }
            }	
        }
        break;
      
#ifdef ENABLE_PBAP                  
        case EventPbapDialMch:
        {         
            /* pbap dial from missed call history */
            MAIN_DEBUG(("EventPbapDialMch\n"));  
                        
            if ( theHeadset.PowerOffIsEnabled )
            {
                /* If voice dial is active, cancel the voice dial if the feature bit is set */
                if (theHeadset.features.LNRCancelsVoiceDialIfActive   && 
                    theHeadset.VoiceRecognitionIsActive)
                {
                    MessageSend(&theHeadset.task , EventInitateVoiceDial , 0 ) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {					
                    pbapDialPhoneBook(pbap_mch);
                }
			}
            else
            {
                lIndicateEvent = FALSE ;
            }
        }
        break;      
        
        case EventPbapDialIch:
        {            
            /* pbap dial from incoming call history */
            MAIN_DEBUG(("EventPbapDialIch\n"));
           
            if ( theHeadset.PowerOffIsEnabled )
            {
                /* If voice dial is active, cancel the voice dial if the feature bit is set */
                if (theHeadset.features.LNRCancelsVoiceDialIfActive   && 
                    theHeadset.VoiceRecognitionIsActive)
                {
                    MessageSend(&theHeadset.task , EventInitateVoiceDial , 0 ) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {					
                    pbapDialPhoneBook(pbap_ich);
                }
			}
            else
            {
                lIndicateEvent = FALSE ;
            }
        }
        break;        
#endif        
        
#ifdef WBS_TEST
        /* TEST EVENTS for WBS testing */
        case EventSetWbsCodecs:
            if(theHeadset.RenegotiateSco)
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd wbs\n")) ;
                theHeadset.RenegotiateSco = 0;
                HfpWbsSetSupportedCodecs((hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc), FALSE);
            }
            else
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd only\n")) ;
                theHeadset.RenegotiateSco = 1;
                HfpWbsSetSupportedCodecs(hfp_wbs_codec_mask_cvsd , FALSE);           
            }
            
        break;
    
        case EventSetWbsCodecsSendBAC:
            if(theHeadset.RenegotiateSco)
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd wbs\n")) ;
                theHeadset.RenegotiateSco = 0;
                HfpWbsSetSupportedCodecs((hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc), TRUE);
            }
           else
           {
               MAIN_DEBUG(("HS : AT+BAC = cvsd only\n")) ;
               theHeadset.RenegotiateSco = 1;
               HfpWbsSetSupportedCodecs(hfp_wbs_codec_mask_cvsd , TRUE);           
           }
           break;
 
         case EventOverrideResponse:
                   
           if(theHeadset.FailAudioNegotiation)
           {
               MAIN_DEBUG(("HS : Fail Neg = off\n")) ;
               theHeadset.FailAudioNegotiation = 0;
           }
           else
           {
               MAIN_DEBUG(("HS : Fail Neg = on\n")) ;
               theHeadset.FailAudioNegotiation = 1;
           }
       break; 
    
       case EventCreateAudioConnection:
           MAIN_DEBUG(("HS : Create Audio Connection\n")) ;
           
           CreateAudioConnection();
       break;

#endif
       
       case EventEnableIntelligentPowerManagement:
		   /* reuse for ha only mode toggle */
#if 0	   	
           MAIN_DEBUG(("HS : Enable LBIPM\n")) ;           
            /* enable LBIPM operation */
           /* only enable if not disabled for Oval */
           theHeadset.lbipmEnable = 1;          
           /* send plugin current power level */           
           AudioSetPower(LBIPMPowerLevel());
            /* and store in PS for reading at next power up */
		   configManagerWriteSessionData () ;     
#endif			
       break;
       
       case EventDisableIntelligentPowerManagement:
		   /* reuse for ha only mode toggle */
#if 0	   	
           MAIN_DEBUG(("HS : Disable LBIPM\n")) ;           
            /* disable LBIPM operation */
           theHeadset.lbipmEnable = 0;
           /* notify the plugin Low power mode is no longer required */           
           AudioSetPower(LBIPMPowerLevel());
            /* and store in PS for reading at next power up */
		   configManagerWriteSessionData () ; 
#endif			
       break;
       
       case EventToggleIntelligentPowerManagement:
			/* reuse for ha only mode toggle */
			if(theHeadset.ha_mode_only_enable)
			{
				theHeadset.ha_mode_only_enable = FALSE;
				MessageSend( &theHeadset.task , EventEnableIntelligentPowerManagement, 0 ) ;

                theHeadset.conf->NoOfReconnectionAttempts = theHeadset.conf->timeouts.ReconnectionAttempts ;
                slcEstablishSLCRequest() ;
			}
			else
			{
				uint8 index;
				theHeadset.ha_mode_only_enable = TRUE;
				MessageSend( &theHeadset.task , EventDisableIntelligentPowerManagement, 0 ) ;
				headsetDisconnectAllSlc();
				/* disconnect any a2dp signalling channels */
				for(index = a2dp_primary; index < (a2dp_secondary+1); index++)
				{
					/* is a2dp connected? */
					if(theHeadset.a2dp_link_data->connected[index])
					{
						/* disconnect signalling channel */
						A2dpSignallingDisconnectRequest(theHeadset.a2dp_link_data->device_id[index]);
					}
				}  
			}
#if 0		
           MAIN_DEBUG(("HS : Toggle LBIPM\n")) ;
           if(theHeadset.lbipmEnable)
           {
               MessageSend( &theHeadset.task , EventDisableIntelligentPowerManagement , 0 ) ;
           }
           else
           {
               MessageSend( &theHeadset.task , EventEnableIntelligentPowerManagement , 0 ) ;
           }
#endif
       break; 
       
#ifdef ENABLE_AVRCP       
       case EventAvrcpPlayPause:
           MAIN_DEBUG(("HS : EventAvrcpPlayPause\n")) ;  
           headsetAvrcpPlayPause();
       break;
            
       case EventAvrcpStop:
           MAIN_DEBUG(("HS : EventAvrcpStop\n")) ; 
           headsetAvrcpStop();
       break;
            
       case EventAvrcpSkipForward:
           MAIN_DEBUG(("HS : EventAvrcpSkipForward\n")) ;  
           headsetAvrcpSkipForward();
       break;
 
       case EventEnterBootMode2:
            MAIN_DEBUG(("Reboot into different bootmode [2]\n")) ;
           BootSetMode(2) ;
       break ;
      
       case EventAvrcpSkipBackward:
           MAIN_DEBUG(("HS : EventAvrcpSkipBackward\n")) ; 
           headsetAvrcpSkipBackward();
       break;
            
       case EventAvrcpFastForwardPress:
           MAIN_DEBUG(("HS : EventAvrcpFastForwardPress\n")) ;  
           headsetAvrcpFastForwardPress();
       break;
            
       case EventAvrcpFastForwardRelease:
           MAIN_DEBUG(("HS : EventAvrcpFastForwardRelease\n")) ;
           headsetAvrcpFastForwardRelease();
       break;
            
       case EventAvrcpRewindPress:
           MAIN_DEBUG(("HS : EventAvrcpRewindPress\n")) ; 
           headsetAvrcpRewindPress();
       break;
            
       case EventAvrcpRewindRelease:
           MAIN_DEBUG(("HS : EventAvrcpRewindRelease\n")) ; 
           headsetAvrcpRewindRelease();
       break;
#endif       
       
       default :
           MAIN_DEBUG (( "HS : UE unhandled!! [%x]\n", id ));     
       break ;           

        
    }   
    

        /* Inform theevent indications that we have received a user event*/
        /* For all events except the end event notification as this will just end up here again calling itself...*/
    if ( lIndicateEvent )
    {
        if ( id != EventLEDEventComplete )
        {
            LEDManagerIndicateEvent ( id ) ;
        }           
		
        TonesPlayEvent ( id ) ;
    }
    
#ifdef TEST_HARNESS 
    vm2host_send_event(id);
#endif
    
}


/*************************************************************************
NAME    
    handleHFPMessage

DESCRIPTION
    handles the messages from the user events

RETURNS

*/
static void handleHFPMessage  ( Task task, MessageId id, Message message )
{   
    MAIN_DEBUG(("HFP = [%x]\n", id)) ;   
    
    switch(id)
    {
        /* -- Handsfree Profile Library Messages -- */
    case HFP_INIT_CFM:              
        
        /* Init configuration that is required now */
	    InitEarlyUserFeatures();
    
        MAIN_DEBUG(("HFP_INIT_CFM - enable streaming[%x]\n", theHeadset.features.EnableA2dpStreaming)) ;   
        if(theHeadset.features.EnableA2dpStreaming)
        {
            ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | AV_COD_RENDER | AV_MAJOR_DEVICE_CLASS | AV_MINOR_HEADSET);        
        }
        else
        {
            ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | AV_MAJOR_DEVICE_CLASS | AV_MINOR_HEADSET);    
        }

        ConnectionWriteScanEnable(hci_scan_enable_inq_and_page);
        if  ( stateManagerGetState() == headsetLimbo ) 
        {
            if ( ((HFP_INIT_CFM_T*)message)->status == hfp_success )
                headsetInitComplete( (HFP_INIT_CFM_T*)message );
            else
                Panic();                
        }        
        
    break;
 
    case HFP_SLC_CONNECT_IND:
        MAIN_DEBUG(("HFP_SLC_CONNECT_IND\n"));
        if (stateManagerGetState() != headsetLimbo)
        {   
            headsetHandleSlcConnectInd((HFP_SLC_CONNECT_IND_T *) message);
        }
    break;

    case HFP_SLC_CONNECT_CFM:
        MAIN_DEBUG(("HFP_SLC_CONNECT_CFM [%x]\n", ((HFP_SLC_CONNECT_CFM_T *) message)->status ));
        if (stateManagerGetState() == headsetLimbo)
        {
            if ( ((HFP_SLC_CONNECT_CFM_T *) message)->status == hfp_success )
            {
                /*A connection has been made and we are now logically off*/
                headsetDisconnectAllSlc();   
            }
        }
        else
        {
            headsetHandleSlcConnectCfm((HFP_SLC_CONNECT_CFM_T *) message);
        }
                            
        break;
		
    case HFP_SLC_LINK_LOSS_IND:
    {
        HFP_SLC_LINK_LOSS_IND_T* ind = (HFP_SLC_LINK_LOSS_IND_T*)message;
        if(ind->status == hfp_link_loss_recovery)
            MessageSend( &theHeadset.task , EventLinkLoss , 0 ) ;
        
        /* determine whether it is required that the headset goes connectable during link loss */
        if(theHeadset.features.GoConnectableDuringLinkLoss)
        {
            /* go connectable during link loss */
            headsetEnableConnectable();   
        }
    }    
    break;
    
    case HFP_SLC_DISCONNECT_IND:
        MAIN_DEBUG(("HFP_SLC_DISCONNECT_IND\n"));
        MAIN_DEBUG(("Handle Disconnect\n"));
        headsetHandleSlcDisconnectInd((HFP_SLC_DISCONNECT_IND_T *) message);
    break;
    case HFP_SERVICE_IND:
        MAIN_DEBUG(("HFP_SERVICE_IND [%x]\n" , ((HFP_SERVICE_IND_T*)message)->service  ));
        headsetHandleServiceIndicator ( ((HFP_SERVICE_IND_T*)message) ) ;       
    break;
    /* indication of call status information, sent whenever a change in call status 
       occurs within the hfp lib */
    case HFP_CALL_STATE_IND:
        /* the Call Handler will perform headset state changes and be
           used to determine multipoint functionality */
        /* don't process call indications if in limbo mode */
        if(stateManagerGetState()!= headsetLimbo)
            headsetHandleCallInd((HFP_CALL_STATE_IND_T*)message);            
    break;

    case HFP_RING_IND:
        MAIN_DEBUG(("HFP_RING_IND\n"));		
		headsetHandleRingInd((HFP_RING_IND_T *)message);
    break;
    case HFP_VOICE_TAG_NUMBER_IND:
        MAIN_DEBUG(("HFP_VOICE_TAG_NUMBER_IND\n"));
        headsetWriteStoredNumber((HFP_VOICE_TAG_NUMBER_IND_T*)message);
    break;
    case HFP_DIAL_LAST_NUMBER_CFM:
        MAIN_DEBUG(("HFP_LAST_NUMBER_REDIAL_CFM\n"));       
        handleHFPStatusCFM (((HFP_DIAL_LAST_NUMBER_CFM_T*)message)->status ) ;
    break;      
    case HFP_DIAL_NUMBER_CFM:
        MAIN_DEBUG(("HFP_DIAL_NUMBER_CFM %d %d\n", stateManagerGetState(), ((HFP_DIAL_NUMBER_CFM_T *) message)->status));
		handleHFPStatusCFM (((HFP_DIAL_NUMBER_CFM_T*)message)->status ) ;    
	break;
    case HFP_DIAL_MEMORY_CFM:
        MAIN_DEBUG(("HFP_DIAL_MEMORY_CFM %d %d\n", stateManagerGetState(), ((HFP_DIAL_MEMORY_CFM_T *) message)->status));        
    break ;     
    case HFP_CALL_ANSWER_CFM:
        MAIN_DEBUG(("HFP_ANSWER_CALL_CFM\n"));
    break;
    case HFP_CALL_TERMINATE_CFM:
        MAIN_DEBUG(("HFP_TERMINATE_CALL_CFM %d\n", stateManagerGetState()));       
    break;
    case HFP_VOICE_RECOGNITION_IND:
        MAIN_DEBUG(("HS: HFP_VOICE_RECOGNITION_IND_T [%c]\n" ,TRUE_OR_FALSE( ((HFP_VOICE_RECOGNITION_IND_T* )message)->enable) )) ;            
            /*update the state of the voice dialling on the back of the indication*/
        theHeadset.VoiceRecognitionIsActive = ((HFP_VOICE_RECOGNITION_IND_T* ) message)->enable ;            
    break;
    case HFP_VOICE_RECOGNITION_ENABLE_CFM:
        MAIN_DEBUG(("HFP_VOICE_RECOGNITION_ENABLE_CFM s[%d] w[%d]i", (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) , theHeadset.VoiceRecognitionIsActive));

            /*if the cfm is in error then we did not succeed - toggle */
        if  ( (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) )
            theHeadset.VoiceRecognitionIsActive = 0 ;
            
        MAIN_DEBUG(("[%d]\n", theHeadset.VoiceRecognitionIsActive));
        
        handleHFPStatusCFM (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) ;            
    break;
    case HFP_CALLER_ID_ENABLE_CFM:
        MAIN_DEBUG(("HFP_CALLER_ID_ENABLE_CFM\n"));
    break;
    case HFP_VOLUME_SYNC_SPEAKER_GAIN_IND:
    {
        bool set_gain = FALSE;
        HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *ind = (HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *) message;

        MAIN_DEBUG(("HFP_VOLUME_SYNC_SPEAKER_GAIN_IND %d\n", ind->volume_gain));        

        if( theHeadset.sco_sink && stateManagerGetState() != headsetA2DPStreaming )
        {
            hfp_link_priority priority = HfpLinkPriorityFromAudioSink(theHeadset.sco_sink);
          
            /* If there's an active call with audio and IND message is from the same AG, set speaker gain   */
            set_gain = (priority == ind->priority );
        }
        
        VolumeSetHeadsetVolume ( ind->volume_gain , theHeadset.features.PlayLocalVolumeTone , ind->priority, set_gain) ;
    }
    break;
    case HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND:
		{
        	MAIN_DEBUG(("HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND %d\n", ((HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *) message)->mic_gain));        
			/*microphone indications deliberately not handled - this allows the audio plugin to control the mic volume at all times*/
	
			if(theHeadset.features.EnableSyncMuteMicophones)
			{
				HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *ind = (HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *) message;
				
				uint16	mic_gain = ind->mic_gain;
				
       			/* However, +VGM=0 or +VGA=x may be used to mute on/off microphone */
				if(mic_gain == 0)
				{
					if(!theHeadset.gMuted)
					{
						theHeadset.profile_data[PROFILE_INDEX(ind->priority)].audio.gAgSyncMuteFlag = 1;
						VolumeMuteOn();
					}
				}
				else
				{
					/* if headset has been muted on, mute off now */
					if(theHeadset.gMuted)
					{
						theHeadset.profile_data[PROFILE_INDEX(ind->priority)].audio.gAgSyncMuteFlag = 1;
						VolumeMuteOff();
					}
				}
			}
		}
	
	break;
    
	case HFP_CALLER_ID_IND:
        {
      		HFP_CALLER_ID_IND_T *ind = (HFP_CALLER_ID_IND_T *) message;
 
            /* ensure this is not a HSP profile */
          	DEBUG(("HFP_CALLER_ID_IND number %s", ind->caller_info + ind->offset_number));
           	DEBUG((" name %s\n", ind->caller_info + ind->offset_name));
			
			if ( !theHeadset.sco_sink )
			{
                /* Attempt to play caller name */
                if(!TTSPlayCallerName (ind->size_name, ind->caller_info + ind->offset_name))
                {
                    /* Caller name not present or not supported, try to play number */
				    TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
                }
			}
        }
	
	break;           
    
case HFP_UNRECOGNISED_AT_CMD_IND:
        {   
            uint16 i = 0 ;    
            char* searchString = "AT+MICTEST";
            bool found = FALSE;
            
            MAIN_DEBUG(("HFP_UNRECOGNISED_AT_CMD_IND_T\n" )) ;
            DEBUG(("AT command = %s", ((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->data));
            
            while(i< ((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->size_data && (found == FALSE))
            {
                DEBUG(("%s\n" ,(char*)(((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->data + i) )) ;
                if (strncmp((char*)(((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->data + i), searchString, strlen(searchString)) == 0)
                {
                    DEBUG(("string found AT+MICTEST\n"));
                    audioHandleMicSwitch();        
                    found = TRUE;
                }
                i++;    
            }
            
        }
    break ;
    case HFP_HS_BUTTON_PRESS_CFM:
        {
            MAIN_DEBUG(("HFP_HS_BUTTON_PRESS_CFM\n")) ;
        }
    break ;
     /*****************************************************************/

#ifdef THREE_WAY_CALLING	
    case HFP_CALL_WAITING_ENABLE_CFM :
            MAIN_DEBUG(("HS3 : HFP_CALL_WAITING_ENABLE_CFM_T [%c]\n", (((HFP_CALL_WAITING_ENABLE_CFM_T * )message)->status == hfp_success) ?'T':'F' )) ;
    break ;    
    case HFP_CALL_WAITING_IND:
        {
            /*change state accordingly*/
            stateManagerEnterThreeWayCallWaitingState();
            /* pass the indication to the multipoint handler which will determine if the call waiting tone needs
               to be played, this will depend upon whether the indication has come from the AG with
               the currently routed audio */
            mpHandleCallWaitingInd((HFP_CALL_WAITING_IND_T *)message);
        }
    break;

#endif	
    case HFP_SUBSCRIBER_NUMBERS_CFM:
        MAIN_DEBUG(("HS3: HFP_SUBSCRIBER_NUMBERS_CFM [%c]\n" , (((HFP_SUBSCRIBER_NUMBERS_CFM_T*)message)->status == hfp_success)  ? 'T' :'F' )) ;
    break ;
    case HFP_SUBSCRIBER_NUMBER_IND:
#ifdef DEBUG_MAIN            
    {
        uint16 i=0;
            
        MAIN_DEBUG(("HS3: HFP_SUBSCRIBER_NUMBER_IND [%d]\n" , ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->service )) ;
        for (i=0;i< ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->size_number ; i++)
        {
            MAIN_DEBUG(("%c", ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->number[i])) ;
        }
        MAIN_DEBUG(("\n")) ;
    } 
#endif
    break ;
    case HFP_CURRENT_CALLS_CFM:
        MAIN_DEBUG(("HS3: HFP_CURRENT_CALLS_CFM [%c]\n", (((HFP_CURRENT_CALLS_CFM_T*)message)->status == hfp_success)  ? 'T' :'F' )) ;
    break ;
    case HFP_CURRENT_CALLS_IND:
        MAIN_DEBUG(("HS3: HFP_CURRENT_CALLS_IND id[%d] mult[%d] status[%d]\n" ,
                                        ((HFP_CURRENT_CALLS_IND_T*)message)->call_idx , 
                                        ((HFP_CURRENT_CALLS_IND_T*)message)->multiparty  , 
                                        ((HFP_CURRENT_CALLS_IND_T*)message)->status)) ;
    break;
    case HFP_AUDIO_CONNECT_IND:
        MAIN_DEBUG(("HFP_AUDIO_CONNECT_IND\n")) ;
        audioHandleSyncConnectInd( (HFP_AUDIO_CONNECT_IND_T *)message ) ;
    break ;
    case HFP_AUDIO_CONNECT_CFM:
        MAIN_DEBUG(("HFP_AUDIO_CONNECT_CFM[%x][%x][%s%s%s] r[%d]t[%d]\n", ((HFP_AUDIO_CONNECT_CFM_T *)message)->status ,
                                                      (int)((HFP_AUDIO_CONNECT_CFM_T *)message)->audio_sink ,
                                                      ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_sco) ? "SCO" : "" )      ,  
                                                      ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_esco) ? "eSCO" : "" )    ,
                                                      ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_unknown) ? "unk?" : "" ) ,
                                                      (int)((HFP_AUDIO_CONNECT_CFM_T *)message)->rx_bandwidth ,
                                                      (int)((HFP_AUDIO_CONNECT_CFM_T *)message)->tx_bandwidth 
                                                      )) ;
        audioHandleSyncConnectCfm ( (HFP_AUDIO_CONNECT_CFM_T *)message ) ;            
    break ;
    case HFP_AUDIO_DISCONNECT_IND:
        MAIN_DEBUG(("HFP_AUDIO_DISCONNECT_IND [%x]\n", ((HFP_AUDIO_DISCONNECT_IND_T *)message)->status)) ;
        audioHandleSyncDisconnectInd ((HFP_AUDIO_DISCONNECT_IND_T *)message) ;
    break ;
	case HFP_SIGNAL_IND:
        MAIN_DEBUG(("HS: HFP_SIGNAL_IND [%d]\n", ((HFP_SIGNAL_IND_T* )message)->signal )) ; 
    break ;
	case HFP_ROAM_IND:
        MAIN_DEBUG(("HS: HFP_ROAM_IND [%d]\n", ((HFP_ROAM_IND_T* )message)->roam )) ;
    break; 
	case HFP_BATTCHG_IND:     
        MAIN_DEBUG(("HS: HFP_BATTCHG_IND [%d]\n", ((HFP_BATTCHG_IND_T* )message)->battchg )) ;
    break;
    
/*******************************************************************/
	
	case HFP_CSR_FEATURES_TEXT_IND:
		csr2csrHandleTxtInd () ;
	break ;
    
    case HFP_CSR_FEATURES_NEW_SMS_IND:
	   csr2csrHandleSmsInd () ;   
	break ;
    
	case HFP_CSR_FEATURES_GET_SMS_CFM:
	   csr2csrHandleSmsCfm() ;
	break ;
    
	case HFP_CSR_FEATURES_BATTERY_LEVEL_REQUEST_IND:
	   csr2csrHandleAgBatteryRequestInd() ;
    break ;
    
/*******************************************************************/
	
/*******************************************************************/

    /*******************************/
    
    default :
        MAIN_DEBUG(("HS :  HFP ? [%x]\n",id)) ;
    break ;
    }
}

#ifdef ENABLE_SPP
static void handleSppConnectInd(Task task,SPP_CONNECT_IND_T *ind)
{
	SppConnectResponse(task, ind->addr, TRUE, ind->sink, ind->server_channel, 0);
}

static void handleSppConnectCfm(SPP_CLIENT_CONNECT_CFM_T *cfm)
{

}

static void handleSppDisconnectInd(SPP_DISCONNECT_IND_T *ind)
{
	SppDisconnectResponse(ind->spp);
}

static void handleSppDisconnectCfm(SPP_DISCONNECT_CFM_T *cfm)
{

}

static void handleSppMoreData(SPP_MESSAGE_MORE_DATA_T *data)
{
	/* parse command */

	/* send response */
}

static void handleSppMoreSpace(SPP_MESSAGE_MORE_SPACE_T *space)
{

}

static void handleSppMessage ( Task task, MessageId id, Message message )
{
    MAIN_DEBUG(("SPP = [%x]\n", id)) ;   
    
    switch(id)
    {
        /* -- Serial Port Profile Library Messages -- */
    case SPP_START_SERVICE_CFM:
		break;
	case SPP_STOP_SERVICE_CFM:
		break;
	case SPP_CLIENT_CONNECT_CFM:
		handleSppConnectCfm((SPP_CLIENT_CONNECT_CFM_T *)message);
		break;
	case SPP_SERVER_CONNECT_CFM:
		handleSppConnectCfm((SPP_CLIENT_CONNECT_CFM_T *)message);
		break;
	case SPP_CONNECT_IND:
		handleSppConnectInd(task, (SPP_CONNECT_IND_T *)message);
		break;
	case SPP_MESSAGE_MORE_DATA:
		handleSppMoreData((SPP_MESSAGE_MORE_DATA_T *)message);
		break;
	case SPP_MESSAGE_MORE_SPACE:
		handleSppMoreSpace((SPP_MESSAGE_MORE_SPACE_T *)message);
		break;
	case SPP_DISCONNECT_IND:
		handleSppDisconnectInd((SPP_DISCONNECT_IND_T *)message);
		break;
	case SPP_DISCONNECT_CFM:
		handleSppDisconnectCfm((SPP_DISCONNECT_CFM_T *)message);
		break;
	default:
		break;
    }
}
#endif

/*************************************************************************
NAME    
    handleCodecMessage
    
DESCRIPTION
    handles the codec Messages

RETURNS
    
*/
static void handleCodecMessage  ( Task task, MessageId id, Message message )
{
    MAIN_DEBUG(("CODEC MSG received [%x]\n", id)) ;
      
    if (id == CODEC_INIT_CFM ) 
    {       /* The codec is now initialised */
    
        if ( ((CODEC_INIT_CFM_T*)message)->status == codec_success) 
        {
            MAIN_DEBUG(("CODEC_INIT_CFM\n"));   
            headsetHfpInit();
            theHeadset.codec_task = ((CODEC_INIT_CFM_T*)message)->codecTask ;                   
        }
        else
        {
            Panic();
        }
    }
}

#ifdef CVC_PRODTEST
static void handleKalimbaMessage ( Task task, MessageId id, Message message )
{
    const DSP_REGISTER_T *m = (const DSP_REGISTER_T *) message;
	MAIN_DEBUG(("CVC: msg id[%x] a[%x] b[%x] c[%x] d[%x]\n", m->id, m->a, m->b, m->c, m->d));
	
    switch (m->id)
    {
        case 0x1000:
            MAIN_DEBUG(("CVC_READY\n"));
            /*CVC_LOADPARAMS_MSG, CVC_PS_BASE*/
            KalimbaSendMessage(0x1012, 0x2280, 0, 0, 0);
            break;
            
        case 0x1006:
            MAIN_DEBUG(("CVC_CODEC_MSG\n"));
            /*MESSAGE_SCO_CONFIG, sco_encoder, sco_config*/
            KalimbaSendMessage(0x2000, 0, 3, 0, 0);
            break;
        
        case 0x100c:
            MAIN_DEBUG (("CVC_SECPASSED_MSG\n"));
            exit(CVC_PRODTEST_PASS);
            break;
            
        case 0x1013:
            MAIN_DEBUG (("CVC_SECFAILD_MSG\n"));
            exit(CVC_PRODTEST_FAIL);
            break;
            
        default:
            MAIN_DEBUG(("m->id [%x]\n", m->id));
            break;    
     }
    
}
#endif

/* Handle any audio plugin messages */
static void handleAudioPluginMessage( Task task, MessageId id, Message message )
{
	switch (id)
    {        
		case AUDIO_PLUGIN_DSP_IND:
			/* Make sure this is the clock mismatch rate, sent from the DSP via the a2dp decoder common plugin */
			if (((AUDIO_PLUGIN_DSP_IND_T*)message)->id == KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE)
			{
				handleA2DPStoreClockMismatchRate(((AUDIO_PLUGIN_DSP_IND_T*)message)->value);
			}
			break;
        default:
            MAIN_DEBUG(("HS :  AUDIO ? [%x]\n",id)) ;
        break ;           
	}	
}

/*************************************************************************
NAME    
    app_handler
    
DESCRIPTION
    This is the main message handler for the Headset Application.  All
    messages pass through this handler to the subsequent handlers.

RETURNS

*/
static void app_handler(Task task, MessageId id, Message message)
{
    /*MAIN_DEBUG(("MSG [%x][%x][%x]\n", (int)task , (int)id , (int)&message)) ;*/
	
#ifdef ENABLE_ENERGY_FILTER
    if (id == MESSAGE_ENERGY_CHANGED) {
		Filter_EnergyChanged(message);
		return;
	}
#endif
	
    /* determine the message type based on base and offset */
    if ( ( id >= EVENTS_MESSAGE_BASE ) && ( id <= EVENTS_LAST_EVENT ) )
    {
        handleUEMessage(task, id,  message);          
    }
    else  if ( (id >= CL_MESSAGE_BASE) && (id <= CL_MESSAGE_TOP) )
    {
        handleCLMessage(task, id,  message);        
    #ifdef TEST_HARNESS 
        vm2host_connection(task, id, message);
    #endif 
    }
    else if ( (id >= HFP_MESSAGE_BASE ) && (id <= HFP_MESSAGE_TOP) )
    {     
        handleHFPMessage(task, id,  message);     
    #ifdef TEST_HARNESS 
        vm2host_hfp(task, id, message);
    #endif 
    }    
    else if ( (id >= CODEC_MESSAGE_BASE ) && (id <= CODEC_MESSAGE_TOP) )
    {     
        handleCodecMessage (task, id, message) ;     
    }
    else if ( (id >= POWER_MESSAGE_BASE ) && (id <= POWER_MESSAGE_TOP) )
    {     
        handleBatteryMessage (task, id, message) ;     
    }
#ifdef ENABLE_PBAP    
    else if ( (id >= PBAPC_INIT_CFM ) && (id <= PBAPC_MESSAGE_TOP) )
    {     
        handlePbapMessages (task, id, message) ;     
    }
#endif
    
#ifdef ENABLE_AVRCP
    else if ( (id >= AVRCP_INIT_CFM ) && (id <= AVRCP_MESSAGE_TOP) )
    {     
        headsetAvrcpHandleMessage (task, id, message) ;     
    }
#endif
    
#ifdef CVC_PRODTEST
    else if (id == MESSAGE_FROM_KALIMBA)
    {
        handleKalimbaMessage(task, id, message);
    }
#endif    
	else if ( (id >= A2DP_MESSAGE_BASE ) && (id <= A2DP_MESSAGE_TOP) )
    {     
        handleA2DPMessage(task, id,  message);
		return;
    }
	else if ( (id >= AUDIO_UPSTREAM_MESSAGE_BASE ) && (id <= AUDIO_UPSTREAM_MESSAGE_TOP) )
    {     
        handleAudioPluginMessage(task, id,  message);
		return;
    }    
#ifdef ENABLE_SPP
	else if ( (id >= SPP_MESSAGE_BASE ) && (id <= SPP_MESSAGE_TOP) )
	{	  
		handleSppMessage(task, id,	message);
		return;
	}	 
#endif
    else 
    { 
        MAIN_DEBUG(("MSGTYPE ? [%x]\n", id)) ;
    }       
}


/* The Headset Application starts here...*/
int main(void)
{  
	DEBUG (("Main [%s]\n",__TIME__));
	
    /* Initialise headset state */
	AuthResetConfirmationFlags();
    
    /*the internal regs must be latched on (smps and LDO)*/
    /*set the feature bits - these will be overwitten on configuration*/
    theHeadset.features.PowerOnSMPS    = TRUE ;
    theHeadset.features.PowerOnLDO    = TRUE ;
    PioSetPowerPin ( TRUE ) ;


#ifdef CVC_PRODTEST
    /* set boot mode to 4 to check cVc license key in production */
    if( BootGetMode() == 4) 
    {
        /* check to see if license key checking is enabled */
        uint16 * buffer = PanicUnlessMalloc( sizeof(uint16) * sizeof(feature_config_type));
        char* kap_file = NULL;
        uint16 audio_plugin;
        uint16 lConfigID = get_config_id ( PSKEY_CONFIGURATION_ID ) ;
	    
        ConfigRetrieve(lConfigID , PSKEY_FEATURE_BLOCK, buffer, sizeof(feature_config_type)) ; 	
	                
        audio_plugin = (buffer[3] >> 8) & 0x7;
        MAIN_DEBUG(("buffer[3] = [%x]\n", buffer[3]));
        MAIN_DEBUG(("audio_plugin = [%x]\n", audio_plugin));
	   
        switch (audio_plugin)
        {
            case 2:
                /* 1 mic cvc */
                /* security check is the same in 1 mic */
                /* and 1 mic wideband */
                kap_file = "cvc_headset/cvc_headset.kap";
                break;
            case 3:
                /* 2 mic cvc */
                /* security check is the same in 2 mic */
                /* and 2 mic wideband */
                kap_file = "cvc_headset_2mic/cvc_headset_2mic.kap";
                break;
            default:
                /* no dsp */
                /* pass thru */
                /* default */
                /* no cvc license key check required for these states */
                /* so exit the application now */
                free(buffer);
                exit(CVC_PRODTEST_NO_CHECK);
                break;
        }
            
        theHeadset.task.handler = app_handler;
        MessageKalimbaTask(&theHeadset.task);
            
        
        if (FileFind(FILE_ROOT,(const char *) kap_file ,strlen(kap_file)) == FILE_NONE)
        {
            free(buffer);
            exit(CVC_PRODTEST_FILE_NOT_FOUND);
        }
        else
        {
            DEBUG (("KalimbaLoad [%s]\n", kap_file));
            KalimbaLoad(FileFind(FILE_ROOT,(const char *) kap_file ,strlen(kap_file)));
        }
        
        DEBUG (("StreamConnect\n"));
        StreamConnect(StreamKalimbaSource(0), StreamHostSink(0));
           
        free(buffer);
               
    }
    
    /* added this code here to construct else if statement */
    /* with BootGetMode check below */
    else 
        
#endif
    
    if ( BootGetMode()  !=0 ) /* not in DFU mode or cVc license check mode*/
    {
            /* Set up the Application task handler */
        theHeadset.task.handler = app_handler;
 
            /* Initialise the Connection Library */
        ConnectionInit(&theHeadset.task);

#ifdef TEST_HARNESS
        test_init();
#endif
    }
    

    /* Start the message scheduler loop */
    MessageLoop();
        /* Never get here...*/
    return 0;  
}




#ifdef DEBUG_MALLOC 

#include "vm.h"

void * MallocPANIC ( const char file[], int line , size_t pSize )
{
    static uint16 lSize = 0 ;
    static uint16 lCalls = 0 ;
    void * lResult;
 
    lCalls++ ;
    lSize += pSize ;    
	printf("%s,l[%d][%d] s[%d] t[%d] a [%d]\n",file , line ,lCalls, pSize, lSize , (uint16)VmGetAvailableAllocations() ); 
                
    lResult = malloc ( pSize ) ;
      
        /*and panic if the malloc fails*/
    if ( lResult == NULL )
    {
        printf("MA : !\n") ;
        Panic() ;
    }
    
    return lResult ; 
                
}
#endif

/*************************************************************************
NAME    
    headsetInitCodecTask
    
DESCRIPTION
    Initialises the codec task

RETURNS

*/
static void headsetInitCodecTask ( void ) 
{
	/* The Connection Library has been successfully initialised, 
       initialise the HFP library to instantiate an instance of both
       the HFP and the HSP */
       			   
	/*CodecInitWolfson (Task appTask, wolfson_init_params *init) */
        
	/*init the codec task*/		
	CodecInitCsrInternal (&theHeadset.task) ;
}

/*************************************************************************
NAME    
    handleHFPStatusCFM
    
DESCRIPTION
    Handles a status response from the HFP and sends an error message if one was received

RETURNS

*/
static void handleHFPStatusCFM ( hfp_lib_status pStatus ) 
{

    if (pStatus != hfp_success )
    {
        MAIN_DEBUG(("HS: HFP CFM Err [%d]\n" , pStatus)) ;
        MessageSend ( &theHeadset.task , EventError , 0 ) ;
#ifdef ENABLE_PBAP
        if(theHeadset.pbap_dial_state != pbapc_no_dial)
        {
            MessageSend ( &theHeadset.task , EventPbapDialFail , 0 ) ; 
        } 
#endif        
    }
    else
    {
#ifdef ENABLE_PBAP
        if(theHeadset.pbap_dial_state != pbapc_no_dial)
        {
            MessageSend ( &theHeadset.task , EventPbapDialSuccessful , 0 ) ; 
        }
#endif             
    }

#ifdef ENABLE_PBAP
    theHeadset.pbap_dial_state = pbapc_no_dial;
#endif
}

