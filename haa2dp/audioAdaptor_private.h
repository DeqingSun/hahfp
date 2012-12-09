/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2009
Part of Audio-Adaptor-SDK 2009.R1

DESCRIPTION
    Defines data structures used internally throughout the application.
*/

#ifndef AUDIOADAPTOR_PRIVATE_H
#define AUDIOADAPTOR_PRIVATE_H

#include <a2dp.h> 
#include <audio.h>
#include <avrcp.h> 
#include <battery.h>
#include <message.h>
#include <stdio.h>

#include "audioAdaptor_buttons.h"
#include "audioAdaptor_states.h"


/* ------------------------------------------------------------------------ */
/* Debug print defines */ 

#define ENABLE_DEBUG_MAIN
#define ENABLE_DEBUG_A2DP
#define ENABLE_DEBUG_AGHFP
#define ENABLE_DEBUG_AVRCP
#define ENABLE_DEBUG_CL
#define ENABLE_DEBUG_HIDx
#define ENABLE_DEBUG_KALIMBA
#define ENABLE_DEBUG_USBx
#define ENABLE_DEBUG_DEV_INST
#define ENABLE_DEBUG_STATES
#define ENABLE_DEBUG_CALLx
#define ENABLE_DEBUG_POWERx
#define ENABLE_DEBUG_CONFIG
#define ENABLE_DEBUG_EVENT
#define ENABLE_DEBUG_CONN
#define ENABLE_DEBUG_SCAN
#define ENABLE_DEBUG_STREAM
#define ENABLE_DEBUG_CODEC

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_MAIN
    #define DEBUG(x)    {printf x;}
#else
    #define DEBUG(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_A2DP
    #define DEBUG_A2DP(x)    {printf x;}
#else
    #define DEBUG_A2DP(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_AGHFP
    #define DEBUG_AGHFP(x)    {printf x;}
#else
    #define DEBUG_AGHFP(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_AVRCP
    #define DEBUG_AVRCP(x)    {printf x;}
#else
    #define DEBUG_AVRCP(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_CL
    #define DEBUG_CL(x)    {printf x;}
#else
    #define DEBUG_CL(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_HID
    #define DEBUG_HID(x)    {printf x;}
#else
    #define DEBUG_HID(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_KALIMBA
    #define DEBUG_KALIMBA(x)    {printf x;}
#else
    #define DEBUG_KALIMBA(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_USB
    #define DEBUG_USB(x)    {printf x;}
#else
    #define DEBUG_USB(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_DEV_INST
    #define DEBUG_DEV(x)    {printf x;}
#else
    #define DEBUG_DEV(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_STATES
    #define DEBUG_STATES(x)    {printf x;}
#else
    #define DEBUG_STATES(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_CALL
    #define DEBUG_CALL(x)    {printf x;}
#else
    #define DEBUG_CALL(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_POWER
    #define DEBUG_POWER(x)    {printf x;}
#else
    #define DEBUG_POWER(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_CONFIG
    #define DEBUG_CONFIG(x)    {printf x;}
#else
    #define DEBUG_CONFIG(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_EVENT
    #define DEBUG_EVENT(x)    {printf x;}
#else
    #define DEBUG_EVENT(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_CONN
    #define DEBUG_CONN(x)    {printf x;}
#else
    #define DEBUG_CONN(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_SCAN
    #define DEBUG_SCAN(x)    {printf x;}
#else
    #define DEBUG_SCAN(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_STREAM
    #define DEBUG_STREAM(x)    {printf x;}
#else
    #define DEBUG_STREAM(x)
#endif

#if defined ENABLE_DEBUG && defined ENABLE_DEBUG_CODEC
    #define DEBUG_CODEC(x)    {printf x;}
#else
    #define DEBUG_CODEC(x)
#endif

/* ------------------------------------------------------------------------ */
/* Set board specific defines - the board type should be defined in the project settings */

#if defined DEV_PC_1645_USB || defined DEV_PC_1645_ANALOGUE
    #define PIO_MFB           0x0001    /* PIO 0 */

    #define DUT_PIO           0x0100    /* PIO 8 used to enter DUT mode */
#endif

#if (defined RED_PC_141) || (defined RED_PC_142)
    #define PIO_MFB           0x0004    /* PIO 2 */

    #define DUT_PIO           0x0200    /* PIO 9 used to enter DUT mode */
#endif

/* ------------------------------------------------------------------------ */
/* Define the class of device fields */

#define AV_MAJOR_DEVICE_CLASS    0x000400  
#define AV_MINOR_HIFI            0x000028
#define AV_COD_CAPTURE           0x080000

/* ------------------------------------------------------------------------ */
/* Defines for connection attempts */

#define MAX_APP_CONNECT_ATTEMPTS     3
#define MAX_PROFILE_CONNECT_ATTEMPTS 2

/* ------------------------------------------------------------------------ */
/* Message IDs for communicating with the DSP */

#define KALIMBA_SET_BITPOOL_MESSAGE              0x7070
#define KALIMBA_SET_POOR_LINK_BITPOOL_MESSAGE    0x7080
#define KALIMBA_ENCODER_SELECT                   0x7300
#define KALIMBA_AUDIO_USB_OUT_STATUS             0x7301
#define KALIMBA_AUDIO_USB_IN_STATUS              0x7302
#define KALIMBA_CODEC_TYPE_MESSAGE               0x7303

/* ------------------------------------------------------------------------ */
/* Size of inquiry results buffer */

#define INQUIRY_SCAN_BUFFER_SIZE 10

/* ------------------------------------------------------------------------ */
/* Enable SCO/eSCO packet support */

#define AUDIO_PACKET_TYPES (sync_all_esco|sync_all_sco)

/* ------------------------------------------------------------------------ */
/* Defines for A2DP Stream End Points */

#define KALIMBA_RESOURCE_ID     1                  /*!< @brief Resource ID for Kalimba */


#ifdef DUAL_STREAM

    #define MAX_NUM_DEV_CONNECTIONS 2

    #define KALIMBA_DS_RESOURCE_ID  2                  /*!< @brief Resource ID for Kalimba for 2nd set of Stream End Points when using DualStream */

    /* Local stream end point codec IDs */
    #define SBC_SEID                1                  /*!< @brief Local Stream End Point ID for SBC codec */
    #define SBC_DS_SEID             2                  /*!< @brief Local Stream End Point ID for SBC codec */    

    #define NUM_SEPS              (SBC_DS_SEID)  /*!< @brief The total number of SEPs */
    
    #define a2dpSeidIsSbc(seid)           ((seid == SBC_SEID) || (seid == SBC_DS_SEID))

#else /* DUAL_STREAM not defined */

    #define MAX_NUM_DEV_CONNECTIONS    1

    /* Local stream end point codec IDs */
    #define SBC_SEID                1                  /*!< @brief Local Stream End Point ID for SBC codec */    

    #define NUM_SEPS              (SBC_SEID)  /*!< @brief The total number of SEPs */

    #define a2dpSeidIsSbc(seid)           (seid == SBC_SEID)
    #define a2dpSeidIsFaststream(seid)    (seid == FASTSTREAM_SEID)

#endif /* DUAL_STREAM */



/* ------------------------------------------------------------------------ */
/* Determines how often encryption keys are refreshed */

#define EPR_TIMEOUT                0x0F               /*!< @brief EPR Interval */

/* ------------------------------------------------------------------------ */
/* Enums and structs */

/*! @brief Audio Codec Type Bitmask */
typedef enum 
{
    audio_codec_none = 0,
    audio_codec_cvsd = 1,
    audio_codec_auristream_2_bit = 2,
    audio_codec_auristream_4_bit = 4
} audio_codec_type ;


typedef enum
{
    AppEventUiShortPress,
    AppEventUiLongPress,
    
    AppEventMicActive,
    AppEventMicInactive,
    AppEventSpeakerActive,
    AppEventSpeakerInactive,
    
    AppEventA2dpLinkLost,
    AppEventA2dpLinkResumed,
    AppEventA2dpLinkDisconnected,
    AppEventA2dpStreamingStarted,
    AppEventA2dpStreamingSuspended,
    
    AppEventAvrcpLinkLost,
    AppEventAvrcpLinkResumed,
    AppEventAvrcpLinkDisconnected,
    AppEventAvrcpPlay,
    AppEventAvrcpPause,
    AppEventAvrcpStop,
    AppEventAvrcpPrev,
    AppEventAvrcpNext,
    
    MaxAppEvents
} mvdAppEvent;


typedef enum
{
    StateLedIdle,
    StateLedPhase,
    StateLedMark,
    StateLedSpace,
    StateLedDelay
} mvdLedState;


typedef struct
{
    uint16 phase_time;
    uint16 mark_time;
    uint16 space_time;
    uint16 delay_time;
    uint16 repeat_count;
    uint16 repeats;
    uint16 pio_lines;
    mvdLedState state;
} mvdLedConfig;


#define PS_PIN_CODE_LIST_MAX_SIZE  4
#define PS_BATTERY_CONFIG_SIZE     5 

#define PS_REMOTE_DEVICE_LIST_MAX_ENTRIES 5
#define PS_REMOTE_DEVICE_LIST_ELEMENT_SIZE 4
#define PS_REMOTE_DEVICE_LIST_MAX_SIZE (PS_REMOTE_DEVICE_LIST_MAX_ENTRIES * PS_REMOTE_DEVICE_LIST_ELEMENT_SIZE)
#define PS_HID_CONFIG_SIZE     8
#define PS_HID_MAPPING_SIZE    (MaxAppEvents)
#define PS_LED_CONFIG_ELEMENT_SIZE 2
#define PS_LED_CONFIG_KEY_SIZE (PS_LED_CONFIG_ELEMENT_SIZE * MaxAppStates)
#define PS_HID_SEQUENCE_MAX_SIZE 30

typedef struct
{
    struct
    {
        uint16 count;
        uint16 buffer[PS_REMOTE_DEVICE_LIST_MAX_SIZE];
    } remote_device_list;
} mvdPsKeyData;


typedef struct
{
    uint16 play;
    uint16 pause;
    uint16 playpause;
    uint16 stop;
    uint16 prev;
    uint16 next;
    uint16 answer;
    uint16 hangup;
} mvdHidConfig;


typedef enum
{
    ProfileNone   = 0x00,
    ProfileA2dp   = 0x04,
    ProfileAvrcp  = 0x08,
    ProfileAll    = 0x0f
} mvdProfiles;


typedef struct
{    
    int16        path_loss;
    mvdProfiles  profiles:4;
    unsigned     profiles_complete:1;
} mvdEirData;


typedef struct
{
    uint16             read_idx;
    uint16             write_idx;
    bdaddr             buffer[INQUIRY_SCAN_BUFFER_SIZE];
    mvdEirData         eir_data[INQUIRY_SCAN_BUFFER_SIZE];
} mvdInquiryScanData;


typedef enum
{
    CommActionNone,                 /* Do nothing */
    CommActionStream,               /* Streams audio, via A2DP, to either last/current remote device */
    CommActionEndStream,            /* Stops streaming and returns to Connected state */
    CommActionConnect,              /* Creates an SLC connection with last remote device - a call will be created if audio exists */
    CommActionDisconnect,           /* Disconnect from all remote devices - any existing call will be ended */
    CommActionInquire,              /* Searches for a suitable remote device and connects to it */
    CommActionDiscover,             /* Discovery from any current device then issues a CommActionInquire */
    CommActionDiscoverWhileConnected/* Discovery from any current device then issues a CommActionInquire. Doesn't need to disconnect before doing discovery. */
} mvdCommAction;


typedef enum
{
    EncoderNone,
    EncoderAv,
    EncoderAnalog
} mvdCodecMode;


typedef enum
{
    CodecSbc = 0
} mvdCodecType;


/* For Analogue mode, only two streaming state exit: 
   Pending and active, no active state     
*/
typedef enum
{
    StreamingInactive,      /* No audio is being streamed over USB */
    StreamingPending,       /* No audio is being streamed over USB but audio link to headset remains open */
    StreamingActive         /* Audio being streamed over USB */
    
} mvdStreamingState;


typedef enum
{
    SourceUsb = 0,
    SourceAnalog
} a2dpSouceType;


/*! @brief Source codec data. */
/* This struct must match with the codec_data_type in a2dp.h. We've just missed off the last uint16 which isn't needed here */
typedef struct
{
    uint8 content_protection;
    uint32 voice_rate;
    unsigned bitpool:8;
    unsigned format:8;
    uint16 packet_size;
    Sink media_sink_b;
} source_codec_data_type;


typedef struct
{
    bool connect_sink;
    Sink media_sink;
} dualstream_mode_params;


/*! @brief A2DP Specific data */
typedef struct
{
    device_sep_list *sep_entries;       /*!< The Stream End Point (SEP) pointer */
    source_codec_data_type codecData;     /*!< The audio codec related data */
} a2dpData;


/*! @brief Definition of the battery monitoring configuration */
typedef struct
{
    unsigned unused:16;

    unsigned low_threshold:8 ;
    unsigned shutdown_threshold:8 ;  
    
    unsigned level_1_threshold:8 ;
    unsigned level_2_threshold:8 ;
    
    unsigned monitoring_period:8 ;
    unsigned unused2:8;
    
    /* the time period at which the low batt tone is played */
    unsigned low_batt_tone_period:16 ;

} battery_config_type;


/*! @brief Definition of the power configuration */
typedef struct
{
    battery_config_type battery;
} power_config_type;


/*! @brief Task to receive AIO readings */
typedef struct
{
    TaskData        task;
    BatteryState    state;
    uint16          current_reading;
} aioTask;


/*! @brief Enumeration of Charger states */
typedef enum
{
    disconnected,       /*!< Charger is disconnected */
    trickle_charge,     /*!< Charger is trickle charging */
    fast_charge,        /*!< Charger is fast charging */
    charge_error        /*!< Charger error */
} charging_state_type;


/*! @brief Definition of the power state */
typedef struct
{
    power_config_type   config;     /*!< Configuration */
    aioTask             vbat_task;  /*!< Vbat */
} power_type;


typedef struct devInstanceData
{
    TaskData        task;      /*!< Task */
    
    bdaddr          bd_addr;        /*!< Bluetooth Address of remote device */

    A2DP            *a2dp;          /*!< A2DP profile library instance */
    AVRCP           *avrcp;         /*!< AVRCP profile library instance */

    Sink            audio_sink;
    Sink            a2dp_sig_sink;
    Sink            a2dp_media_sink;
    
    uint8           a2dp_seid;    
    
    uint16          start_pin_idx;
    uint16          pin_idx;
    uint16          a2dp_closing;
    
    mvdA2dpState    a2dp_state:4;
    mvdAvrcpState   avrcp_state:3; 
    mvdProfiles     remote_profiles:4;        /* Which profiles remote device supports */
    unsigned        connect_timer_expired:1;
    
    mvdProfiles     responding_profiles:4; 
    unsigned        pin_requested:1;
    unsigned        pin_authorised:1;
    unsigned        pin_wrapped:1;
    unsigned        pin_list_exhausted:1;    
    unsigned        paired_list_read:1;    
    unsigned        available_profiles_connected:1;
    unsigned        a2dp_reopen:1; 
    unsigned        role:2; 
    unsigned        a2dp_reopen_tries:3;
    
    unsigned        a2dp_audio_quality:3;
    unsigned        a2dp_reopen_codec:3; 
    unsigned        unused:10;
    
    unsigned        sbc_min_bitpool:8; 
    unsigned        sbc_max_bitpool:8;
    
} devInstanceTaskData;  


typedef struct
{
    TaskData        task;
    TaskData        audio_control;
    TaskData        audio_streaming;    
    
    Task            codecTask;                      /*!< Codec task */
    Task            a2dp_audio_plugin;              /*!< A2DP audio plugin */
    
    a2dpData        a2dp_data;                      /*!< Local A2DP data */
    
    power_type      *power;                         /*!< Pointer to headset power configuration */
    
    devInstanceTaskData *dev_inst[MAX_NUM_DEV_CONNECTIONS];
    
    bdaddr          search_bdaddr;
    bdaddr          connect_bdaddr;
    
    AUDIO_SINK_T    sink_type;
    
    PioState        pio_states;
    
    mvdHidConfig    hid_config;
    
    uint32          a2dp_sample_rate;               /*!< A2DP Sample Rate */
    
    uint16          adc_volume;
    uint16          pskey_sbc_codec_config;
    uint16          pskey_mp3_codec_config;
    uint16          pskey_faststream_codec_config;
    
    uint16          local_supported_features[4];
    
    mvdAppState     app_state:4;
    unsigned        bidirect_faststream:1;    
    unsigned        clearing_pdl:1;
    unsigned        a2dp_channel_mode:4;            /*!< A2DP Channel Mode */
    unsigned        a2dpCodecsEnabled:4;            /*!< Enabled Codecs for A2DP */
    
    signed          inquiry_tx:8;                    /* Inquiry Tx Power of local device */
    unsigned        remote_profiles_complete:1;        /* Remote Profile list complete */
    unsigned        connect_attempts: 3;            /* All profiles connection attempts  */
    unsigned        s_connect_attempts: 2;          /* Single profile connection attempts */
    unsigned        audioAdaptorPoweredOn:1;        /* Audio adaptor has been powered on */
    unsigned        PowerOffIsEnabled:1;            /* Enable poweroff the audio adaptor */
        
    charging_state_type charger_state:2;            /*!< The current charger state */
    unsigned        low_battery_warning:1;            /*!< Flag to indicate if to send the low battery warning */
    unsigned        initial_battery_reading:1;      /*!< Flag to indicate if the initail battery reading is being taken */
    unsigned        chargerconnected: 1;            /*!< Flag to indicate if charger is connected */
    mvdCodecMode    active_encoder:3;        
    unsigned        auto_connect_in_progress:1;  /* Dongle is attempting to connect to one of the devices in the recent device list */
    unsigned        disconnect_requested:1;
    mvdStreamingState   audio_streaming_state:2;
    unsigned         a2dp_media_stream_holdoff:1;
    unsigned         a2dp_media_stream_requested:1;
    
    unsigned         app_inquiry_timer_expired:1;
    unsigned         app_reconnect_timer_expired:1;
    unsigned         inquiring:1;
    
    mvdCommAction    comm_action:4;            /* Connect/disconnect action being performed */
    mvdCommAction    pending_comm_action:4;    /* queued comm action */
    mvdProfiles      initialised_profiles:4;    /* Which profiles (aghsp/aghfp/a2dp/avrcp) have been initialised */
    mvdProfiles      supported_profiles:4;        /* Which profiles (aghsp/aghfp/a2dp/avrcp) local device supports */
    
    mvdProfiles      remote_profiles:4;        /* Which profiles (hsp/hfp) remote device supports */
    mvdProfiles      connecting_profile:4;     /* Profile being connected by local device */
    unsigned         a2dp_active_seid:8;

} mvdTaskData;


/* ------------------------------------------------------------------------ */
/* Application messages */

#define APP_MESSAGE_BASE 0x5000
enum
{
    APP_INIT = APP_MESSAGE_BASE,
    APP_INIT_CFM,    
    APP_POWERON_REQ,            /* power on the audio adaptor automatically after being initialised or power on after powering off */
    APP_POWEROFF_REQ,           /* power off the kalimba and enter the connectable state */
    APP_LIMBO_TIMEOUT,
    APP_RESET_PDL_REQ,          /* delete the PDL */ 
    APP_CHARGER_CONNECTED,
    APP_CHARGER_DISCONNECTED,
    APP_CHARGER_MONITOR,
    APP_CHECK_FOR_LOW_BATT,
    APP_LOW_BATTERY,
    APP_FAST_CHARGE,
    APP_TRICKLE_CHARGE,
    APP_CANCEL_LED_INDICATION,
    APP_OK_BATTERY,    
    APP_KICK_COMM_ACTION_REQ,    /* Kick comm action via a message rather than direct call */    
    APP_REFRESH_ENCRYPTION_REQ,    
    APP_DEVICE_CONNECT_REQ,
    APP_DEVICE_DISCOVER_REQ,
    APP_DEVICE_CONNECT_SECOND_REQ,    
    APP_ENTER_DFU_MODE,    
    APP_AUDIO_STREAMING_ACTIVE,
    APP_AUDIO_STREAMING_INACTIVE,
    APP_AUDIO_STREAMING_TIMER,    
    APP_A2DP_MEDIA_STREAM_HOLDOFF,    
    APP_INQUIRY_TIMER,
    APP_CONNECT_TIMER,
    APP_RECONNECT_TIMER,
    APP_PROFILE_CONNECT_TIMER,    
    APP_LED_TIMER,    
    APP_INTERNAL_DESTROY_REQ,    
    APP_STREAM_CLOSE_COMPLETE,    
    APP_INTERNAL_CONNECT_MEDIA_CHANNEL_REQ,    
    APP_MEDIA_CHANNEL_REOPEN_REQ,    
    APP_CONNECT_A2DP_AUDIO,
    APP_PROCESS_INQUIRE_RESULT,
    APP_CONNECT_CFM,
    
    APP_MESSAGE_TOP
};

typedef struct
{
    mvdCommAction comm_action;
} APP_KICK_COMM_ACTION_REQ_T;

typedef struct
{
    uint16 id;
    uint32 ticks;
} APP_LED_TIMER_T;

typedef struct
{
    bool status;
} APP_CALL_OPEN_CFM_T;

typedef struct
{
    bool status;
} APP_CALL_CLOSE_IND_T;

typedef struct
{
    bool status;
} APP_CALL_CLOSE_CFM_T;

typedef struct
{
    bool status;
} APP_STREAM_OPEN_CFM_T;

typedef struct
{
    bool status;
} APP_STREAM_CLOSE_CFM_T;

typedef struct
{
    bool disconnect_current;
} APP_DEVICE_CONNECT_REQ_T;    

typedef struct
{
    bool disconnect_current;
} APP_DEVICE_DISCOVER_REQ_T;

typedef struct
{
    Sink media_sink;
} APP_CONNECT_A2DP_AUDIO_T;

typedef struct
{
    bool success;
} APP_CONNECT_CFM_T;

/* ------------------------------------------------------------------------ */
/* Helper defines for messages */

#define MAKE_APP_MESSAGE(TYPE) TYPE##_T *message = malloc(sizeof(TYPE##_T));

#define A2DP_MESSAGE(id) (id>=A2DP_MESSAGE_BASE && id<A2DP_MESSAGE_TOP)
#define AVRCP_MESSAGE(id) (id>=AVRCP_MESSAGE_BASE && id<AVRCP_MESSAGE_TOP)
#define AGHFP_MESSAGE(id) (id>=AGHFP_MESSAGE_BASE && id<AGHFP_MESSAGE_TOP)
#define APP_MESSAGE(id) (id>=APP_MESSAGE_BASE && id<APP_MESSAGE_TOP)
#define CL_MESSAGE(id) (id>=CL_MESSAGE_BASE && id<CL_MESSAGE_TOP)
#define CODEC_MESSAGE(id) (id >= CODEC_MESSAGE_BASE ) && (id <= CODEC_MESSAGE_TOP)
#define SYSTEM_MESSAGE(id) (id>=MESSAGE_FROM_HOST && id<= MESSAGE_CHARGER_CHANGED)

/* ------------------------------------------------------------------------ */
/* The task data structure - visible to all (so no need to pass it between functions) */

extern mvdTaskData *the_app ;


#endif
