/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    a2dp_private.h

DESCRIPTION
    This file contains data private to the a2dp library.

*/

#ifndef A2DP_PRIVATE_H_
#define A2DP_PRIVATE_H_

/*
#ifndef DEBUG_PRINT_ENABLED
#define DEBUG_PRINT_ENABLED
#endif
*/

#include "a2dp.h"

#include <panic.h>

/* Provide a reference for all A2DP lib modules - should not be exposed at API level */
extern A2DP *a2dp;


/* Macros for creating messages */
#define MAKE_A2DP_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);
#define MAKE_A2DP_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicUnlessMalloc(sizeof(TYPE##_T) + LEN);


/*
#ifndef A2DP_DEBUG_LIB
#define A2DP_DEBUG_LIB
#endif
*/


/* Macro used to generate debug lib printfs. */
#ifdef A2DP_DEBUG_LIB
#define DEBUG_PRINT_ENABLED
#include <panic.h>
#include <stdio.h>
#include <print.h>
#define A2DP_DEBUG(x)   {printf x; Panic();}
#else
#define A2DP_DEBUG(x)
#endif


/* L2CAP PSM */
#define AVDTP_PSM                       (0x19)

/*
    The watchdog timeouts.
*/
/*
    The GAVDP specifies TGAVDP100 which is a
    short response timeout for certain signals
    in order to prevent the user seeing an
    obvious hang.
    The range proposed by the spec is 0.5 to
    3 seconds.

    In practice, when the AV device is in a
    scatternet with a SCO connection, baseband
    interactions can stall signals excessively.
*/
#ifndef A2DP_DEBUG_LIB
#define WATCHDOG_TGAVDP100              D_SEC(5)
#else
/* the timeout should be longer in debug mode
   as printf takes a long time. */
#define WATCHDOG_TGAVDP100              D_SEC(10)
#endif

/*
    For signals not specified to use TGAVDP100,
    we still need a timeout to prevent our device
    hanging.  This needs to be long as the spec
    does not specify a timeout and so the remote
    device could potentially prompt the user
    before responding to a signal.
*/
#define WATCHDOG_GENERAL                D_SEC(30)

/*
    Maximum number words needed to contain a
    bit flag for all possible SEID values
*/
#define NUM_SEID_PACKED_WORDS           (4)


/*
    Timeout for the signalling channel.
    When this fires if the signalling channel is
    still connected, drop it.
*/
#define SIGNAL_TIMER                    (1300)


/* A2dp Profile Library private messages */
#define A2DP_MSG_BASE     (0x0)


#define A2DP_DEVICE_ID_UNKNOWN  0xFF
#define A2DP_MAX_REMOTE_DEVICES 2   /* Maximum number of separate remote devices supported */
#define A2DP_MAX_MEDIA_CHANNELS 1   /* Maximum number of supported media channels per remote device */

#define AVDTP_NO_PAYLOAD 0

#define CONFIGURATION_BY_CLIENT     0
#define CONFIGURATION_SELECTED      1
#define CONFIGURATION_NOT_SELECTED  2

enum
{
    /* Initialisation */
    A2DP_INTERNAL_SIGNALLING_CONNECT_REQ = A2DP_MSG_BASE,
    A2DP_INTERNAL_SIGNALLING_CONNECT_RES,
    A2DP_INTERNAL_SIGNALLING_DISCONNECT_REQ,
    A2DP_INTERNAL_MEDIA_CONFIGURE_RSP,
    A2DP_INTERNAL_MEDIA_OPEN_REQ,
    A2DP_INTERNAL_MEDIA_OPEN_RES,
    A2DP_INTERNAL_MEDIA_START_REQ,
    A2DP_INTERNAL_MEDIA_SUSPEND_REQ,
    A2DP_INTERNAL_MEDIA_CLOSE_REQ,
    A2DP_INTERNAL_LINKLOSS_TIMEOUT_BASE,
    A2DP_INTERNAL_WATCHDOG_BASE = (A2DP_INTERNAL_LINKLOSS_TIMEOUT_BASE + 7),
    A2DP_INTERNAL_MESSAGE_TOP = (A2DP_INTERNAL_WATCHDOG_BASE + 7) 
};


/*!
    @brief Error codes.
*/
typedef enum
{
    avdtp_ok                            = (0x00),   /*!< Used internally to indicate that the command is valid. */
    avdtp_bad_header_format             = (0x01),   /*!< The request packet header format error that is not specified above ERROR_CODE. */
    avdtp_bad_length                    = (0x11),   /*!< The request packet length is not match the assumed length. */
    avdtp_bad_acp_seid                  = (0x12),   /*!< The requested command indicates an invalid ACP SEID (not addressable). */
    avdtp_sep_in_use                    = (0x13),   /*!< The SEP is in use. */
    avdtp_sep_not_in_use                = (0x14),   /*!< The SEP is not in use. */
    avdtp_bad_serv_category             = (0x17),   /*!< The value of Service Category in the request packet is not defined in AVDTP. */
    avdtp_bad_payload_format            = (0x18),   /*!< The requested command has an incorrect payload format (Format errors not specified in this ERROR_CODE). */
    avdtp_not_supported_command         = (0x19),   /*!< The requested command is not supported by the device. */
    avdtp_invalid_capabilities          = (0x1a),   /*!< The reconfigure command is an attempt to reconfigure a transport service capabilities of the SEP. Reconfigure is only permitted for application service capabilities. */
    avdtp_bad_recovery_type             = (0x22),   /*!< The requested Recovery Type is not defined in AVDTP. */
    avdtp_bad_media_transport_format    = (0x23),   /*!< The format of Media Transport Capability is not correct. */
    avdtp_bad_recovery_format           = (0x25),   /*!< The format of Recovery Service Capability is not correct. */
    avdtp_bad_rohc_format               = (0x26),   /*!< The format of Header Compression Service. */
    avdtp_bad_cp_format                 = (0x27),   /*!< The format of Content Protection Service Capability is not correct. */
    avdtp_bad_multiplexing_format       = (0x28),   /*!< The format of Multiplexing Service Capability is not correct. */
    avdtp_unsupported_configuration     = (0x29),   /*!< Configuration not supported. */
    avdtp_bad_state                     = (0x31)    /*!< Indicates that the ACP state machine is in an invalid state in order to process the signal.*/
} avdtp_error_code;

/* AVDTP header message type */
enum
{
    avdtp_message_type_command = (0x0),
    avdtp_message_type_accept = (0x02),
    avdtp_message_type_reject = (0x03)
};

/* Signalling header packet type */
enum
{
    avdtp_packet_type_single = (0x0),
    avdtp_packet_type_start = (0x01),
    avdtp_packet_type_continue = (0x02),
    avdtp_packet_type_end = (0x03)
};

/* AVDTP signals */
typedef enum
{
    avdtp_null              = (0x00),
    avdtp_discover,
    avdtp_get_capabilities,
    avdtp_set_configuration,
    avdtp_get_configuration,
    avdtp_reconfigure,
    avdtp_open,
    avdtp_start,
    avdtp_close,
    avdtp_suspend,
    avdtp_abort,
    avdtp_security_control,
    avdtp_max_signal_id
} avdtp_signal_id;


#define AVDTP_OUTGOING_CONNECTION_ID 0

/* Connection state for signalling and media channels */
typedef enum
{
    avdtp_connection_idle,
    avdtp_connection_paging,
    avdtp_connection_paged,
    avdtp_connection_crossover,
    avdtp_connection_connected,
    avdtp_connection_disconnecting,
    avdtp_connection_disconnect_pending
} avdtp_connection_state;

#if 1
/* Stream state for a media channel */
typedef enum
{
    avdtp_stream_idle,
    avdtp_stream_discovering,
    avdtp_stream_reading_caps,
    avdtp_stream_processing_caps,
    avdtp_stream_configuring,
    avdtp_stream_configured,
    avdtp_stream_local_opening,
    avdtp_stream_remote_opening,
    avdtp_stream_open,
    avdtp_stream_streaming,
    avdtp_stream_local_starting,
    avdtp_stream_local_suspending,
    avdtp_stream_local_closing,
    avdtp_stream_remote_closing,
    avdtp_stream_reconfig_reading_caps,
    avdtp_stream_reconfiguring,
    avdtp_stream_local_aborting,
    avdtp_stream_remote_aborting
} avdtp_stream_state;
#endif
#if 0
/* Stream state for a media channel */
typedef enum
{
    avdtp_stream_idle                   = 0x0001,
    avdtp_stream_discovering            = 0x0002,
    avdtp_stream_reading_caps           = 0x0004,
    avdtp_stream_processing_caps        = 0x0008,
    avdtp_stream_configuring            = 0x0010,
    avdtp_stream_configured             = 0x0020,
    avdtp_stream_opening                = 0x0040,
    avdtp_stream_open                   = 0x0080,
    avdtp_stream_streaming              = 0x0100,
    avdtp_stream_starting               = 0x0200,
    avdtp_stream_suspending             = 0x0400,
    avdtp_stream_closing                = 0x0800,
    avdtp_stream_reconfig_reading_caps  = 0x1000,
    avdtp_stream_reconfiguring          = 0x2000,
    avdtp_stream_aborting               = 0x4000,
    avdtp_action_remote                 = 0x8000    /* Set for stream states initiated by remote device, clear for states initiated by local device */
} avdtp_stream_state;
#endif

/* The signalling connection information */
typedef struct signalling_channel
{
    union
    {
        struct
        {   /* Only be valid during an channel establishment */
            uint16      outbound_cid;
            uint16      inbound_cid;                  
            uint8       inbound_id;
        } setup;
        struct
        {   /* Only valid once a channel has been established */
            Sink        sink;                           
            unsigned    mtu:12;
            unsigned    issued_transaction_label:4;
            unsigned    received_transaction_label:4;
            unsigned    received_packet_length:12;
            uint8      *received_packet;
        } active;
    } connection;

    struct
    {
        unsigned                unused:6;
        unsigned                pending_issued_transaction:1;
        unsigned                pending_received_transaction:1;
        avdtp_connection_state  connection_state:3;
        avdtp_stream_state      stream_state:5;         /* TODO: This should be linked to the media channel */
    } status;
} signalling_channel;

/* The media connection information */
typedef struct media_channel
{
    union
    {
        struct
        {   /* Only be valid during an channel establishment */
            uint16      outbound_cid;
            uint16      inbound_cid;                  
            uint8       inbound_id;
        } setup;
        struct
        {   /* Only valid once a channel has been established */
            Sink        sink;                           
            unsigned    unused:4;
            unsigned    mtu:12;
            unsigned    remote_seid:8;                  /* TODO: Why is this not just list_discovered_remote_seids[current_discovered_remote_seid]? */
            unsigned    local_seid:8;                   /* TODO: Can be obtained directly from the appriopriate data block */
        } active;
    } connection;
    
    struct
    {
        unsigned                unused:9;
        unsigned                instantiated:1;                 /* Flag to indicate if this data structure is valid */
        unsigned                media_id:3;                     /* Used to quickly determine the media id */
        avdtp_connection_state  connection_state:3;
    } status;
} media_channel;

typedef struct
{
    bdaddr                  bd_addr;
    unsigned                unused:2;
    unsigned                linkloss:1;                             /* Flag to indicate that a linkloss situation is being managed */
    unsigned                linkloss_timer_expired:1;               /* Flag to indicate that no further reconnection attempts should be made */
    unsigned                instantiated:1;                         /* Flag to indicate if this data structure is valid */
    unsigned                device_id:3;                            /* Used to quickly determine the device id */
    unsigned                remote_seid:8;                          /* Holds the current remote SEID */
    signalling_channel      signal_conn;                            /* Single signalling channel per device */
    media_channel           media_conn[A2DP_MAX_MEDIA_CHANNELS];    /* One or more media channels per device */
} remote_device;

typedef struct
{
    unsigned                block_size:9;
    unsigned                element_size:7;
    unsigned                current_element:6;
    unsigned                offset:10;
} data_block_info;

typedef enum
{
    data_block_sep_list,
    data_block_list_preferred_local_seids,
    data_block_list_discovered_remote_seids,
    data_block_configured_service_caps,
    max_data_blocks
} data_block_id;

#define DATA_BLOCK_INDEX_INVALID    0xFF
#define DATA_BLOCK_INDEX_NEXT       0xFF
#define DATA_BLOCK_INDEX_PREVIOUS   0xFE

typedef struct
{
    uint16          size_blocks;
    data_block_info block[A2DP_MAX_REMOTE_DEVICES][max_data_blocks];
} data_block_header;

/* The sep information */
typedef struct sep_info
{
    uint8                   *reconfigure_caps;
    uint16                  reconfigure_caps_size;
#if 0
    data_block_header       *data_blocks;
#else
    data_block_header       data_blocks;
#endif

} sep_info;


struct __A2DP
{
    TaskData                    task;
    Task                        clientTask;

    uint8                       sdp_register_outstanding;
    uint16                      linkloss_timeout;
    remote_device               remote_conn[A2DP_MAX_REMOTE_DEVICES];
    sep_info                    sep;
};

typedef struct
{
    bdaddr          addr;
} A2DP_INTERNAL_SIGNALLING_CONNECT_REQ_T;


typedef struct
{
    remote_device  *device;
    bool            accept;
} A2DP_INTERNAL_SIGNALLING_CONNECT_RES_T;


typedef struct
{
    remote_device  *device;
} A2DP_INTERNAL_SIGNALLING_DISCONNECT_REQ_T;


typedef struct
{
    bool            accept;
    uint16          size_codec_service_caps;
    uint8           codec_service_caps[1];
} A2DP_INTERNAL_MEDIA_CONFIGURE_RSP_T;


typedef struct
{
    remote_device  *device;
} A2DP_INTERNAL_MEDIA_OPEN_REQ_T;


typedef struct
{
    remote_device  *device;
    bool           accept;
} A2DP_INTERNAL_MEDIA_OPEN_RES_T;


typedef struct
{
    remote_device  *device;
    media_channel  *media;
} A2DP_INTERNAL_MEDIA_START_REQ_T;


typedef struct
{
    remote_device  *device;
    media_channel  *media;
} A2DP_INTERNAL_MEDIA_SUSPEND_REQ_T;


typedef struct
{
    remote_device  *device;
    media_channel  *media;
} A2DP_INTERNAL_MEDIA_CLOSE_REQ_T;


typedef struct
{
    remote_device  *device;
} A2DP_INTERNAL_LINKLOSS_TIMEOUT_T;


#endif /* A2DP_PRIVATE_H_ */
