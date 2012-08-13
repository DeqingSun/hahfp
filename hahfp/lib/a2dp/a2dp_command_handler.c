/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    a2dp_command_handler.c

DESCRIPTION

NOTES

*/



/****************************************************************************
    Header files
*/

#include "a2dp_command_handler.h"
#include "a2dp_data_block.h"
#include "a2dp_packet_handler.h"
#include "a2dp_process_command.h"
#include "a2dp_caps_parse.h"
#include "a2dp_api.h"
#include "a2dp_l2cap_handler.h"
#include "a2dp_codec_handler.h"

#include <print.h>
#include <stdlib.h>
#include <string.h>
#include <sink.h>

static uint8 getReceivedMessageType (remote_device *device)
{
    return *device->signal_conn.connection.active.received_packet & 0x03;
}


/*****************************************************************************/
static bool sendSetConfiguration(remote_device *device)
{
    uint8 *configured_service_caps = (uint8 *)PanicNull( blockGetBase( device->device_id, data_block_configured_service_caps ) );
    uint16 configured_service_caps_size = blockGetSize( device->device_id, data_block_configured_service_caps );

    return a2dpSendCommand(device, avdtp_set_configuration, configured_service_caps, configured_service_caps_size);
}

#if 0
/****************************************************************************/
static bool sendReconfigure(remote_device *device, const uint8* config, uint16 config_size)
{
    return a2dpSendCommand(device, avdtp_reconfigure, a2dp->sep.reconfigure_caps, a2dp->sep.reconfigure_caps_size);
}
#endif


/****************************************************************************/
void a2dpSetStreamState(remote_device *device, avdtp_stream_state state)
{
    PRINT(("a2dpSetStreamState: O:%d N:%d\n", device->signal_conn.status.stream_state, state));

    device->signal_conn.status.stream_state = state;

    /* Cancel the watchdog timeout*/
    (void) MessageCancelAll((Task)&a2dp->task, A2DP_INTERNAL_WATCHDOG_BASE + device->device_id);

    /* do we need to restart the watchdog? */
    switch (state)
    {
        case avdtp_stream_idle:
        case avdtp_stream_configured:
        case avdtp_stream_open:
        case avdtp_stream_streaming:
            /* These states are stable and therefore
               do not require the watchdog. */
            break;

        /*
           From Generic Audio/Video Distribution Profile, Table 4-1
           Some signals use TGAVDP100 and some don't.  For those
           that don't we apply our own timeout to prevent lock-up.
        */
        case avdtp_stream_configuring:
        case avdtp_stream_local_opening:
        case avdtp_stream_local_starting:
        case avdtp_stream_local_suspending:
        case avdtp_stream_local_closing:
        case avdtp_stream_remote_closing:
        case avdtp_stream_reconfiguring:
        case avdtp_stream_local_aborting:
        case avdtp_stream_remote_aborting:
            MessageSendLater((Task)&a2dp->task, A2DP_INTERNAL_WATCHDOG_BASE + device->device_id, 0, WATCHDOG_TGAVDP100);
            break;

        case avdtp_stream_discovering:
        case avdtp_stream_reading_caps:
        case avdtp_stream_processing_caps:
        case avdtp_stream_reconfig_reading_caps:
        case avdtp_stream_remote_opening:
            MessageSendLater((Task)&a2dp->task, A2DP_INTERNAL_WATCHDOG_BASE + device->device_id, 0, WATCHDOG_GENERAL);
            break;
    }
}


/****************************************************************************/
void a2dpHandleInternalWatchdogTimeout(MessageId msg_id)
{
    PanicFalse((uint16)(msg_id-A2DP_INTERNAL_WATCHDOG_BASE) < A2DP_MAX_REMOTE_DEVICES);

    {    
        remote_device *device = &a2dp->remote_conn[(uint16)(msg_id-A2DP_INTERNAL_WATCHDOG_BASE)];

        if ((device->signal_conn.status.stream_state == avdtp_stream_local_aborting) ||
            (device->signal_conn.status.stream_state == avdtp_stream_remote_aborting))
        {   /* The watchdog fired while we were aborting, kill. */
            a2dpDisconnectAllMedia(device);
            a2dpStreamReset(device);
        }
        else
        {   /* Abort */
            a2dpStreamAbort(device);
        }
    }
}


/*****************************/
/****  Inbound responses  ****/
/*****************************/


void a2dpHandleDiscoverResponse (remote_device *device)
{
    if (device->signal_conn.status.stream_state != avdtp_stream_discovering)
    {   /* Only expecting to receive this message in the approriate state - ignore it */
        return;
    }
    
    if ( (getReceivedMessageType(device) == avdtp_message_type_accept) && a2dpProcessDiscoverResponse(device) )
    {
        /* Process discover reponse fn will have generated a new list_discovered_remote_seids data block with a current index of zero. */
        /* This will extract the first remote seid from the list.                                                                      */
        device->remote_seid = *(uint8 *)PanicNull( blockGetCurrent( device->device_id, data_block_list_discovered_remote_seids ) );
        if ( a2dpSendCommand(device, avdtp_get_capabilities, NULL, 0) )
        {
            a2dpSetStreamState(device, avdtp_stream_reading_caps);
        }
        else
        {
            a2dpStreamAbort(device);
            a2dpMediaOpenCfm(device, NULL, a2dp_operation_fail);
        }
    }
    else
    {
        a2dpStreamReset(device);
        a2dpMediaOpenCfm(device, NULL, a2dp_operation_fail);
    }
}


void a2dpHandleGetCapabilitiesResponse (remote_device *device)
{
    const uint8 *caps_ptr = device->signal_conn.connection.active.received_packet + 2;
    uint16 caps_size = device->signal_conn.connection.active.received_packet_length - 2;
    uint8 error_cat, error_code;
    
    if (device->signal_conn.status.stream_state != avdtp_stream_reading_caps)
    {   /* Only expecting to receive this message in the approriate state - ignore it */
        return;
    }
    
    if ((getReceivedMessageType(device) == avdtp_message_type_accept) && a2dpValidateServiceCaps(caps_ptr, caps_size, FALSE, FALSE, &error_cat, &error_code))
    {
        a2dpSetStreamState(device, avdtp_stream_processing_caps);
                
        switch ( a2dpProcessGetCapabilitiesResponse(device) )
        {
        case CONFIGURATION_BY_CLIENT:       
            /* Issue request to client (app) to select an appropriate configuration */
            a2dpMediaCodecConfigureInd(device, &device->media_conn[0]);     /* TODO: Make multi-stream aware */
            break;
            
        case CONFIGURATION_SELECTED:        
            /* Library has managed to select a suitable configuation */
            if ( sendSetConfiguration(device) )
            {
                a2dpSetStreamState(device, avdtp_stream_configuring);
            }
            else
            {
                a2dpStreamAbort(device);
                a2dpMediaOpenCfm(device, NULL, a2dp_operation_fail);
            }
            break;
            
        case CONFIGURATION_NOT_SELECTED:    
            /* Library unable to determine a suitable configuration - need to try the next seid */
            if ( a2dpSelectNextConfigurationSeid(device) )
            {   /* Next seid selected */
                device->remote_seid = *(uint8 *)PanicNull( blockGetCurrent( device->device_id, data_block_list_discovered_remote_seids ) );
                if ( a2dpSendCommand(device, avdtp_get_capabilities, NULL, 0) )
                {
                    a2dpSetStreamState(device, avdtp_stream_reading_caps);
                }
                else
                {
                    a2dpStreamAbort(device);
                    a2dpMediaOpenCfm(device, NULL, a2dp_operation_fail);
                }
            }
            else
            {   /* All known seids exhausted - unable to select a suitable configuration */
                a2dpStreamReset(device);
                a2dpMediaOpenCfm(device, NULL, a2dp_operation_fail);
            }
            break;
        }
    }
    else
    {
        a2dpStreamReset(device);
        a2dpMediaOpenCfm(device, NULL, a2dp_operation_fail);
    }
}

void a2dpHandleSetConfigurationResponse (remote_device *device)
{
    if (device->signal_conn.status.stream_state != avdtp_stream_configuring)
    {   /* Only expecting to receive this message in the approriate state - ignore it */
        return;
    }
    
    if (getReceivedMessageType(device) == avdtp_message_type_accept)
    {
        /* Mark the current SEP as in use */
        ((sep_data_type *)PanicNull( blockGetCurrent( device->device_id, data_block_sep_list ) ))->in_use = TRUE;
        a2dpSetStreamState(device, avdtp_stream_configured);
        
        if ( a2dpSendCommand(device, avdtp_open, NULL, 0) )
        {
            a2dpSetStreamState(device, avdtp_stream_local_opening);
        }
        else
        {
            a2dpStreamAbort(device);
            a2dpMediaOpenCfm(device, NULL, a2dp_operation_fail);
        }
    }
    else
    {
        a2dpStreamReset(device);
        a2dpMediaOpenCfm(device, NULL, a2dp_operation_fail);
    }
}

void a2dpHandleOpenResponse (remote_device *device)
{
    if (device->signal_conn.status.stream_state != avdtp_stream_local_opening)
    {   /* Only expecting to receive this message in the approriate state - ignore it */
        return;
    }
    
    if (getReceivedMessageType(device) == avdtp_message_type_accept)
    {
        a2dpL2capMediaConnectReq(device);   /* TODO: Add support for different flush timeout values */
    }
    else
    {
        a2dpStreamAbort(device);
        a2dpMediaOpenCfm(device, NULL, a2dp_operation_fail);
    }
}

void a2dpHandleStartResponse (remote_device *device)
{
    if ( (device->signal_conn.status.stream_state != avdtp_stream_open) &&
         (device->signal_conn.status.stream_state != avdtp_stream_local_starting) &&
         (device->signal_conn.status.stream_state != avdtp_stream_streaming) )
    {   /* Only expecting to receive this message in the approriate state - inform app */
        a2dpMediaStartCfm(device, &device->media_conn[0], a2dp_wrong_state);
        return;
    }
    
    /* If we are already streaming, don't worry about any error response from the remote device */
    if ((getReceivedMessageType(device) == avdtp_message_type_accept) || (device->signal_conn.status.stream_state == avdtp_stream_streaming))
    {
        /* end point is now streaming */
        a2dpSetStreamState(device, avdtp_stream_streaming);
        /* tell application the good news! */
        a2dpMediaStartCfm(device, &device->media_conn[0], a2dp_success);
    }
    else
    {
        a2dpSetStreamState(device, avdtp_stream_open);
        a2dpMediaStartCfm(device, &device->media_conn[0], a2dp_rejected_by_remote_device);
    }
}

void a2dpHandleSuspendResponse (remote_device *device)
{
    if ( (device->signal_conn.status.stream_state != avdtp_stream_local_suspending) &&
         (device->signal_conn.status.stream_state != avdtp_stream_open) )
    {   /* Only expecting to receive this message in the approriate state - inform app */
        a2dpMediaSuspendCfm(device, &device->media_conn[0], a2dp_wrong_state);
        return;
    }
    
    /* If we have already suspended, don't worry about any error response from the remote device */
    if ((getReceivedMessageType(device) == avdtp_message_type_accept) || (device->signal_conn.status.stream_state != avdtp_stream_open))
    {
        /* end point has now suspended */
        a2dpSetStreamState(device, avdtp_stream_open);
        /* tell application the good news! */
        a2dpMediaSuspendCfm(device, &device->media_conn[0], a2dp_success);
    }
    else
    {
        a2dpSetStreamState(device, avdtp_stream_streaming);
        a2dpMediaSuspendCfm(device, &device->media_conn[0], a2dp_rejected_by_remote_device);
    }
}

void a2dpHandleCloseResponse (remote_device *device)
{
    signalling_channel *signalling = &device->signal_conn;
    
    if ((signalling->status.stream_state != avdtp_stream_open) &&
        (signalling->status.stream_state != avdtp_stream_local_starting) &&
        (signalling->status.stream_state != avdtp_stream_local_suspending) &&
        (signalling->status.stream_state != avdtp_stream_streaming) &&
        (signalling->status.stream_state != avdtp_stream_reconfig_reading_caps) &&
        (signalling->status.stream_state != avdtp_stream_reconfiguring) &&
        (signalling->status.stream_state != avdtp_stream_remote_closing) &&
        (signalling->status.stream_state != avdtp_stream_local_closing))
    {   /* Only expecting to receive this message in the approriate state - inform app */
        a2dpMediaCloseInd(device, &device->media_conn[0], a2dp_wrong_state);
        return;
    }
    
    if (getReceivedMessageType(device) == avdtp_message_type_accept)
    {
        /* Mark the current SEP as no longer being in use */
        ((sep_data_type *)PanicNull( blockGetCurrent( device->device_id, data_block_sep_list ) ))->in_use = FALSE;
        
        /* Reset the local and remote SEIDs */
        blockSetCurrent( device->device_id, data_block_sep_list, DATA_BLOCK_INDEX_INVALID );
        device->remote_seid = 0;
        
        /* Remove stream specific data */
        blockRemove( device->device_id, data_block_configured_service_caps );
        blockRemove( device->device_id, data_block_list_discovered_remote_seids );
        
        a2dpSetStreamState(device, avdtp_stream_local_closing);
        a2dpL2capMediaDisconnectReq(device->media_conn[0].connection.active.sink); /* TODO: Make multi-stream aware */
    }
    else
    {   /* Don't take no for an answer - if our request is rejected then abort to force a close */
        a2dpStreamAbort(device);
    }
}

void a2dpHandleAbortResponse (remote_device *device)
{
    /* If we get a response then it means the remote device has accepted our abort request so, reset the stream state */
    a2dpDisconnectAllMedia(device);
    a2dpStreamReset(device);
}


/*****************************/
/****  Inbound commands  ****/
/*****************************/

void a2dpHandleDiscoverCommand (remote_device *device)
{
    uint16 error_code;
    uint16 payload_size;
    
    if ( (error_code = a2dpProcessDiscoverCommand(device, &payload_size))!=(uint16)avdtp_ok )
    {
        a2dpSendReject(device, avdtp_discover, error_code);
    }
    else
    {
        a2dpSendAccept(device, avdtp_discover, payload_size);
    }
}

void a2dpHandleGetCapabilitiesCommand (remote_device *device)
{
    uint16 error_code;
    uint16 payload_size;
    
    if ( (error_code = a2dpProcessGetCapabilitiesCommand(device, &payload_size))!=(uint16)avdtp_ok )
    {
        a2dpSendReject(device, avdtp_get_capabilities, error_code);
    }
    else
    {
        a2dpSendAccept(device, avdtp_get_capabilities, payload_size);
    }
}

void a2dpHandleSetConfigurationCommand (remote_device *device)
{
    uint16 error_code;
    uint16 payload_size;
    signalling_channel *signalling = &device->signal_conn;
    
    if (signalling->status.stream_state != avdtp_stream_idle)
    {
        a2dpSendReject(device, avdtp_set_configuration, avdtp_bad_state);
    }
    else if ( (error_code = a2dpProcessSetConfigurationCommand(device, &payload_size))!=(uint16)avdtp_ok )
    {
        a2dpSendReject(device, avdtp_set_configuration, error_code);
    }
    else
    {
        a2dpSetStreamState(device, avdtp_stream_configured);
        a2dpSendAccept(device, avdtp_set_configuration, payload_size);
    }
}

void a2dpHandleGetConfigurationCommand (remote_device *device)
{
    uint16 error_code;
    uint16 payload_size;
    
    if ( (error_code = a2dpProcessGetConfigurationCommand(device, &payload_size))!=(uint16)avdtp_ok )
    {
        a2dpSendReject(device, avdtp_get_configuration, error_code);
    }
    else
    {
        a2dpSendAccept(device, avdtp_get_configuration, payload_size);
    }
}

void a2dpHandleReconfigureCommand (remote_device *device)
{
    uint16 error_code;
    uint16 payload_size;
    signalling_channel *signalling = &device->signal_conn;
    
    if (signalling->status.stream_state != avdtp_stream_open)
    {
        a2dpSendReject(device, avdtp_reconfigure, avdtp_sep_not_in_use);
    }
    else if ( (error_code = a2dpProcessReconfigureCommand(device, &payload_size))!=(uint16)avdtp_ok )
    {
        a2dpSendReject(device, avdtp_reconfigure, error_code);
    }
    else
    {
        a2dpSendAccept(device, avdtp_reconfigure, payload_size);
#if 0
        {   /* TODO: */
            /* Choose the codec params that the app needs to know about and send it this information */
            MAKE_A2DP_MESSAGE(A2DP_INTERNAL_SEND_CODEC_PARAMS_REQ);
            message->send_reconfigure_message = 0;
            MessageSend(&a2dp->task, A2DP_INTERNAL_SEND_CODEC_PARAMS_REQ, message);
        }
#endif
    }
}

void a2dpHandleOpenCommand (remote_device *device)
{
    uint16 error_code;
    signalling_channel *signalling = &device->signal_conn;
    
    if (signalling->status.stream_state != avdtp_stream_configured)
    {
        a2dpSendReject(device, avdtp_open, avdtp_bad_state);
    }
    else if ( (error_code = a2dpProcessOpenCommand(device))!=(uint16)avdtp_ok )
    {
        a2dpSendReject(device, avdtp_open, error_code);
    }
    else
    {
        a2dpSetStreamState(device, avdtp_stream_remote_opening);
        a2dpMediaOpenInd(device);
    }
}

void a2dpHandleCloseCommand (remote_device *device)
{
    uint16 error_code;
    
    if ( (device->signal_conn.status.stream_state != avdtp_stream_open) &&
         (device->signal_conn.status.stream_state != avdtp_stream_local_starting) &&
         (device->signal_conn.status.stream_state != avdtp_stream_local_suspending) &&
         (device->signal_conn.status.stream_state != avdtp_stream_reconfig_reading_caps) &&
         (device->signal_conn.status.stream_state != avdtp_stream_reconfiguring) &&
         (device->signal_conn.status.stream_state != avdtp_stream_streaming) )
    {
        a2dpSendReject(device, avdtp_close, avdtp_bad_state);
    }
    else if ( (error_code = a2dpProcessCloseCommand(device))!=(uint16)avdtp_ok )
    {
        a2dpSendReject(device, avdtp_close, error_code);
    }
    else
    {
        a2dpSetStreamState(device, avdtp_stream_remote_closing);
        a2dpSendAccept(device, avdtp_close, AVDTP_NO_PAYLOAD);
        /* App is informed on l2cap media channel close */
    }
}

bool a2dpHandleStartCommand (remote_device *device)
{
    uint16 error_code;

    if ( (device->signal_conn.status.stream_state == avdtp_stream_remote_opening) ||
         (device->signal_conn.status.stream_state == avdtp_stream_local_opening) )
    {   /* Return FALSE to indicate that processing of AVDTP_START command will be deferred until later */
        return FALSE;
    }
    else if ( (device->signal_conn.status.stream_state != avdtp_stream_open) &&
              (device->signal_conn.status.stream_state != avdtp_stream_local_starting) )
    {
        a2dpSendReject(device, avdtp_start, avdtp_bad_state);
    }
    else if ( (error_code = a2dpProcessStartCommand(device))!=(uint16)avdtp_ok )
    {
        a2dpSendReject(device, avdtp_start, error_code);
    }
    else
    {
        a2dpSetStreamState(device, avdtp_stream_streaming);
        a2dpSendAccept(device, avdtp_start, AVDTP_NO_PAYLOAD);
        a2dpMediaStartInd(device, &device->media_conn[0]);
    }

    return TRUE;
}

void a2dpHandleSuspendCommand (remote_device *device)
{
    uint16 error_code;
    
    if ( (device->signal_conn.status.stream_state != avdtp_stream_streaming) &&
         (device->signal_conn.status.stream_state != avdtp_stream_local_suspending) )
    {
        a2dpSendReject(device, avdtp_suspend, avdtp_bad_state);
    }
    else if ( (error_code = a2dpProcessSuspendCommand(device))!=(uint16)avdtp_ok )
    {
        a2dpSendReject(device, avdtp_suspend, error_code);
    }
    else
    {
        a2dpSetStreamState(device, avdtp_stream_open);
        a2dpSendAccept(device, avdtp_suspend, AVDTP_NO_PAYLOAD);
        a2dpMediaSuspendInd(device, &device->media_conn[0]);
    }
}

void a2dpHandleAbortCommand (remote_device *device)
{
    if ( a2dpProcessAbortCommand(device) )
    {   /* If the supplied seid is recognised, we must abort */
        a2dpSendAccept(device, avdtp_abort, AVDTP_NO_PAYLOAD);
        a2dpDisconnectAllMedia(device);
        a2dpStreamReset(device);
    }
}


/*****************************/
/****  Outbound commands  ****/
/*****************************/

bool a2dpStreamClose (remote_device *device)
{
    signalling_channel *signalling = &device->signal_conn;
    media_channel *media = &device->media_conn[0];      /* TODO; Make multi-stream aware */
    
    if (signalling->status.connection_state != avdtp_connection_connected)
    {
        return FALSE;
    }
    else if (media->status.connection_state != avdtp_connection_connected)
    {
        return FALSE;
    }
    else if ((signalling->status.stream_state != avdtp_stream_open) &&
             (signalling->status.stream_state != avdtp_stream_streaming) &&
             (signalling->status.stream_state != avdtp_stream_local_starting) &&
             (signalling->status.stream_state != avdtp_stream_local_suspending))
    {
        return FALSE;
    }
    else
    {
        if ( a2dpSendCommand(device, avdtp_close, NULL, 0) )    /* TODO: Support multiple streams */
        {
            a2dpSetStreamState(device, avdtp_stream_local_closing);
        }
        else
        {
            a2dpStreamAbort(device);
        }
        
        return TRUE;
    }
}


void a2dpStreamEstablish (const A2DP_INTERNAL_MEDIA_OPEN_REQ_T *req)
{
    signalling_channel *signalling = &((remote_device *)PanicNull(req->device))->signal_conn;
    
    if ( signalling->status.connection_state != avdtp_connection_connected )
    {
        a2dpMediaOpenCfm(req->device, NULL, a2dp_no_signalling_connection);
    }
    else if (signalling->status.stream_state != avdtp_stream_idle)
    {
        a2dpMediaOpenCfm(req->device, NULL, a2dp_wrong_state);
    }
    else
    {   /* Signalling channel open - start discovery */
        if( a2dpSendCommand(req->device, avdtp_discover, NULL, 0) )
        {
            a2dpSetStreamState(req->device, avdtp_stream_discovering);
        }
        else
        {
            a2dpStreamAbort(req->device);
            a2dpMediaOpenCfm(req->device, NULL, a2dp_operation_fail);
        }
    }
}


void a2dpStreamOpenResponse (const A2DP_INTERNAL_MEDIA_OPEN_RES_T *res)
{
    signalling_channel *signalling = &((remote_device *)PanicNull(res->device))->signal_conn;
    
    if ( signalling->status.connection_state != avdtp_connection_connected )
    {
        a2dpMediaOpenCfm(res->device, NULL, a2dp_no_signalling_connection);
    }
    else if (signalling->status.stream_state != avdtp_stream_remote_opening)
    {
        a2dpMediaOpenCfm(res->device, NULL, a2dp_wrong_state);
    }
    else
    {
        if ( res->accept )
        {
            a2dpSendAccept(res->device, avdtp_open, AVDTP_NO_PAYLOAD);
            /* App is informed on L2cap media connect */
        }
        else
        {
            a2dpSendReject(res->device, avdtp_open, avdtp_bad_state);
            a2dpSetStreamState(res->device, avdtp_stream_idle);
            a2dpStreamReset(res->device);
        }
    }
}


void a2dpStreamStart (const A2DP_INTERNAL_MEDIA_START_REQ_T *req)
{
    signalling_channel *signalling = &((remote_device *)PanicNull(req->device))->signal_conn;
    media_channel *media= (media_channel *)PanicNull(req->media);
    
    if (signalling->status.connection_state != avdtp_connection_connected)
    {
        a2dpMediaStartCfm(req->device, media, a2dp_no_signalling_connection);
    }
    else if (media->status.connection_state != avdtp_connection_connected)
    {
        a2dpMediaStartCfm(req->device, media, a2dp_no_media_connection);
    }
    else if (signalling->status.stream_state != avdtp_stream_open)
    {
        a2dpMediaStartCfm(req->device, media, a2dp_wrong_state);
    }
    else
    {
        if( a2dpSendCommand(req->device, avdtp_start, NULL, 0) ) /* TODO: Support multiple streams */
        {
            a2dpSetStreamState(req->device, avdtp_stream_local_starting);
        }
        else
        {
            a2dpStreamAbort(req->device);
            a2dpMediaStartCfm(req->device, media, a2dp_operation_fail);
        }
    }
}

void a2dpStreamRelease (const A2DP_INTERNAL_MEDIA_CLOSE_REQ_T *req)
{
    signalling_channel *signalling = &((remote_device *)PanicNull(req->device))->signal_conn;
    media_channel *media = (media_channel *)PanicNull(req->media);
    
    if (signalling->status.connection_state != avdtp_connection_connected)
    {
        a2dpMediaCloseInd(req->device, media, a2dp_no_signalling_connection);
    }
    else if (media->status.connection_state != avdtp_connection_connected)
    {
        a2dpMediaCloseInd(req->device, media, a2dp_no_media_connection);
    }
    else if ( !((signalling->status.stream_state == avdtp_stream_open) ||
                (signalling->status.stream_state == avdtp_stream_streaming) ||
                (signalling->status.stream_state == avdtp_stream_local_starting) ||
                (signalling->status.stream_state == avdtp_stream_local_suspending) ||
                (signalling->status.stream_state == avdtp_stream_reconfig_reading_caps) ||
                (signalling->status.stream_state == avdtp_stream_reconfiguring))
            )
    {
        a2dpMediaCloseInd(req->device, media, a2dp_wrong_state);
    }
    else
    {
        if ( a2dpSendCommand(req->device, avdtp_close, NULL, 0) )    /* TODO: Support multiple streams */
        {
            a2dpSetStreamState(req->device, avdtp_stream_local_closing);
        }
        else
        {
            a2dpStreamAbort(req->device);
            a2dpMediaCloseInd(req->device, media, a2dp_operation_fail);
        }
    }
}

void a2dpStreamSuspend (const A2DP_INTERNAL_MEDIA_SUSPEND_REQ_T *req)
{
    signalling_channel *signalling = &((remote_device *)PanicNull(req->device))->signal_conn;
    media_channel *media= (media_channel *)PanicNull(req->media);
    
    if (signalling->status.connection_state != avdtp_connection_connected)
    {
        a2dpMediaSuspendCfm(req->device, media, a2dp_no_signalling_connection);
    }
    else if (media->status.connection_state != avdtp_connection_connected)
    {
        a2dpMediaSuspendCfm(req->device, media, a2dp_no_media_connection);
    }
    else if (signalling->status.stream_state != avdtp_stream_streaming)
    {
        a2dpMediaSuspendCfm(req->device, media, a2dp_wrong_state);
    }
    else
    {
        if ( a2dpSendCommand(req->device, avdtp_suspend, NULL, 0) )  /* TODO: Support multiple streams */
        {
            a2dpSetStreamState(req->device, avdtp_stream_local_suspending);
        }
        else
        {
            a2dpStreamAbort(req->device);
            a2dpMediaSuspendCfm(req->device, media, a2dp_operation_fail);
        }
    }
}

void a2dpStreamAbort (remote_device *device)
{
    if ( (device->signal_conn.status.stream_state == avdtp_stream_local_aborting) ||
         (device->signal_conn.status.stream_state == avdtp_stream_remote_aborting) )
    {   /* Already aborting */
        return;
    }

    if (device->signal_conn.status.connection_state == avdtp_connection_connected)
    {
        if ( a2dpSendCommand(device, avdtp_abort, NULL, 0) )
        {
            a2dpSetStreamState(device, avdtp_stream_local_aborting);
        }
        else
        {   /* Unable to issue Abort command, so just reset */
            a2dpDisconnectAllMedia(device);
            a2dpStreamReset(device);
        }
    }
}


void a2dpStreamReset (remote_device *device)
{
    sep_data_type *current_sep;
    
    /* Mark current SEP as not in use anymore */
    if ( (current_sep = (sep_data_type *)blockGetCurrent( device->device_id, data_block_sep_list ))!=NULL )
    {
        current_sep->in_use = FALSE;
    }
        
    /* Reset the local and remote SEIDs */
    blockSetCurrent( device->device_id, data_block_sep_list, DATA_BLOCK_INDEX_INVALID );
    device->remote_seid = 0;

    /* Remove stream specific data */
    blockRemove( device->device_id, data_block_configured_service_caps );
    blockRemove( device->device_id, data_block_list_discovered_remote_seids );

    /* Cancel the watchdog timeout */
    (void) MessageCancelAll((Task)&a2dp->task, A2DP_INTERNAL_WATCHDOG_BASE + device->device_id);

    /* Clear transaction logs */
    device->signal_conn.status.pending_issued_transaction = FALSE;
    device->signal_conn.status.pending_received_transaction = FALSE;
    device->signal_conn.connection.active.issued_transaction_label = 0;
    device->signal_conn.connection.active.received_transaction_label = 0;

    a2dpSetStreamState(device, avdtp_stream_idle);
}


#if 0
bool a2dpChangeParameters (remote_device *device)
{
    return FALSE;
}
#endif

#if 0
bool a2dpSecurityControl (remote_device *device)
{
    return FALSE;
}
#endif

