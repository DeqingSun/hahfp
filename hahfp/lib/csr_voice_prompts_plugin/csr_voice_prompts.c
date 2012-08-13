/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_voice_prompts.h
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
#include <string.h>
#include <source.h>
#include <transform.h>
#include <i2c.h>

#ifdef INSTALL_SPI_VP_SUPPORT
#include <spi.h>
#include <app\spi\spi_if.h>
#endif
#ifdef INSTALL_FILE_VP_SUPPORT
#include <file.h>
#include <stdio.h>
#endif
#ifdef INSTALL_DSP_VP_SUPPORT
#include <kalimba.h>
#include <kalimba_standard_messages.h>

#define MESSAGE_SET_SAMPLE_RATE (0x7050)
#endif

#include "audio.h"
#include "audio_plugin_if.h"
#include "csr_tone_plugin.h"
#include "csr_voice_prompts_plugin.h"
#include "csr_voice_prompts.h"

typedef struct
{
    uint16                     no_prompts;
    uint16                     no_prompts_per_lang;
    const voice_prompts_index* location;
} voice_prompts_header;

typedef struct
{
    voice_prompts_index location;
    uint32              size;
    voice_prompts_codec decompression;
    uint16              playback_rate;
} voice_prompt;

typedef struct 
{
    /*! The codec task to use to connect the audio*/
    Task                codec_task;
    /*! Data index */
    uint16              idx;
    /*! VP source */
    Source              source;
    /*! Decompression to use */
    voice_prompts_codec decompression;
    /*! Playback rate */
    uint16              playback_rate;
    /*! The language */
    uint16              language;
    /*! The volume at which to play the tone */
    uint16              tone_volume;
    /*! whether or not to route mono / stereo audio*/
    bool                stereo; 
    /*! the number of bytes in the payload */
    uint16              size_data;
    /*! pointer to the payload */
    uint16              data[1];
} PHRASE_DATA_T ;

static voice_prompts_header     header;
static PHRASE_DATA_T            *phrase_data = NULL;

#define SIZE_PROMPT_DATA   (12)

static Source csrVoicePromptsGetPrompt(voice_prompt* prompt)
{
    const uint8* rx_array;
    Source lSource = NULL;
    uint16 index;
#if defined (INSTALL_SPI_VP_SUPPORT) || !defined (REMOVE_I2C_VP_SUPPORT)
    uint32 array_addr;
#endif
#ifdef INSTALL_FILE_VP_SUPPORT
    char file_name[17];
#endif

    if(!phrase_data)
        return NULL;
    
    /* Work out the index of the prompt */
    index = phrase_data->data[phrase_data->idx++];
    /* If size_data is set then this is a numeric string, convert to an index */
    if(phrase_data->size_data) index -= 0x30;
    /* Adjust for language */
    index += (phrase_data->language * header.no_prompts_per_lang);
    
    /* Sanity checking */
    if(index >= header.no_prompts || !prompt || !header.location)
        return NULL;
    
    PRINT(("VP: Play prompt %d of %d\n", index+1, header.no_prompts));
    
    switch(header.location->type)
    {
#ifndef REMOVE_I2C_VP_SUPPORT
        case voice_prompts_type_i2c:
        {
            /* Get the offset of this prompt */
            array_addr = index * SIZE_PROMPT_DATA;
            /* Read from offset from header */
            array_addr += header.location->addr.i2c.array_addr;
            /* Create header Source */
            lSource = StreamI2cSource(header.location->addr.i2c.slv_addr, array_addr, SIZE_PROMPT_DATA);
        }
        break;
#endif
#ifdef INSTALL_SPI_VP_SUPPORT
        case voice_prompts_type_spi:
        {
            /* Get the offset of this prompt */
            array_addr = index * SIZE_PROMPT_DATA;
            /* Read from offset from header */
            array_addr += header.location->addr.spi.array_addr;
            /* Create header Source */
            lSource = StreamSpiSource(array_addr, SIZE_PROMPT_DATA, spi_protocol_none);
        }
        break;
#endif
#ifdef INSTALL_FILE_VP_SUPPORT
        case voice_prompts_type_file:
        {
            /* Get the header file name */
            sprintf(file_name, "headers/%d.idx", index);
            lSource = StreamFileSource(FileFind(FILE_ROOT, file_name, strlen(file_name)));
        }
#endif
        default:
        break;
    }
    
    /* Check source created successfully */
    if(SourceSize(lSource) < SIZE_PROMPT_DATA)
    {
        /* Finished with header source, close it */
        SourceClose(lSource);
        return NULL;
    }
    
    /* Map in header */
    rx_array = SourceMap(lSource);
    
    /* Pack data into result */
    prompt->location.type = rx_array[0];
    prompt->size          = ((uint32)rx_array[5] << 24) | ((uint32)rx_array[6] << 16) | ((uint16)rx_array[7] << 8) | (rx_array[8]);
    prompt->decompression = rx_array[9];
    prompt->playback_rate = ((uint16)rx_array[10] << 8) | (rx_array[11]);
    
    switch(prompt->location.type)
    {
#ifndef REMOVE_I2C_VP_SUPPORT
        case voice_prompts_type_i2c:
            /* Pack device addr and array addr */
            prompt->location.addr.i2c.slv_addr    = ((uint16)rx_array[1] << 8) | (rx_array[2]);
            prompt->location.addr.i2c.array_addr  = ((uint16)rx_array[3] << 8) | (rx_array[4]);
            
            PRINT(("I2C Prompt %d: addr %X %X %X %X size %lX dec %X\n", index, rx_array[1], rx_array[2], rx_array[3], rx_array[4], prompt->size, prompt->decompression));
            
            /* Finished with header source, close it */
            if(!SourceClose(lSource))
                Panic();
                
            return StreamI2cSource(prompt->location.addr.i2c.slv_addr, prompt->location.addr.i2c.array_addr, prompt->size);
        break;
#endif
#ifdef INSTALL_SPI_VP_SUPPORT
        case voice_prompts_type_spi:
            /* Array addr is actually 24 bit, ignore rx_array[1] */
            prompt->location.addr.spi.array_addr = ((uint32)rx_array[2] << 16) | ((uint16)rx_array[3] << 8) | (rx_array[4]);
            
            PRINT(("SPI Prompt %d: addr %X %X %X %X size %lX dec %X\n", index, rx_array[1], rx_array[2], rx_array[3], rx_array[4], prompt->size, prompt->decompression));
    
            /* Finished with header source, close it */
            if(!SourceClose(lSource))
                Panic();
                
            return StreamSpiSource(prompt->location.addr.spi.array_addr, prompt->size, spi_protocol_none);
        break;
#endif
#ifdef INSTALL_FILE_VP_SUPPORT
        case voice_prompts_type_file:
            /* Get the prompt file name */
            sprintf(file_name, "prompts/%d.prm", rx_array[1]);
            
            PRINT(("File Prompt: %s dec %X\n", file_name, prompt->decompression));
            
            /* Finished with header source, close it */
            if(!SourceClose(lSource))
                Panic();
            
            return StreamFileSource(FileFind(FILE_ROOT, file_name, strlen(file_name)));
        break;
#endif
        default:
        break;
    }
    
    return NULL;
}

/****************************************************************************
DESCRIPTION
    Initialise indexing information that tells the lib where in EEPROM to pick up phrases 
*/

void CsrVoicePromptsPluginInit ( uint16 no_prompts, const voice_prompts_index* header_location, uint16 no_languages )
{
    PRINT(("VP: Init %d prompts %d languages ", no_prompts, no_languages));
    header.no_prompts = no_prompts;
    header.no_prompts_per_lang = no_prompts / no_languages;
    header.location   = header_location;
    PRINT((" Type: %X\n", header.location->type));
}

#ifdef INSTALL_DSP_VP_SUPPORT
/****************************************************************************
DESCRIPTION
    Plays back a voice prompt once DSP has loaded
*/

void CsrVoicePromptPluginPlayDsp(void)
{
    Sink lSink;
    
    /* Connect prompt source to kalimba */
    StreamConnect(phrase_data->source, StreamKalimbaSink(0));
    
    /* Connect PCM channel A */
    lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
    SinkConfigure(lSink, STREAM_CODEC_OUTPUT_RATE, phrase_data->playback_rate);
    PanicFalse(StreamConnectAndDispose(StreamKalimbaSource(0), lSink));
    
    /* Connect PCM channel B */
    if(phrase_data->stereo)
    {
        lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
        SinkConfigure(lSink, STREAM_CODEC_OUTPUT_RATE, phrase_data->playback_rate);
        PanicFalse(StreamConnectAndDispose(StreamKalimbaSource(1), lSink));
    }
    
    /*Set the output Gain immediately - scaled down to (roughly) match tone volume */
    CodecSetOutputGainNow(phrase_data->codec_task, phrase_data->tone_volume * 2/3, left_and_right_ch );
    
    /* Set the playback rate */
    KalimbaSendMessage(MESSAGE_SET_SAMPLE_RATE,phrase_data->playback_rate,0,0,1);
    
    /* Ready to go... */
    if (!KalimbaSendMessage(KALIMBA_MSG_GO,0,0,0,0))
    {
        PRINT(("VP: DSP failed to send go to kalimba\n"));
        Panic();
    }
}
#endif

/****************************************************************************
DESCRIPTION
    plays One digital number using the audio plugin    
*/

static void CsrVoicePromptsPluginPlayDigit(void) 
{
    Source lSource ;
    voice_prompt prompt;
    
    /* Get the prompt data*/
    lSource = csrVoicePromptsGetPrompt(&prompt);
    if(!lSource) Panic();
    
    AUDIO_BUSY = (TaskData*) &(csr_voice_prompts_plugin);

    /* Stash the source */
    phrase_data->source = lSource;
    phrase_data->decompression = prompt.decompression;
    phrase_data->playback_rate =  (prompt.playback_rate ? prompt.playback_rate : 8000);
    
    /* Connect the stream to the DAC */
    switch(prompt.decompression)
    {
        case voice_prompts_codec_ima_adpcm:
        case voice_prompts_codec_none:
        {
            /* Configure port 0 to be routed to internal codec A (and B), with a sample rate of 8k or configured rate. */
            Sink lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, (phrase_data->stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A));
            SinkConfigure(lSink, STREAM_CODEC_OUTPUT_RATE, phrase_data->playback_rate);
            /*Set the output Gain immediately - scaled down to (roughly) match tone volume */
            CodecSetOutputGainNow(phrase_data->codec_task, phrase_data->tone_volume * 2/3, left_and_right_ch );
            /* Get messages when source has finished */
            MessageSinkTask( lSink , (TaskData*) &csr_voice_prompts_plugin);
            /* Connect source to PCM */
            if(prompt.decompression == voice_prompts_codec_none)
                PanicFalse(StreamConnect(lSource, lSink));
            else
                PanicFalse(TransformStart(TransformAdpcmDecode(lSource, lSink)));
        }
        break;
        
#ifdef INSTALL_DSP_VP_SUPPORT
        case voice_prompts_codec_sbc:
        case voice_prompts_codec_mp3:
        {
            static const char sbc_decoder[] = "sbc_decoder/sbc_decoder.kap";
            static const char mp3_decoder[] = "mp3_decoder/mp3_decoder.kap";
            FILE_INDEX dsp_file;
            
            /* Find the DSP file to use */
            if(prompt.decompression == voice_prompts_codec_sbc)
                dsp_file = FileFind(FILE_ROOT, sbc_decoder, sizeof(sbc_decoder)-1);
            else
                dsp_file = FileFind(FILE_ROOT, mp3_decoder, sizeof(mp3_decoder)-1);
                
            /* Get messages from DSP */
            MessageKalimbaTask( (TaskData*) &csr_voice_prompts_plugin );
            /* Load DSP */
            if (!KalimbaLoad(dsp_file))
                Panic();
        }
        break;
#endif

        default:
            PRINT(("VP: Codec Invalid\n"));
            Panic();
        break;
    }
}

/****************************************************************************
DESCRIPTION
    plays a number phrase using the audio plugin    
*/

void CsrVoicePromptsPluginPlayPhrase (uint16 id , uint8 * data , uint16 size_data , uint16 language, Task codec_task , uint16 tone_volume , bool stereo )
{
    if(phrase_data != NULL)
        Panic();
    
    PRINT(("VP: Play Phrase\n"));
    
    /* Allocate the memory */
    phrase_data = (PHRASE_DATA_T *) PanicUnlessMalloc(sizeof(PHRASE_DATA_T) + size_data);
    
    /* Set up params */
    phrase_data->idx           = 0;
    phrase_data->language      = language;
    phrase_data->codec_task    = codec_task;
    phrase_data->tone_volume   = tone_volume;
    phrase_data->stereo        = stereo;
    phrase_data->size_data     = size_data;
    
    /* If size_data is 0 we use the ID, otherwise copy data to phrase_data */
    if(size_data)
        memmove((phrase_data->data), data, size_data);
    else
        phrase_data->data[0]   = id;
    
    MessageCancelAll((TaskData*) &csr_voice_prompts_plugin, MESSAGE_STREAM_DISCONNECT );
    MessageCancelAll((TaskData*) &csr_voice_prompts_plugin, MESSAGE_FROM_KALIMBA);
    
    CsrVoicePromptsPluginPlayDigit(); 
}

/****************************************************************************
DESCRIPTION
    Play a digit or kick off a new number playing
*/

void CsrVoicePromptsPluginPlayBack(void)
{
    if(phrase_data->idx >= phrase_data->size_data)
    {
        /* Stop the CSR Simple TTS */
        MessageSend( (TaskData*) &csr_voice_prompts_plugin, AUDIO_PLUGIN_STOP_TTS_MSG, NULL );
    }
    else
    {
        CsrVoicePromptsPluginPlayDigit () ;
    }
}

/****************************************************************************
DESCRIPTION
    Stop a tone from currently playing
*/
void CsrVoicePromptsPluginStopPhrase ( void ) 
{
    Sink lSink;

    if(!phrase_data)
        Panic();
        
    PRINT(("VP: Terminated\n"));
    
    switch(phrase_data->decompression)
    {
        case voice_prompts_codec_ima_adpcm:
        case voice_prompts_codec_none:
            lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, (phrase_data->stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A));
            
            /* Cancel all the messages relating to VP that have been sent */
            (void)MessageSinkTask(lSink, NULL);
            MessageCancelAll((TaskData*) &csr_voice_prompts_plugin, MESSAGE_STREAM_DISCONNECT);
            
            /* Disconnect PCM source/sink */
            StreamDisconnect(StreamSourceFromSink(lSink), lSink); 
            SinkClose(lSink);
        break;
        
#ifdef INSTALL_DSP_VP_SUPPORT
        case voice_prompts_codec_sbc:
        case voice_prompts_codec_mp3:
            /* Cancel all the messages relating to VP that have been sent */
            (void)MessageKalimbaTask(NULL);
            MessageCancelAll((TaskData*) &csr_voice_prompts_plugin, MESSAGE_FROM_KALIMBA);
            
            /* Disconnect PCM sources/sinks */
            lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
            StreamDisconnect(StreamSourceFromSink(lSink), lSink); 
            SinkClose(lSink);
            if(phrase_data->stereo)
            {
                lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
                StreamDisconnect(StreamSourceFromSink(lSink), lSink); 
                SinkClose(lSink);
            }
            /* PCM connected to kalimba, make sure prompt source is disconnected */
            StreamDisconnect(phrase_data->source, NULL);
            KalimbaPowerOff();
        break;
#endif
        
        default:
        break;
    }
    
    /* Make sure prompt source is disposed */
    if(SourceIsValid(phrase_data->source))
        StreamConnectDispose(phrase_data->source);
        
    /* Tidy up */
    free(phrase_data);
    phrase_data = NULL;
    AUDIO_BUSY = NULL ;
}


