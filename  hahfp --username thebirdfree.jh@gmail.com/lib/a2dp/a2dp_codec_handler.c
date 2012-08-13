/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    a2dp_sep_handler.c

DESCRIPTION
    This file contains

NOTES

*/


/****************************************************************************
    Header files
*/

#include "a2dp_codec_handler.h"
#include "a2dp_data_block.h"
#include "a2dp_caps_parse.h"
#include "a2dp_codec_aac.h"
#include "a2dp_codec_atrac.h"
#include "a2dp_codec_csr_faststream.h"
#include "a2dp_codec_mp3.h"
#include "a2dp_codec_sbc.h"
#include "a2dp_command_handler.h"

#include <string.h>
#include <stdlib.h>


/*****************************************************************************/
static bool getCodecFromCaps(const uint8 *local_caps, uint8 *codec_type)
{
    const uint8 *local_codec = local_caps;

    /* find the codec specific info in caps */
    if (!a2dpFindCodecSpecificInformation(&local_codec,0))
        return FALSE;

    /* return the codec type (SBC, MP3, etc) */
    *codec_type = local_codec[3];
    return TRUE;
}

#ifndef A2DP_SBC_ONLY
static bool isCodecCsrFaststream(const uint8 *local_caps)
{
    const uint8 *local_codec = local_caps;
    uint32 local_vendor_id;
    uint16 local_codec_id;

    /* find the codec specific info in caps */
    if (!a2dpFindCodecSpecificInformation(&local_codec,0))
        return FALSE;

    local_vendor_id = a2dpConvertUint8ValuesToUint32(&local_codec[4]);
    local_codec_id = (local_codec[8] << 8) | local_codec[9];

    if ((local_vendor_id == A2DP_CSR_VENDOR_ID) && (local_codec_id == A2DP_CSR_FASTSTREAM_CODEC_ID))
        return TRUE;

    return FALSE;
}
#endif


/* TODO: Rename to something more appropriate */
/*****************************************************************************/
bool a2dpHandleSelectingCodecSettings(remote_device *device, uint16 size_service_caps, uint8 *service_caps)
{
    uint8 codec_type;
    bool accept = FALSE;
    sep_data_type *current_sep = (sep_data_type *)PanicNull( blockGetCurrent( device->device_id, data_block_sep_list ) );

    if (getCodecFromCaps(current_sep->sep_config->caps, &codec_type))
    {
        /* If this is a sink we need to inform the client of the codec settings */
        if (current_sep->sep_config->role == a2dp_sink)
        {
            /* Determine the optimal codec settings */
            switch (codec_type)
            {
            case AVDTP_MEDIA_CODEC_SBC:
                selectOptimalSbcCapsSink(current_sep->sep_config->caps, service_caps);
                accept = TRUE;
                break;
#ifndef A2DP_SBC_ONLY
            case AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO:
                selectOptimalMp3CapsSink(current_sep->sep_config->caps, service_caps);
                accept = TRUE;
                break;
#endif
#ifndef A2DP_SBC_ONLY
            case AVDTP_MEDIA_CODEC_MPEG2_4_AAC:
                selectOptimalAacCapsSink(service_caps);
                accept = TRUE;
                break;
#endif
#ifdef A2DP_CODEC_ATRAC
            case AVDTP_MEDIA_CODEC_ATRAC:
                selectOptimalAtracCapsSink(current_sep->sep_config->caps, service_caps);
                accept = TRUE;
                break;
#endif /* A2DP_CODEC_ATRAC */
#ifndef A2DP_SBC_ONLY
            case AVDTP_MEDIA_CODEC_NONA2DP:
                if (isCodecCsrFaststream(current_sep->sep_config->caps))
                {
                    selectOptimalCsrFastStreamCapsSink(current_sep->sep_config->caps, service_caps);
                    accept = TRUE;
                }
                break;
#endif
            default:
                break;
            }
        }
        else
        {
            /* Local device is a source of one type or another */
            switch (codec_type)
            {
            case AVDTP_MEDIA_CODEC_SBC:
                selectOptimalSbcCapsSource(current_sep->sep_config->caps, service_caps);
                accept = TRUE;
                break;
#ifndef A2DP_SBC_ONLY
            case AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO:
                selectOptimalMp3CapsSource(current_sep->sep_config->caps, service_caps);
                accept = TRUE;
                break;
#endif
#ifndef A2DP_SBC_ONLY
            case AVDTP_MEDIA_CODEC_MPEG2_4_AAC:
                /* Not Yet Implemented */
                /*
                selectOptimalAacCapsSource(current_sep->sep_config->caps, service_caps);
                accept = TRUE;
                */
                accept = FALSE;
                break;
#endif
#ifdef A2DP_CODEC_ATRAC
            case AVDTP_MEDIA_CODEC_ATRAC:
                /* Not Yet Implemented */
                /*
                selectOptimalAtracCapsSource(current_sep->sep_config->caps, service_caps);
                accept = TRUE;
                */
                accept = FALSE;
                break;
#endif
#ifndef A2DP_SBC_ONLY
            case AVDTP_MEDIA_CODEC_NONA2DP:
                if (isCodecCsrFaststream(current_sep->sep_config->caps))
                {
                    selectOptimalCsrFastStreamCapsSource(current_sep->sep_config->caps, service_caps);
                    accept = TRUE;
                }
                break;
#endif
            default:
                break;
            }
        }
    }

    return accept;
}


/*****************************************************************************/
a2dp_codec_settings * a2dpGetCodecAudioParams (remote_device *device)
{
    uint8 codec_type;
    uint32 rate = 0, voice_rate = 0;
    a2dp_channel_mode mode = a2dp_mono;
    uint16 packet_size = 0;
    uint8 bitpool = 0, bitpool_min = 0, bitpool_max = 0, format = 0;
    codec_data_type codecData;
    sep_data_type *current_sep;
    uint8 *returned_caps = NULL;
    uint16 size_service_caps;
    const uint8 *service_caps;
    a2dp_content_protection content_protection;
    a2dp_codec_settings *codec_settings;

    if ((service_caps = blockGetBase( device->device_id, data_block_configured_service_caps )) == NULL)
    {
        return NULL;
    }

    size_service_caps = blockGetSize( device->device_id, data_block_configured_service_caps );
    content_protection = a2dpGetContentProtection(service_caps, size_service_caps, returned_caps);

    if (!a2dpFindCodecSpecificInformation(&service_caps, &size_service_caps))
    {
        return NULL;
    }

    if ((current_sep = (sep_data_type *)blockGetCurrent( device->device_id, data_block_sep_list )) == NULL)
    {
        return NULL;
    }

    if (!getCodecFromCaps(service_caps, &codec_type))
    {
        return NULL;
    }

    switch (codec_type)
    {
    case AVDTP_MEDIA_CODEC_SBC:
        if (current_sep->sep_config->role != a2dp_sink)
        {
            /*
            As we support the full SBC range, we limit our bit pool range
            to the values passed by the other side.
            */
            bitpool_min = service_caps[6];
            bitpool_max = service_caps[7];

            /* Store configuration in SBC format */
            format = a2dpSbcFormatFromConfig(service_caps);

            /* Calculate the optimal bitpool to use for the required data rate */   /* TODO: Make multi-stream aware */
            if ((format & 0x0c) == 0)
            {   /* Mono mode - 1 channel */
                bitpool = a2dpSbcSelectBitpoolAndPacketSize(format, SBC_ONE_CHANNEL_RATE, device->media_conn[0].connection.active.mtu, &packet_size);
            }
            else
            {   /* All other modes are 2 channel */
                bitpool = a2dpSbcSelectBitpoolAndPacketSize(format, SBC_TWO_CHANNEL_RATE, device->media_conn[0].connection.active.mtu, &packet_size);
            }

            /* Clamp bitpool to remote device's limits. TODO: B-4407 we could try and use a lower data rate. */
            if (bitpool < bitpool_min)
            {
                bitpool = bitpool_min;
            }

            if (bitpool > bitpool_max)
            {
                bitpool = bitpool_max;
            }
        }
        getSbcConfigSettings(service_caps, &rate, &mode);
        break;

#ifndef A2DP_SBC_ONLY
    case AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO:
        getMp3ConfigSettings(service_caps, &rate, &mode);
        break;
#endif

#ifndef A2DP_SBC_ONLY
    case AVDTP_MEDIA_CODEC_MPEG2_4_AAC:
        getAacConfigSettings(service_caps, &rate, &mode);
        break;
#endif

#ifdef A2DP_CODEC_ATRAC
    case AVDTP_MEDIA_CODEC_ATRAC:
        getAtracConfigSettings(service_caps, &rate, &mode);
        break;
#endif /* A2DP_ATRAC */

#ifndef A2DP_SBC_ONLY
    case AVDTP_MEDIA_CODEC_NONA2DP:
        if (isCodecCsrFaststream(service_caps))
        {
            /* Get the config settings so they can be sent to the client */
            getCsrFastStreamConfigSettings(service_caps, current_sep->sep_config->role, &rate, &mode, &voice_rate, &bitpool, &format);
        }
        break;
#endif

    default:
        return NULL;
    }

    /* Tell the client so it can configure the codec */
    codecData.bitpool = bitpool;
    codecData.packet_size = packet_size;
    codecData.format = format;
    codecData.content_protection = content_protection;
    codecData.voice_rate = voice_rate;

    /* Local vars potentially modified by a2dpFindCodecSpecificInformation(), so need to update them */
    service_caps = blockGetBase( device->device_id, data_block_configured_service_caps );
    size_service_caps = blockGetSize( device->device_id, data_block_configured_service_caps );

    codec_settings = (a2dp_codec_settings *)PanicNull( malloc(sizeof(a2dp_codec_settings) + size_service_caps) );
    codec_settings->rate = rate;
    codec_settings->channel_mode = mode;
    codec_settings->seid = current_sep->sep_config->seid;
    codec_settings->sink = device->media_conn[0].connection.active.sink;  /* TODO: Make multi-stream aware */
    codec_settings->codecData = codecData;
    codec_settings->size_configured_codec_caps = size_service_caps;
    memmove(codec_settings->configured_codec_caps, service_caps, size_service_caps);

    return codec_settings;
}

