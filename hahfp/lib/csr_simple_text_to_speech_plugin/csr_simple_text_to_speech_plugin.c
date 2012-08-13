/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_simple_text_to_speech_plugin.c
DESCRIPTION
    an audio plugin
NOTES
*/

#include <codec.h>
#include <stdlib.h>
#include <print.h>
#include <stream.h> /*for the ringtone_note*/
#include <string.h>

#include "audio_plugin_if.h" /*the messaging interface*/
#include "csr_simple_text_to_speech_plugin.h"
#include "csr_simple_text_to_speech.h"

	/*the task message handler*/
static void message_handler (Task task, MessageId id, Message message) ;

	/*the local message handling functions*/
static void handleAudioMessage ( Task task , MessageId id, Message message ) 	;
static void handleInternalMessage ( Task task , MessageId id, Message message ) 	;
	
	/*the plugin task*/
const TaskData csr_simple_text_to_speech_plugin = { message_handler };


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
		case (AUDIO_PLUGIN_PLAY_TTS_MSG ):
		{
			
			AUDIO_PLUGIN_PLAY_TTS_MSG_T * tts_message = (AUDIO_PLUGIN_PLAY_TTS_MSG_T *)message ;
		
			if (AUDIO_BUSY) 
			{	
				if ( tts_message->can_queue) 
				{				
					MAKE_AUDIO_MESSAGE_WITH_LEN( AUDIO_PLUGIN_PLAY_TTS_MSG, tts_message->size_data ) ; 
					
					message->id          = tts_message->id;
					message->size_data   = tts_message->size_data;
					message->language    = tts_message->language;
				    message->can_queue   = tts_message->can_queue;
				    message->codec_task  = tts_message->codec_task;
				    message->tts_volume  = tts_message->tts_volume;
				    message->stereo      = tts_message->stereo; 
					memmove(message->data, tts_message->data, tts_message->size_data);
					
					PRINT(("TTS:Q\n")); 
					
					
					MessageSendConditionally ( task , AUDIO_PLUGIN_PLAY_TTS_MSG, message ,(const uint16 *)&AUDIO_BUSY ) ;			
				}
			}
			else
			{
				PRINT(("TTS:start\n"));

				/* TODO */
				CsrSimpleTextToSpeechPluginPlayPhrase ( tts_message->id, 
														tts_message->data, 
														tts_message->size_data, 
														tts_message->language, 
														tts_message->codec_task,
														tts_message->tts_volume, 
														tts_message->stereo       ) ;
				
			} 
		}
		
		break ;
		
		case (AUDIO_PLUGIN_PLAY_TONE_MSG ):
		{
			/* For csr_simple_tts plugin, this message does not need to be handled */
			

		}
		break ;
		
		case (AUDIO_PLUGIN_STOP_TTS_MSG ):
		{
			if(AUDIO_BUSY) 
			{
				CsrSimpleTextToSpeechPluginStopPhrase() ;
			}
		}
		break ;		
		
		default:
		{
		  /*other messages do not need to be handled by the simple tts plugin*/
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
        case MESSAGE_STREAM_DISCONNECT: /*a tts number has completed*/
        {
            PRINT(("TTS: End\n"));
			
			if(AUDIO_BUSY && (AUDIO_BUSY == (TaskData*) &csr_simple_text_to_speech_plugin))
			{
				CsrSimpleTextToSpeechPluginPlayBack();
			}
        }    
		break ;
		
		default:
		  Panic() ;
		break ;
	}
}
