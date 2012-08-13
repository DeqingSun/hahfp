/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    a2dp_codec_mp3.c

DESCRIPTION
    This file contains

NOTES

*/

#ifndef A2DP_SBC_ONLY

/****************************************************************************
    Header files
*/
#include "a2dp.h"
#include "a2dp_caps_parse.h"
#include "a2dp_private.h"
#include "a2dp_codec_mp3.h"

/* Channel Mode */
const uint16 mp3CapsChannelMode[] =
{
    0x0401, 0x0401, 0x04F1, 0xFFFF,
    0x0402, 0x0402, 0x04F2, 0xFFFF,
    0x0404, 0x0404, 0x04F4, 0xFFFF,
    0x04FF, 0x04FF, 0x04F8, 0xFFFF,
    0xFFFF
};

/* Sample Rate */
const uint16 mp3CapsSampleRate[] =
{
    0x0502, 0x0502, 0x05C2, 0xFFFF,
    0x0501, 0x0501, 0x05C1, 0xFFFF,
    0x0504, 0x0504, 0x05C4, 0xFFFF,
    0x0508, 0x0508, 0x05C8, 0xFFFF,
    0x0510, 0x0510, 0x05D0, 0xFFFF,
    0x05FF, 0x05FF, 0x05E0, 0xFFFF,
    0xFFFF
};


/* Bit Rate.                                                                                       */
/* We try and use the rate nearest to 128kbps.  In reality the source won't encode MP3 in realtime */
/* so it will probably only have a single bit set representing it's pre-compressed file.           */
const uint16 mp3CapsBitRate[] =
{
    0x0602, 0x0602, 0x0682, 0x0700,
    0x0601, 0x0601, 0x0681, 0x0700,
    0x0604, 0x0604, 0x0684, 0x0700,
    0x0780, 0x0780, 0x0680, 0x0780,
    0x0608, 0x0608, 0x0688, 0x0700,
    0x0740, 0x0740, 0x0680, 0x0740,
    0x0610, 0x0610, 0x0690, 0x0700,
    0x0720, 0x0720, 0x0680, 0x0720,
    0x0620, 0x0620, 0x06A0, 0x0700,
    0x0710, 0x0710, 0x0680, 0x0710,
    0x0640, 0x0640, 0x06C0, 0x0700,
    0x0708, 0x0708, 0x0680, 0x0708,
    0x0704, 0x0704, 0x0680, 0x0704,
    0x0702, 0x0702, 0x0680, 0x0702,
    0xFFFF
};

const uint16 * mp3CapsTable[] =
{
    mp3CapsChannelMode,
    mp3CapsSampleRate,
    mp3CapsBitRate,
    (uint16 *)0xFFFF
};

static void selectMp3Caps(uint8 *caps, const uint8 *local_codec)
{
    const uint16 **caps_table = mp3CapsTable;

    while (*caps_table != (uint16 *)0xFFFF)
    {
        const uint16 *parse_table = *caps_table;

        while (parse_table[0] != 0xFFFF)
        {
            if ((caps[(uint8)(parse_table[0]>>8)] & (uint8)parse_table[0]) && (local_codec[(uint8)(parse_table[1]>>8)] & (uint8)parse_table[1]))
            {
                caps[(uint8)(parse_table[2]>>8)] &= (uint8)parse_table[2];

                if (parse_table[3] != 0xFFFF)
                {
                    caps[(uint8)(parse_table[3]>>8)] &= (uint8)parse_table[3];
                }

                break;
            }

            parse_table += 4;
        }

        caps_table++;
    }
}

/**************************************************************************/
void selectOptimalMp3CapsSink(const uint8 *local_caps, uint8 *caps)
{
    const uint8 *local_codec = local_caps;

    /* find the codec specific info in caps */
    if (!a2dpFindCodecSpecificInformation(&local_codec,0))
        return;

    if (!caps)
        return;

    selectMp3Caps(caps, local_codec);
}


/**************************************************************************/
void selectOptimalMp3CapsSource(const uint8 *local_caps, uint8 *caps)
{
    const uint8 *local_codec = local_caps;

    /* find the codec specific info in caps */
    if (!a2dpFindCodecSpecificInformation(&local_codec,0))
        return;

    selectMp3Caps(caps, local_codec);
}


/**************************************************************************/
void getMp3ConfigSettings(const uint8 *service_caps, uint32 *rate, a2dp_channel_mode *channel_mode)
{
    if (!service_caps)
    {
        *rate = 0;
        *channel_mode = a2dp_mono;
        return;
    }

    /* Set sample rate for both channels based on codec configuration */
    if (service_caps[5] & 0x01)
        *rate = 48000;
    else if (service_caps[5] & 0x02)
        *rate = 44100;
    else if (service_caps[5] & 0x04)
        *rate = 32000;
    else if (service_caps[5] & 0x08)
        *rate = 24000;
    else if (service_caps[5] & 0x10)
        *rate = 22050;
    else
        *rate = 16000;

    if (service_caps[4] & 0x08)
        /* Mono */
        *channel_mode = a2dp_mono;
    else if (service_caps[4] & 0x04)
        /* Stereo - dual channel */
        *channel_mode = a2dp_dual_channel;
    else if (service_caps[4] & 0x02)
        /* Stereo */
        *channel_mode = a2dp_stereo;
    else
        /* Stereo - joint */
        *channel_mode = a2dp_joint_stereo;
}
#else
    static const int dummy;
#endif
