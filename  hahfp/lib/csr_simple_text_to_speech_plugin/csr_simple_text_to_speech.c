/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_simple_text_to_speech.h
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
#include <file.h>
#include <kalimba.h>
#include <string.h>

#include "audio.h"
#include "audio_plugin_if.h" /*for the audio_mode*/
#include "csr_tone_plugin.h"
#include "csr_simple_text_to_speech_plugin.h"
#include "csr_simple_text_to_speech.h"

static const char file0[] = "tts/0.raw";
static const char file1[] = "tts/1.raw";
static const char file2[] = "tts/2.raw";
static const char file3[] = "tts/3.raw";
static const char file4[] = "tts/4.raw";
static const char file5[] = "tts/5.raw";
static const char file6[] = "tts/6.raw";
static const char file7[] = "tts/7.raw";
static const char file8[] = "tts/8.raw";
static const char file9[] = "tts/9.raw";

typedef struct
{
	const char * str;
	uint16 len; /*excluding terminator*/
} files_t ;

static const files_t files[] =  
{
	{ (file0), (sizeof(file0) - 1) },			
	{ (file1), (sizeof(file1) - 1) },			
	{ (file2), (sizeof(file2) - 1) },			
	{ (file3), (sizeof(file3) - 1) },			
	{ (file4), (sizeof(file4) - 1) },			
	{ (file5), (sizeof(file5) - 1) },			
	{ (file6), (sizeof(file6) - 1) },			
	{ (file7), (sizeof(file7) - 1) },			
	{ (file8), (sizeof(file8) - 1) },			
	{ (file9), (sizeof(file9) - 1) }			
} ;

typedef struct 
{
    unsigned    ilength:       5 ;
	unsigned    iRepeatCycles: 3;
	
	/*! the number of bytes in the payload */
    uint16 size_data ;
 		/*! The codec task to use to connect the audio*/
    Task   codec_task ;
    	/*! The volume at which to play the tone 0 - current volume*/
    uint16 tone_volume ;
		/*! whether or not to route mono / stereo audio*/
    bool   stereo ; 
		/*! pointer to the payload */
	uint8  data[1];
    
} CSR_SIMPLE_TTS_DATA_T ;


static CSR_SIMPLE_TTS_DATA_T *sStartTTSData = NULL;

#define MAX_REPEAT_CYCLES  (1)


/****************************************************************************
DESCRIPTION
    plays One digital number using the audio plugin    
*/

void CsrSimpleTextToSpeechPluginPlayDigit ( ringtone_note * tone, Task codec_task, uint16 tone_volume , bool stereo) 
{    
    /*here, the audio note is actually a file string to play instead of a tone*/
    Sink lSink ;    
	Source lSource0 = NULL;
			
    FILE_INDEX file_index;
        
	/*the tone here is actually the requested phrase*/
        
    SIMPLE_TEXT_TO_SPEECH_PHRASES_T phrase = (SIMPLE_TEXT_TO_SPEECH_PHRASES_T) tone ;
    
    if ( ( phrase < 0 ) || ( phrase >= SIMPLE_TEXT_TO_SPEECH_PHRASES_TOP ) )
    {
        Panic();
    }
    
    AUDIO_BUSY = (TaskData*) &(csr_simple_text_to_speech_plugin);    
	
    file_index = FileFind(FILE_ROOT, files[phrase].str , files[phrase].len );
        
    /* Check we have the digit file */
    if (file_index == FILE_NONE)
    {
        Panic() ;
    }
    
    PRINT(("TTS:PlayDigit\n" ));
		
    lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A);	
		
	PanicFalse(SinkConfigure(lSink, STREAM_CODEC_OUTPUT_RATE, (uint32) 8000));
	
	lSource0 = StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
	PanicFalse(SourceConfigure(lSource0, STREAM_CODEC_INPUT_RATE, (uint32) 8000));
		
    {/*vol set inline so as not to store codec_task*/
            /*Set the output Gain immediately*/
        CodecSetOutputGainNow( codec_task, tone_volume + 2, left_and_right_ch );  
    }    

        /* Get messages when source has finished */
    MessageSinkTask( lSink , (TaskData*) &csr_simple_text_to_speech_plugin);

    StreamConnectAndDispose ( StreamFileSource(file_index) , lSink );
	
}


/****************************************************************************
DESCRIPTION
    plays a number phrase using the audio plugin    
*/

void CsrSimpleTextToSpeechPluginPlayPhrase (uint16 id , 
											uint8 * data , 
											uint16 size_data , 
											uint16 language , 
											Task codec_task , 
											uint16 tone_volume , 
											bool stereo )
{
	/* allocate the memory */
	if(sStartTTSData == NULL)
	{
		sStartTTSData = (CSR_SIMPLE_TTS_DATA_T *) PanicUnlessMalloc(sizeof(CSR_SIMPLE_TTS_DATA_T) + size_data);
	}
	else
	{
		Panic();
	}
	
	/* copy the data */
	memmove((sStartTTSData->data), data, size_data);
	
	sStartTTSData->size_data     = size_data;
	sStartTTSData->codec_task    = codec_task;
	sStartTTSData->tone_volume   = tone_volume;
	sStartTTSData->stereo        = stereo;
	
	sStartTTSData->ilength       = 0;
	sStartTTSData->iRepeatCycles = MAX_REPEAT_CYCLES; /* no repeat */
	
	/* kick off the number speech directly*/
	{
		CsrSimpleTextToSpeechPluginPlayDigit( (ringtone_note *) (data[0] - 0x30),
											  codec_task, 
											  tone_volume, 
											  stereo); 
	}
	
}

/****************************************************************************
DESCRIPTION
	Play a digit or kick off a new number playing
*/

void CsrSimpleTextToSpeechPluginPlayBack(void)
{
	sStartTTSData->ilength += 1;
	
	MessageCancelAll((TaskData*)&(csr_tone_plugin),AUDIO_PLUGIN_PLAY_TONE_MSG); 
	
	if(sStartTTSData->ilength == sStartTTSData->size_data)
	{
		
		sStartTTSData->iRepeatCycles -= 1;
		sStartTTSData->ilength        = 0;
		
		if(sStartTTSData->iRepeatCycles)
		{ 
						
			CsrSimpleTextToSpeechPluginPlayDigit( (ringtone_note *) ((sStartTTSData->data)[0] - 0x30),
											  	   sStartTTSData->codec_task, 					  
				        				  	  	   sStartTTSData->tone_volume, 
										  	  	   sStartTTSData->stereo); 
		}
		else
		{
			/* Stop the CSR Simple TTS */
			
			MessageSend( (TaskData*) &csr_simple_text_to_speech_plugin, 
						 AUDIO_PLUGIN_STOP_TTS_MSG, 
						 NULL );
		} 
	}
	else
	{
		CsrSimpleTextToSpeechPluginPlayDigit ( (ringtone_note *) ((sStartTTSData->data)[sStartTTSData->ilength] - 0x30), 
										  	  sStartTTSData->codec_task, 					  
				        				  	  sStartTTSData->tone_volume, 
										  	  sStartTTSData->stereo  ) ;	
				
	}
}

/****************************************************************************
DESCRIPTION
	Stop a tone from currently playing
*/
void CsrSimpleTextToSpeechPluginStopPhrase ( void ) 
{  
    if(sStartTTSData != NULL)
	{
        Sink lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, sStartTTSData->stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A);
    
	    PRINT(("TTS: Terminated\n"));
   
	    /* cancel all the messages relating to Simple TTS that have been sent */
	    MessageCancelAll((TaskData*) &csr_simple_text_to_speech_plugin,
					 MESSAGE_STREAM_DISCONNECT );
        
        MessageSinkTask(lSink, NULL);
	
	    StreamDisconnect(0, lSink); 
        SinkClose(lSink);
		
		free(sStartTTSData);
		sStartTTSData = NULL;
	}
     
	KalimbaPowerOff();
	
	AUDIO_BUSY = NULL ;
}
