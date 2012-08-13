/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004

FILE NAME
    codec_volume.c
    
DESCRIPTION
	
*/

#include "codec.h"
#include "codec_private.h"
#include "codec_wm8731_message_handler.h"
#include "codec_csr_internal_message_handler.h"
#include "codec_wm8731_volume_handler.h"

#include <codec.h>


void CodecSetInputGain(Task codecTask, uint16 volume, codec_channel channel)
{
	MAKE_CODEC_MESSAGE(CODEC_INTERNAL_INPUT_GAIN_REQ);
	message->volume = volume;
    message->channel = channel;
	MessageSend(codecTask, CODEC_INTERNAL_INPUT_GAIN_REQ, message);
}


void CodecSetInputGainNow(Task codecTask, uint16 volume, codec_channel channel)
{
    Source input_source;
    
    if (codecTask->handler == csrInternalMessageHandler)
    {
        if (channel != right_ch)
		{
            input_source = StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
            SourceConfigure(input_source, STREAM_CODEC_INPUT_GAIN, volume); 
            SourceClose(input_source);
		}
        if (channel != left_ch)
		{
            input_source = StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
            SourceConfigure(input_source, STREAM_CODEC_INPUT_GAIN, volume); 			
            SourceClose(input_source);
		}
    }
#ifndef CODEC_EXCLUDE_WOLFSON
    else if (codecTask->handler == wolfsonMessageHandler)
    {
        Wm8731SetInputGain((WolfsonCodecTaskData *) codecTask, volume, channel);
    }
#endif /* CODEC_EXCLUDE_WOLFSON */
    else
    {
        CodecSetInputGain(codecTask, volume, channel);
    }
}


void CodecSetOutputGain(Task codecTask, uint16 volume, codec_channel channel)
{
	MAKE_CODEC_MESSAGE(CODEC_INTERNAL_OUTPUT_GAIN_REQ);
    message->volume = volume;
	message->channel = channel;
	MessageSend(codecTask, CODEC_INTERNAL_OUTPUT_GAIN_REQ, message);
}


void CodecSetOutputGainNow(Task codecTask, uint16 volume, codec_channel channel)
{
    Sink output_sink;
    
    if (codecTask->handler == csrInternalMessageHandler)
    {
        if (channel == left_and_right_ch)
        {
            if (!StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A))
            {
                output_sink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A_AND_B);            
                SinkConfigure(output_sink, STREAM_CODEC_OUTPUT_GAIN, volume);
                SinkClose(output_sink);
                return;
            }            
        }
        
        if (channel != right_ch)
		{
			output_sink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
            SinkConfigure(output_sink, STREAM_CODEC_OUTPUT_GAIN, volume);
            SinkClose(output_sink);
		}
        if (channel != left_ch)
    	{
			output_sink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
            SinkConfigure(output_sink, STREAM_CODEC_OUTPUT_GAIN, volume);	
            SinkClose(output_sink);
		}
    }
#ifndef CODEC_EXCLUDE_WOLFSON
    else if (codecTask->handler == wolfsonMessageHandler)
    {
        Wm8731SetOutputGain((WolfsonCodecTaskData *) codecTask, volume, channel);
    }
#endif /* CODEC_EXCLUDE_WOLFSON */
    else
    {
        CodecSetOutputGain(codecTask, volume, channel);
    }
}
