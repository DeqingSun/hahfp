/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_wav_ringtone.h
DESCRIPTION
    plugin implentation which plays wav ringtones
NOTES
*/

#include <codec.h>
#include <stdlib.h>
#include <panic.h>
#include <stream.h>
#include <print.h>
#include <stream.h> 
#include <message.h> 
#include <file.h>
#include <print.h>
                                                                                                                    
#include "audio_plugin_if.h" /*for the audio_mode*/
#include "csr_wav_ringtone_plugin.h"
#include "csr_wav_ringtone.h"

static const char file0[] = "wav/ringtone.raw";
static const char file1[] = "wav/powerup.raw";

typedef struct
{
	const char * str;
	uint16 len; /*excluding terminator*/
} files_t ;

static const files_t files[] =  
{
	{ (file0), (sizeof(file0) - 1) },			
	{ (file1), (sizeof(file1) - 1) }			
} ;

typedef struct
{
    Sink audio_sink;
} RINGTONE_T;

static RINGTONE_T WAV_RINGTONE;

/****************************************************************************
DESCRIPTION
    plays a tone using the audio plugin    
*/
void CsrWavRingtonePluginPlayTone ( ringtone_note * tone, Task codec_task, uint16 tone_volume , bool stereo) 
{    
        /*here, the audio note is actually a file string to play instead of a tone*/
    Sink lSink = NULL ;    
	
    FILE_INDEX file_index;
        /*the tone here is actually the requested phrase*/
        
    WAV_RINGTONES_T phrase = (WAV_RINGTONES_T) tone ;
    
    if ((phrase < 0) || (phrase >= WAV_RINGTONES_TOP))
    {
    	PRINT(("[ERROR] WRONG RINGTONE INDEX: [%d]\n", phrase));
    	Panic();
    }
    
    AUDIO_BUSY = (TaskData*) &csr_wav_ringtone_plugin;    
				        
    file_index = FileFind(FILE_ROOT, files[phrase].str , files[phrase].len );
        
    /* Check we have the digit file */
    if (file_index == FILE_NONE)
    {
    	PRINT(("[ERROR] FILE NONE: %s\n", files[phrase].str));
    	Panic() ;
    }
    
    PRINT(("WAV:PlayRingTone\n" ));

    lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A);
	
	PanicFalse(SinkConfigure(lSink, STREAM_CODEC_OUTPUT_RATE, (uint32)8000 ));
		
    WAV_RINGTONE.audio_sink = lSink;
		
    {/*vol set inline so as not to store codec_task*/
            /*Set the output Gain immediately*/
        CodecSetOutputGainNow( codec_task, tone_volume, left_and_right_ch );  
    }    
 
        /* Get messages when source has finished */
    MessageSinkTask( lSink , (TaskData*) &csr_wav_ringtone_plugin);

    StreamConnect ( StreamFileSource(file_index) , lSink );
}

/****************************************************************************
DESCRIPTION
	Stop a tone from currently playing
*/
void CsrWavRingtonePluginStopTone ( void ) 
{  
    if (WAV_RINGTONE.audio_sink)
    {
        StreamDisconnect(0, WAV_RINGTONE.audio_sink); 
        MessageSinkTask(WAV_RINGTONE.audio_sink, NULL);	
        SinkClose(WAV_RINGTONE.audio_sink);
        WAV_RINGTONE.audio_sink = 0;
    }
}

/****************************************************************************
DESCRIPTION
	a tone has completed
*/
void CsrWavRingtonePluginToneComplete (void )
{
    CsrWavRingtonePluginStopTone();
}

