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
#include "csr_voice_prompts_plugin.h"
#include "csr_voice_prompts.h"

#ifdef INSTALL_DSP_VP_SUPPORT
/* Messages from DSP */
#define MUSIC_READY_MSG           (0x1000)
#define MUSIC_SETMODE_MSG         (0x1001)
#define MUSIC_VOLUME_MSG          (0x1002)
#define MUSIC_CODEC_MSG           (0x1006)
#define MUSIC_TONE_END            (0x1080)
#define MUSIC_LOADPARAMS_MSG      (0x1012)

/* DSP message structure */
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;

/*the local kalimba message handling function*/
static void handleKalimbaMessage (Task task , Message message);
#endif

/*the task message handler*/
static void message_handler (Task task, MessageId id, Message message);

/*the local message handling functions*/
static void handleAudioMessage (Task task , MessageId id, Message message);
static void handleInternalMessage (Task task , MessageId id, Message message);
    
/*the plugin task*/
const TaskData csr_voice_prompts_plugin = { message_handler };

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
#ifdef INSTALL_DSP_VP_SUPPORT
    else if (id == MESSAGE_FROM_KALIMBA)
    {
        handleKalimbaMessage (task , message ) ;
    }
#endif
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
        case (AUDIO_PLUGIN_VOICE_PROMPTS_INIT_MSG ):
        {
            AUDIO_PLUGIN_VOICE_PROMPTS_INIT_MSG_T * init_message = (AUDIO_PLUGIN_VOICE_PROMPTS_INIT_MSG_T *)message;
            CsrVoicePromptsPluginInit(init_message->no_prompts, init_message->index, init_message->no_languages);
        }
        break;
        
        case (AUDIO_PLUGIN_PLAY_TTS_MSG ):
        {
            AUDIO_PLUGIN_PLAY_TTS_MSG_T * tts_message = (AUDIO_PLUGIN_PLAY_TTS_MSG_T *)message ;
        
            if (AUDIO_BUSY) 
            {    
                if ( tts_message->can_queue) 
                {                
                    MAKE_AUDIO_MESSAGE_WITH_LEN( AUDIO_PLUGIN_PLAY_TTS_MSG, tts_message->size_data ) ; 
                    memmove(message, tts_message, sizeof(AUDIO_PLUGIN_PLAY_TTS_MSG_T) + tts_message->size_data);
                    MessageSendConditionally ( task , AUDIO_PLUGIN_PLAY_TTS_MSG, message ,(const uint16 *)&AUDIO_BUSY ) ;
                    PRINT(("VP:Queue\n")); 
                }
            }
            else
            {
                CsrVoicePromptsPluginPlayPhrase ( tts_message->id, tts_message->data, tts_message->size_data, tts_message->language, 
                                                  tts_message->codec_task, tts_message->tts_volume, tts_message->stereo);
                PRINT(("VP:start\n"));
            } 
        }
        break ;
        
        case (AUDIO_PLUGIN_STOP_TTS_MSG ):
        {
            if(AUDIO_BUSY) 
            {
                CsrVoicePromptsPluginStopPhrase() ;
            }
        }
        break;
        
        default:
            /*other messages do not need to be handled by the voice prompts plugin*/
        break ;
    }
}

#ifdef INSTALL_DSP_VP_SUPPORT
/****************************************************************************
DESCRIPTION
    kalimba messages to the task are handled here
*/ 
static void handleKalimbaMessage ( Task task , Message message )
{
    const DSP_REGISTER_T *m = (const DSP_REGISTER_T *) message;
    PRINT(("handleKalimbaMessage: msg id[%x] a[%x] b[%x] c[%x] d[%x]\n", m->id, m->a, m->b, m->c, m->d));
    
    switch ( m->id ) 
    {
        case MUSIC_READY_MSG:
        {
            PRINT(("VP: DSP Play\n"));
            CsrVoicePromptPluginPlayDsp();
        }
        break;
        
        case MUSIC_CODEC_MSG:
        break;
        
        case MUSIC_TONE_END:
        {
            PRINT(("VP: DSP End\n"));
            
            if(AUDIO_BUSY && (AUDIO_BUSY == (TaskData*) &csr_voice_prompts_plugin))
                CsrVoicePromptsPluginPlayBack();
        }
        break;
        
        default:
        {
            PRINT(("handleKalimbaMessage: unhandled %X\n", m->id));        
        }
    }
}
#endif

/****************************************************************************
DESCRIPTION
    Internal messages to the task are handled here
*/ 
static void handleInternalMessage ( Task task , MessageId id, Message message )     
{
    switch (id)
    {
        case MESSAGE_STREAM_DISCONNECT:
        {
            PRINT(("VP: End\n"));
            
            if(AUDIO_BUSY && (AUDIO_BUSY == (TaskData*) &csr_voice_prompts_plugin))
                CsrVoicePromptsPluginPlayBack();
        }
        break ;
        
        default:
            Panic() ;
        break ;
    }
}
