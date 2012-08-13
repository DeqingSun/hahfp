/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    tone.h
DESCRIPTION
    plugin implentation which plays tones
NOTES
*/

#include <codec.h>
#include <stdlib.h>
#include <panic.h>
#include <stream.h>
#include <print.h>
#include <stream.h> 
#include <message.h>
                                                                                                                    
#include "audio_plugin_if.h" /*for the audio_mode*/
#include "csr_tone.h"
#include "csr_tone_plugin.h"


typedef struct
{
    Sink audio_sink;
} TONE_T;

static TONE_T TONE;


/****************************************************************************
DESCRIPTION
    plays a tone using the audio plugin    
*/
void CsrTonePluginPlayTone ( ringtone_note * tone, Task codec_task, uint16 tone_volume , bool stereo) 
{    
    Source lSource ;
	Sink speaker_snk = NULL;
    
    CodecSetOutputGainNow(codec_task, 0, left_and_right_ch);   
    
    speaker_snk = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, (stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A));
    
    TONE.audio_sink = speaker_snk;
    
    PRINT(("TONE: Play tone, stereo[%d] sink[0x%x] vol[%d]\n", stereo, (uint16)speaker_snk, tone_volume));    
    
	PanicFalse(SinkConfigure(speaker_snk, STREAM_CODEC_OUTPUT_RATE, 8000));
   
        /*request an indication that the tone has completed / been disconnected*/
    MessageSinkTask ( speaker_snk , (TaskData*)&csr_tone_plugin ) ;
        
    	/*connect the tone*/		
	lSource = StreamRingtoneSource ( (const ringtone_note *) tone ) ;    
            
    PanicFalse( StreamConnectAndDispose( lSource , speaker_snk ) );
    
    CodecSetOutputGainNow(codec_task, tone_volume, left_and_right_ch);                
}

/****************************************************************************
DESCRIPTION
	Stop a tone from currently playing
*/
void CsrTonePluginStopTone ( void ) 
{  
    PRINT(("TONE: Terminated\n"));
   
    if (TONE.audio_sink)
    {
        StreamDisconnect(0, TONE.audio_sink); 
        MessageSinkTask(TONE.audio_sink, NULL);	
        SinkClose(TONE.audio_sink);
        TONE.audio_sink = 0;
    }
}

/****************************************************************************
DESCRIPTION
	a tone has completed
*/
void CsrTonePluginToneComplete (void )
{
    PRINT(("TONE: Complete\n"));
  
    CsrTonePluginStopTone();
}

