/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_decoder.c
DESCRIPTION
    plugin implentation which routes the sco audio though the dsp
NOTES
*/

#include <audio.h>
#include <codec.h>
#include <stdlib.h>
#include <panic.h>
#include <print.h>
#include <file.h>
#include <stream.h> /*for the ringtone_note*/
#include <kalimba.h>
#include <kalimba_standard_messages.h>
#include <message.h> 
#include <ps.h>
#include <Transform.h> 
#include <string.h>
#include <pio.h>
                                                                                                                    
#include "audio_plugin_if.h" /*for the audio_mode*/
#include "csr_a2dp_decoder_common_plugin.h"
#include "csr_a2dp_decoder_common.h"
#include "csr_a2dp_decoder_common_if.h"


static void MusicConnectAudio (A2dpPluginTaskdata *task
#ifdef ENABLE_STEREO                               
							   , bool stereo 
#endif							 
							   );
          
#ifdef INCLUDE_FASTSTREAM          
static void MusicConnectFaststream(A2dpPluginConnectParams *codecData 
#ifdef ENABLE_STEREO
                                   , bool stereo
#endif
                                   );                               
#endif


/* dsp message structure*/
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;

typedef struct sync_Tag
{
    Sink media_sink ;    
    Task codec_task ;
    /*! mono or stereo*/
#ifdef ENABLE_STEREO
    unsigned stereo:1;
	unsigned unused:7;
#endif
    /*! The current mode */
    unsigned mode:8 ;   
    /* Additional mode parameters */
    uint16 mode_params;
    /*! The current volume level*/
    uint16 volume ;
    uint16 params;
    uint32 rate;    
    Task app_task;
}DECODER_t ;

    /*the synchronous audio data structure*/
static DECODER_t * DECODER = NULL ;

static bool pskey_read = FALSE;
static uint16 val_pskey_max_mismatch = 0;
static uint16 val_clock_mismatch = 0;

/*  The following PS Key can be used to define a non-default maximum clock mismatch between SRC and SNK devices.
    If the PS Key is not set, the default maximum clock mismatch value will be used.
    The default value has been chosen to have a very good THD performance and to avoid audible pitch shifting 
    effect even during harsh conditions (big jitters, for example). While the default covers almost all phones 
    and other streaming sources by a big margin, some phones could prove to have a larger percentage clock drift. 
*/
#define PSKEY_MAX_CLOCK_MISMATCH    0x2258 /* PSKEY_DSP0 */

/****************************************************************************
DESCRIPTION
	This function connects a synchronous audio stream to the pcm subsystem
*/ 

void CsrA2dpDecoderPluginConnect( A2dpPluginTaskdata *task, 
                                  Sink audio_sink , 
								  Task codec_task , 
								  uint16 volume , 
								  uint32 rate , 
#ifdef ENABLE_STEREO								  
								  bool stereo , 
#endif								  
								  AUDIO_MODE_T mode , 
								  const void * params, 
								  Task app_task ) 
{	
	FILE_INDEX index = FILE_NONE;
	char* kap_file = NULL ;

	PioSetDir32((1<<10),(1<<10));
	PioSet32((1<<10),(1<<10));
    
    /* Only need to read the PS Key value once */
    if (!pskey_read)
    {
        if (PsFullRetrieve(PSKEY_MAX_CLOCK_MISMATCH, &val_pskey_max_mismatch, sizeof(uint16)) == 0)
            val_pskey_max_mismatch = 0;
        pskey_read = TRUE;
    }
    
    switch ((A2DP_DECODER_PLUGIN_TYPE_T)task->a2dp_plugin_variant)
	{
	case SBC_DECODER:
		kap_file = "sbc_decoder/sbc_decoder.kap";
      break;
#ifdef INCLUDE_MP3	  
	case MP3_DECODER:
		kap_file = "mp3_decoder/mp3_decoder.kap";
		break;
#endif
#ifdef INCLUDE_AAC		
	case AAC_DECODER:
		kap_file = "aac_decoder/aac_decoder.kap";
		break;
#endif		
#ifdef INCLUDE_FASTSTREAM
    case FASTSTREAM_SINK:
		kap_file = "faststream_sink/faststream_sink.kap";
		break;
#endif        
	default:
		Panic();
		break;
	}
   
   
   /*ensure that the messages received are from the correct kap file*/ 
   (void) MessageCancelAll( (TaskData*) task, MESSAGE_FROM_KALIMBA);
   MessageKalimbaTask( (TaskData*) task );
    
    /* audio busy until DSP returns ready message */
    AUDIO_BUSY = (TaskData*) task;
    
	index = FileFind(FILE_ROOT,(const char *) kap_file ,strlen(kap_file));

	if (index == FILE_NONE)
		Panic();
	if (!KalimbaLoad(index))
		Panic();	
   
    DECODER = (DECODER_t*)PanicUnlessMalloc (sizeof (DECODER_t) ) ;
    
    DECODER->media_sink = audio_sink ;
    DECODER->codec_task = codec_task ;
    DECODER->volume     = volume;
    DECODER->mode       = mode;
    DECODER->mode_params = 0;
#ifdef ENABLE_STEREO	
	DECODER->stereo     = stereo;
#endif
    DECODER->params     = (uint16) params;
    DECODER->rate       = rate;    
	DECODER->app_task	= app_task;  
    
#ifdef INCLUDE_AAC	
	if ((A2DP_DECODER_PLUGIN_TYPE_T)task->a2dp_plugin_variant == AAC_DECODER)
	{
		/* Workaround for AAC+ sources that negotiate sampling frequency at half the actual value */
		if (rate < 32000)
			DECODER->rate = rate * 2;
	}
#endif
	
	CodecSetOutputGainNow(DECODER->codec_task, 0, left_and_right_ch);	
   
   /* For sinks disconnect the source in case its currently being disposed. */
   	StreamDisconnect(StreamSourceFromSink(audio_sink), 0);
   
	PRINT(("DECODER: CsrA2dpDecoderPluginConnect completed\n"));	
	
	PioSet32((1<<10),0);
}

/****************************************************************************
DESCRIPTION
	Disconnect Sync audio
*/
void CsrA2dpDecoderPluginDisconnect( A2dpPluginTaskdata *task ) 
{       
    Sink audio_sink_a = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);    
    
    if (!DECODER)
        Panic() ;

	PioSetDir32((1<<10),(1<<10));
	PioSet32((1<<10),(1<<10));
           
        /*disconnect the pcm streams*/  
#ifdef INCLUDE_FASTSTREAM  
    {
        Source mic_src = StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
        if (mic_src)
        {
            A2dpPluginConnectParams *codecData = (A2dpPluginConnectParams *) DECODER->params;
            if (codecData && codecData->voice_rate)
            {
                StreamDisconnect(mic_src, 0);                  
            }
            SourceClose(mic_src);
        }
    }
#endif    
    if (audio_sink_a)
    {
        StreamDisconnect(0, audio_sink_a);
        SinkClose(audio_sink_a);
    }
#ifdef ENABLE_STEREO
    {
        Sink audio_sink_b = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
        if (audio_sink_b)
        {
            if (DECODER->stereo)
            {
                StreamDisconnect(0, audio_sink_b);            
            }
            SinkClose(audio_sink_b);
        }
    }
#endif    
    
	   /* For sinks disconnect the source in case its currently being disposed. */
    StreamDisconnect(StreamSourceFromSink(DECODER->media_sink ), 0);
    StreamConnectDispose (StreamSourceFromSink(DECODER->media_sink)) ;
    
    (void) MessageCancelAll( (TaskData*) task, MESSAGE_FROM_KALIMBA);
    MessageKalimbaTask( NULL );

    KalimbaPowerOff() ;

    free (DECODER);
    DECODER = NULL ;        

	PioSet32((1<<10),0);
}

/****************************************************************************
DESCRIPTION
	Indicate the volume has changed
*/
void CsrA2dpDecoderPluginSetVolume(A2dpPluginTaskdata *task, uint16 volume ) 
{
    if (volume > 0xf)
		volume = 0xf;
    if (volume < 0)
        volume = 0;

    KalimbaSendMessage(MUSIC_VOLUME_MSG, 0, 0, volume, volume);	
}


/****************************************************************************
DESCRIPTION
	Sets the audio mode
*/
void CsrA2dpDecoderPluginSetMode ( AUDIO_MODE_T mode , A2dpPluginTaskdata *task , const void * params ) 
{   
    A2dpPluginModeParams *mode_params = NULL;
    A2DP_MUSIC_PROCESSING_T music_processing = A2DP_MUSIC_PROCESSING_PASSTHROUGH;
    
    if (!DECODER)
       	Panic() ;
	    
    DECODER->mode = mode;  

    switch (mode)
    {
        case AUDIO_MODE_MUTE_SPEAKER:        
        case AUDIO_MODE_MUTE_BOTH:
            {     
                PRINT(("DECODER: Set Audio Mode Mute [%x]\n" , mode ));
                KalimbaSendMessage(MUSIC_SETMODE_MSG, MUSIC_SYSMODE_STANDBY, 1, 0, 0);
            }
            break ;
        case AUDIO_MODE_MUTE_MIC:
        case AUDIO_MODE_CONNECTED:
            {
                PRINT(("DECODER: Set Audio Mode Processing [%x]\n" , mode ));
                if (params)
                {
                    /* if mode parameters supplied then use these */
                    mode_params = (A2dpPluginModeParams *)params;
                    music_processing = mode_params->music_mode_processing;
                    DECODER->mode_params = (uint16)params;
                }
                else if (DECODER->mode_params)
                {
                    /* if previous mode params exist then revert to back to use these */
                    mode_params = (A2dpPluginModeParams *)DECODER->mode_params;
                    music_processing = mode_params->music_mode_processing;
                }
                
                switch (music_processing)
                {
                    case A2DP_MUSIC_PROCESSING_PASSTHROUGH:
                        {
                            KalimbaSendMessage (MUSIC_SETMODE_MSG , MUSIC_SYSMODE_PASSTHRU , 0, 0, 0 ) ;
                            PRINT(("DECODER: Set Music Mode SYSMODE_PASSTHRU\n"));
                        }
                        break;             
                    case A2DP_MUSIC_PROCESSING_FULL_EQ1:
                        {
                            KalimbaSendMessage (MUSIC_SETMODE_MSG , MUSIC_SYSMODE_FULLPROC , 0, 0, 0);
                            PRINT(("DECODER: Set Music Mode SYSMODE_FULLPROC eq 1\n"));
                        }
                        break;
                    case A2DP_MUSIC_PROCESSING_FULL_NO_EQ:
                        {
                            KalimbaSendMessage (MUSIC_SETMODE_MSG , MUSIC_SYSMODE_FULLPROC , 1, 0, 0); 
                            PRINT(("DECODER: Set Music Mode SYSMODE_FULLPROC eq 2\n"));
                        }
                        break;
                    case A2DP_MUSIC_PROCESSING_FULL_EQ2:
                        {
                            KalimbaSendMessage (MUSIC_SETMODE_MSG , MUSIC_SYSMODE_FULLPROC , 2, 0, 0);
                            PRINT(("DECODER: Set Music Mode SYSMODE_FULLPROC eq 3\n"));
                        }
                        break;
                    default:  
                        {
                            PRINT(("DECODER: Set Music Mode Invalid [%x]\n" , mode ));
                        }
                        break;
                }
            }
            break;
        default:    
            {
                PRINT(("DECODER: Set Audio Mode Invalid [%x]\n" , mode ));
            }
            break;
    }
}


/****************************************************************************
DESCRIPTION
    plays a tone using the audio plugin    
*/
void CsrA2dpDecoderPluginPlayTone ( A2dpPluginTaskdata *task, ringtone_note * tone, Task codec_task, uint16 tone_volume 
#ifdef ENABLE_STEREO
                                    , bool stereo
#endif								
									) 
{    
    Source lSource ;  
    Sink lSink ; 
	
    if (!DECODER)
        Panic() ;
    
    PRINT(("DECODER: Tone Start\n")) ;

    lSink = StreamKalimbaSink(3) ;
    
        /*request an indication that the tone has completed / been disconnected*/
    MessageSinkTask ( lSink , (TaskData*) task ) ;

        /*connect the tone*/
    lSource = StreamRingtoneSource ( (const ringtone_note *) (tone) ) ;    
        /*mix the tone to the SBC*/    
    StreamConnectAndDispose( lSource , lSink ) ; 
	
}

/****************************************************************************
DESCRIPTION
	Stop a tone from currently playing
*/
void CsrA2dpDecoderPluginStopTone ( void ) 
{  
    PRINT(("DECODER: Stop Tone\n")) ;
    if (!DECODER)
        Panic() ;
        
     StreamDisconnect ( 0 , StreamKalimbaSink(3) ) ;
}

/****************************************************************************
DESCRIPTION
	Reconnects the audio after a tone has completed 
*/
void CsrA2dpDecoderPluginToneComplete ( void ) 
{
    PRINT(("DECODER: Tone Complete\n")) ;
        /*we no longer want to receive stream indications*/
    MessageSinkTask ( StreamKalimbaSink(3) , NULL) ;
}

/****************************************************************************
DESCRIPTION
	handles the internal cvc messages /  messages from the dsp
*/
void CsrA2dpDecoderPluginInternalMessage( A2dpPluginTaskdata *task ,uint16 id , Message message ) 
{
	switch(id)
	{
        case MESSAGE_FROM_KALIMBA:
		{
			const DSP_REGISTER_T *m = (const DSP_REGISTER_T *) message;
	        PRINT(("DECODER: msg id[%x] a[%x] b[%x] c[%x] d[%x]\n", m->id, m->a, m->b, m->c, m->d));
		
            switch ( m->id ) 
			{
              case MUSIC_READY_MSG:
                {
					if (DECODER)
					{
                    	KalimbaSendMessage(MUSIC_LOADPARAMS_MSG, MUSIC_PS_BASE, 0, 0, 0);
                    
                    	/*A2dp is now loaded, signal that tones etc can be scheduled*/
                    	AUDIO_BUSY = NULL ;
                    
                    	PRINT(("DECODER: DECODER_READY \n"));
                                        
                    	MusicConnectAudio (task
#ifdef ENABLE_STEREO
						                   , DECODER->stereo
#endif										   
										   ); 
										   
					}
                }
                break;				
			    case MUSIC_CODEC_MSG:
	            {                    	            
                    uint16 lOutput_gain_l = m->a;
                    uint16 lOutput_gain_r = m->b;
      
					if (DECODER)
					{
                    	CodecSetOutputGainNow(DECODER->codec_task, lOutput_gain_l, left_ch);
                    	CodecSetOutputGainNow(DECODER->codec_task, lOutput_gain_r, right_ch);                    
					}
	            }
                break;								
                case KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE:
				{
                    if (DECODER)
					{
					    MAKE_AUDIO_MESSAGE(AUDIO_PLUGIN_DSP_IND);
						PRINT(("DECODER: Send CLOCK_MISMATCH_RATE\n"));
					    message->id = KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE;
					    message->value = m->a;
					    MessageSend(DECODER->app_task, AUDIO_PLUGIN_DSP_IND, message);
                    }
					break;	
				}
            }
		}
		break;
		
        default:
        break ;
	}		
}	

/****************************************************************************
DESCRIPTION
	Connect the encoded audio input and pcm audio output streams
*/
static void MusicConnectAudio (A2dpPluginTaskdata *task
#ifdef ENABLE_STEREO
                               , bool stereo 
#endif							  
                                )
{
	A2dpPluginConnectParams *codecData = (A2dpPluginConnectParams *) DECODER->params;
    A2dpPluginModeParams *mode_params = NULL;    
	uint8 content_protection;
	uint16 scms_enabled = 0;
	Transform rtp_transform = 0;
	Sink speaker_snk_a = NULL;

	if(codecData != NULL)
	{
		val_clock_mismatch = codecData->clock_mismatch;
		content_protection = codecData->content_protection;
        if (codecData->mode_params != NULL)
            mode_params = codecData->mode_params;
            
	}
	else
	{
		val_clock_mismatch = 0;
		content_protection = 0;
	}
	
	switch ((A2DP_DECODER_PLUGIN_TYPE_T)task->a2dp_plugin_variant)
	{
	case SBC_DECODER:
		rtp_transform = TransformRtpSbcDecode(StreamSourceFromSink(DECODER->media_sink) , StreamKalimbaSink(0));
		break;
#ifdef INCLUDE_MP3		
	case MP3_DECODER:
		rtp_transform = TransformRtpMp3Decode(StreamSourceFromSink(DECODER->media_sink) , StreamKalimbaSink(0));
		break;
#endif
#ifdef INCLUDE_AAC		
	case AAC_DECODER:
		rtp_transform = TransformRtpAacDecode(StreamSourceFromSink(DECODER->media_sink) , StreamKalimbaSink(0));
		break;
#endif		
#ifdef INCLUDE_FASTSTREAM
    case FASTSTREAM_SINK:
        if (codecData)
            MusicConnectFaststream(codecData
#ifdef ENABLE_STEREO        
                                    , stereo
#endif                                
                                    );
        break;
#endif
	default:
		break;
	}

#ifdef INCLUDE_FASTSTREAM    
    if ((A2DP_DECODER_PLUGIN_TYPE_T)task->a2dp_plugin_variant != FASTSTREAM_SINK)
#endif    
    {
        /* Configure the content protection */
        if (content_protection)
            scms_enabled = 1;

        TransformConfigure(rtp_transform, VM_TRANSFORM_RTP_SCMS_ENABLE, scms_enabled);

        /*start the transform decode*/
        (void)TransformStart( rtp_transform ) ;

        /* is it mono playback? */
    #ifdef ENABLE_STEREO
        if ( !stereo )
        {
    #endif
            PRINT(("DECODER: Mono\n"));

            /* always AUDIO_ROUTE_INTERNAL:*/
            speaker_snk_a = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
            PanicFalse(SinkConfigure(speaker_snk_a, STREAM_CODEC_OUTPUT_RATE, DECODER->rate));
            
            /* plug port 0 into both DACs */
            (void) PanicFalse(StreamConnect(StreamKalimbaSource(0),speaker_snk_a));
            
    #ifdef ENABLE_STEREO
        }
        else
        {
            Sink speaker_snk_b = NULL;
            
            PRINT(("DECODER: Stereo\n"));

            /*always AUDIO_ROUTE_INTERNAL:*/
            speaker_snk_a = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
            speaker_snk_b = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
            PanicFalse(SinkConfigure(speaker_snk_a, STREAM_CODEC_OUTPUT_RATE, DECODER->rate));
            PanicFalse(SinkConfigure(speaker_snk_b, STREAM_CODEC_OUTPUT_RATE, DECODER->rate));
            PanicFalse(SinkSynchronise(speaker_snk_a, speaker_snk_b));
                    
            /* plug port 0 into Left DAC */
            PanicFalse(StreamConnect(StreamKalimbaSource(0),speaker_snk_a));
            /* plug port 1 into Right DAC */
            PanicFalse(StreamConnect(StreamKalimbaSource(1),speaker_snk_b));
        }
    #endif
    }

    CsrA2dpDecoderPluginSetMode(DECODER->mode, task, mode_params);
    
	CsrA2dpDecoderPluginSetVolume(task,DECODER->volume) ;	

	/* The DSP must know the sample rate for tone mixing */
	KalimbaSendMessage(MESSAGE_SET_SAMPLE_RATE,DECODER->rate, val_pskey_max_mismatch, val_clock_mismatch,0);
	PRINT(("DECODER: Send Go message to DSP now\n"));
	if(!KalimbaSendMessage(KALIMBA_MSG_GO,0,0,0,0))
	{
		PRINT(("DECODER: Message KALIMBA_MSG_GO failed!\n"));
		Panic();
	}	
}


#ifdef INCLUDE_FASTSTREAM
static void MusicConnectFaststream(A2dpPluginConnectParams *codecData 
#ifdef ENABLE_STEREO
                                ,bool stereo
#endif
                             )
{
    uint32 voice_rate = 0;
    Sink speaker_snk_a = NULL;
    /*
        FastStream does not use RTP.
        L2CAP frames enter/leave via port 2
    */

    /*
        Initialise PCM.
        Output stereo at 44k1Hz or 48kHz, input from left ADC at 16kHz.
    */

    /* If no voice rate is set just make the ADC rate equal to 16kHz */
    if (!codecData->voice_rate)
        voice_rate = 16000;
    else
        voice_rate = codecData->voice_rate;

    PRINT(("DECODER: FastStream rate=0x%lx voice_rate=0x%lx\n format=0x%x bitpool=0x%x",DECODER->rate,codecData->voice_rate,codecData->format,codecData->bitpool));
    speaker_snk_a = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
    PanicFalse(SinkConfigure(speaker_snk_a, STREAM_CODEC_OUTPUT_RATE, DECODER->rate));

#ifdef ENABLE_STEREO    
    /* is it mono playback? */
    if ( !stereo )
#endif    
    {
        PRINT(("DECODER: Mono\n"));
        /* Connect Kalimba to PCM */
        if (DECODER->rate)
        {
            StreamDisconnect(StreamSourceFromSink(DECODER->media_sink), 0);
            PanicFalse(StreamConnect(StreamKalimbaSource(0),speaker_snk_a));
            PanicFalse(StreamConnect(StreamSourceFromSink(DECODER->media_sink),StreamKalimbaSink(2)));
        }
    }
#ifdef ENABLE_STEREO    
    else
    {        
        Sink speaker_snk_b = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
        
        PRINT(("DECODER: Stereo\n"));
        PanicFalse(SinkConfigure(speaker_snk_b, STREAM_CODEC_OUTPUT_RATE, DECODER->rate));
        PanicFalse(SinkSynchronise(speaker_snk_a, speaker_snk_b));        
        /* Connect Kalimba to PCM */
        if (DECODER->rate)
        {
            StreamDisconnect(StreamSourceFromSink(DECODER->media_sink), 0);
            PanicFalse(StreamConnect(StreamKalimbaSource(0),speaker_snk_a));
            PanicFalse(StreamConnect(StreamKalimbaSource(1),speaker_snk_b));
            PanicFalse(StreamConnect(StreamSourceFromSink(DECODER->media_sink),StreamKalimbaSink(2)));
        }
    }
#endif    

    if (codecData->voice_rate)
    {
        Source mic_src = StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
        
        PanicFalse(SourceConfigure(mic_src, STREAM_CODEC_INPUT_RATE, codecData->voice_rate));
        StreamDisconnect(0, DECODER->media_sink);
        /* configure parameters */
        PanicFalse(KalimbaSendMessage(KALIMBA_MSG_SBCENC_SET_PARAMS, codecData->format, 0, 0, 0));
        PanicFalse(KalimbaSendMessage(KALIMBA_MSG_SBCENC_SET_BITPOOL, codecData->bitpool, 0, 0, 0));
        PanicFalse(StreamConnect(mic_src, StreamKalimbaSink(0)));
        PanicFalse(StreamConnect(StreamKalimbaSource(2),DECODER->media_sink));
    }
}
#endif /* INCLUDE_FASTSTREAM */
