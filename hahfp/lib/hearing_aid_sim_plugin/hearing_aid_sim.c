/****************************************************************************

FILE NAME
    hearing_aid_sim.c
    
DESCRIPTION
NOTES
*/

#include <codec.h>
#include <stdlib.h>
#include <panic.h>
#include <stream.h>
#include <print.h>
#include <kalimba.h>
#include <file.h>
#include <stream.h>     /*for the ringtone_note*/
#include <connection.h> /*for the link_type */
#include <string.h>
#include <kalimba_standard_messages.h>
#include "audio_plugin_if.h"        /*for the audio_mode*/
#include "hearing_aid_sim_plugin.h"
#include "hearing_aid_sim.h"

#include <source.h>
#include <transform.h>
#include <app/vm/vm_if.h>

/* Location of DSP kap file in the file system */
static const char kal[] = "hearing_aid_sim/hearing_aid_sim.kap";
static Task has_codec_task = NULL;
uint16 has_volume;
uint32 has_rate;

static void connect_streams(void)
{
    /* Access left and right ADC and DAC */
    Sink audio_sink_a = StreamAudioSink( AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A );
    Sink audio_sink_b = StreamAudioSink( AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B );
    Source audio_source_a = StreamAudioSource( AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A );
    Source audio_source_b = StreamAudioSource( AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B );

    /* Configure 44.1kHz for both channels and synchronise left and right channels */
    PanicFalse( SinkConfigure(audio_sink_a, STREAM_CODEC_OUTPUT_RATE, has_rate) );
    PanicFalse( SinkConfigure(audio_sink_b, STREAM_CODEC_OUTPUT_RATE, has_rate) );
    PanicFalse( SinkSynchronise(audio_sink_a, audio_sink_b) );
    PanicFalse( SourceConfigure(audio_source_a, STREAM_CODEC_INPUT_RATE, has_rate) );
    PanicFalse( SourceConfigure(audio_source_b, STREAM_CODEC_INPUT_RATE, has_rate) );
    PanicFalse( SourceSynchronise(audio_source_a, audio_source_b) );

    /* Set up codec gains */
    PanicFalse( SourceConfigure(audio_source_a, STREAM_CODEC_INPUT_GAIN, 6) );
    PanicFalse( SourceConfigure(audio_source_b, STREAM_CODEC_INPUT_GAIN, 6) );
    PanicFalse( SourceConfigure(audio_source_a, STREAM_CODEC_MIC_INPUT_GAIN_ENABLE, 1) );
    PanicFalse( SourceConfigure(audio_source_b, STREAM_CODEC_MIC_INPUT_GAIN_ENABLE, 1) );
    PanicFalse( SinkConfigure(audio_sink_a, STREAM_CODEC_OUTPUT_GAIN, has_volume) );
    PanicFalse( SinkConfigure(audio_sink_b, STREAM_CODEC_OUTPUT_GAIN, has_volume) );

#ifdef BYPASS_KALIMBA
    /* Plug Left ADC directly into left DAC */
    PanicFalse( StreamConnect(audio_source_a, audio_sink_a) );
    /* Plug Right ADC directly into right DAC */
    PanicFalse( StreamConnect(audio_source_b, audio_sink_b) );
#else
    /* Plug Left ADC into port 0 */
    PanicFalse( StreamConnect(audio_source_a, StreamKalimbaSink(0)) );
    /* Plug Right ADC into port 1 */
    PanicFalse( StreamConnect(audio_source_b, StreamKalimbaSink(1)) );
    /* Plug port 0 into Left DAC */
    PanicFalse( StreamConnect(StreamKalimbaSource(0), audio_sink_a) );
    /* Plug port 1 into Right DAC */
    PanicFalse( StreamConnect(StreamKalimbaSource(1), audio_sink_b) );
#endif
}

static void disconnect_streams(void)
{
    /* Access left and right ADC and DAC */
    Sink audio_sink_a = StreamAudioSink( AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A );
    Sink audio_sink_b = StreamAudioSink( AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B );
    Source audio_source_a = StreamAudioSource( AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A );
    Source audio_source_b = StreamAudioSource( AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B );

	PanicFalse( SinkConfigure(audio_sink_a, STREAM_CODEC_OUTPUT_GAIN, 0) );
	PanicFalse( SinkConfigure(audio_sink_b, STREAM_CODEC_OUTPUT_GAIN, 0) );

#ifdef BYPASS_KALIMBA
    /* Plug Left ADC directly into left DAC */
    StreamDisconnect(audio_source_a, audio_sink_a);
    /* Plug Right ADC directly into right DAC */
    StreamDisconnect(audio_source_b, audio_sink_b);
#else
#if 1
    /* Plug Left ADC into port 0 */
    StreamDisconnect(audio_source_a, StreamKalimbaSink(0));
    /* Plug Right ADC into port 1 */
    StreamDisconnect(audio_source_b, StreamKalimbaSink(1));
    /* Plug port 0 into Left DAC */
    StreamDisconnect(StreamKalimbaSource(0), audio_sink_a);
    /* Plug port 1 into Right DAC */
    StreamDisconnect(StreamKalimbaSource(1), audio_sink_b);
#endif
#endif
	SinkClose( audio_sink_a );
	SinkClose( audio_sink_b );
	SourceClose( audio_source_a );
	SourceClose( audio_source_b );
}

void HearingAidSimPluginConnect( Task task, 
                          Sink audio_sink , 
						  AUDIO_SINK_T sink_type, 
						  Task codec_task , 
						  uint16 volume , 
						  uint32 rate , 					  
						  bool stereo , 
						  bool mic_switch,						  
						  AUDIO_MODE_T mode,
						  AUDIO_POWER_T power  )
{
#ifndef BYPASS_KALIMBA
    /* Find the codec file in the file system */
    FILE_INDEX index = FileFind( FILE_ROOT, (const char *)kal, strlen(kal) );

    /* Did we find the desired file? */
    PanicFalse( index != FILE_NONE );

    /* Load the codec into Kalimba */
    PanicFalse( KalimbaLoad( index ) );
#endif

	has_codec_task = codec_task;
	has_volume = volume;
	has_rate = rate;

	connect_streams();
	
#ifndef BYPASS_KALIMBA
    /* Start the Kalimba */
    PanicFalse( KalimbaSendMessage(KALIMBA_MSG_GO,0,0,0,0) ); 
#endif
}

void HearingAidSimPluginDisconnect( Task task )
{
	disconnect_streams();
	
    /* Cancel any outstanding cvc messages */
    MessageCancelAll( task , MESSAGE_FROM_KALIMBA);
    MessageCancelAll( task , MESSAGE_STREAM_DISCONNECT);

    KalimbaPowerOff();        
}


void HearingAidSimPluginSetVolume( Task task, uint16 volume )
{
	has_volume = volume;
	CodecSetOutputGainNow( has_codec_task, volume , left_and_right_ch );
}

void HearingAidSimPluginPlayTone (Task task, ringtone_note * tone , uint16 tone_volume)  
{   
    Source lSource ;  
    Sink lSink ; 

	disconnect_streams();
        
    lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A_AND_B);
	
	PanicFalse(SinkConfigure(lSink, STREAM_CODEC_OUTPUT_RATE, 8000));	

	if (0)	{	
		lSink = StreamKalimbaSink(3);
	}
    
    /*request an indication that the tone has completed / been disconnected*/
    MessageSinkTask ( lSink , task );

    /*connect the tone*/
    lSource = StreamRingtoneSource( (const ringtone_note *) (tone) );    
 
	/* connect the tone (mix the tone if this is CVC) */    
    PanicFalse( StreamConnectAndDispose( lSource , lSink ) );
	
	/* use the tone volume if present */
/*	CodecSetOutputGainNow( CVC->codec_task, tone_volume ? tone_volume : CVC->volume, left_and_right_ch );*/
}

void HearingAidSimPluginStopTone ( Task task ) 
{
	Sink speaker_sink;
	if (1)
	{
		speaker_sink = StreamAudioSink(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0,  AUDIO_CHANNEL_A_AND_B);
		MessageSinkTask ( speaker_sink, NULL ) ;
	}
	else
	{
		speaker_sink = StreamAudioSink(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, AUDIO_CHANNEL_A_AND_B);
	}
	StreamDisconnect(NULL, speaker_sink);
	SinkClose(speaker_sink);
	
	connect_streams();

	if(0)
	{
		StreamDisconnect( 0 , StreamKalimbaSink(3) ) ; 
	}
}

      

/****************************************************************************
DESCRIPTION
	a tone has completed
*/
void HearingAidSimPluginToneComplete( Task task ) 
{
    Sink speaker_sink = StreamAudioSink(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, AUDIO_CHANNEL_A_AND_B);
    MessageSinkTask ( speaker_sink, NULL ) ;
    SinkClose(speaker_sink);

	connect_streams();
    AUDIO_BUSY = NULL ;

	if(0)
	{	/* DSP mode */
	   /* We no longer want to receive stream indications */
	   MessageSinkTask (StreamKalimbaSink(3) , NULL);
	}
}

