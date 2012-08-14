/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_cvc_common_plugin.c
DESCRIPTION
    Interface file for an audio_plugin
NOTES
*/
#ifndef SCO_DSP_BOUNDARIES
#define SCO_DSP_BOUNDARIES
#endif

#include <codec.h>
#include <stdlib.h>
#include <print.h>
#include <stream.h> /*for the ringtone_note*/

#include "audio_plugin_if.h" /*the messaging interface*/
#include "hearing_aid_sim_plugin.h"
#include "hearing_aid_sim.h"
	/*the task message handler*/
static void message_handler (Task task, MessageId id, Message message) ;

	/*the local message handling functions*/
static void handleAudioMessage ( Task task , MessageId id, Message message ) 	;
static void handleInternalMessage ( Task task , MessageId id, Message message ) 	;
	
const TaskData hearing_aid_sim_plugin = {message_handler};

/****************************************************************************
DESCRIPTION
	The main task message handler
*/
static void message_handler ( Task task, MessageId id, Message message ) 
{
	if ( (id >= AUDIO_DOWNSTREAM_MESSAGE_BASE ) && (id <= AUDIO_DOWNSTREAM_MESSAGE_TOP) )
	{
		handleAudioMessage (task , id, message ) ;
	}
	else
	{
		handleInternalMessage (task , id , message ) ;
	}
}	

/****************************************************************************
DESCRIPTION

	messages from the audio library are received here. 
	and converted into function calls to be implemented in the 
	plugin module
*/ 
static void handleAudioMessage ( Task task , MessageId id, Message message ) 	
{
	switch (id)
	{
		case (AUDIO_PLUGIN_CONNECT_MSG ):
		{
			AUDIO_PLUGIN_CONNECT_MSG_T * connect_message = (AUDIO_PLUGIN_CONNECT_MSG_T *)message ;
	
			if (AUDIO_BUSY)
			{ 		/*Queue the connect message until the audio task is available*/
				MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_CONNECT_MSG ) ; 
				
				message->audio_sink = connect_message->audio_sink ;
				message->sink_type  = connect_message->sink_type ;
				message->codec_task = connect_message->codec_task ;
				message->volume     = connect_message->volume ;
				message->rate       = connect_message->rate ;
				message->mode 		= connect_message->mode ;
				message->stereo     = connect_message->stereo ;
				message->params     = connect_message->params ;
				message->power      = connect_message->power ;
				
			    MessageSendConditionally ( task, AUDIO_PLUGIN_CONNECT_MSG , message , (const uint16 *)&AUDIO_BUSY ) ;
			} 
			else
			{		/*connect the audio*/
				HearingAidSimPluginConnect( task,
                                      connect_message->audio_sink , 
				                      connect_message->sink_type  ,
         							  connect_message->codec_task ,
									  connect_message->volume , 
									  connect_message->rate , 								  								  
									  connect_message->stereo ,								  
									  FALSE,									  
	     							  connect_message->mode,
									  connect_message->power) ;
			}			
		}	
		break ;
		
		case (AUDIO_PLUGIN_DISCONNECT_MSG ):
		{
			if (AUDIO_BUSY)
			{
				MessageSendConditionally ( task, AUDIO_PLUGIN_DISCONNECT_MSG , 0 ,(const uint16 *)&AUDIO_BUSY ) ;
    		}
			else
			{		
				HearingAidSimPluginDisconnect(task) ;
			}
		}	
		break ;
		
		case (AUDIO_PLUGIN_SET_VOLUME_MSG ): 
		{
			AUDIO_PLUGIN_SET_VOLUME_MSG_T * volume_message = (AUDIO_PLUGIN_SET_VOLUME_MSG_T *)message ;			
			
			if (AUDIO_BUSY)
			{
			     MAKE_AUDIO_MESSAGE (AUDIO_PLUGIN_SET_VOLUME_MSG ) ;
			     message->volume = volume_message->volume ;
			     
		         MessageSendConditionally ( task, AUDIO_PLUGIN_SET_VOLUME_MSG , message ,(const uint16 *)&AUDIO_BUSY ) ;    	
            }
            else
            {
                HearingAidSimPluginSetVolume ( task, volume_message->volume ) ;
            }			
		}		
		break ;
		
		case (AUDIO_PLUGIN_PLAY_TONE_MSG ):
		{
			AUDIO_PLUGIN_PLAY_TONE_MSG_T * tone_message = (AUDIO_PLUGIN_PLAY_TONE_MSG_T *)message ;
			
			if (AUDIO_BUSY) 
			{	
				if ( tone_message->can_queue) /*then re-queue the tone*/
				{				
					MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_PLAY_TONE_MSG ) ; 
					
					message->tone        = tone_message->tone       ;
					message->can_queue   = tone_message->can_queue  ;
					message->codec_task  = tone_message->codec_task ;
					message->tone_volume = tone_message->tone_volume;
					message->stereo      = tone_message->stereo     ;
	
					PRINT(("TONE:Q\n"));
					
					MessageSendConditionally ( task , AUDIO_PLUGIN_PLAY_TONE_MSG, message ,(const uint16 *)&AUDIO_BUSY ) ;			
				}
			}
			else
			{
				break;
				PRINT(("TONE:start\n"));				
                AUDIO_BUSY = (TaskData*) task;    
        		HearingAidSimPluginPlayTone ( task, tone_message->tone, tone_message->tone_volume ) ;		     				
			}
							     
		}
		break ;
		
		case (AUDIO_PLUGIN_STOP_TONE_MSG ):
		{
			if (AUDIO_BUSY)
			{
				HearingAidSimPluginStopTone(task) ;
			}
		}
		break ;	
        
		default:
		{
		}
		break ;
	}
}

/****************************************************************************
DESCRIPTION
	Internal messages to the task are handled here
*/ 
static void handleInternalMessage ( Task task , MessageId id, Message message ) 	
{
	switch (id)
	{
        case MESSAGE_STREAM_DISCONNECT: /*a tone has completed*/
        {
            PRINT(("CVC: Tone End\n"));
            AUDIO_BUSY = NULL ;    
            
            HearingAidSimPluginToneComplete(task) ;            
        }    
		break ;
		
		default:
		break ;
	}	
}
