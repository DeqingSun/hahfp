/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    a2dp_api.c

DESCRIPTION


NOTES

*/


/****************************************************************************
    Header files
*/
#include "a2dp.h"
#include "a2dp_private.h"
#include "a2dp_api.h"
#include "a2dp_codec_handler.h"

#include <bdaddr.h>
#include <stdlib.h>
#include <print.h>

/****************************************************************************/
static const a2dp_signalling_state A2dpSignallingState[ 6 ] =
{
    a2dp_signalling_idle,
    a2dp_signalling_connecting,
    a2dp_signalling_connecting,
    a2dp_signalling_connecting,
    a2dp_signalling_connected,
    a2dp_signalling_disconnecting
};

static const a2dp_stream_state A2dpStreamState[ 18 ] =
{
    a2dp_stream_idle,
    a2dp_stream_discovering,
    a2dp_stream_configuring,
    a2dp_stream_configuring,
    a2dp_stream_configuring,
    a2dp_stream_configured,
    a2dp_stream_opening,
    a2dp_stream_opening,
    a2dp_stream_open,
    a2dp_stream_streaming,
    a2dp_stream_opening,
    a2dp_stream_suspending,
    a2dp_stream_closing,
    a2dp_stream_closing,
    a2dp_stream_reconfiguring,
    a2dp_stream_reconfiguring,
    a2dp_stream_aborting,
    a2dp_stream_aborting
};

#if 0
static a2dp_stream_state getStreamState (uint16 state_mask)
{
    uint8 state = 0;

    if (state_mask & 0xFF00)
    {
        state_mask >>= 8;
        state = 8;
    }
    if (state_mask & 0xF0)
    {
        state_mask >>= 4;
        state += 4;
    }
    if (state_mask & 0xC)
    {
        state_mask >>= 2;
        state += 2;
    }
    if (state_mask & 0x2)
    {
        state++;
    }

    return A2dpStreamState[ state ];
}
#endif


static uint8 findDeviceId (remote_device *device)
{
    PRINT((" device_id="));

    if ((device != NULL) && device->instantiated)
    {
        PRINT(("%u",device->device_id));
        return device->device_id;
    }

    PRINT(("INVALID_DEVICE_ID"));
    return INVALID_DEVICE_ID;
}


static uint8 findStreamId (media_channel *media)
{
    PRINT((" stream_id="));

    if ((media != NULL) && media->status.instantiated)
    {
        PRINT(("%u",media->status.media_id));
        return media->status.media_id;
    }

    PRINT(("INVALID_STREAM_ID"));
    return INVALID_STREAM_ID;
}


void a2dpSignallingConnectInd (remote_device *device)
{
    MAKE_A2DP_MESSAGE(A2DP_SIGNALLING_CONNECT_IND);

    PRINT(("a2dpSignallingConnectInd"));

    message->device_id = findDeviceId(device);

    if (device != NULL)
    {
        message->addr = device->bd_addr;
    }
    else
    {
        BdaddrSetZero(&message->addr);
    }

    MessageSend(a2dp->clientTask, A2DP_SIGNALLING_CONNECT_IND, message);
}


void a2dpSignallingConnectCfm (remote_device *device, a2dp_status_code status)
{
    MAKE_A2DP_MESSAGE(A2DP_SIGNALLING_CONNECT_CFM);

    PRINT(("a2dpSignallingConnectCfm status=%u", status));

    message->device_id = findDeviceId(device);
    message->status = status;

    if (device != NULL)
    {
        message->addr = device->bd_addr;
    }
    else
    {
        BdaddrSetZero(&message->addr);
    }

    MessageSend(a2dp->clientTask, A2DP_SIGNALLING_CONNECT_CFM, message);
}


void a2dpSignallingDisconnectInd (remote_device *device, a2dp_status_code status)
{
    MAKE_A2DP_MESSAGE(A2DP_SIGNALLING_DISCONNECT_IND);

    PRINT(("a2dpSignallingDisconnectInd status=%u", status));

    message->device_id = findDeviceId(device);
    message->status = status;

    if (device != NULL)
    {
        message->addr = device->bd_addr;
    }
    else
    {
        BdaddrSetZero(&message->addr);
    }

    MessageSend(a2dp->clientTask, A2DP_SIGNALLING_DISCONNECT_IND, message);
}

void a2dpSignallingLinklossInd (remote_device *device)
{
    MAKE_A2DP_MESSAGE(A2DP_SIGNALLING_LINKLOSS_IND);

    PRINT(("a2dpSignallingLinklossInd"));

    message->device_id = findDeviceId(device);

    MessageSend(a2dp->clientTask, A2DP_SIGNALLING_LINKLOSS_IND, message);
}


void a2dpMediaOpenInd (remote_device *device)
{
    MAKE_A2DP_MESSAGE(A2DP_MEDIA_OPEN_IND);

    PRINT(("a2dpMediaOpenInd"));

    message->device_id = findDeviceId(device);

    MessageSend(a2dp->clientTask, A2DP_MEDIA_OPEN_IND, message);
}

void a2dpMediaOpenCfm (remote_device *device, media_channel *media, a2dp_status_code status)
{
    MAKE_A2DP_MESSAGE(A2DP_MEDIA_OPEN_CFM);

    PRINT(("a2dpMediaOpenCfm status=%u", status));

    message->device_id = findDeviceId(device);
    message->stream_id = findStreamId(media);
    message->status = status;

    MessageSend(a2dp->clientTask, A2DP_MEDIA_OPEN_CFM, message);
}


void a2dpMediaCloseInd (remote_device *device, media_channel *media, a2dp_status_code status)
{
    MAKE_A2DP_MESSAGE(A2DP_MEDIA_CLOSE_IND);

    PRINT(("a2dpMediaCloseInd status=%u", status));

    message->device_id = findDeviceId(device);
    message->stream_id = findStreamId(media);
    message->status = status;

    MessageSend(a2dp->clientTask, A2DP_MEDIA_CLOSE_IND, message);
}


void a2dpMediaStartInd (remote_device *device, media_channel *media)
{
    MAKE_A2DP_MESSAGE(A2DP_MEDIA_START_IND);

    PRINT(("a2dpMediaStartInd"));

    message->device_id = findDeviceId(device);
    message->stream_id = findStreamId(media);

    MessageSend(a2dp->clientTask, A2DP_MEDIA_START_IND, message);
}


void a2dpMediaStartCfm (remote_device *device, media_channel *media, a2dp_status_code status)
{
    MAKE_A2DP_MESSAGE(A2DP_MEDIA_START_CFM);

    PRINT(("a2dpMediaStartCfm status=%u", status));

    message->device_id = findDeviceId(device);
    message->stream_id = findStreamId(media);
    message->status = status;

    MessageSend(a2dp->clientTask, A2DP_MEDIA_START_CFM, message);
}


void a2dpMediaSuspendInd (remote_device *device, media_channel *media)
{
    MAKE_A2DP_MESSAGE(A2DP_MEDIA_SUSPEND_IND);

    PRINT(("a2dpMediaSuspendInd"));

    message->device_id = findDeviceId(device);
    message->stream_id = findStreamId(media);

    MessageSend(a2dp->clientTask, A2DP_MEDIA_SUSPEND_IND, message);
}


void a2dpMediaSuspendCfm (remote_device *device, media_channel *media, a2dp_status_code status)
{
    MAKE_A2DP_MESSAGE(A2DP_MEDIA_SUSPEND_CFM);

    PRINT(("a2dpMediaSuspendCfm status=%u", status));

    message->device_id = findDeviceId(device);
    message->stream_id = findStreamId(media);
    message->status = status;

    MessageSend(a2dp->clientTask, A2DP_MEDIA_SUSPEND_CFM, message);
}


void a2dpMediaCodecConfigureInd (remote_device *device, media_channel *media)
{
    MAKE_A2DP_MESSAGE(A2DP_MEDIA_CODEC_CONFIGURE_IND);

    PRINT(("a2dpMediaCodecConfigureInd"));

    message->device_id = findDeviceId(device);
    message->stream_id = findStreamId(media);

    MessageSend(a2dp->clientTask, A2DP_MEDIA_CODEC_CONFIGURE_IND, message);
}


#if 0
/****************************************************************************/
void sendGetCurrentSepCapabilitiesCfm(a2dp_status_code status, const uint8 *caps, uint16 size_caps)
{
    MAKE_A2DP_MESSAGE_WITH_LEN(A2DP_GET_CURRENT_SEP_CAPABILITIES_CFM, size_caps);

    message->a2dp = a2dp;
    message->status = status;

    if (status == a2dp_success)
    {
        memmove(message->caps, caps, size_caps);
        message->size_caps = size_caps;
    }
    else
    {
        message->caps[0] = 0;
        message->size_caps = 0;
    }

    MessageSend(a2dp->clientTask, A2DP_GET_CURRENT_SEP_CAPABILITIES_CFM, message);
}
#endif




/****************************************************************************/
bool A2dpSignallingConnectRequest (bdaddr *addr)
{
    if ((a2dp == NULL) || (addr == NULL) || BdaddrIsZero(addr))
    {
        return FALSE;
    }
    else
    {
        MAKE_A2DP_MESSAGE(A2DP_INTERNAL_SIGNALLING_CONNECT_REQ);
        message->addr = *addr;

        MessageSend(&a2dp->task, A2DP_INTERNAL_SIGNALLING_CONNECT_REQ, message);
        return TRUE;
    }
}

bool A2dpSignallingConnectResponse (uint16 device_id, bool accept)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES))
    {
        return FALSE;
    }
    else
    {
        MAKE_A2DP_MESSAGE(A2DP_INTERNAL_SIGNALLING_CONNECT_RES);
        message->device = &a2dp->remote_conn[device_id];
        message->accept = accept;

        MessageSend(&a2dp->task, A2DP_INTERNAL_SIGNALLING_CONNECT_RES, message);
        return TRUE;
    }
}


bool A2dpSignallingDisconnectRequest (uint16 device_id)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES))
    {
        return FALSE;
    }
    else
    {
        MAKE_A2DP_MESSAGE(A2DP_INTERNAL_SIGNALLING_DISCONNECT_REQ);
        message->device = &a2dp->remote_conn[device_id];

        MessageSend(&a2dp->task, A2DP_INTERNAL_SIGNALLING_DISCONNECT_REQ, message);
        return TRUE;
    }
}


Sink A2dpSignallingGetSink (uint16 device_id)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES))
    {
        return (Sink)NULL;
    }

    if (a2dp->remote_conn[device_id].signal_conn.status.connection_state == avdtp_connection_connected)
    {
        return a2dp->remote_conn[device_id].signal_conn.connection.active.sink;
    }
    else
    {
        return (Sink)NULL;
    }
}

a2dp_signalling_state A2dpSignallingGetState (uint16 device_id)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES))
    {
        return a2dp_signalling_idle;
    }

    return A2dpSignallingState[ a2dp->remote_conn[device_id].signal_conn.status.connection_state ];
}


bool A2dpMediaOpenRequest (uint16 device_id)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES))
    {
        return FALSE;
    }
    else
    {
        MAKE_A2DP_MESSAGE(A2DP_INTERNAL_MEDIA_OPEN_REQ);
        message->device = &a2dp->remote_conn[device_id];

        MessageSend(&a2dp->task, A2DP_INTERNAL_MEDIA_OPEN_REQ, message);
        return TRUE;
    }
}


bool A2dpMediaOpenResponse(uint16 device_id, bool accept)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES))
    {
        return FALSE;
    }
    else
    {
        MAKE_A2DP_MESSAGE(A2DP_INTERNAL_MEDIA_OPEN_RES);
        message->device = &a2dp->remote_conn[device_id];
        message->accept = accept;

        MessageSend(&a2dp->task, A2DP_INTERNAL_MEDIA_OPEN_RES, message);
        return TRUE;
    }
}


bool A2dpMediaCloseRequest (uint16 device_id, uint16 stream_id)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES) || (stream_id >= A2DP_MAX_MEDIA_CHANNELS))
    {
        return FALSE;
    }
    else
    {
        MAKE_A2DP_MESSAGE(A2DP_INTERNAL_MEDIA_CLOSE_REQ);
        message->device = &a2dp->remote_conn[device_id];
        message->media = &message->device->media_conn[stream_id];

        MessageSend(&a2dp->task, A2DP_INTERNAL_MEDIA_CLOSE_REQ, message);
        return TRUE;
    }
}


bool A2dpMediaStartRequest (uint16 device_id, uint16 stream_id)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES) || (stream_id >= A2DP_MAX_MEDIA_CHANNELS))
    {
        return FALSE;
    }
    else
    {
        MAKE_A2DP_MESSAGE(A2DP_INTERNAL_MEDIA_START_REQ);
        message->device = &a2dp->remote_conn[device_id];
        message->media = &message->device->media_conn[stream_id];

        MessageSend(&a2dp->task, A2DP_INTERNAL_MEDIA_START_REQ, message);
        return TRUE;
    }
}


bool A2dpMediaSuspendRequest (uint16 device_id, uint16 stream_id)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES) || (stream_id >= A2DP_MAX_MEDIA_CHANNELS))
    {
        return FALSE;
    }
    else
    {
        MAKE_A2DP_MESSAGE(A2DP_INTERNAL_MEDIA_SUSPEND_REQ);
        message->device = &a2dp->remote_conn[device_id];
        message->media = &message->device->media_conn[stream_id];

        MessageSend(&a2dp->task, A2DP_INTERNAL_MEDIA_SUSPEND_REQ, message);
        return TRUE;
    }
}


Sink A2dpMediaGetSink (uint16 device_id, uint16 stream_id)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES) || (stream_id >= A2DP_MAX_MEDIA_CHANNELS))
    {
        return (Sink)NULL;
    }

    if (a2dp->remote_conn[device_id].media_conn[stream_id].status.connection_state == avdtp_connection_connected)
    {
        return a2dp->remote_conn[device_id].media_conn[stream_id].connection.active.sink;
    }
    else
    {
        return (Sink)NULL;
    }
}


a2dp_stream_state A2dpMediaGetState (uint16 device_id, uint16 stream_id)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES) || (stream_id >= A2DP_MAX_MEDIA_CHANNELS))
    {
        return a2dp_stream_idle;
    }

    /* TODO: Make multiple stream aware */
    return A2dpStreamState[ a2dp->remote_conn[device_id].signal_conn.status.stream_state ];
}

a2dp_codec_settings * A2dpCodecGetSettings (uint16 device_id, uint16 stream_id)
{
    if ((a2dp == NULL) || (device_id >= A2DP_MAX_REMOTE_DEVICES) || (stream_id >= A2DP_MAX_MEDIA_CHANNELS))
    {
        return NULL;
    }

    /* TODO: Make multiple stream aware */
    return a2dpGetCodecAudioParams( &a2dp->remote_conn[device_id] );
}

#if 0
/*****************************************************************************/
void A2dpCodecConfigureResponse(uint16 device_id, uint16 stream_id, bool accept, uint16 size_codec_service_caps, uint8 *codec_service_caps)
{
    MAKE_A2DP_MESSAGE_WITH_LEN(A2DP_INTERNAL_CODEC_CONFIGURE_RSP, size_codec_service_caps);

    message->accept = accept;
    message->size_codec_service_caps = size_codec_service_caps;
    memmove(message->codec_service_caps, codec_service_caps, size_codec_service_caps);

    MessageSend(&a2dp->task, A2DP_INTERNAL_CODEC_CONFIGURE_RSP, message);
}
#endif

#if 0
/*****************************************************************************/
void A2dpGetCurrentSepCapabilities(A2DP *dep_a2dp)
{
    dep_a2dp = dep_a2dp;

#ifdef A2DP_DEBUG_LIB
    if (!a2dp)
        A2DP_DEBUG(("A2dpGetCurrentSepCapabilities NULL instance\n"));
#endif

    if ((a2dp->remote_conn[0].signal_conn.stream_state == avdtp_stream_open) ||
        (a2dp->remote_conn[0].signal_conn.stream_state == avdtp_stream_streaming) ||
        (a2dp->remote_conn[0].signal_conn.stream_state == avdtp_stream_local_starting) ||
        (a2dp->remote_conn[0].signal_conn.stream_state == avdtp_stream_local_suspending) ||
        (a2dp->remote_conn[0].signal_conn.stream_state == avdtp_stream_reconfig_reading_caps) ||
        (a2dp->remote_conn[0].signal_conn.stream_state == avdtp_stream_reconfiguring)
        )
    {
        if (a2dpSendGetCapabilities(a2dp->sep.remote_seid))
        {
            /* Start watchdog */
            PRINT(("A2dpGetCurrentSepCapabilities seid=%d\n",a2dp->sep.remote_seid));
            MessageSendLater(&a2dp->task, A2DP_INTERNAL_GET_CAPS_TIMEOUT_IND, 0, WATCHDOG_TGAVDP100);
            return;
        }
    }

    sendGetCurrentSepCapabilitiesCfm(a2dp_wrong_state, 0, 0);
}
#endif

#if 0
/*****************************************************************************/
uint8 *A2dpGetCurrentSepConfiguration(A2DP *a2dp, uint16 *size_caps)
{
    if ((a2dp->remote_conn[0].signal_conn.stream_state == avdtp_stream_open) ||
        (a2dp->remote_conn[0].signal_conn.stream_state == avdtp_stream_streaming) ||
        (a2dp->remote_conn[0].signal_conn.stream_state == avdtp_stream_local_starting) ||
        (a2dp->remote_conn[0].signal_conn.stream_state == avdtp_stream_local_suspending)
        )
    {
        uint8 *service_caps = blockGetBase( data_block_configured_service_caps );
        if (service_caps)
        {
            uint8 *caps;
            uint16 size_service_caps = blockGetSize( data_block_configured_service_caps );

            /* Create copy of service caps for app, since the library one could move */
            /* Expectation is that app will free this memory when it has finished with it */
            caps = (uint8 *)malloc( size_service_caps );
            if (caps != NULL)
            {
                memmove( caps, service_caps, size_service_caps);
                *size_caps = size_service_caps;
                return caps;
            }
        }
    }

    /* Control only reaches here if we haven't got valid caps to return */
    *size_caps = 0;
    return 0;
}
#endif

