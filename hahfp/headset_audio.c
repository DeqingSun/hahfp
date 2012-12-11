/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010

FILE NAME
    headset_audio.c

DESCRIPTION
    This file handles all Synchronous connection messages

NOTES

*/


/****************************************************************************
    Header files
*/
#include "headset_private.h"
#include "headset_audio.h"
#include "headset_statemanager.h"
#include "headset_pio.h"
#include "headset_slc.h"
#include "headset_energy_filter.h"
#include "headset_tones.h"
#include "headset_volume.h"

#include <connection.h>
#include <hfp.h>
#include <stdlib.h>
#include <audio.h>
#include <audio_plugin_if.h>
	
#include <csr_cvc_common_plugin.h>
#include <hearing_aid_sim_plugin.h>

    #define NODSPCVSD		(TaskData *)&csr_cvsd_no_dsp_plugin

#ifdef INCLUDE_DSP

	#include <csr_common_example_plugin.h>

    #define CVC1MIC	    	     (TaskData *)&csr_cvsd_cvc_1mic_headset_plugin	
	#define CVSD1MIC_EXAMPLE	(TaskData *)&csr_cvsd_8k_1mic_plugin	
    #define CVC2MIC    	        (TaskData *)&csr_cvsd_cvc_2mic_headset_plugin
    #define SBC1MIC_EXAMPLE		(TaskData *)&csr_sbc_1mic_plugin
    #define CVSD2MIC_EXAMPLE	(TaskData *)&csr_cvsd_8k_2mic_plugin
    #define SBC2MIC_EXAMPLE     (TaskData *)&csr_sbc_2mic_plugin

	#define MAX_NUM_AUDIO_PLUGINS_PER_CODEC	(6)

	/* the column to use is selected by user PSKEY
	   the row depends upon the audio link (codec) negotiated */
	TaskData * const gCsrPlugins [] [ MAX_NUM_AUDIO_PLUGINS_PER_CODEC ] = 
		{   /*	 0			1			      2			  3	        4                    5*/
	/*CVSD*/	{NODSPCVSD, CVSD1MIC_EXAMPLE, CVC1MIC, CVC2MIC,  CVSD2MIC_EXAMPLE ,   NULL  } ,
    /*MSBC*/    {NULL     , SBC1MIC_EXAMPLE , NULL   , NULL   ,  SBC2MIC_EXAMPLE  ,   NULL  } ,
		};  
#else

	#define MAX_NUM_AUDIO_PLUGINS_PER_CODEC	(1)
	
	/* the column to use is selected by user PSKEY
	   the row depends upon the audio link (codec) negotiated */
	TaskData * const gCsrPlugins [] [ MAX_NUM_AUDIO_PLUGINS_PER_CODEC ] = 
	{	/*	0	*/
	/*CVSD*/	{NODSPCVSD},
	/*SBC*/		{NULL},
	};  

#endif	

#define	NUM_CSR_CODECS		(sizeof(gCsrPlugins)/(sizeof(TaskData*)*MAX_NUM_AUDIO_PLUGINS_PER_CODEC))

#ifdef DEBUG_AUDIO

#ifdef DEBUG_PRINT_ENABLED
    static const char * const gScoPacketStrings [ 10 ]= {
                        "hv1",
                        "hv2",
                        "hv3",
                        "ev3",
                        "ev4",
                        "ev5",
                        "2ev3",
                        "3ev3",
                        "2ev5",
                        "3ev5"
                        };    
#endif

#define AUD_DEBUG(x) DEBUG(x)
#else
#define AUD_DEBUG(x) 
#endif      


/****************************************************************************
NAME    
    audioGetLinkPriority
    
DESCRIPTION
	Common method of getting the link we want to manipulate audio settings on

RETURNS
    
*/
hfp_link_priority audioGetLinkPriority ( bool audio )
{
    hfp_link_priority priority;
    
    /* See if we can get a link from the headset audio sink... */
    priority = HfpLinkPriorityFromAudioSink(theHeadset.sco_sink);

    /* If that fails see if we have an active call... */
    if(!priority)
        priority = HfpLinkPriorityWithActiveCall(audio);
    
    /* If we got something return it, otherwise return primary link */
    return (priority ? priority : hfp_primary_link);
}


/****************************************************************************
NAME    
    audioHandleRouting
    
DESCRIPTION
	Handle the routing of the audio connections or connection based on
    sco priority level

RETURNS
    
*/
void audioHandleRouting ( void )
{   
    Sink sinkAG1, sinkAG2; 
    Sink a2dpSinkPri = 0;
    Sink a2dpSinkSec = 0;
    a2dp_stream_state a2dpStatePri = a2dp_stream_idle;
    a2dp_stream_state a2dpStateSec = a2dp_stream_idle;
    hfp_call_state stateAG1 = hfp_call_state_idle;
    hfp_call_state stateAG2 = hfp_call_state_idle;  

    /* get AG1 and AG2 call current states if connected */
    HfpLinkGetCallState(hfp_primary_link, &stateAG1);
    HfpLinkGetCallState(hfp_secondary_link, &stateAG2);
    
    /* get audio sink for AG1 if available */
    if(!(HfpLinkGetAudioSink(hfp_primary_link, &sinkAG1)))
        sinkAG1 = 0;

    /* get audio sink for AG2 if available */
    if(!(HfpLinkGetAudioSink(hfp_secondary_link, &sinkAG2)))
        sinkAG2 = 0;

    AUD_DEBUG(("AUD: route - A2dpPriId = %x, A2dpSecId = %x \n" , theHeadset.a2dp_link_data->device_id[a2dp_primary],theHeadset.a2dp_link_data->device_id[a2dp_secondary])) ;   
    
    /* if a2dp connected obtain the current streaming state for primary a2dp connection */
    if(theHeadset.a2dp_link_data->connected[a2dp_primary])
    {
        a2dpStatePri = A2dpMediaGetState(theHeadset.a2dp_link_data->device_id[a2dp_primary], theHeadset.a2dp_link_data->stream_id[a2dp_primary]) ;
        a2dpSinkPri = A2dpMediaGetSink(theHeadset.a2dp_link_data->device_id[a2dp_primary], theHeadset.a2dp_link_data->stream_id[a2dp_primary]) ;
    }    

    /* if a2dp connected obtain the current streaming state for secondary a2dp connection */
    if(theHeadset.a2dp_link_data->connected[a2dp_secondary])
    {
        a2dpStateSec = A2dpMediaGetState(theHeadset.a2dp_link_data->device_id[a2dp_secondary], theHeadset.a2dp_link_data->stream_id[a2dp_secondary]) ;
        a2dpSinkSec = A2dpMediaGetSink(theHeadset.a2dp_link_data->device_id[a2dp_secondary], theHeadset.a2dp_link_data->stream_id[a2dp_secondary]) ;
    }   
    
    AUD_DEBUG(("AUD: route - A2dpPri Sig State = %x, A2dpSec Sig State = %x :\n" , (uint16)A2dpSignallingGetState(a2dp_primary),(uint16)A2dpSignallingGetState(a2dp_secondary))) ;   
    AUD_DEBUG(("AUD: route - A2dpPri State = %x, A2dpSec State = %x :\n" , (uint16)a2dpStatePri,(uint16)a2dpStateSec)) ;   
    AUD_DEBUG(("AUD: route - sinkAG1 = [%d] sinkAG2 = [%d]:\n" , (uint16)sinkAG1,(uint16)sinkAG2 )) ;   

    /* is there currently audio routed ? */
    if(theHeadset.sco_sink)
    {
    	AUD_DEBUG(("AUD: route - got sco sink = [%x], A2dpPri = %x, A2dpSec = %x :\n" , (uint16)theHeadset.sco_sink,(uint16)a2dpSinkPri,(uint16)a2dpSinkSec)) ;   
    	AUD_DEBUG(("AUD: route - got sco sink, hfpPriority = %x \n" , HfpLinkPriorityFromAudioSink(theHeadset.sco_sink))) ;           
        /* is the current sco_sink still valid ?, hfp or a2dp */    
        if(((theHeadset.sco_sink == a2dpSinkPri)&&(a2dpStatePri != a2dp_stream_streaming))||
           ((theHeadset.sco_sink == a2dpSinkSec)&&(a2dpStateSec != a2dp_stream_streaming))||
           ((((sinkAG1)||(sinkAG2))&&((theHeadset.sco_sink == a2dpSinkPri)||(theHeadset.sco_sink == a2dpSinkSec))))||
           ((theHeadset.sco_sink != a2dpSinkPri)&&(theHeadset.sco_sink != a2dpSinkSec)&&(theHeadset.sco_sink != sinkAG1)&&(theHeadset.sco_sink != sinkAG2)))
        {    
        	AUD_DEBUG(("AUD: route - audio disconnect = [%d] :\n" , (uint16)theHeadset.sco_sink)) ;   
            /* sco no longer valid so disconnect sco */
            AudioDisconnect();
            /* clear sco_sink value to indicate no routed audio */
            theHeadset.sco_sink = 0;   
			theHeadset.dsp_plugin = NULL;
        }
    }

    /***********************************************************************************/
    /**** Process the A2DP audio routing prior to processing the Call audio routing ****/
    /**** to give a faster turnaround on routing Call audio                         ****/
    /***********************************************************************************/
    
    /* check for the precense of any A2dp streams and determine whether these need
       routing, disconnecting or suspending */   
    if((((theHeadset.sco_sink)&&(sinkAG1||sinkAG2))||
        (stateAG1 > hfp_call_state_idle)||
        (stateAG2 > hfp_call_state_idle))&&
       ((a2dpStatePri == a2dp_stream_streaming)||(a2dpStateSec == a2dp_stream_streaming)))
    {
     	AUD_DEBUG(("AUD: route - sco and a2dp streaming PriSt=%x PriSS=%x SecSt=%x SecSS=%x:\n",a2dpStatePri,theHeadset.a2dp_link_data->SuspendState[a2dp_primary],a2dpStateSec,theHeadset.a2dp_link_data->SuspendState[a2dp_secondary])) ;   
        
        /* sco exists so suspend the a2dp streaming on Pri channel if existing*/
        if((a2dpStatePri == a2dp_stream_streaming)&&(!theHeadset.a2dp_link_data->SuspendState[a2dp_primary]))
        {
         	AUD_DEBUG(("AUD: route - sco and a2dp streaming suspend pri\n")) ;   
            /* suspend or close media stream */
            SuspendA2dpStream(a2dp_primary);
        }
        /* media streaming has resumed by src desipte the headset wanting it to be suspended, therefore
           close it */
        else if(a2dpStatePri == a2dp_stream_streaming)
        {
         	AUD_DEBUG(("AUD: route - sco and a2dp streaming close pri\n")) ;   
            /* close media streaming */
            A2dpMediaCloseRequest(theHeadset.a2dp_link_data->device_id[a2dp_primary], theHeadset.a2dp_link_data->stream_id[a2dp_primary]);
        }
        /* sco exists so suspend the a2dp streaming on Sec channel if existing */
        if((a2dpStateSec == a2dp_stream_streaming)&&(!theHeadset.a2dp_link_data->SuspendState[a2dp_secondary]))
        {
         	AUD_DEBUG(("AUD: route - sco and a2dp streaming suspend sec\n")) ;   
            /* suspend or close media stream */
            SuspendA2dpStream(a2dp_secondary);
        }
        else if(a2dpStateSec == a2dp_stream_streaming)
        {
         	AUD_DEBUG(("AUD: route - sco and a2dp streaming close sec\n")) ;   
            /* close media streaming */
            A2dpMediaCloseRequest(theHeadset.a2dp_link_data->device_id[a2dp_secondary], theHeadset.a2dp_link_data->stream_id[a2dp_secondary]);            
        }

        /* sink now no longer valid so disconnect sco */
        AudioDisconnect();
        /* clear sco_sink value to indicate no routed audio */
        theHeadset.sco_sink = 0; 
		theHeadset.dsp_plugin = NULL;
    }
    /* are there any suspended a2dp streaming connections?, if there is no sco_sink then this can be restarted */
    else if(((!theHeadset.sco_sink)&&(!stateAG1)&&(!stateAG2))&&
            ((theHeadset.a2dp_link_data->SuspendState[a2dp_primary])||(theHeadset.a2dp_link_data->SuspendState[a2dp_secondary])))
    {    
     	AUD_DEBUG(("AUD: route - no sco and a2dp suspended :\n" )) ;   
        /* there is now no sco sink but an a2dp link was suspended as a result of an active
           call from another phone so reconnect/restream previously disconnected a2dp connection */
        if(theHeadset.a2dp_link_data->SuspendState[a2dp_primary])
        {
            /* need to check whether the signalling channel hsa been dropped by the AV/AG source */
            if(A2dpSignallingGetState(theHeadset.a2dp_link_data->device_id[a2dp_primary]) == a2dp_signalling_connected)
            {
                /* is media channel still open? or is it streaming already? */
                if(a2dpStatePri == a2dp_stream_open)
                {
                   	AUD_DEBUG(("AUD: route - pri no sco and a2dp suspended media start:\n" )) ;   
                    A2dpMediaStartRequest(theHeadset.a2dp_link_data->device_id[a2dp_primary], theHeadset.a2dp_link_data->stream_id[a2dp_primary]);
                    /* reset the SuspendState indicator */
                    theHeadset.a2dp_link_data->SuspendState[theHeadset.a2dp_link_data->device_id[a2dp_primary]] = FALSE;
                }
                /* media channel wasn't open, source not supporting suspend */            
                else if(a2dpStatePri < a2dp_stream_open) 
                {
                   	AUD_DEBUG(("AUD: route - pri no sco and a2dp suspended media open:\n" )) ;   
                    A2dpMediaOpenRequest(theHeadset.a2dp_link_data->device_id[a2dp_primary]);
                }
                /* recovery if media has resumed streaming reconnect its audio */
                else if(a2dpStatePri == a2dp_stream_streaming)
                {
                    theHeadset.a2dp_link_data->SuspendState[a2dp_primary] = FALSE;
                    A2dpRouteAudio(a2dp_primary, a2dpSinkPri);
                }
            }
            /* signalling channel is no longer present so attempt to reconnect it */
            else
            {
               	AUD_DEBUG(("AUD: route - pri no sco and a2dp signalling closed:\n" )) ;   
                A2dpSignallingConnectRequest(&theHeadset.a2dp_link_data->bd_addr[a2dp_primary]);
            }
        }
        /* no primary so is there a secondary suspended? */
        else if(theHeadset.a2dp_link_data->SuspendState[a2dp_secondary])
        {
            /* need to check whether the signalling channel hsa been dropped by the AV/AG source */
            if(A2dpSignallingGetState(theHeadset.a2dp_link_data->device_id[a2dp_secondary]) == a2dp_signalling_connected)
            {
                /* is media channel still open? or is it streaming already? */
                if(a2dpStateSec == a2dp_stream_open)
                {
                    AUD_DEBUG(("AUD: route - sec no sco and a2dp suspended media start:\n" )) ;   
                    A2dpMediaStartRequest(theHeadset.a2dp_link_data->device_id[a2dp_secondary], theHeadset.a2dp_link_data->stream_id[a2dp_secondary]);
                    /* reset the SuspendState indicator */
                    theHeadset.a2dp_link_data->SuspendState[theHeadset.a2dp_link_data->device_id[a2dp_secondary]] = FALSE;
                }
                /* media channel wasn't open, source not supporting suspend */            
                else if(a2dpStateSec < a2dp_stream_open) 
                {
                   	AUD_DEBUG(("AUD: route - sec no sco and a2dp suspended media open:\n" )) ;   
                    A2dpMediaOpenRequest(theHeadset.a2dp_link_data->device_id[a2dp_secondary]);
                }
                /* recovery if media has resumed streaming reconnect its audio */
                else if(a2dpStateSec == a2dp_stream_streaming)
                {
                    theHeadset.a2dp_link_data->SuspendState[a2dp_secondary] = FALSE;
                    A2dpRouteAudio(a2dp_secondary, a2dpSinkSec);
                }
            }                               
            /* signalling channel is no longer present so attempt to reconnect it */
            else
            {
               	AUD_DEBUG(("AUD: route - sec no sco and a2dp signalling closed:\n" )) ;   
                A2dpSignallingConnectRequest(&theHeadset.a2dp_link_data->bd_addr[a2dp_secondary]);
            }
        }
    }
    /* are there any streaming connections that need the audio routing ? */
    else if((!theHeadset.sco_sink)&&(!stateAG1)&&(!stateAG2)&&(a2dpStatePri == a2dp_stream_streaming))
    {
     	AUD_DEBUG(("AUD: route - no sink and a2dp streaming connect Pri A2dp:\n" )) ;           
        A2dpRouteAudio(a2dp_primary, a2dpSinkPri);
    }
    else if((!theHeadset.sco_sink)&&(!stateAG1)&&(!stateAG2)&&(a2dpStateSec == a2dp_stream_streaming))
    {
     	AUD_DEBUG(("AUD: route - no sink and a2dp streaming connect Sec A2dp:\n" )) ;           
        A2dpRouteAudio(a2dp_secondary, a2dpSinkSec);
    }
    
    /***********************************************************************************/
    /**** Process the SCO audio routing if not routing a2dp media                   ****/
    /***********************************************************************************/

    /* ensure not currently routing a2dp before attempting to route sco audio */
    if((!theHeadset.sco_sink) ||
       ((theHeadset.sco_sink)&&(theHeadset.sco_sink != a2dpSinkPri)&&(theHeadset.sco_sink != a2dpSinkSec)))
    {
        /* are there any sco's avaialable ? are two scos available ?*/
        if(sinkAG1 && sinkAG2)
        {
          	 AUD_DEBUG(("AUD: route - two scos present = [%d] [%d] :\n" , (uint16)sinkAG1, (uint16)sinkAG2)) ;   
             /* determine which is the highest priority sco sink of the two and route that */   
             if(getScoPriorityFromHfpPriority(hfp_secondary_link) > getScoPriorityFromHfpPriority(hfp_primary_link))
             {
                 	AUD_DEBUG(("AUD: route - sec > pri = [%d] [%d] :\n" , (uint16)getScoPriorityFromHfpPriority(hfp_primary_link), (uint16)getScoPriorityFromHfpPriority(hfp_secondary_link))) ;   
                    /* is there a sco still routed?,  if so is it this one?, if not connect primary */
                    if((theHeadset.sco_sink) && (theHeadset.sco_sink != sinkAG2))
                    {
                    	AUD_DEBUG(("AUD: route - sec > pri - audio disconnect = [%d] :\n" , (uint16)theHeadset.sco_sink)) ;   
                        /* need to change sco */
                        AudioDisconnect();
                        /* clear sco_sink value to indicate no routed audio */
                        theHeadset.sco_sink = 0;                    
						theHeadset.dsp_plugin = NULL;
                    }
                    /* if no audio routed then connect it up, it could already be routed to intended sco */
                    if(!theHeadset.sco_sink)
                    {
                        /* update headset sco sink value */
                        theHeadset.sco_sink = sinkAG2;
                    	AUD_DEBUG(("AUD: route - sec > pri - audio connect = [%d] :\n" , (uint16)theHeadset.sco_sink)) ;   
                        /* perform the audio connection */
                        audioHfpConnectAudio(hfp_secondary_link);                                                                                                                                  
                    }
             }
             /* primary is higher priority so route that instead */
             else
             {
               	AUD_DEBUG(("AUD: route - pri > sec = [%d] [%d] :\n" , (uint16)getScoPriorityFromHfpPriority(hfp_primary_link), (uint16)getScoPriorityFromHfpPriority(hfp_secondary_link))) ;   
                 /* is there a sco still routed?,  if so is it this one?, if not connect secondary */
                 if((theHeadset.sco_sink) && (theHeadset.sco_sink != sinkAG1))
                 {
                    	AUD_DEBUG(("AUD: route - pri > sec - audio disconnect = [%d] :\n" , (uint16)theHeadset.sco_sink)) ;   
                        /* need to change sco */
                        AudioDisconnect();
                        /* clear sco_sink value to indicate no routed audio */
                        theHeadset.sco_sink = 0;                    
						theHeadset.dsp_plugin = NULL;
                 }
                 /* if no audio routed then connect it up, it could already be routed to intended sco */
                 if(!theHeadset.sco_sink)
                 { 
                     /* update headset sco sink value */
                     theHeadset.sco_sink = sinkAG1;
                     AUD_DEBUG(("AUD: route - sec > pri - audio connect = [%d] :\n" , (uint16)theHeadset.sco_sink)) ;   
                     /* perform the audio connection */
                     audioHfpConnectAudio(hfp_primary_link);                                                                                                                                   
                 }
             }
        }
        /* is primary only sco available ? */
        else if(sinkAG1)
        {
            /* is there a sco still routed?,  if so is it this one?, if not connect secondary */
            if((theHeadset.sco_sink) && (theHeadset.sco_sink != sinkAG1))
            {
               	AUD_DEBUG(("AUD: route - pri only - audio disconnect = [%d] :\n" , (uint16)theHeadset.sco_sink)) ;   
                /* need to change sco */
                AudioDisconnect();
                /* clear sco_sink value to indicate no routed audio */
                theHeadset.sco_sink = 0;                    
				theHeadset.dsp_plugin = NULL;
            }
            /* if no audio routed then connect it up, it could already be routed to intended sco */
            if(!theHeadset.sco_sink)
            {
                /* update headset sco sink value */
                theHeadset.sco_sink = sinkAG1;
                AUD_DEBUG(("AUD: route - pri only - audio connect = [%d] :\n" , (uint16)theHeadset.sco_sink)) ;   
                /* perform the audio connection */
                audioHfpConnectAudio(hfp_primary_link);                                                                                                                    
            }
        }
        /* is seconary sco only available ? */
        else if(sinkAG2)
        {
            /* is there a sco still routed?,  if so is it this one?, if not connect secondary */
            if((theHeadset.sco_sink) && (theHeadset.sco_sink != sinkAG2))
            {
               	AUD_DEBUG(("AUD: route - sec only - audio disconnect = [%d] :\n" , (uint16)theHeadset.sco_sink)) ;   
                /* need to change sco */
                AudioDisconnect();
                /* clear sco_sink value to indicate no routed audio */
                theHeadset.sco_sink = 0;                    
				theHeadset.dsp_plugin = NULL;
            }
            /* if no audio routed then connect it up, it could already be routed to intended sco */
            if(!theHeadset.sco_sink)
            {
                /* update headset sco sink value */
                theHeadset.sco_sink = sinkAG2;
                AUD_DEBUG(("AUD: route - sec only - audio connect = [%d] :\n" , (uint16)theHeadset.sco_sink)) ;   
                /* perform the audio connection */
                audioHfpConnectAudio(hfp_secondary_link);                                                                                                                                
            }
        }
        /* no sco is available */
        else if(theHeadset.sco_sink)
        {
            /* no sco's available so do nothing */        
            AudioDisconnect();
            /* clear sco_sink value to indicate no routed audio */
            theHeadset.sco_sink = 0;                    
			theHeadset.dsp_plugin = NULL;
        }
    }    
	
	if(theHeadset.dsp_plugin == NULL)
	{
		audioEnterLoopbackMode();
	}
}

/****************************************************************************
NAME    
    audioHandleSyncConnectInd
    
DESCRIPTION
	Handle HFP_AUDIO_CONNECT_IND.  This indicates that an incoming sychronous 
	connection is being requested

RETURNS
    
*/
void audioHandleSyncConnectInd ( const HFP_AUDIO_CONNECT_IND_T *pInd )
{    
    hfp_audio_params lParams;
    Sink sink;
    
    /* Get the priority of the other link */
    hfp_link_priority priority = (pInd->priority == hfp_primary_link) ? hfp_secondary_link : hfp_primary_link;
    
	unsigned int packet_types = theHeadset.HFP_supp_features.supportedSyncPacketTypes;
    
	AUD_DEBUG(("AUD: Synchronous Connect Ind [%d] from [%x]:\n" , pInd->codec, PROFILE_INDEX(pInd->priority))) ;   

    /* if the headset configuration has additional link parameters specfied pass these
       through, these params are not used in the case of wbs codec use */
	if (theHeadset.HFP_supp_features.Additional_Parameters_Enabled )
    {
		lParams.bandwidth		= theHeadset.HFP_supp_features.bandwidth ;	  
		lParams.max_latency 	= theHeadset.HFP_supp_features.max_latency ;
		lParams.voice_settings	= theHeadset.HFP_supp_features.voice_settings ;
		lParams.retx_effort 	= theHeadset.HFP_supp_features.retx_effort ;  
	
		AUD_DEBUG(("AUD : [%x][%x][%x][%x]\n" , (int)lParams.bandwidth ,lParams.max_latency   ,
												lParams.voice_settings ,(int)lParams.retx_effort)) ;
	}
    
    /* if this is the second sco request limit its choice of packet type to EV3 S1 or HV3 settings only
       this is to prevent to the reserved slot violation problems seen on some phones when trying to use
       2 x 2EV3 links */
    if( (theHeadset.MultipointEnable) && (theHeadset.features.ForceEV3S1ForSco2) && (HfpLinkGetAudioSink(priority, &sink)) )
    {
        /* set up for safe EV3 S1 parameters or hv3 should esco not be available */    
        lParams.bandwidth		= 8000 ;	  
     	lParams.max_latency 	= 7;
		lParams.voice_settings	= sync_air_coding_cvsd ;
		lParams.retx_effort 	= sync_retx_power_usage ;
		packet_types			= (sync_hv3 | sync_ev3 | sync_all_edr_esco );
        /* request EV3 S1 packet type instead */
        HfpAudioConnectResponse(pInd->priority, TRUE, packet_types, &lParams, FALSE);
    }
    else
    {
        /* test case, force sco negotiaion failure */
        if(!theHeadset.FailAudioNegotiation)        
        {
            AUD_DEBUG(("AUD Pass Audio Negotiation\n"));
		    HfpAudioConnectResponse(pInd->priority, TRUE, packet_types, &lParams, FALSE);
        }
        else
        {
            HfpAudioConnectResponse(pInd->priority, TRUE, sync_all_sco, &lParams, TRUE);
            AUD_DEBUG(("AUD Fail Audio Negotiation\n"));
        }
    }
}


/****************************************************************************
NAME    
    audioHandleSyncConnectCfm
    
DESCRIPTION
	Handle HFP_AUDIO_CONNECT_CFM.  This indicates that an incoming sychronous 
	connection has been established

RETURNS
    
*/
void audioHandleSyncConnectCfm ( const HFP_AUDIO_CONNECT_CFM_T * pCfm )
{	    
    uint8 index = PROFILE_INDEX(pCfm->priority);
    hfp_call_state CallState;
    
	AUD_DEBUG(("Synchronous Connect Cfm from [%x]:\n", (uint16)pCfm->priority)) ;
 
    /* if successful */
    if ( pCfm->status == hfp_success)
    {      
        Sink sink;
        
        /* obtain sink for this audio connection */
        if(HfpLinkGetSlcSink(pCfm->priority, &sink))
        {
            /* Send our link policy settings for active SCO role */
        	slcSetLinkPolicy(pCfm->priority, sink);
        }

		/* store in individual hfp struct as it may be necessary to disconnect and reconnect
           audio on a per hfp basis for multipoint multiparty calling */
		theHeadset.profile_data[index].audio.tx_bandwidth= pCfm->tx_bandwidth;
        theHeadset.profile_data[index].audio.link_type= pCfm->link_type;           
        theHeadset.profile_data[index].audio.codec_selected = pCfm->codec;           
         		
        /* Send an event to indicate that a SCO has been opened */           
        /* this indicates that an audio connection has been successfully created to the AG*/
    	MessageSend ( &theHeadset.task , EventSCOLinkOpen , 0 ) ;

        /* update the audio priority state 
           is this sco a streaming audio sco?, check call state for this ag */
        if(HfpLinkGetCallState(pCfm->priority, &CallState))
        {
            /* determine sco priority based on call status */
            switch(CallState)
            {
                /* no call so this is a steaming audio connection */
                case hfp_call_state_idle:
                    setScoPriorityFromHfpPriority(pCfm->priority, sco_streaming_audio);
                break;
                
                /* incoming call so this is an inband ring sco */
                case hfp_call_state_incoming:
                case hfp_call_state_incoming_held:
                case hfp_call_state_twc_incoming:
                    setScoPriorityFromHfpPriority(pCfm->priority, sco_inband_ring);                    
                break;
   
                /* active call states so this sco has highest priority */
                case hfp_call_state_outgoing:
                case hfp_call_state_active:
                case hfp_call_state_twc_outgoing:
                    setScoPriorityFromHfpPriority(pCfm->priority, sco_active_call);                    
                break;
                
                /* this call is held so the sco is put to on hold priority which is lower than
                   active call but higher than streaming */
                case hfp_call_state_held_active:
                case hfp_call_state_held_remaining:
                    setScoPriorityFromHfpPriority(pCfm->priority, sco_held_call);                    
                break;
   
                /* non covered states treat as highest priority sco connection */
                default:
                    setScoPriorityFromHfpPriority(pCfm->priority, sco_active_call);                    
                break;
            }
        }
        
        /* route the appropriate audio connection */
        audioHandleRouting();

        /*change the active call state if necessary*/
        if ((stateManagerGetState() == headsetActiveCallNoSCO) )
        {
            stateManagerEnterActiveCallState();
        }
                       
#ifdef DEBUG_AUDIO
        switch (pCfm->link_type)
        {
            case (sync_link_unknown):
                AUD_DEBUG(("AUD: Link = ?\n")) ;
            break ;
            case (sync_link_sco) :
                AUD_DEBUG(("AUD: Link = SCO\n")) ;
            break;
            case sync_link_esco:
                AUD_DEBUG(("AUD: Link = eSCO\n")) ;
            break ;    
        }
#endif        

    }
    else
    {
    	AUD_DEBUG(("Synchronous Connect Cfm: FAILED\n")) ;       
    }
    AUD_DEBUG(("AUD : Sco->\n")) ;
}


/****************************************************************************
NAME    
    audioHandleSyncDisconnectInd
    
DESCRIPTION
	Handle HFP_AUDIO_DISCONNECT_IND.  This indicates that an incoming sychronous 
	connection has been disconnected

RETURNS
    
*/
void audioHandleSyncDisconnectInd ( const HFP_AUDIO_DISCONNECT_IND_T * pInd )
{
    Sink sink;   
    AUD_DEBUG(("AUD: Synchronous Disconnect Ind [%x]:\n",pInd->priority)) ;
    
#ifdef ENABLE_ENERGY_FILTER
    if(theHeadset.iir_enabled)
	    Filter_Off();
#endif	

    /* ensure disconnection was succesfull */
    if(pInd->status == hfp_audio_disconnect_success)
    {
       
        MessageSend ( &theHeadset.task , EventSCOLinkClose , 0 ) ;
	
        /*set the mic bias*/
        PioSetMicrophoneBias ( FALSE ) ;
	
        if (theHeadset.PIO->HeadsetActivePIO != 0xF )
        {
            PioSetPio ( theHeadset.PIO->HeadsetActivePIO , FALSE) ;
        }    

        /* update sco priority */
        setScoPriorityFromHfpPriority(pInd->priority, sco_none);                    

        AUD_DEBUG(("AUD: Synchronous Disconnect Ind [%x]: sco_pri = %d\n",pInd->priority, HfpLinkPriorityFromAudioSink(theHeadset.sco_sink) )) ;
        /* if this indication is the disconnection of the currently routed audio, disconnect it */
        if(HfpLinkPriorityFromAudioSink(theHeadset.sco_sink) == pInd->priority)
        {
            /* disconnect audio */
            AudioDisconnect();
            /* clear sco_sink value to indicate no routed audio */
            theHeadset.sco_sink = 0;                    
			theHeadset.dsp_plugin = NULL;
        }
    
        /* deroute the audio */
        audioHandleRouting();    

        /*change the active call state if necessary*/
        if ((stateManagerGetState() == headsetActiveCallSCO))
        {
            stateManagerEnterActiveCallState();
        }
  
        /*if we are muted - then un mute at disconnection*/
        if (theHeadset.gMuted )
        {
            MessageSend(&theHeadset.task , EventMuteOff , 0) ;   
	    }
    
        /* Send our link policy settings for normal role  now SCO is dropped */
        if(HfpLinkGetSlcSink(pInd->priority, &sink))
          	slcSetLinkPolicy(pInd->priority, sink);
    }
    
}

bool audioIsLoopbackMode(void)
{
	return (theHeadset.dsp_plugin == (TaskData *)&hearing_aid_sim_plugin);
}

void audioEnterLoopbackMode(void)
{
	AUD_DEBUG(("AUD: audioEnterLoopbackMode (vol=%d)\n",theHeadset.ha_volume));
	theHeadset.dsp_plugin = (TaskData *)&hearing_aid_sim_plugin;
	/* go into loopback mode */
	AudioConnect ( (TaskData *)&hearing_aid_sim_plugin,
				   NULL ,
				   0 ,
				   theHeadset.codec_task ,
				   theHeadset.ha_volume ,
				   16000 ,
				   TRUE,
				   AUDIO_MODE_CONNECTED,
				   AUDIO_ROUTE_INTERNAL,
				   LBIPMPowerLevel(),
				   NULL,
				   NULL ) ; 	  

}

void audioExitLoopbackMode(void)
{
	AUD_DEBUG(("AUD: audioExitLoopbackMode\n"));
    /* disable loopback */
	if(audioIsLoopbackMode())
        AudioDisconnect();
}

/****************************************************************************
NAME    
    audioHfpConnectAudio
    
DESCRIPTION
	attempt to reconnect an audio connection from the sink value associcated 
    with the passed hfp instance

RETURNS
    
*/
void audioHfpConnectAudio (hfp_link_priority priority )
{    
    uint8 index = PROFILE_INDEX(priority);
    Sink sink;
    
    /*the mode to connect - connected as default*/
    AUDIO_MODE_T lMode = AUDIO_MODE_CONNECTED ;
    
    /* get audio sink for AG1 if available */
    if(HfpLinkGetAudioSink(priority, &sink))
        theHeadset.sco_sink = sink;
    
    /* ensure sink is valid before trying to route audio */
    if ( theHeadset.sco_sink ) 
	{
		TaskData	*plugin = NULL;
		TaskData	* const (*gPlugins)[MAX_NUM_AUDIO_PLUGINS_PER_CODEC]; /* Pointer to an array. Will be a pointer to a row of the plugin table. */

		gPlugins = NULL;

        /* ensure a valid codec is negotiated, should be at least cvsd */
        if(theHeadset.profile_data[index].audio.codec_selected)       
        {                
            /* select row in plugin table, cvsd is row 0, msbc is row 1 currently */
        	gPlugins = &gCsrPlugins[theHeadset.profile_data[index].audio.codec_selected - hfp_wbs_codec_mask_cvsd];

            AUD_DEBUG(("AUD: gplugins [%x] \n" , (uint16)gPlugins));
                       
    		/* Plugin row found? */
    		if((gPlugins != NULL) && (theHeadset.features.audio_plugin <= MAX_NUM_AUDIO_PLUGINS_PER_CODEC))
    			plugin = (*gPlugins)[theHeadset.features.audio_plugin]; /* Now reference the plugin based on the plugin selected (column). */

            AUD_DEBUG(("AUD: plugin [%x] \n" , (uint16)plugin));

	    	AUD_DEBUG(("AUD: plugin [%d] [%d], sink [%x]\n" , theHeadset.features.audio_plugin 
                                                            , theHeadset.profile_data[index].audio.codec_selected
                                                            , (uint16)theHeadset.sco_sink)) ;
			if(theHeadset.dsp_plugin)
				AudioDisconnect();
			theHeadset.dsp_plugin = plugin;
            /* connect audio using the audio plugin selected above */            
    		AudioConnect ( plugin,
           				   theHeadset.sco_sink  ,
               	           theHeadset.profile_data[index].audio.link_type ,
                           theHeadset.codec_task ,
                   	       theHeadset.audioData.gVolMaps[ theHeadset.profile_data[index].audio.gSMVolumeLevel ].VolGain ,
                       	   theHeadset.profile_data[index].audio.tx_bandwidth ,
                           theHeadset.features.stereo/*FALSE*/ ,
                           AUDIO_MODE_CONNECTED,
                           AUDIO_ROUTE_INTERNAL,
                           LBIPMPowerLevel(),
                           NULL,
                           NULL ) ;       
            
#ifdef ENABLE_ENERGY_FILTER	
            if(theHeadset.iir_enabled)
        		Filter_On();
#endif
            AUD_DEBUG(("AUD: Route SCO\n"));

         
            /*mute  control*/
            if (theHeadset.gMuted )
                 lMode = AUDIO_MODE_MUTE_MIC ;
	
            AUD_DEBUG(("Audio Connect[%d][%x]\n", theHeadset.features.audio_plugin , theHeadset.profile_data[index].audio.gSMVolumeLevel )) ;
	   
            AudioSetVolume ( theHeadset.audioData.gVolMaps[ theHeadset.profile_data[index].audio.gSMVolumeLevel ].VolGain , theHeadset.codec_task ) ;
            AudioSetMode   ( lMode, NULL ) ;
   
            /* Headset active PIO is asserted if configured */
            if (theHeadset.PIO->HeadsetActivePIO != 0xF )
            {
               PioSetPio ( theHeadset.PIO->HeadsetActivePIO , TRUE) ;
            }   

            /*set the mic bias*/
            PioSetMicrophoneBias ( TRUE ) ;   
            
            
        }
    }
}

/****************************************************************************
NAME    
    A2dpRouteAudio
    
DESCRIPTION
	attempt to connect an audio connection from a2dp device via the passed in 
    deviceID

RETURNS
    
*/
void A2dpRouteAudio(uint8 Index, Sink sink)
{      
    AUD_DEBUG(("AudioA2dpRoute Index[%x] Sink[%x]\n", Index , (uint16) sink )) ;

    /* ensure sink is valid before attempting the connection */
    if(sink)
    {       
        a2dp_codec_settings * codec_settings;
            
        AUD_DEBUG(("AudioA2dpRoute Index[%d] DevId[%x]\n", Index , theHeadset.a2dp_link_data->device_id[Index] )) ;

        /* get the rate information for connection */
        codec_settings = A2dpCodecGetSettings(theHeadset.a2dp_link_data->device_id[Index], theHeadset.a2dp_link_data->stream_id[Index]);

            /* ensure stream is valid */
            if(codec_settings)
            {                
                AUDIO_MODE_T mode = AUDIO_MODE_CONNECTED;
                uint16 a2dp_gain = theHeadset.audioData.gVolMaps[ theHeadset.a2dp_link_data->gAvVolumeLevel[Index] ].A2dpGain;
                
                if (theHeadset.audioData.gVolMaps[ theHeadset.a2dp_link_data->gAvVolumeLevel[Index] ].A2dpGain == VOLUME_A2DP_MUTE_GAIN)
                {
                    mode = AUDIO_MODE_MUTE_SPEAKER;
                }
                else
                {
                    /* must take off 1 when passing to audio plugin if not mute; 0 = MUTE, 1 = Gain 0, 2 = Gain 1, etc. */
                    a2dp_gain--;
                }
                             
                /* initialise the AudioConnect extra parameters required to pass in additional codec information */
                theHeadset.a2dp_audio_connect_params.content_protection = codec_settings->codecData.content_protection; /* content protection retrieved from a2dp library */            
                theHeadset.a2dp_audio_connect_params.clock_mismatch = theHeadset.a2dp_link_data->clockMismatchRate[Index]; /* clock mismatch rate for this device */      
                theHeadset.a2dp_audio_mode_params.music_mode_processing = A2DP_MUSIC_PROCESSING_FULL_EQ1; /* set the A2DP audio processing mode */
                theHeadset.a2dp_audio_connect_params.mode_params = &theHeadset.a2dp_audio_mode_params; /* TODO the EQ mode could be changable in the application */
#ifdef A2DP_EXTRA_CODECS                
#ifdef INCLUDE_FASTSTREAM                
                theHeadset.a2dp_audio_connect_params.voice_rate = codec_settings->codecData.voice_rate; /* voice rate retrieved from a2dp library */
                theHeadset.a2dp_audio_connect_params.bitpool = codec_settings->codecData.bitpool; /* bitpool retrieved from a2dp library */
                theHeadset.a2dp_audio_connect_params.format = codec_settings->codecData.format; /* format retrieved from a2dp library */
#endif                   
#endif  
                
                AUD_DEBUG(("AudioA2dpRoute Index[%d] DevId[%x] Gain[%x] Codec[%x] ClkMismatch[%x]\n", 
                           Index , 
                           theHeadset.a2dp_link_data->device_id[Index],
                           a2dp_gain,
                           codec_settings->seid,
                           theHeadset.a2dp_audio_connect_params.clock_mismatch)) ;

				if(theHeadset.dsp_plugin)
					AudioDisconnect();
				theHeadset.dsp_plugin = getA2dpPlugin(codec_settings->seid);

                /* connect the audio via the audio plugin */	
                AudioConnect(   getA2dpPlugin(codec_settings->seid),
                                sink , 
    		     				AUDIO_SINK_AV ,
    		    				theHeadset.codec_task,
    		    	            a2dp_gain, 
                                codec_settings->rate,
    		    				theHeadset.features.stereo ,
    		     				mode,
                                AUDIO_ROUTE_INTERNAL,
    		    				LBIPMPowerLevel(), 
                                &theHeadset.a2dp_audio_connect_params,
                                &theHeadset.task);                              

            /* caller responsible for freeing memory */
            free(codec_settings);
        }
            
        /* update the current sink being routed */            
        theHeadset.sco_sink = sink;
    }
}

/****************************************************************************
NAME    
    audioHandleMicSwitch
    
DESCRIPTION
	Handle AT+MICTEST AT command from TestAg. 
    This command swaps between the two microphones to test 2nd mic in production.

RETURNS
    
*/
void audioHandleMicSwitch( void )
{
    AUD_DEBUG(("audioHandleMicSwitch\n"));
    
    /*only attempt to swap between the microphones if we have a sco connection present*/
	if (theHeadset.sco_sink)
    {
    	AudioMicSwitch() ;		
 	}
    else
    {
        AUD_DEBUG(("audioHandleMicSwitch - no sco present\n"));
    }
}
        
#ifdef WBS_TEST
void CreateAudioConnection(void)
{
	hfp_audio_params * audio_params = NULL;
    HfpAudioTransferConnection(hfp_primary_link, 
                               hfp_audio_to_hfp , 
                               theHeadset.HFP_supp_features.supportedSyncPacketTypes,
                               audio_params );
    
    
}
#endif
