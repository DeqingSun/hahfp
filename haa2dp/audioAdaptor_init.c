/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2009
Part of Audio-Adaptor-SDK 2009.R1

DESCRIPTION
    Initialisation code.
*/

#include "audioAdaptor_private.h"
#include "audioAdaptor_configure.h"
#include "audioAdaptor_a2dp_slc.h"
#include "audioAdaptor_avrcp_slc.h"
#include "audioAdaptor_charger.h"
#include "audioAdaptor_init.h"
#include "audioAdaptor_a2dp.h"
#include "audioAdaptor_statemanager.h"
#include "audioAdaptor_a2dp_msg_handler.h"
#include "audioAdaptor_avrcp_msg_handler.h"
#include "audioAdaptor_buttons.h"

#include <ps.h>
#include <string.h>
#include <panic.h>
#include <stdlib.h>
#include <pio.h>
#include <codec.h>
#include <charger.h>


#define NUM_SOURCE_TYPE  2 /* 0: USB, 1: ANALOG */

#ifdef USER_CONFIGURE_CODEC
    static const sep_config_type sbc_sep_usb = { SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, FALSE, 0, sizeof(sbc_caps_source_usb), sbc_caps_source_usb };
    static const sep_config_type sbc_sep_analogue = { SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, FALSE, 0, sizeof(sbc_caps_source_analogue), sbc_caps_source_analogue };

    #ifdef DUAL_STREAM       
        static const sep_config_type sbc_sep_ds_analogue = { SBC_DS_SEID, KALIMBA_DS_RESOURCE_ID, sep_media_type_audio, a2dp_source, FALSE, 0, sizeof(sbc_caps_source_analogue), sbc_caps_source_analogue };       
        static const sep_config_type faststream_sep_ds_analogue = { FASTSTREAM_DS_SEID, KALIMBA_DS_RESOURCE_ID, sep_media_type_audio, a2dp_source, FALSE, 14, sizeof(faststream_caps_source_analogue), faststream_caps_source_analogue };
    #endif
#else

    static const sep_config_type sbc_sep_usb = { SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, TRUE, 0, sizeof(sbc_caps_source_usb), sbc_caps_source_usb };
    static const sep_config_type sbc_sep_analogue = { SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, TRUE, 0, sizeof(sbc_caps_source_analogue), sbc_caps_source_analogue };

    #ifdef DUAL_STREAM
        static const sep_config_type sbc_sep_ds_analogue = { SBC_DS_SEID, KALIMBA_DS_RESOURCE_ID, sep_media_type_audio, a2dp_source, TRUE, 0, sizeof(sbc_caps_source_analogue), sbc_caps_source_analogue };
        static const sep_config_type faststream_sep_ds_analogue = { FASTSTREAM_DS_SEID, KALIMBA_DS_RESOURCE_ID, sep_media_type_audio, a2dp_source, FALSE, 14, sizeof(faststream_caps_source_analogue), faststream_caps_source_analogue };
    #endif

#endif

/****************************************************************************
  MAIN FUNCTIONS
*/

/****************************************************************************
NAME 
    initA2dp

DESCRIPTION
    Initialises the A2DP profile.
    
*/
void initA2dp (void)
{
    uint8 source_codec_enabled;
    
    sep_data_type seps[NUM_SEPS];
    uint8 number_of_seps = 0;
    
    if (PsRetrieve(PSKEY_CODEC_ENABLED, &source_codec_enabled, sizeof(uint8)))
    {
        the_app->a2dpCodecsEnabled = source_codec_enabled;
    }
    
#ifdef USER_CONFIGURE_CODEC
    /* Retrieve the Codec configuration from PS */
    PsRetrieve(PSKEY_SBC_CODEC_CONFIGURATION, &the_app->pskey_sbc_codec_config, sizeof(uint16));
    PsRetrieve(PSKEY_FASTSTREAM_CODEC_CONFIGURATION, &the_app->pskey_faststream_codec_config, sizeof(uint16));
    the_app->bidirect_faststream = (uint8)((the_app->pskey_faststream_codec_config & 0x1000) >> 12); 
    #ifdef INCLUDE_MP3_ENCODER_PLUGIN
        PsRetrieve(PSKEY_MP3_CODEC_CONFIGURATION, &the_app->pskey_mp3_codec_config, sizeof(uint16));
    #endif
#endif    
  
    seps[number_of_seps].sep_config = &sbc_sep_analogue;
    seps[number_of_seps].in_use = FALSE; 
    number_of_seps++;
#ifdef DUAL_STREAM
    seps[number_of_seps].sep_config = &sbc_sep_ds_analogue;
    seps[number_of_seps].in_use = FALSE; 
    number_of_seps++;
#endif
    
    A2dpInit(&the_app->task, A2DP_INIT_ROLE_SOURCE, NULL, number_of_seps, seps);
}


/****************************************************************************
NAME 
    initAvrcp

DESCRIPTION
    Initialises the AVRCP profile.
 
*/
void initAvrcp (void)
{
    avrcp_init_params avrcp_config;
        
    avrcp_config.device_type = avrcp_target;
            
    AvrcpInitLazy(&the_app->task, &the_app->task, &avrcp_config);
}


/****************************************************************************
NAME 
    initSeidConnectPriority

DESCRIPTION
    To decide the priority of SEIDs (Stream End Point IDs) for A2dp connection.
 
RETURNS
      The size of seid list. 
    
*/
uint16 initSeidConnectPriority(uint8 *seid_list)
{
    uint16 size_seids = 0;
    
    seid_list[size_seids] = SBC_SEID;
    size_seids += 1;
    
#ifdef DUAL_STREAM   
    seid_list[size_seids] = SBC_DS_SEID;
    size_seids += 1;
#endif    

    return size_seids;
}


/****************************************************************************
NAME 
    initA2dpPlugin

DESCRIPTION
    To decide the plugin based on the SEID of A2dp connection.
 
RETURNS
    The plugin to be connected for audio streaming.
    
*/
Task initA2dpPlugin(uint8 seid)
{
    if (a2dpSeidIsSbc(seid))      
    {
        return (TaskData *)&csr_sbc_encoder_plugin;
    }
    
    /* No plugin found so Panic */
    Panic();
    
    return 0;
}


/****************************************************************************
NAME 
    initCodec

DESCRIPTION
    To initialise the codec employed in A2dp profile.
 
*/
void initCodec(void)
{
#ifdef ENABLE_EXTERNAL_ADC
    /* Init the Wolfson codec with default params. */
    CodecInitWolfson( &the_app->task, 0 );
#else
    CodecInitCsrInternal( &the_app->task ) ;
#endif
    
}


/****************************************************************************
  NAME 
      initUserFeatures

DESCRIPTION
     To initialise the features employed in audio adaptor.
 
*/
void initUserFeatures (void) 
{    
    /* Read local supported features to determine data rates */
    PanicFalse(PsFullRetrieve(0x00EF, &the_app->local_supported_features, 4));
 
    /* Read PS config into cache memory */
    configureGetConfigCache();
    
    /* Initialise the Codec Library for analogue mode */
    initCodec();
    
    /* Setup power management of audio adaptor */
    configureGetPowerManagerConfig();

    if (chargerIsConnected())
    {
        MessageSend(&the_app->task, APP_CHARGER_CONNECTED, 0);
        the_app->chargerconnected = TRUE;
    }
}


/****************************************************************************
NAME 
    initApp

DESCRIPTION
    To start the application initialisation, get the supported profiles and 
    start connection initialisation.
 
*/
void initApp (void)
{
    /* Retrieve supported profiles */
    the_app->initialised_profiles = 0;
    configureGetSupportedProfiles();
    /* Initialise the connection library */
    ConnectionInit(&the_app->task);
}
    

/****************************************************************************
NAME 
    initProfile

DESCRIPTION
    The request of profile initialisation for all supported profiles.
 
*/
void initProfile (void)
{
    /* Panic if no profiles have been initialised */
    if (the_app->supported_profiles == ProfileNone)
    {
        Panic();
    }
    
    if (the_app->supported_profiles & ProfileA2dp)
    {
        initA2dp(); 
    }
    
    if (the_app->supported_profiles & ProfileAvrcp)
    {
        initAvrcp(); 
    }
}


/****************************************************************************
NAME 
    initProfileCfm

DESCRIPTION
    The confirmation of profile initialisation for all supported profiles.
 
*/
void initProfileCfm (mvdProfiles profile, bool success)
{
    if (success)
    {
        switch (profile)
        {
            case ProfileA2dp:
            {
                if (!(the_app->initialised_profiles & ProfileA2dp))
                {
                    the_app->initialised_profiles |= ProfileA2dp;
                }
                else
                {    /* Should never get here */
                    Panic();
                }
                break;
            }
            case ProfileAvrcp:
            {
                if (!(the_app->initialised_profiles & ProfileAvrcp))
                {
                    the_app->initialised_profiles |= ProfileAvrcp;
                }
                else
                {    /* Should never get here */
                    Panic();
                }
                break;
            }
            default:
            {
                /* Should never get here */
                Panic();
                break;
            }
        }
        
        if (the_app->initialised_profiles==the_app->supported_profiles)
        {    /* All profile profiles have been initialised */
            MessageSend(&the_app->task, APP_INIT_CFM, 0);
        }
    }
    else
    {    /* Initialisation of a library failed - this should not happen so we have a major issue */
        Panic();
    }
}
