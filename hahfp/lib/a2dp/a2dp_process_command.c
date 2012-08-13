/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    a2dp_process_command.c

DESCRIPTION

NOTES

*/



/****************************************************************************
    Header files
*/

#include "a2dp_process_command.h"
#include "a2dp_command_handler.h"
#include "a2dp_data_block.h"
#include "a2dp_packet_handler.h"
#include "a2dp_caps_parse.h"
#include "a2dp_api.h"
#include "a2dp_l2cap_handler.h"
#include "a2dp_codec_handler.h"

#include <print.h>
#include <stdlib.h>
#include <string.h>
#include <sink.h>

static uint8 getLocalSeid (remote_device *device)
{
    sep_data_type *current_sep = (sep_data_type *)blockGetCurrent( device->device_id, data_block_sep_list );
    
    if (current_sep)
    {
        return current_sep->sep_config->seid;
    }

    return 0;
}

static uint8 getSepIndexBySeid (remote_device *device, uint16 seid)
{
    uint8 index;
    uint8 max_index = blockGetSize( device->device_id, data_block_sep_list ) / sizeof( sep_data_type );
    sep_data_type *pSeps = (sep_data_type *)PanicNull( blockGetBase( device->device_id, data_block_sep_list ) );

    for ( index=0; index<max_index; index++ )
    {
        if (pSeps->sep_config->seid == seid)
        {
            return index;
        }
        
        pSeps++;
    }

    return DATA_BLOCK_INDEX_INVALID;
}


/****************************************************************************
NAME
    processDiscover

DESCRIPTION
    Process an incoming discover request.

RETURNS
    void
*/
uint16 a2dpProcessDiscoverCommand (remote_device *device, uint16 *payload_size)
{
    Sink sink = device->signal_conn.connection.active.sink;
    sep_data_type *sep_ptr = (sep_data_type *)PanicNull( blockGetBase( device->device_id, data_block_sep_list ) );
    uint8 sep_cnt = blockGetSize( device->device_id, data_block_sep_list ) / sizeof(sep_data_type);
    uint8 *payload;
    
    *payload_size = 0;
    if ((payload = a2dpGrabSink(sink, 2*sep_cnt)) == NULL)
    {
        return avdtp_bad_state;
    }
    
    while (sep_cnt--)
    {
        const sep_config_type *sep_config = sep_ptr->sep_config;

        if (sep_ptr->in_use)
        {
            *payload++ = (sep_config->seid << 2) | 0x02;
        }
        else
        {
            *payload++ = sep_config->seid << 2;
        }
        
        *payload++ = (uint8) ((sep_config->media_type << 4) | ((sep_config->role) << 3));
        
        *payload_size += 2;
        sep_ptr++;
    }

    return avdtp_ok;
}


/****************************************************************************
NAME
    processGetCapabilitiesCommand

DESCRIPTION
    Process an incoming Get_Capabilities request

RETURNS
    void
*/
uint16 a2dpProcessGetCapabilitiesCommand (remote_device *device, uint16 *payload_size)
{
    const uint8 *ptr = device->signal_conn.connection.active.received_packet;
    uint8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);
    Sink sink = device->signal_conn.connection.active.sink;
    const sep_config_type *sep_config = ((sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) ))->sep_config;
    uint8 *payload;

    *payload_size = 0;
    if ( sep_index==DATA_BLOCK_INDEX_INVALID )
    {
        return avdtp_bad_acp_seid;
    }
    
    if ((payload = a2dpGrabSink(sink, sep_config->size_caps)) == NULL)
    {
        return avdtp_bad_state;
    }

    *payload_size = sep_config->size_caps;
    memmove(payload, sep_config->caps, sep_config->size_caps);
    return avdtp_ok;
}


/****************************************************************************
NAME
    processSetConfigurationCommand

DESCRIPTION
    Process an incoming Set_Configuration request

RETURNS
    void
*/
uint16 a2dpProcessSetConfigurationCommand (remote_device *device, uint16 *payload_size)
{
    const uint8 *ptr = device->signal_conn.connection.active.received_packet;
    uint8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);
    uint16 packet_size = device->signal_conn.connection.active.received_packet_length;
    uint8 unsupported_service;
    uint8 error_cat, error_code;

    *payload_size = 0;
    if (sep_index == DATA_BLOCK_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else 
    {
        sep_data_type *sep_ptr = (sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) );
        const sep_config_type *sep_config = sep_ptr->sep_config;
        
        if (sep_ptr->in_use)
        {   /* SEP is already in use - reject (service capabilities were not the problem)*/
            return avdtp_sep_in_use;
        }
        else if (!a2dpValidateServiceCaps(&ptr[4], packet_size-4, FALSE, FALSE, &error_cat, &error_code))
        {   /* bad caps - reject */
            return (error_cat < 8) | error_code;
        }
        else if (!a2dpAreServicesCategoriesCompatible(sep_config->caps, sep_config->size_caps, &ptr[4], packet_size-4, &unsupported_service))
        {   /* Check that configuration only asks for services the local SEP supports set config does not match our caps - reject */
            return (unsupported_service <<8) | avdtp_unsupported_configuration;
        }
        else if (a2dpFindMatchingCodecSpecificInformation(sep_config->caps, &ptr[4], 0) == NULL)
        {   /*  Check the codec specific attributes are compatible set config does not match our caps - reject */
            return (AVDTP_SERVICE_MEDIA_CODEC << 8) | avdtp_unsupported_configuration;
        }
        else
        {
            /* Set the index to the current SEID data */
            blockSetCurrent( device->device_id, data_block_sep_list, sep_index );
    
            /* Mark this SEP as in use */
            sep_ptr->in_use = TRUE;
    
            /* Store remote SEID */
            device->remote_seid = (ptr[3] >> 2) & 0x3f;
            
            /* Set current index for list of discovered remote seids */
            /* TODO */
    
            /* Free any old configuration */
            blockRemove( device->device_id, data_block_configured_service_caps );
            
            /* Store configuration */
            PanicFalse( blockAdd( device->device_id, data_block_configured_service_caps, packet_size-4, sizeof(uint8) ) );
            memmove( PanicNull( blockGetBase( device->device_id, data_block_configured_service_caps ) ), &ptr[4], packet_size-4 );
            
            return avdtp_ok;
        }
    }    
}


/****************************************************************************
NAME
    processGetConfigurationCommand

DESCRIPTION
    Process an incoming Get_Capabilities request.

RETURNS
    void
*/
uint16 a2dpProcessGetConfigurationCommand (remote_device *device, uint16 *payload_size)
{
    const uint8 *ptr = device->signal_conn.connection.active.received_packet;
    uint8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);
    Sink sink = device->signal_conn.connection.active.sink;
    
    *payload_size = 0;
    if (sep_index == DATA_BLOCK_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else if (getLocalSeid(device) != ((ptr[2]>>2) & 0x3f))
    {
        return avdtp_bad_state;
    }
    else
    {
        uint8 *payload;
                
        *payload_size = blockGetSize( device->device_id, data_block_configured_service_caps );
        /* build header with transaction label from request. */
        if ((payload = a2dpGrabSink(sink, *payload_size)) == NULL)
        {
            return avdtp_bad_state;
        }

        /* copy in the agreed configuration for this SEP.
           Note that this only returns the last SET_CONFIG or RECONFIGURE
           parameters and not the global configuration - is this ok? */
        memmove( payload, PanicNull( blockGetBase( device->device_id, data_block_configured_service_caps ) ), *payload_size );
        return avdtp_ok;
    }
}


/****************************************************************************
NAME
    processReconfigure

DESCRIPTION
    Process an incoming reconfigure request.

RETURNS
    void
*/
uint16 a2dpProcessReconfigureCommand (remote_device *device, uint16 *payload_size)
{
    const uint8 *reconfig_caps = device->signal_conn.connection.active.received_packet + 3;
    uint16 reconfig_caps_size = device->signal_conn.connection.active.received_packet_length - 3;
    uint8 error_cat, error_code;
    uint8 unsupported_service;
    uint8 sep_index = getSepIndexBySeid(device, (device->signal_conn.connection.active.received_packet[2]>>2) & 0x3f);
    const sep_config_type *sep_config = ((sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) ))->sep_config;

    *payload_size = 0;
    /*
        AVDTP test TP/SIG/SMG/BI-14-C is very pedantic about the order in which
        we should return errors, even though the spec does not state a validation
        procedure.  This code is therefore more verbose than it needs to be
        in order to generate the correct errors in the correct order.
    */
    if (!a2dpValidateServiceCaps(reconfig_caps, reconfig_caps_size, FALSE, TRUE, &error_cat, &error_code))
    {   /* bad caps - reject */
        return (error_cat << 8) | error_code;
    }
    else if (getLocalSeid(device) != ((device->signal_conn.connection.active.received_packet[2]>>2) & 0x3f))
    {   /* SEP is already in use - reject */
        return avdtp_sep_not_in_use;
    }
    /* check that the service caps are valid for reconfigure */
    else if (!a2dpValidateServiceCaps(reconfig_caps, reconfig_caps_size, TRUE, FALSE, &error_cat, &error_code))
    {   /*  bad caps - reject */
        return (error_cat << 8) | error_code;
    }
    else if (!a2dpAreServicesCategoriesCompatible(sep_config->caps, sep_config->size_caps, reconfig_caps, reconfig_caps_size, &unsupported_service))
    {   /* set config does not match our caps - reject */
        return (unsupported_service << 8) | avdtp_unsupported_configuration;
    }
    else if (a2dpFindMatchingCodecSpecificInformation(sep_config->caps, reconfig_caps, 0) == NULL)
    {   /* requested codec is not compatible with our caps */
        return (AVDTP_SERVICE_MEDIA_CODEC << 8) | avdtp_unsupported_configuration;
    }
    else
    {
        /* Free any old configuration */
        blockRemove( device->device_id, data_block_configured_service_caps );

        /* Store configuration */
        PanicFalse( blockAdd( device->device_id, data_block_configured_service_caps, reconfig_caps_size, sizeof(uint8) ) );
        memmove( PanicNull( blockGetBase( device->device_id, data_block_configured_service_caps ) ), reconfig_caps, reconfig_caps_size );
        return avdtp_ok;
    }
}


/****************************************************************************
NAME
    processOpen

DESCRIPTION
    Process an incoming Open request.

RETURNS
    void
*/
uint16 a2dpProcessOpenCommand (remote_device *device)
{
    const uint8 *ptr = device->signal_conn.connection.active.received_packet;
    uint8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);

    if (sep_index == DATA_BLOCK_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else if (getLocalSeid(device) != ((ptr[2]>>2) & 0x3f))
    {
        return avdtp_bad_state;
    }
    else
    {
        return avdtp_ok;
    }
}


/****************************************************************************
NAME
    processStart

DESCRIPTION
    Process a Start request.

RETURNS
    void
*/
uint16 a2dpProcessStartCommand (remote_device *device)
{
    const uint8 *ptr = device->signal_conn.connection.active.received_packet + 2;
    uint16 seids = device->signal_conn.connection.active.received_packet_length - 2;

    while (seids--)
    {
        uint8 seid = (*ptr++ >> 2) & 0x3f;
        uint8 sep_index = getSepIndexBySeid(device, seid);

        if (sep_index == DATA_BLOCK_INDEX_INVALID)
        {
            return (seid << 10) | avdtp_bad_acp_seid;
        }
        else if (getLocalSeid(device) != seid)
        {
            return (seid << 10) | avdtp_bad_state;
        }
    }

    /* All SEIDs valid, accept */
    return avdtp_ok;
}


/****************************************************************************
NAME
    processClose

DESCRIPTION
    Process a Close request.

RETURNS
    void
*/
uint16 a2dpProcessCloseCommand(remote_device *device)
{
    const uint8 *ptr = device->signal_conn.connection.active.received_packet;
    uint8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);
    
    if (sep_index == DATA_BLOCK_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else if (getLocalSeid(device) != ((ptr[2]>>2) & 0x3f))
    {
        return avdtp_bad_state;
    }
    else
    {
        /* Mark the SEP as no longer being in use */
        ((sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) ))->in_use = FALSE;
        
        /* Reset local and remote SEIDs */
        blockSetCurrent( device->device_id, data_block_sep_list, DATA_BLOCK_INDEX_INVALID );
        device->remote_seid = 0;
        
        /* Remove stream specific data */
        blockRemove( device->device_id, data_block_configured_service_caps );
        blockRemove( device->device_id, data_block_list_discovered_remote_seids );
        
        return avdtp_ok;
    }
}


/****************************************************************************
NAME
    processSuspend

DESCRIPTION
    Process a suspend request.

RETURNS
    void
*/
uint16 a2dpProcessSuspendCommand (remote_device *device)
{
    const uint8 *ptr = device->signal_conn.connection.active.received_packet + 2;
    uint16 seids = device->signal_conn.connection.active.received_packet_length - 2;
    
    while (seids--)
    {
        uint8 seid = (*ptr++ >> 2) & 0x3f;
        uint8 sep_index = getSepIndexBySeid(device, seid);

        if (sep_index == DATA_BLOCK_INDEX_INVALID)
        {
            return (seid << 10) | avdtp_bad_acp_seid;
        }
        else if (getLocalSeid(device) != seid)
        {
            return (seid << 10) | avdtp_bad_state;
        }
    }

    /* all SEIDs valid, accept */
    return avdtp_ok;
}


/****************************************************************************
NAME
    processAbort

DESCRIPTION
    Process an Abort request.

RETURNS
    void
*/
bool a2dpProcessAbortCommand(remote_device *device)
{
    const uint8* ptr = device->signal_conn.connection.active.received_packet;
    sep_data_type *current_sep;

    if ( (current_sep = (sep_data_type *)blockGetCurrent( device->device_id, data_block_sep_list )) != NULL )
    {
        if (((ptr[2]>>2) & 0x3f) == current_sep->sep_config->seid)
        {
            return TRUE;
        }
    }
    
    return FALSE;
}


bool a2dpProcessDiscoverResponse(remote_device *device)
{
    const sep_config_type *sep_config = ((sep_data_type *)PanicNull( blockGetCurrent( device->device_id, data_block_sep_list ) ))->sep_config;
    const uint8* ptr = (const uint8 *)PanicNull(device->signal_conn.connection.active.received_packet);
    uint16 packet_size = device->signal_conn.connection.active.received_packet_length;
    uint8 sep_style;
    uint8 discovered_remote_seids;
    uint16 store_seps;
    uint16 sep_mask;
    uint16 i;
    uint8 *remote_seids;

    /* clear record of discovered SEPs */
    blockRemove( device->device_id, data_block_list_discovered_remote_seids );

    ptr += 2;

    sep_style = (uint8)sep_config->media_type << 4;
    sep_style |= ((uint8)sep_config->role ^ 0x01) << 3;  /* Note: Making an assumption about an enumerated type */
    discovered_remote_seids = 0;
    store_seps = 0;
    sep_mask = 0x0001;
    for (i=0; i<(packet_size-2) && sep_mask; i++)  /* Note: This algorithm will limit discovery to the first 16 remote seids */
    {
        /* Check that the SEP is not in use */
        if ( (ptr[i++] & 2)==0 )
        {
            /* Check that the media type matches and also that the SEP's role is the opposite */
            if ( sep_style == (ptr[i] & 0xF8) )
            {
                discovered_remote_seids++;
                store_seps |= sep_mask;    /* Record that this is a seid we are interested in */
            }
            sep_mask <<= 1;
        }
    }
    
    if (discovered_remote_seids)
    {
        remote_seids = (uint8 *)PanicNull( blockAdd( device->device_id, data_block_list_discovered_remote_seids, discovered_remote_seids, sizeof(uint8) ) );
        
        /* Re-iterate through data and... */
        while( store_seps )
        {
            if ( store_seps & 0x0001 )
            {   /* ...store seids we are interested in */
                *remote_seids++ = *ptr >> 2;
            }
            store_seps >>= 1;
            ptr += 2;
        }
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



uint16 a2dpProcessGetCapabilitiesResponse(remote_device *device)
{
    const uint8 *remote_caps = (const uint8 *)PanicNull(device->signal_conn.connection.active.received_packet) + 2;
    uint16 size_caps = device->signal_conn.connection.active.received_packet_length - 2;
    const uint8 *remote_codec;
    /*const uint8 *ptr = NULL;*/
    uint8 *config;
    uint8 *returned_caps = NULL;
    uint16 config_size;
    bool content_protection = FALSE;
    const sep_config_type *sep_config = ((sep_data_type *)PanicNull( blockGetCurrent( device->device_id, data_block_sep_list ) ))->sep_config;

/***************  Is there any need for this since the codec caps defined in each plugin library already contain all this info */
    /* Codec Configuration. */
    if ((remote_codec = a2dpFindMatchingCodecSpecificInformation(sep_config->caps, remote_caps, TRUE)) == NULL)
    {   /* Couldn't find matching codec */
        return CONFIGURATION_NOT_SELECTED;
    }
    
    /* Increase size of config if content protection is enabled */
    config_size = 2;    /* Size of Media Transport Capabilities */
    if ((a2dpGetContentProtection(remote_caps, size_caps, returned_caps) == avdtp_scms_protection) && (sep_config->role == a2dp_sink))
    {
        config_size += 4;   /* Size of Content Protection Capabilities */
        content_protection = TRUE;
    }
/*******************************************************************************************************************************/

    /* Grow the configuration list and store it, so that codec can be written at the end when the application sends AVDTP_CONFIGURE_CODEC_RSP. */
    config_size += remote_codec[1] + 2;   /* Size of Media Codec capabilities header */

    /* Remove old service caps and allocate space for new caps */
    blockRemove( device->device_id, data_block_configured_service_caps );
    config = (uint8 *)PanicNull( blockAdd( device->device_id, data_block_configured_service_caps, config_size, sizeof(uint8) ));
    
    config[0] = AVDTP_SERVICE_MEDIA_TRANSPORT;  /* How does configuration by client affect the need for this? - client just supplies an altered Media Codec capabilities block of the same size as remote_codec[1]+2 */
    config[1] = 0; /* LOSC is always zero */
    config += 2;
    
    if ( content_protection )
    {    
        const uint8 AvdtpServiceContentProtection[ 4 ] = {AVDTP_SERVICE_CONTENT_PROTECTION, 2, AVDTP_CP_TYPE_SCMS_LSB, AVDTP_CP_TYPE_SCMS_MSB};  /* LOSC is 2 */

        memmove( config, AvdtpServiceContentProtection, 4);
        config += 4;
    }
    
    /* Codec capabilities as given to the library */
    memmove( config, remote_codec, remote_codec[1] + 2 );
    
    /* TODO: This is too specific to be done here */
    /* For SBC codec clamp the bitpools */
    if ((sep_config->caps[4] == (AVDTP_MEDIA_TYPE_AUDIO<<2)) && (sep_config->caps[5] == AVDTP_MEDIA_CODEC_SBC))
    {   /* Clamp bitpool to local maximum, if required */
        if (remote_codec[7] > sep_config->caps[9])
        {
            config[7] = sep_config->caps[9];
        }
    }

    if (sep_config->library_selects_settings)
    {
        if (!a2dpHandleSelectingCodecSettings(device, remote_codec[1] + 2, config))
        {
            return CONFIGURATION_NOT_SELECTED;
        }
        else
        {
            return CONFIGURATION_SELECTED;
        }
    }
    else
    {
        return CONFIGURATION_BY_CLIENT;
    }
}

bool a2dpSelectNextConfigurationSeid (remote_device *device)
{
    if ( !blockSetCurrent( device->device_id, data_block_list_discovered_remote_seids, DATA_BLOCK_INDEX_NEXT ) )
    {
        /* Select next local SEID */
        if ( !blockSetCurrent( device->device_id, data_block_list_preferred_local_seids, DATA_BLOCK_INDEX_NEXT ) )
        {   /* oh dear, no match and we've run out of SEPs to query */
            return FALSE;
        }
        else
        {
            uint8 sep_index = getSepIndexBySeid(device, *(uint8 *)PanicNull( blockGetCurrent( device->device_id, data_block_list_preferred_local_seids )) );

            if (sep_index == DATA_BLOCK_INDEX_INVALID)
            {   /* If the SEID is not valid then don't proceed. */
                return FALSE;
            }

            blockSetCurrent( device->device_id, data_block_sep_list, sep_index );
            blockSetCurrent( device->device_id, data_block_list_discovered_remote_seids, 0 );
        }
    }
        
    return TRUE;
}

