/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010

FILE NAME
    headset_private.h
    
DESCRIPTION
    
*/

/*!

@file	headset_private.h
@brief The private data structures used by the headset application
    
    THe main application task and global data is declared here
*/


#ifndef _HEADSET_PRIVATE_H_
#define _HEADSET_PRIVATE_H_


#ifdef A2DP_EXTRA_CODECS
#ifndef INCLUDE_MP3
#define INCLUDE_MP3
#endif
#ifndef INCLUDE_AAC
#define INCLUDE_AAC
#endif
#ifndef INCLUDE_FASTSTREAM
#define INCLUDE_FASTSTREAM
#endif
#endif /* A2DP_EXTRA_CODECS */


#include <connection.h>
#include <hfp.h>
#include <message.h>
#include <app/message/system_message.h>
#include <stdio.h>
#include <stdlib.h>
#include <charger.h>


#include "headset_buttonmanager.h"
    /*needed for the LED manager task definition*/
#include "headset_leddata.h"

#include "headset_powermanager.h"

#include "headset_debug.h"

#ifdef ENABLE_PBAP
#include "headset_pbap.h"
#endif

#include "headset_a2dp.h"

#ifdef ENABLE_AVRCP
#include "headset_avrcp.h"
#endif

/*needed for the AUDIO_SINK_T*/
#include <audio.h>
/*needed for the a2dp plugin AudioConnect params*/
#include <csr_a2dp_decoder_common_plugin.h>

/*! 
    @brief Class Of Device definitions 
*/
#define AUDIO_MAJOR_SERV_CLASS  0x200000
#define AV_MAJOR_DEVICE_CLASS   0x000400
#define AV_MINOR_HEADSET        0x000004
#define AV_MINOR_MICROPHONE     0x000010
#define AV_MINOR_SPEAKER        0x000014
#define AV_MINOR_HEADPHONES     0x000018
#define AV_MINOR_PORTABLE       0x00001c
#define AV_MINOR_HIFI           0x000028
#define AV_COD_RENDER           0x040000
#define AV_COD_CAPTURE          0x080000


#define INPUT_PIO_UNASSIGNED	(0xf)

#define ATTRIBUTE_PSKEY_BASE    (32)
#define ATTRIBUTE_SIZE          ( sizeof(uint16) * 4 )

#define PDL_PSKEY_BASE          (42)

/* swap between profiles, when called with primary will return secondary and vice versa */
#define OTHER_PROFILE(x) (x ^ 0x3)

/* hfp profiles have offset of 1, this needs to be removed to be used as index into data array */
#define PROFILE_INDEX(x) (x - 1)

#define HFP_PROFILE_TYPE 0
#define A2DP_PROFILE_TYPE 1

/*! 
    @brief HFP Version
    
    Enum defining the values in the HFP_Version field of PSKEY_ADDITIONAL_HFP_SUPPORTED_FEATURES
*/
typedef enum
{
	headset_hfp_version_1_0 = 0x00,
	headset_hfp_version_1_5 = 0x02,
	headset_hfp_version_1_6 = 0x01
} HFP_Version;

typedef enum
{
    a2dp_primary = 0x00,
    a2dp_secondary = 0x01
} a2dp_link_priority;
        
/*! 
    @brief Feature Block
    
    Please refer to the headset configuration user guide document for details on the 
    features listed here 
*/
typedef struct
{
    unsigned    MicBiasUsesLDO:1;   
    unsigned    OverideFilterPermanentlyOn:1 ;
    unsigned    MuteSpeakerAndMic:1 ;
    unsigned    PlayTonesAtFixedVolume:1 ;
    
    unsigned    RebootAfterReset:1 ;
    unsigned    RemainDiscoverableAtAllTimes:1;    
    unsigned    DisablePowerOffAfterPowerOn:1;    
    unsigned    AutoAnswerOnConnect:1;

    unsigned    show_single_sdp:1 ;    
    unsigned    PowerOnLDO:1;
    unsigned    PowerOnSMPS:1;
    unsigned    MuteLocalVolAction:1; /*whether or not to update the global vol whilst muted*/   
    
    unsigned    OverideMute:1 ;/*whether or not to unmute if a vol msg is received*/    
    unsigned    sdp_hide_enable:1;
    unsigned    unused3:1;
    unsigned    pair_mode_en:1;
    
     /*---------------------------*/           
	unsigned    GoConnectableButtonPress:1;    
    unsigned    DisableTTSTerminate:1;   
    unsigned    AutoReconnectPowerOn:1 ;
    unsigned    AutoReconnectLinkLoss:1 ;
    unsigned    SeparateLNRButtons:1 ;
    unsigned 	SeparateVDButtons: 1;
	unsigned    ActionOnDisconnect:2;
    unsigned 	PowerDownOnDiscoTimeout: 2;
    unsigned    ActionOnCallTransfer:2;    
    unsigned 	LedTimeMultiplier: 2; /* multiply the times of the LED settings (x1 x2 x4 x8) */
    unsigned    ActionOnPowerOn:2;
    
     /*---------------------------*/   

    unsigned    DiscoIfPDLLessThan:4 ; 
    
    unsigned    DoNotDiscoDuringLinkLoss:1;
    unsigned    ManInTheMiddle:1;
    unsigned    UseDiffConnectedEventAtPowerOn:1;    
    unsigned    EncryptOnSLCEstablishment:1 ;    
    
    unsigned    UseLowPowerAudioCodecs:1;
	unsigned	PlayLocalVolumeTone:1;    
    unsigned    SecurePairing:1 ;
    unsigned    InitialBatteryLevelCheck:1;
    
  	unsigned    QueueVolumeTones:1;
    unsigned    QueueEventTones:1;
    unsigned    QueueLEDEvents:1;
    unsigned    MuteToneFixedVolume:1;  /* play mute tone at default volume level when set */

     /*---------------------------*/   
	
	unsigned    ResetLEDEnableStateAfterReset:1;    /* if set the LED disable state is reset after boot */
	unsigned    ResetAfterPowerOffComplete:1 ;
	unsigned    AutoPowerOnAfterInitialisation:1 ;	
	unsigned    DisableRoleSwitching:1; 	/* disable the headset role switching in multipoint connections */
	unsigned    audio_sco:1;    /* See B-34179: Transfer audio from ag to headset directly with a SCO */
	unsigned    audio_plugin:3; /* which of the audio plugins we want to use */
	
	unsigned    DefaultVolume:4 ;     
    unsigned    IgnoreButtonPressAfterLedEnable:1 ; /* if set the button press that enabled the LED's is ignored */
    unsigned    LNRCancelsVoiceDialIfActive:1 ; 
	unsigned    GoConnectableDuringLinkLoss:1 ; 
	unsigned    stereo:1 ;        /*mono or stereo connection*/
	
	/*------------------------------*/
    unsigned    ChargerTerminationLEDOveride:1; /* used to force trickle led on when full and stop led toggling */
    unsigned    FixedToneVolumeLevel:5;         /* the level at which the tones are played when using the play at fixed level */
	unsigned    unused2:1 ;
    unsigned    ForceEV3S1ForSco2:1;            /* force use of EV3 safe settings for second sco */
    unsigned    VoicePromptPairing:1;           /* Read out PIN/Passkey/Confirmation using voice prompts */
    unsigned    RssiConnCheckPdl:1;             /* Check discovered device is in PDL before connecting */
    unsigned    PairIfPDLLessThan: 2;           /* Use RSSI on inquiry responses to pair to nearest AG */
	unsigned    EnableSyncMuteMicophones: 1;
	unsigned    EnableCSR2CSRBattLevel: 1 ;     /* enable CSR2CSR Battery Level feature */
	unsigned    unused: 1 ;                
	unsigned    VoicePromptNumbers:1;           /* Read out numbers using voice prompts */
    
    /*------------------------------*/
  	unsigned    DefaultA2dpVolLevel:4 ;         /* Default A2dp Vol Level */
    unsigned    Mono_Speaker: 1 ;  	            /* Mono format output */
	unsigned    AutoA2dpReconnectPowerOn:1 ;    /* Auto A2dp Reconnect after power on */
	unsigned    EnableA2dpStreaming:1 ;	        /* Enable A2DP streaming  */
    unsigned    A2dpOptionalCodecsEnabled:4;    /* Optional A2DP codecs */
    unsigned    unused4:5;

}feature_config_type;


typedef struct 
{
	unsigned event:8 ;
	unsigned tone:8 ;
}HeadsetTone_t ;

typedef struct
{
	unsigned event:8 ;
	unsigned tts_id:8 ;
    unsigned unused:1 ;
    unsigned sco_block:1;
    unsigned state_mask:14 ;
}HeadsetTts_t;

#define TONE_NOT_DEFINED 0

/*!
    @brief the volume mapping structure - one for each volume level
*/
typedef struct  VolMappingTag
{
        /*! The hfp volume level to go to when volume up is pressed*/
    unsigned       IncVol:4;
        /*! The hfp volume level to go to when volume down is pressed*/
    unsigned       DecVol:4;
        /*! The tone associated with a given volume level*/
    unsigned       Tone:8;	
        /*! The a2dp gain index to use for the given volume level */
	unsigned       A2dpGain:8;
	   /*! the hfp DAC gain to use for the given volume level */
	unsigned       VolGain:8;    
	
}VolMapping_t ;

#define NUM_FIXED_TONES            (94)    
#define MAX_NUM_VARIABLE_TONES     (8)

typedef struct VarTonesDataTag
{
    ringtone_note    *gVariableTones;    
}ConfigTone_t;
        
/*!
    @brief the audio data - part of the main data structure
    
        contains the audio specific global data    
*/
typedef struct audioDataTag
{    
        /*! pointer to the volume mapping structures - one for each of the 15 HFP volume levels*/
    VolMapping_t  * gVolMaps ;
        /*! pointer to the array of tones associated with user events */
    HeadsetTone_t * gEventTones   ;
#ifdef TEXT_TO_SPEECH_PHRASES
	/*! pointer to the array of tts phrases configured for user events */	
    HeadsetTts_t  * gEventTTSPhrases;
#endif
    /*! pointer to the array of variable tone which the user can be configured */
    ConfigTone_t  * gConfigTones;
}audioData_t ;


/*!
    @brief Block containing the PIOs assigned to fixed events the bit fields define if a PIO has been set
*/
typedef struct PIO_block_t
{
    unsigned    CallActivePIO:4   ;
    unsigned    IncomingRingPIO:4 ;   
    unsigned    OutgoingRingPIO:4 ;
    unsigned    HeadsetActivePIO:4;
    
    unsigned    PowerOnPIO:4      ;
    unsigned    ChargerEnablePIO:4;
    unsigned    LedEnablePIO:4 ;
    unsigned    unused:4;
    
} PIO_block_t ;

typedef struct
{
	unsigned reserved:8;
    unsigned charger_input:4;
	unsigned dut_pio:4;
}input_pio_config_type;

/* Radio configuration data */
typedef struct
{
	uint16	page_scan_interval;
	uint16  page_scan_window;
	uint16	inquiry_scan_interval;
	uint16	inquiry_scan_window;
}radio_config_type;

typedef struct 
{
    unsigned    HFP_Version:2;
    unsigned    reserved:13;
    unsigned    Additional_Parameters_Enabled:1;
    
    unsigned    reserved2:6;
    unsigned    supportedSyncPacketTypes:10 ;
    
    /*additional optional parameters for HFP */
    uint32      bandwidth;
    uint16      max_latency;
    uint16      voice_settings;
    uint16      retx_effort;  
    
} HFP_features_type ;

	/*the application timeouts / counts */
typedef struct TimeoutsTag
{
	uint16 AutoSwitchOffTime_s ;
	uint16 AutoPowerOnTimeout_s ;
	uint16 NetworkServiceIndicatorRepeatTime_s ;	
	uint16 DisablePowerOffAfterPowerOnTime_s ;
	uint16 PairModeTimeout_s ;
	uint16 MuteRemindTime_s ;
    uint16 ConnectableTimeout_s ; 	
	uint16 PairModeTimeoutIfPDL_s;
    uint16 ReconnectionAttempts ;       /* number of times to try and reconnect before giving up */
	uint16 EncryptionRefreshTimeout_m ;
    uint16 InquiryTimeout_s ;
    uint16 CheckRoleDelayTime_s;
    uint16 SecondAGConnectDelayTime_s;
    uint16 MissedCallIndicateTime_s ;  /* The period in second between two indications */
    uint16 MissedCallIndicateAttemps ; /* number of times to indicate before stopping indication */
    uint16 A2dpLinkLossReconnectionTime_s; /* the amount of time in seconds to attempt to reconnect a2dp */
}Timeouts_t ;

typedef struct
{
	uint16 coefficients[10];
	uint16 vad_threshold;
	uint16 echo_reduction_gain;
} filter_t;

typedef struct
{
    uint16 tx_power;
	uint16 threshold;
	uint16 diff_threshold;
    uint32 cod_filter;
	uint16 conn_threshold;
	uint16 conn_diff_threshold;
}rssi_pairing_t ;

/* Because of dynamically allocation constraints we have to use a single malloc for different PSKeys */
typedef struct 
{	
	Timeouts_t 		timeouts;
	filter_t 		filter;
	rssi_pairing_t	rssi;
	power_type		power; 
	unsigned    	no_of_profiles_connected:4 ;
    uint16			supp_features_local;
	uint16          supp_features_remote;
	unsigned 		gEventQueuedOnConnection:8 ;
    uint16          NoOfReconnectionAttempts;/* counts the number of reconnection attempts whilst scrolling 
                                                         the PDL attempting a connection, for use with ScrollPDLTimerPeriod
                                                         feature bit */
	input_pio_config_type	 input_PIO;    

} Configuration_t;

#define MAX_POWER_TABLE_ENTRIES 8

typedef struct
{
	unsigned		normalRole:2;		/* Master (0), Slave (1) or passive (2) */
    unsigned		normalEntries:2;	/* 0-2 */

	unsigned		SCORole:2;			/* Master (0), Slave (1) or passive (2) */
    unsigned        SCOEntries:2;		/* 0-2 */

    unsigned        A2dpSigRole:2;       /* Master (0), Slave (1) or passive (2) */
    unsigned        A2DPSigEntries:2;   /* 0-2 */
    
    unsigned        A2DPStreamRole:2;   /* Master (0), Slave (1) or passive (2) */
    unsigned        A2DPStreamEntries:2;/* 0-2 */
    
    
    /* pointers to arrays of lp_power_tables */
	lp_power_table powertable[1];

} power_table;


typedef struct
{
	uint16			max_remote_latency;
	uint16			min_remote_timeout;
	uint16			min_local_timeout;
} ssr_params;


typedef struct
{
	ssr_params		slc_params;
	ssr_params		sco_params;
} subrate_t;

/* Maks and shift values for th user WBS CODEC bits in the CODEC bitmap */
#define MASK_USER_AUDIO_CODEC_WBS		(0xff00)
#define SHIFT_USER_AUDIO_CODEC_WBS		(8)

#define	CSR_AUDIO_CODEC_WBS_BASE		(0)
#define	CSR_AUDIO_CODEC_CSR_BASE		(16)
#define	USER_AUDIO_CODEC_WBS_BASE		(32)

typedef enum 
{
	audio_codec_cvsd				= CSR_AUDIO_CODEC_WBS_BASE,
	audio_codec_wbs_sbc
} audio_codec_type ;

typedef enum
{
	hfp_no_hf_initiated_audio_transfer,
	hfp_usual_hf_initiated_audio_transfer,
	hfp_power_on_hf_initiated_audio_transfer
} hfp_hf_initiated_audio_transfer_type;

#define MAX_MULTIPOINT_CONNECTIONS  2        /* max number of mulitpoint connections  */
#define MAX_A2DP_CONNECTIONS        2        
#define MAX_PROFILES                2
typedef struct                              /* 1 word of storage for hfp status info */
{
    unsigned        list_id:8;               /* store the PDL list ID for this connection, used for link loss and preventing reconnection */
	unsigned		play_tone_on_sco_disconnect:8; /* An event tone to play on SCO disconnect (B-48360). Must be one of headsetEvents_t enum */
    
	unsigned		hf_initiated_csr_codec_negotiation:2; /* Indicates that an HF initiated CSR2CSR CODEC negotiation is in progress. */
	unsigned		ag_supported_csr_codecs:3; /* indicates which CSR CODECS the AG supports */
	unsigned		ag_supported_csr_codecs_bandwidths:2; /* indicates which CSR CODECS the AG supports */
    unsigned        profile_type:1;          /* hfp or hsp */

   	unsigned		hf_initiated_csr_codec_index_to_try:4; /* The CSR CODEC to try for an HF initiated CSR CODEC negotiation. */
	unsigned		ag_supports_csr_hf_initiated_codec_negotiation:1; /* Indicates that the AG connected to this instance supports HF initiated CSR2CSR CODEC negotiation. */
    unsigned        unused:3;
}profile_status_info;

/* this is the list of sco audio priorities, it is used to determine which audio is routed by the
   headset in multipoint situations, the further up the list will get routed first so be careful
   not to change the order without careful thought of the implications of doing so */
typedef enum
{
    sco_none,
    sco_about_to_disconnect,
    sco_streaming_audio,
    sco_inband_ring,
    sco_held_call,
    sco_active_call    
}audio_priority;
		
typedef struct                              /* storage of audio connection data on a per hfp isntance basis */
{
    audio_priority      sco_priority;        /* the priority level of the sco */
    uint32              tx_bandwidth;       
    sync_link_type      link_type:2;          /* link type may be different between AG's and needs to be stored for reorouting audio */
    audio_codec_type    codec_selected:6; /* audio codec being used with this profile */
    unsigned            gSMVolumeLevel:7;   /* volume level for this profile */
	unsigned            gAgSyncMuteFlag:1;
}profile_audio_info;

typedef struct 
{
    profile_status_info status;         /* status for each profile, in hfp index order */
    profile_audio_info  audio;           /* audio connection details used for re-routing audio */	
}profile_data_t ;

typedef struct
{
    bdaddr bd_addr;                     /* Address of device */
    int16  rssi;                        /* Highest received signal strength indication from device */
}inquiry_data_t;

typedef enum
{
    rssi_none,
    rssi_pairing,
    rssi_connecting
} rssi_action_t;

typedef enum
{
	mode_ha_only,
	mode_rx_only,
	mode_normal_bt
}ha_mode_t;

/* Headset data */
typedef struct
        
{
    TaskData                 task;
    profile_data_t           profile_data[MAX_MULTIPOINT_CONNECTIONS];
    ButtonsTaskData          *theButtonsTask;
#ifdef ROM_LEDS	
    LedTaskData              *theLEDTask;   
#endif
	a2dp_data                *a2dp_link_data;
    
#ifdef ENABLE_AVRCP
    avrcp_data                *avrcp_link_data;
#endif    
    
  	/*data structures*/
	radio_config_type		 *radio;
    HFP_features_type    	 HFP_supp_features;
	feature_config_type		 features;
    PIO_block_t              *PIO;
    audioData_t              audioData ;
	Configuration_t			 *conf;
	
	   /*global vars*/	
    TaskData    			 *dsp_plugin;
	uint16					 ha_volume;
	ha_mode_t				 ha_mode;
	uint16					 ha_tune;
	uint8					 ha_pending_msg;

    Sink            	     sco_sink;

	bdaddr					 *confirmation_addr;
	   
    power_table              *user_power_table;     /* pointer to user power table if available in ps */
        
	subrate_t				 *ssr_data;

    inquiry_data_t           *inquiry_data;

#ifdef TEXT_TO_SPEECH_LANGUAGESELECTION
	unsigned                 no_tts_languages:4;
    unsigned                 unused2:12;
#endif
	Task codec_task ;
    button_config_type       *buttons_duration;	
    
    A2dpPluginConnectParams  a2dp_audio_connect_params;
    A2dpPluginModeParams     a2dp_audio_mode_params;
 	
		/*! global flags*/
	/*word 1*/
    unsigned                 PowerOffIsEnabled:1; 
    unsigned                 VoiceRecognitionIsActive:1;
    unsigned                 VolumeOrientationIsInverted:1; /*whether or not the vol buttons are inverted*/
    unsigned                 NetworkIsPresent:1;
    
	unsigned 				 HeadsetConnecting:1 ; /*is this conenection a headset initiated connection*/
	unsigned				 SecondIncomingCall:1; /*record second incoming call */
	unsigned                 inquiry_scan_enabled:1;
	unsigned                 page_scan_enabled:1 ;
	
    unsigned                 rssi_action:2;         /* The RSSI action being performed */
    unsigned                 rssi_attempting:1;     /* Index in inquiry_data of device being connected to */
	unsigned				 HSPIncomingCallInd:1 ; /* flag to indicate ring in progress when using HSP */
	
    unsigned                 low_battery_flag_ag:1; /* flag whether low battery has been indicated to AG */
    unsigned                 last_outgoing_ag:2 ;   /* which AG made the last outgoing call */
    unsigned				 block_inq_res:1;
	
	/*word 2*/	
	unsigned				 confirmation:1;
	unsigned				 debug_keys_enabled:1;
	unsigned                 RepeatCallerIDFlag:1;
	unsigned				 iir_enabled:1;
	
	unsigned                 tts_language:4;
    
    unsigned                 MultipointEnable:1;
	unsigned                 spare:1;           		/* */
 	unsigned                 incoming_call_power_up:1;
    unsigned                 powerup_no_connection:1;    /* bit to indicate headset has powered and no connections yet */
	
    unsigned                 paging_in_progress:1;       /* bit to indicate that headset is curretly paging whilst in connectable state */
    unsigned                 gMuted:1 ;                 /*! Whether or not the headset is currently muted*/
    unsigned                 battery_low_state:1;
    unsigned                 gVolButtonsInverted:1;    /*! whether or not the volume button operation is currently inverted*/            

	/*word 3*/
	unsigned				 inquiry_tx:8;
    unsigned                 ag_swap_occurred:1;
    unsigned                 HeldCallIndex:4;        /* which call to route in the case of multiple held calls */
    unsigned                 FailAudioNegotiation:1;
    unsigned                 RenegotiateSco:1;
    unsigned                 lbipmEnable:1;         /* enable Low Battery Intelligent Power Management feature */
    
    /*word 4*/
    unsigned                 MissedCallIndicated:8;
#ifdef ENABLE_PBAP
    unsigned                 pbap_dial_state:2;
    unsigned                 pbap_active_link:2;
    unsigned                 pbap_active_pb:3;
    unsigned                 pbap_ready:1;        /* pbapc library has been initialised */
#else
    unsigned                 unused:8;
#endif
} hsTaskData;

/*malloc wrapper with added panic if malloc returns NULL*/


#ifdef DEBUG_MALLOC
	#define mallocPanic(x) MallocPANIC( __FILE__ , __LINE__ ,x) 
	void * MallocPANIC ( const char file[], int line, size_t pSize ) ;
#else
	#define mallocPanic(x) malloc(x)
#endif


/*the headset task data structure - visible to all (so don't pass it between functions!)*/
extern hsTaskData theHeadset ;


#endif /* HEADSET_PRIVATE_H_ */
