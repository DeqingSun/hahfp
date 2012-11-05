/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_cvc_common.c
    
DESCRIPTION
NOTES
*/

#ifndef SCO_DSP_BOUNDARIES
#define SCO_DSP_BOUNDARIES
#endif

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
#include "csr_cvc_common_if.h"      /*for things common to all CVC systems*/
#include "csr_cvc_common_plugin.h"
#include "csr_cvc_common.h"

#include <source.h>
#include <transform.h>
#include <app/vm/vm_if.h>
#include <pio.h>

#define MESSAGE_SCO_CONFIG       (0x2000)

/*helper functions*/
static void CvcConnectAudio (CvcPluginTaskdata *task);

static void CvcCodecMessage (CvcPluginTaskdata *task, uint16 input_gain_l,uint16 input_gain_r, uint16 output_gain ) ;
static void DisconnectMicNoDsp ( void );
static void DisconnectSpeakerNoDsp ( bool );
static void ConnectMicNoDsp ( void );
static void ConnectSpeakerNoDsp ( void ) ;
#ifdef CVC_ALL
static void ConnectSecondMicNoDsp ( void );
static void DisconnectSecondMicNoDsp ( void );
#endif

typedef struct audio_Tag
{
        /*! Whether or not CVC is running */
    unsigned cvc_running:1 ;
		/* set if in low power(CVC off) or no DSP mode*/
	unsigned no_dsp:1 ;
    unsigned reserved:4 ;
        /*! mono or stereo*/
    unsigned stereo:1;
        /*! The current CVC mode */
    unsigned mode:8 ; 	
        /*! whether mic switch active */
    unsigned mic_switch:1;
        /*! The codec being used*/
    Task codec_task ; 
        /*! The audio sink being used*/
    Sink audio_sink ;
        /*! The link_type being used*/
    AUDIO_SINK_T link_type ;
        /*! The current volume level*/
    uint16 volume ;
        /*! The current tone volume level*/
    uint16 tone_volume ;  
        /*! Over the air rate  */
    uint32 dac_rate;
        /*! Audio rate - used for mic switch */
    uint32 audio_rate;
       /*! is tone mono or stereo*/
    unsigned tone_stereo:1;
}CVC_t ;

/* The CVC task instance pointer*/
static CVC_t * CVC = NULL;

/* dsp message structure*/
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;



/****************************************************************************
NAME	
	CsrCvcPluginConnect

DESCRIPTION
	This function connects cvc to the stream subsystem

RETURNS
	void
*/
void CsrCvcPluginConnect( CvcPluginTaskdata *task, 
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
    FILE_INDEX index=0; 
	 char* kap_file = NULL ;

	/*signal that the audio is busy until the kalimba / parameters are fully loaded so that no tone messages etc will arrive*/
    AUDIO_BUSY = (TaskData*) task;    
    
    if (CVC) Panic();
    CVC = PanicUnlessNew ( CVC_t ); 

	PioSetDir32((1<<10),(1<<10));
	PioSet32((1<<10),(1<<10));
    
	/* The DAC gain must be limited to 0 dB so that no distortion occurs and so the echo canceller works. */
    if (volume > 0xf)
		volume = 0xf;

    CVC->cvc_running     = FALSE;
    CVC->codec_task      = codec_task;                                                        
    CVC->link_type       = sink_type ;                                                        
    CVC->volume          = volume;
    CVC->audio_sink      = audio_sink;
    CVC->mode            = mode;
    CVC->tone_volume     = volume;
    CVC->audio_rate      = rate;
	 CVC->stereo          = stereo;
    CVC->mic_switch      = mic_switch;
    CVC->tone_stereo     = stereo;
	 /* check if this is the no dsp plugin or should be started in low power mode */
	 CVC->no_dsp = (CVC_PLUGIN_TYPE_T)task->cvc_plugin_variant == CVSD_NO_DSP || power <= LPIBM_THRESHOLD;


	PRINT(("CVC: connect [%x] [%x]\n", CVC->cvc_running , (int)CVC->audio_sink));
	  
    
    /* Calculate the DAC rate based on the over-the-air rate value passed in from VM */
	 if(CVC->no_dsp)
    {	 /* force DAC rate to 8k if in low power mode and not WBS */
		 CVC->dac_rate = 8000; 
    }
	 else
    {
		 /* Set ADC & DAC to 16 or 8 kHz  */   
		 CVC->dac_rate = (task->adc_dac_16kHz)?16000:8000; 
    }
    
	/* Fow WBS set SBC Frame size, else sample-based */
    if(((CVC_PLUGIN_TYPE_T)task->sco_encoder) == SCO_ENCODING_SBC)
    {
         SinkConfigure(CVC->audio_sink,VM_SINK_SCO_SET_FRAME_LENGTH,60); 
    }
	
    /* Enable metadata if required */
    /* Notes on Metadata.  It's currently not possible to switch off SCO metadata after it's 
                enabled so if CVC is used the plugin cannot be switched to No DSP on the active SCO link.
                However if it is enabled here and no DSP mode is used first then metadata won't actually be enabled until CVC is used
	      it's just not possible to switch back to No DSP.
	      Commented out calls below to enable/disable metadata are where this should actuallyy be done if it were possible.*/
    SourceConfigure(StreamSourceFromSink( CVC->audio_sink ),VM_SOURCE_SCO_METADATA_ENABLE,1);    

    /* if in no DSP mode then just connect the ports, if this is cVc then continue and load the DSP */
	if (CVC->no_dsp)
	{
		PRINT(("CVC: connect No DSP\n"));
	
		/* disable MetaData */
/*		PanicFalse(SourceConfigure(StreamSourceFromSink( CVC->audio_sink ),VM_SOURCE_SCO_METADATA_ENABLE,0));*/
	
		CvcConnectAudio (task) ;   
			
		CsrCvcPluginSetVolume(task, volume);
		AUDIO_BUSY = NULL ;
		PioSet32((1<<10),0);
		return;
	}  

		/* Enable MetaData */
/*    PanicFalse(SourceConfigure(StreamSourceFromSink( CVC->audio_sink ),VM_SOURCE_SCO_METADATA_ENABLE,1));   */
   
    /*ensure that the messages received are from the correct kap file*/ 
    (void) MessageCancelAll( (TaskData*) task, MESSAGE_FROM_KALIMBA);
    MessageKalimbaTask( (TaskData*) task );
   
	 /* Select which Kap file to be loaded based on the plugin selected */
   switch((CVC_PLUGIN_TYPE_T)task->cvc_plugin_variant)
	{
      case CVSD_CVC_1_MIC_HEADSET:
            PRINT(("CVC: load cvsd 1mic headset kap file\n"));
            kap_file = "cvc_headset/cvc_headset.kap";		
    	break;
      case CVSD_CVC_1_MIC_HANDSFREE:
            PRINT(("CVC: load cvsd 1mic handsfree kap file\n"));
    		kap_file = 	"cvc_handsfree/cvc_handsfree.kap";
    	break;		
    	case CVSD_CVC_2_MIC_HEADSET:
            PRINT(("CVC: load cvsd 2mic headset kap file\n"));
    		kap_file = 	"cvc_headset_2mic/cvc_headset_2mic.kap";
    	break;						
  	             
    	default:		
    		PRINT(("CVC: No Corresponding Kap file (%u)\n", (CVC_PLUGIN_TYPE_T)task->cvc_plugin_variant)) ;
			/* note this function should not be called for the no dsp plugin */
    		Panic() ;
    	break;
	}
	
    index = FileFind(FILE_ROOT,(const char *) kap_file ,strlen(kap_file));
    
    if( index == FILE_NONE )
    {
         PRINT(("CVC: No File\n"));
	     Panic();
    }
   	    
    PRINT(("CVC: Loading %d\n",index));

    /* Load the cvc algorithm into Kalimba*/
    if( !KalimbaLoad( index ) )
    {
        PRINT(("CVC: Kalimba load fail\n"));
        Panic();
    }
         
    /* Now the kap file has been loaded, wait for the CVC_READY_MSG message from the
       dsp to be sent to the message_handler function. */     
	PioSet32((1<<10),0);
}

/****************************************************************************
NAME	
	CsrCvcPluginDisconnect

DESCRIPTION
	Disconnect CVC and power off the Kalimba DSP core
    
RETURNS
	void
*/
void CsrCvcPluginDisconnect( CvcPluginTaskdata *task )
{
    if (!CVC)
        Panic() ;
		
	PioSetDir32((1<<10),(1<<10));
	PioSet32((1<<10),(1<<10));
	
    CodecSetOutputGainNow( CVC->codec_task, DAC_MUTE, left_and_right_ch );  

	if (CVC->no_dsp)
	{
	    PRINT(("CVC: NO DSP: disconnect\n"));

		DisconnectSpeakerNoDsp(FALSE) ;
		DisconnectMicNoDsp() ;
#ifdef CVC_ALL        
        DisconnectSecondMicNoDsp() ;
#endif        

		CodecSetOutputGainNow( CVC->codec_task, CVC->volume , left_and_right_ch );
	}
	else
	{
		Source lSource0 = NULL;
		Sink   lSink0 = NULL;	
		Source lSource1 = NULL;
      Sink   lSink1 = NULL;
			
		/* check cvc running */
		if ( CVC->cvc_running == FALSE )
			Panic() ;

      PRINT(("CVC: Destroy transforms\n")); 

      

      /* Configure port 0 to be routed to internal ADC and DACs */
	    if(CVC->stereo & !(task->two_mic))       /* configure A_AND_B only if 1mic and in stereo  */
          lSink0 = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A_AND_B);
       else		
		    lSink0 = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
		 lSource0 = StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
       
       TransformDisconnect(TransformFromSource(lSource0));
       TransformDisconnect(TransformFromSink(lSink0));

       if( task->two_mic )
       {
		    lSource1 = StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
          lSink1   = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);

          TransformDisconnect(TransformFromSource(lSource1));
          TransformDisconnect(TransformFromSink(lSink1));
       }

		
		StreamDisconnect(lSource0, StreamKalimbaSink(0)); 
		StreamDisconnect(StreamKalimbaSource(0), lSink0); 
        SourceClose(lSource0);
        SinkClose(lSink0);
		PRINT(("CVC: Streams disconnected (left)\n")); 
			
		if( task->two_mic )
		{
         StreamDisconnect( lSource1,StreamKalimbaSink(2)); 
         StreamDisconnect(StreamKalimbaSource(2),lSink1);   
         SourceClose(lSource1);
         SinkClose(lSink1);
			PRINT(("CVC: Streams disconnected (Right)\n")); 
		}
	
		
	    
      TransformDisconnect(TransformFromSink(StreamKalimbaSink(1)));
		TransformDisconnect(TransformFromSource(StreamKalimbaSource(1)));
      StreamDisconnect(StreamSourceFromSink( CVC->audio_sink ),StreamKalimbaSink(1)); /* SCO->DSP */  
      StreamDisconnect( StreamKalimbaSource(1), CVC->audio_sink ); /* DSP->SCO */

        
		PRINT(("CVC: Disconnected\n"));
	}
	
	CVC->cvc_running = FALSE;	
	CVC->audio_sink = NULL;
	CVC->link_type = 0;
 
    /* Cancel any outstanding cvc messages */
    MessageCancelAll( (TaskData*)task , MESSAGE_FROM_KALIMBA);
    MessageCancelAll( (TaskData*)task , MESSAGE_STREAM_DISCONNECT);

    free (CVC);
    CVC = NULL;                
    
    KalimbaPowerOff();        
	PioSet32((1<<10),0);
}

/****************************************************************************
NAME	
	CsrCvcPluginSetVolume

DESCRIPTION
	Tell CVC to update the volume.

RETURNS
	void
*/
void CsrCvcPluginSetVolume( CvcPluginTaskdata *task, uint16 volume )
{
    if (!CVC)
        Panic() ;

	CVC->volume = volume;
	
	if (CVC->no_dsp)
	{
	    PRINT(("CVC: NO DSP: Set volume\n"));
	    /*Set the output Gain immediately*/
		CodecSetOutputGainNow( CVC->codec_task, CVC->volume , left_and_right_ch );
		return;
	}
	
		/* The DAC gain must be limited to 0 dB so that no distortion occurs and so the echo canceller works. */
	if (volume > 0xf)
		volume = 0xf;
          
    PRINT(("CVC: DAC GAIN SET[%x]\n", CVC->volume ));
    
    /* Only update the volume if not in a mute mode */
    if ( CVC->cvc_running && !( (CVC->mode==AUDIO_MODE_MUTE_SPEAKER ) || (CVC->mode==AUDIO_MODE_MUTE_BOTH ) ) )
    { 
       KalimbaSendMessage(CVC_VOLUME_MSG, 0, 0, CVC->volume, CVC->tone_volume); 
    }         
}


/****************************************************************************
NAME
    CsrCvcPluginSetModeNoDsp

DESCRIPTION
    Set the CSR_COMMON_NO_DSP mode

RETURNS
    void
*/
void CsrCvcPluginSetModeNoDsp ( AUDIO_MODE_T mode , const void * params )
{
    CVC->mode = mode;
	
	if(params)
	{	
		free((cvc_extended_parameters_t *) params);
	}

    switch (mode)
    {
        case AUDIO_MODE_MUTE_SPEAKER :
        {
            PRINT(("CVC: NODSP: SetMode MUTE SPEAKER\n"));
            DisconnectSpeakerNoDsp(FALSE);
        }
        break ;
        case AUDIO_MODE_CONNECTED :
        {
            PRINT(("CVC: NODSP: Set Mode CONNECTED\n"));
            ConnectMicNoDsp();
            ConnectSpeakerNoDsp();            
        }
        break ;
        case AUDIO_MODE_MUTE_MIC :
        {
            PRINT(("CVC: NODSP: Set Mode MUTE MIC"));
            DisconnectMicNoDsp() ;
        }
        break ;
        case AUDIO_MODE_MUTE_BOTH :
        {
            PRINT(("CVC: NODSP: Set Mode MUTE BOTH\n"));
            DisconnectMicNoDsp() ;
            DisconnectSpeakerNoDsp(FALSE) ;
        }
        break ;
        default :
        {    
            PRINT(("NODSP: Set Mode Invalid [%x]\n" , mode )) ;
        }
        break ;
    }
}


/****************************************************************************
NAME	
	CsrCvcPluginSetMode

DESCRIPTION
	Set the CVC mode

RETURNS
	void
*/
void CsrCvcPluginSetMode ( CvcPluginTaskdata *task, AUDIO_MODE_T mode , const void * params )
{
    
    if (!CVC)
        Panic();            
		
		/* check if in no dsp mode */
	if (CVC->no_dsp)
	{		               	
		CsrCvcPluginSetModeNoDsp(mode , params) ;
		return;
	}
	
	/* check if cvc running */
	if ( CVC->cvc_running == FALSE )
        Panic();
    
	/* must be in CVC mode */
    if (params)
    {
        cvc_extended_parameters_t * cvc_params = (cvc_extended_parameters_t *) params;
        switch (*cvc_params)
        {
        case CSR_CVC_HFK_ENABLE:
        {
            KalimbaSendMessage(CVC_SETMODE_MSG , SYSMODE_HFK , 0, CALLST_CONNECTED, 0 );
            KalimbaSendMessage(CVC_VOLUME_MSG, 0, 0, CVC->volume, CVC->tone_volume);
            PRINT(("CVC : Set Mode SYSMODE_HFK CALLST_CONNECTED\n"));
        }    
        break;
        case CSR_CVC_PSTHRU_ENABLE:
        {
            KalimbaSendMessage (CVC_SETMODE_MSG , SYSMODE_PSTHRGH , 0, CALLST_CONNECTED, 0 ) ;
            KalimbaSendMessage(CVC_VOLUME_MSG, 0, 0, CVC->volume, 0);
   	        PRINT(("CVC : Set Mode SYSMODE_PSTHRGH CALLST_CONNECTED\n"));
        }
        break;
		
        default:
        break;
        }        
        free(cvc_params);
    }
    else
    {	
		/* pre-initialise with the most common parameters and adjust below as necessary */
		uint16 sysmode = SYSMODE_HFK;
		uint16 callState = CALLST_CONNECTED;
		uint16 volume = CVC->volume;
		
		/* store current mode */
		CVC->mode = mode;	
		
		switch (mode)
		{
			case AUDIO_MODE_MUTE_SPEAKER:
			{     
				sysmode = SYSMODE_ASR;
				/*callState = CALLST_CONNECTED;*/
				volume = DAC_MUTE;
				PRINT(("CVC: Set DAC gain to 0 and SYMODE_ASR\n"));
			 }
			break ;
			case AUDIO_MODE_CONNECTED:
			{
				/*sysmode = SYSMODE_HFK;*/
				/*callState = CALLST_CONNECTED;*/	
				/*volume = CVC->volume;*/
			   PRINT(("CVC: Set Mode SYSMODE_HFK CALLST_CONNECTED\n"));
			}
			break ;
			case AUDIO_MODE_MUTE_MIC:
			{
				/*sysmode = SYSMODE_HFK;*/
				callState = CALLST_MUTE;
				/*volume = CVC->volume;*/
			   PRINT(("CVC: Set Mode SYSMODE_HFK CALLST_MUTE\n"));
			}
			break ;      
			case AUDIO_MODE_MUTE_BOTH:
			{
				sysmode = SYSMODE_STANDBY;
				callState = 0;	
				volume = DAC_MUTE;
				PRINT(("CVC: Set Mode SYSMODE_HFK MUTE BOTH - Standby\n"));
			}
			break;    
			case AUDIO_MODE_SPEECH_REC:
			{
				sysmode = SYSMODE_ASR;
				/*callState = CALLST_CONNECTED;*/		
				/*volume = CVC->volume;*/
				PRINT(("CVC: Set Mode SYMODE_ASR CALLST_CONNECTED\n"));
			 }
			break;   
			default:
			{    /*do not send a message*/    
			   PRINT(("CVC: Set Mode Invalid [%x]\n" , mode ));
			   return;
			}
		}
		

		/*send update to kalimba */
		KalimbaSendMessage(CVC_SETMODE_MSG, sysmode, 0, callState, 0 );
        KalimbaSendMessage(CVC_VOLUME_MSG, 0, 0, volume, CVC->tone_volume);
    }   /* for the else */
}

/****************************************************************************
NAME	
	CsrCvcPluginPlayTone

DESCRIPTION

    queues the tone if can_queue is TRUE and there is already a tone playing

RETURNS
	false if a tone is already playing
    
*/
void CsrCvcPluginPlayTone (CvcPluginTaskdata *task, ringtone_note * tone , uint16 tone_volume, bool stereo)  
{   

    Source lSource ;  
    Sink lSink ; 
        
    if (!CVC)
        Panic() ;    

    PRINT(("CVC: Tone Start\n"));
	
	if (CVC->no_dsp)
	{
        CVC->tone_stereo = stereo;
        
        if ( CVC->audio_sink )
		{
			DisconnectSpeakerNoDsp(FALSE) ;
		}	
        
	    lSink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A);
		
		PanicFalse(SinkConfigure(lSink, STREAM_CODEC_OUTPUT_RATE, 8000));	
	}
	else
	{	
		/*check cvc running */
		if ( CVC->cvc_running == FALSE )
			Panic() ;   	
	
		/* The DAC gain must be limited to 0 dB so that no distortion occurs and so the echo canceller works. */
		if (tone_volume > 0xf)
			tone_volume = 0xf;

		/* set DAC gain to a suitable level for tone play */
		if (tone_volume != CVC->tone_volume)
		{
		  CVC->tone_volume = tone_volume;
		  KalimbaSendMessage(CVC_VOLUME_MSG, 0, 0, CVC->volume, CVC->tone_volume); 
		}

		lSink = StreamKalimbaSink(3);
	}
    
    /*request an indication that the tone has completed / been disconnected*/
    MessageSinkTask ( lSink , (TaskData*) task );

    /*connect the tone*/
    lSource = StreamRingtoneSource( (const ringtone_note *) (tone) );    
 
	/* connect the tone (mix the tone if this is CVC) */    
    PanicFalse( StreamConnectAndDispose( lSource , lSink ) );
	
	if (CVC->no_dsp)
	{
		/* use the tone volume if present */
		CodecSetOutputGainNow( CVC->codec_task, tone_volume ? tone_volume : CVC->volume, left_and_right_ch );
	}
}

/****************************************************************************
NAME	
	CsrCvcPluginStopTone

DESCRIPTION
	Stop a tone from playing

RETURNS
	whether or not the tone was stopped successfully
*/
void CsrCvcPluginStopTone ( CvcPluginTaskdata *task ) 
{
    PRINT(("CVC: Stop Tone\n"));
    if (!CVC)
        Panic() ;
		
	if (CVC->no_dsp)
	{
		DisconnectSpeakerNoDsp(TRUE) ;
		CsrCvcPluginSetModeNoDsp (CVC->mode , NULL );
	}	
	else
	{
		StreamDisconnect( 0 , StreamKalimbaSink(3) ) ; 
	}
}


/****************************************************************************
DESCRIPTION
	Connect the audio stream (Speaker and Microphone)
*/
static void CvcConnectAudio (CvcPluginTaskdata *task)
{             
    if ( CVC->audio_sink )
    {	        
       bool r1 =0, r2 = 0;
       bool r3 =0, r4 = 0;

	    bool stereo = CVC->stereo;

		Source lSource0 = NULL;
		Sink   lSink0   = NULL;	
      Sink   lSink1   = NULL;	
		Source lSource1 = NULL;
    
		/* set dac rate */
		uint32  dacrate = (uint32) CVC->dac_rate;
        
        /* Set DAC gain to minimum value before connecting streams */
        CodecSetOutputGainNow( CVC->codec_task, 0 , left_and_right_ch ); 
        
		/* Configure port 0 to be routed to internal ADC and DACs */
	    if(stereo & !(task->two_mic))       /* configure A_AND_B only if 1mic and in stereo  */
        {
            PRINT(("CVC: CvcConnectAudio stereo\n"));
          lSink0 = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A_AND_B);
      }
       else
        {
            PRINT(("CVC: CvcConnectAudio mono [%d] [%d]\n", stereo, task->two_mic));
		    lSink0 = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
        }

		 PanicFalse(SinkConfigure(lSink0, STREAM_CODEC_OUTPUT_RATE,  dacrate));
		

		lSource0 = StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
		PanicFalse(SourceConfigure(lSource0, STREAM_CODEC_INPUT_RATE, dacrate));
		
	
      /* if in no dsp mode then jump straight to set mode that will plug in the ports */
		if (CVC->no_dsp)
		{
			/* Set the mode */
			CsrCvcPluginSetModeNoDsp ( CVC->mode , NULL );
            CodecSetOutputGainNow( CVC->codec_task, CVC->volume, left_and_right_ch );
			return;
		}

   
		 /* flag DSP is up and running */
        CVC->cvc_running = TRUE ;
        KalimbaSendMessage(MESSAGE_SCO_CONFIG, task->sco_encoder, task->sco_config , task->adc_dac_16kHz?128:64, 0);

        if( task->two_mic )
        {
          PRINT(("CVC: Connect PCM source 2\n"));
		    lSource1 = StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
		    PanicFalse(SourceConfigure(lSource1, STREAM_CODEC_INPUT_RATE, dacrate));	
          SourceSynchronise(lSource0,lSource1);

          lSink1 = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
		    PanicFalse(SinkConfigure(lSink1, STREAM_CODEC_OUTPUT_RATE, dacrate));	
          SinkSynchronise(lSink0,lSink1);
      }

#ifdef CVC_ALL
        if( CVC->mic_switch )
        {			
            /* Connect Ports to DSP */
            PanicFalse(r1 = (bool)StreamConnect( lSource0,StreamKalimbaSink(2)));  
    		   PanicFalse(r2 = (bool)StreamConnect(StreamKalimbaSource(2),lSink0));  
            if( task->two_mic )
            {
                PanicFalse(r3 = (bool)StreamConnect( lSource1,StreamKalimbaSink(0)));  
                PanicFalse(r4 = (bool)StreamConnect(StreamKalimbaSource(0),lSink1));   
    		   }	
        }        
        else
#endif
        {			
    		   /* Connect Ports to DSP */
            PanicFalse(r1 = (bool)StreamConnect( lSource0,StreamKalimbaSink(0)));  
    		   PanicFalse(r2 = (bool)StreamConnect(StreamKalimbaSource(0),lSink0));    
            if( task->two_mic )
            {
                PanicFalse(r3 = (bool)StreamConnect( lSource1,StreamKalimbaSink(2))); 
                PanicFalse(r4 = (bool)StreamConnect(StreamKalimbaSource(2),lSink1));   
    		   }		
        }	
        
        PRINT(("CVC: Streams connected: adc_r= %d, dac_r= %d, adc_l= %d, dac_l= %d\n",r1,r2,r3,r4)); 

        r1 = (bool)StreamConnect(StreamSourceFromSink( CVC->audio_sink ),StreamKalimbaSink(1)); /* SCO->DSP */  
        r2 = (bool)StreamConnect( StreamKalimbaSource(1), CVC->audio_sink ); /* DSP->SCO */
        PRINT(("CVC: Encoded streams connected: SCO->DSP= %d, DSP->SCO= %d \n",r1,r2));  
  

        /* Set the mode */
        CsrCvcPluginSetMode ( task, CVC->mode , NULL );
    }
    else
    {   
        /*Power Down*/
        CsrCvcPluginDisconnect(task);
    }
}                

/****************************************************************************
DESCRIPTION
	Handles a CVC_CODEC message received from CVC
*/
static void CvcCodecMessage (CvcPluginTaskdata *task, uint16 input_gain_l, uint16 input_gain_r, uint16 output_gain )
{   
    PRINT(("CVC: Output gain = 0x%x\n" , output_gain ));
    PRINT(("CVC: Input gain  = 0x%x ,0x%x\n" , input_gain_l,input_gain_r ));
    
    /* check pointer validity as there is a very small window where a message arrives
       as the result of playing a tone after CVC has been powered down */
    if(CVC)
    {
        /*Set the output Gain immediately*/
        CodecSetOutputGainNow( CVC->codec_task, output_gain, left_and_right_ch);
    
        /*only enable the pre amp if asked to do so*/
        /*ie if the top bit (0x8000) is set */
		SourceConfigure(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A),
                                          STREAM_CODEC_MIC_INPUT_GAIN_ENABLE, ( input_gain_l >> 15 ) & 0x1);    
    
        /* Clear the upper bytes of the input gain argument */	
		if( task->two_mic )
		{		    
			SourceConfigure(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B), 
			                                  STREAM_CODEC_MIC_INPUT_GAIN_ENABLE, ( input_gain_r >> 15 ) & 0x1);
										  
			CodecSetInputGainNow( CVC->codec_task, (input_gain_l & 0xFF), left_ch);
			CodecSetInputGainNow( CVC->codec_task, (input_gain_r & 0xFF), right_ch);
		}
		else	
		{	    
			SourceConfigure(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B), 
			                                  STREAM_CODEC_MIC_INPUT_GAIN_ENABLE,  ( input_gain_l >> 15 ) & 0x1);
											  
			CodecSetInputGainNow( CVC->codec_task, (input_gain_l & 0xFF), left_and_right_ch);
		}
    }
}


/****************************************************************************
DESCRIPTION
	handles the internal cvc messages /  messages from the dsp
*/
void CsrCvcPluginInternalMessage( CvcPluginTaskdata *task ,uint16 id , Message message ) 
{
	switch(id)
	{
        case MESSAGE_FROM_KALIMBA:
		{
			const DSP_REGISTER_T *m = (const DSP_REGISTER_T *) message;
	        PRINT(("CVC: msg id[%x] a[%x] b[%x] c[%x] d[%x]\n", m->id, m->a, m->b, m->c, m->d));
		
            switch ( m->id ) 
			{
              case CVC_READY_MSG:
                {
                    if(CVC->dac_rate == 8000)
						KalimbaSendMessage(CVC_LOADPARAMS_MSG, CVC_PS_BASE, 0, 0, 0);
					else if (CVC->dac_rate == 16000)
						KalimbaSendMessage(CVC_LOADPARAMS_MSG, CVC_PS_BASE_2, 0, 0, 0);
					else{
						PRINT(("CVC: Unknown dac_rate.\n"));
				        Panic();
					}
                    
                        /*cvc is now loaded, signal that tones etc can be scheduled*/
                    AUDIO_BUSY = NULL ;
                    
                    PRINT(("CVC: CVC_READY, SysId[%x] BuildVersion[%x] \n",m->a, m->b));
                    
                    CvcConnectAudio (task);                    
                }
                break;				
			    case CVC_CODEC_MSG:
	            {                    	            
                    uint16 lOutput_gain = m->a;
                    uint16 lInput_gain_l  = m->b;
					uint16 lInput_gain_r  = m->c;
      
                    CvcCodecMessage (task, lInput_gain_l,lInput_gain_r, lOutput_gain );    
	            }
                break;
				
			    case CVC_SECPASSED_MSG:
    		        PRINT(("CVC:  Sec passed.\n"));
				    break;

			    case CVC_SECFAILED_MSG:
                  PRINT(("CVC: Security has failed.\n"));
                  break;
            }
		}
		break;
	
        default:
        break ;
	}		
}	

/****************************************************************************
DESCRIPTION
	a tone has completed
*/
void CsrCvcPluginToneComplete( CvcPluginTaskdata *task ) 
{
    PRINT(("CVC: Tone Complete\n"));
	if (CVC->no_dsp)
	{	
        Sink speaker_sink = StreamAudioSink(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, CVC->tone_stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A);
        MessageSinkTask ( speaker_sink, NULL ) ;
        SinkClose(speaker_sink);
        
		if (( CVC->mode != AUDIO_MODE_MUTE_BOTH )&&
			( CVC->mode != AUDIO_MODE_MUTE_SPEAKER))
		{
			/* reconnect sco audio if present */
			ConnectSpeakerNoDsp();
			
			/* check to see if the sco is still valid, if it is not then we will have received the
			   message before the tone has completed playing due to some other issue, therefore
			   allow tone to continue playing for an additional 1.5 seconds to allow the power off
			   tone to be played to completion */
			if(StreamSourceFromSink(CVC->audio_sink))
			{
			   AUDIO_BUSY = NULL ;
			   CsrCvcPluginSetVolume(task, CVC->volume );
               PRINT(("CVC: Tone Complete SCO exists\n"));
			}
			else
			{
			   MessageSendLater((TaskData*) task, MESSAGE_FORCE_TONE_COMPLETE, 0, 1500);
               PRINT(("CVC: Tone Complete SCO not exists [0x%x]\n", (uint16)CVC->audio_sink));
			}
		}
		else
		{
			AUDIO_BUSY = NULL ;
			CsrCvcPluginSetVolume(task, CVC->volume );
		}
	}
	else
	{	/* DSP mode */
	   /* Restore the DAC gain to mute if in mute mode */
	   if ( CVC->cvc_running && (CVC->mode==AUDIO_MODE_MUTE_SPEAKER || CVC->mode==AUDIO_MODE_MUTE_BOTH ) )
		  KalimbaSendMessage(CVC_VOLUME_MSG, 0, 0, DAC_MUTE, CVC->tone_volume);   
	   
	   /* We no longer want to receive stream indications */
	   MessageSinkTask (StreamKalimbaSink(3) , NULL);
	}
}

/****************************************************************************
DESCRIPTION
    Reconnects the audio after a tone has completed in no DSP mode
*/
void CsrCvcPluginToneForceCompleteNoDsp ( CvcPluginTaskdata *task )
{
    PRINT(("CVC: Force Tone Complete No DSP\n"));
    if(AUDIO_BUSY)
    {
        AUDIO_BUSY = NULL ;	
		
        DisconnectSpeakerNoDsp(TRUE);
        DisconnectMicNoDsp();
    }
    
	if(CVC)
	{	
		/* ensure volume is set to correct level after playing tone */
		CsrCvcPluginSetVolume(task, CVC->volume );
	}
}

/****************************************************************************
NAME
    CsrCvcPluginMicSwitch

DESCRIPTION
    Swap between the microphone inputs for production test

RETURNS
    
*/
void CsrCvcPluginMicSwitch( CvcPluginTaskdata *task )
{
#ifdef CVC_ALL
    Sink audio_sink;
    AUDIO_SINK_T sink_type;
    Task codec_task;
    uint16 volume;
    uint32 rate;  	
    bool stereo;	
    AUDIO_MODE_T mode;
	AUDIO_POWER_T power;
    
    if ( CVC->audio_sink )
    {
        /* CVC parameters are lost with audio disconnect */
        audio_sink = CVC->audio_sink;
        sink_type = CVC->link_type;
        codec_task = CVC->codec_task;
        volume = CVC->tone_volume;
        rate = CVC->audio_rate;  		
        stereo = CVC->stereo;
        mode = CVC->mode;
		power = POWER_BATT_LEVEL3; /*TODO set power for mic switch*/		
        
        /* disconnect audio */
        CsrCvcPluginDisconnect(task);
        
        /* reconnect audio with swapped mics */
        CsrCvcPluginConnect(task, 
		                    audio_sink, 
							sink_type, 
							codec_task, 
							volume, 
							rate,  							
							stereo, 							
							TRUE,
							mode,
							power);

    }
#endif	
}

/****************************************************************************
DESCRIPTION
    Disconnect the microphone path
*/
static void DisconnectMicNoDsp ( void )
{
    Source mic_source = StreamAudioSource(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
    PRINT(("CVC: NODSP: Disconnect Mic\n")) ;
	StreamDisconnect(mic_source, CVC->audio_sink);
    SourceClose(mic_source);
}

/****************************************************************************
DESCRIPTION
    Disconnect the Speaker path
*/
static void DisconnectSpeakerNoDsp ( bool tone )
{
    Sink speaker_sink;
    if (tone)
    {
        speaker_sink = StreamAudioSink(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, CVC->tone_stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A);
        MessageSinkTask ( speaker_sink, NULL ) ;
    }
    else
    {
        speaker_sink = StreamAudioSink(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, CVC->stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A);
    }
    PRINT(("CVC: NODSP: Disconnect Speaker tone[%d]\n", tone)) ;
	StreamDisconnect(StreamSourceFromSink(CVC->audio_sink), speaker_sink);
    SinkClose(speaker_sink);
}

/****************************************************************************
DESCRIPTION
    Connect a SCO to the Microphone
*/
static void ConnectMicNoDsp ( void )
{
    PRINT(("CVC: NODSP: Connect Mic\n")) ;
    if ( CVC->audio_sink )
    {
        StreamConnect(StreamAudioSource(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, AUDIO_CHANNEL_A), CVC->audio_sink );		
    }
}

/****************************************************************************
DESCRIPTION
    Connect a SCO to the Speaker
*/
static void ConnectSpeakerNoDsp ( void )
{
    Sink speaker_sink = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, CVC->stereo ? AUDIO_CHANNEL_A_AND_B : AUDIO_CHANNEL_A);
    Source audio_source = StreamSourceFromSink(CVC->audio_sink);
    PRINT(("CVC: NODSP: Connect Speaker[0x%x] speaker_sink[0x%x] audio_source[0x%x] stereo[%d]\n" ,(int)CVC->audio_sink, (int)speaker_sink, (int)audio_source, CVC->stereo));
    if ( CVC->audio_sink )
    {
        StreamConnect( audio_source, speaker_sink);
    }
}

#ifdef CVC_ALL
/****************************************************************************
DESCRIPTION
    Disconnect the second microphone path
*/
static void DisconnectSecondMicNoDsp ( void )
{
    Source mic_source = StreamAudioSource(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
    PRINT(("CVC: NODSP: Disconnect Second Mic\n")) ;
	StreamDisconnect(mic_source, CVC->audio_sink);
    SourceClose(mic_source);
}

/****************************************************************************
DESCRIPTION
    Connect a SCO to the second Microphone
*/
static void ConnectSecondMicNoDsp ( void )
{
    PRINT(("CVC: NODSP: Connect Second Mic\n")) ;
    if ( CVC->audio_sink )
    {
        StreamConnect(StreamAudioSource(AUDIO_HARDWARE_CODEC,AUDIO_INSTANCE_0, AUDIO_CHANNEL_B), CVC->audio_sink );		
    }
}
#endif

/****************************************************************************
NAME
    CsrNoDspPluginMicSwitch

DESCRIPTION
    Swap between the microphone inputs for production test

RETURNS
    
*/
void CsrCvcPluginMicSwitchNoDsp ( void )
{
#ifdef CVC_ALL
    PRINT(("CVC: NODSP: MicSwitch [%x]\n" , (int)CVC->audio_sink )) ;
    if ( CVC->audio_sink )
    {
        CodecSetOutputGainNow( CVC->codec_task, 0 , left_and_right_ch );
        
        /* disconnect mic */
        DisconnectMicNoDsp();
		
        /* disconnect speaker */
		DisconnectSpeakerNoDsp(FALSE);
	
        /*connect second mic*/
        ConnectSecondMicNoDsp();
		
        /*connect speaker*/		
        ConnectSpeakerNoDsp();

        CodecSetOutputGainNow( CVC->codec_task, CVC->volume , left_and_right_ch );

    }
#endif	
}

/****************************************************************************
NAME
    CsrCvcPluginSetPower

DESCRIPTION
    Sets the power mode of the plugin

RETURNS
    
*/
void CsrCvcPluginSetPower( CvcPluginTaskdata *task,  AUDIO_POWER_T power)
{
	/* if actually using the NO DSP plugin disregard set power requests */
	if(task->cvc_plugin_variant == CVSD_NO_DSP)
	{
		PRINT(("CVC : Set Power ignored [%x][%x]\n", CVC->no_dsp, power));
		return;
	}
	
	PRINT(("CVC : Set Power [%x][%x][%x]\n", CVC->cvc_running, CVC->no_dsp, power));			 
	
	/*   These are the state transitions possible during an active SCO:
	
	CVC -> (low power) -> DSP Passthrough
	DSP Passthrough -> (normal power) -> CVC
	No DSP -> (normal power) -> CVC
	
	It's not possible to switch CVC or DSP Pss through to No DSP with an active SCO due to Metadata issues */
		 	
	if(CVC->cvc_running)
	{
		if(power <= LPIBM_THRESHOLD)
		{			
			uint16 * lParam = PanicUnlessMalloc ( sizeof(uint16)) ;
			*lParam = CSR_CVC_PSTHRU_ENABLE;
			
			PRINT(("CVC : Set Power Low - Active SCO\n"));
			
			if (AUDIO_BUSY)
            {
                MAKE_AUDIO_MESSAGE ( AUDIO_PLUGIN_SET_MODE_MSG) ;
                message->mode   = AUDIO_MODE_CONNECTED ;
                message->params = lParam ;
        
        		MessageSendConditionally ( (Task)task, AUDIO_PLUGIN_SET_MODE_MSG , message ,(const uint16 *)&AUDIO_BUSY ) ;
    	    }
            else
            {
				CsrCvcPluginSetMode((CvcPluginTaskdata*)task, AUDIO_MODE_CONNECTED , lParam) ;
            }
		}
		else
		{						
			PRINT(("CVC : Set Power Normal - Active SCO\n"));
			
			if (AUDIO_BUSY)
            {
                MAKE_AUDIO_MESSAGE ( AUDIO_PLUGIN_SET_MODE_MSG) ;
                message->mode   = AUDIO_MODE_CONNECTED ;
                message->params = NULL ;
        
        		MessageSendConditionally ( (Task)task, AUDIO_PLUGIN_SET_MODE_MSG , message ,(const uint16 *)&AUDIO_BUSY ) ;
    	    }
            else
            {			
				CsrCvcPluginSetMode((CvcPluginTaskdata*)task, AUDIO_MODE_CONNECTED , NULL) ;				
            }
		}	
	}
	else /* cvc not running */
	{
		if(power > LPIBM_THRESHOLD)
		{
			/* switch to cVc if now in high power mode, if in low power mode stay in No DSP mode */
			Sink audio_sink;
			AUDIO_SINK_T sink_type;
			Task codec_task;
			uint16 volume;
			uint32 rate;  	
			bool stereo;	
			AUDIO_MODE_T mode;
			AUDIO_POWER_T powerlevel;
			
			if ( CVC->audio_sink )
			{
				/* CVC parameters are lost with audio disconnect */
				audio_sink = CVC->audio_sink;
				sink_type = CVC->link_type;
				codec_task = CVC->codec_task;
				volume = CVC->tone_volume;
				rate = CVC->audio_rate;  		
				stereo = CVC->stereo;
				mode = CVC->mode;
				powerlevel = power; 
				
				/* disconnect audio */
				CsrCvcPluginDisconnect(task);
				
				/* reconnect audio with swapped mics */
				CsrCvcPluginConnect(task, 
									audio_sink, 
									sink_type, 
									codec_task, 
									volume, 
									rate,  									
									stereo, 							
									FALSE,									
									mode,
									powerlevel);

			}
		}
	}
	
	
}
