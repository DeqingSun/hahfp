/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004

FILE NAME
    init.c

DESCRIPTION
    This file contains the initialisation code for the Hfp profile library.

NOTES

*/
/*lint -e655 */

/****************************************************************************
    Header files
*/
#include "hfp.h"
#include "hfp_private.h"
#include "hfp_service_manager.h"
#include "hfp_link_manager.h"
#include "hfp_common.h"
#include "hfp_rfc.h"
#include "hfp_sdp.h"
#include "hfp_init.h"
#include "hfp_profile_handler.h"
#include "hfp_wbs_handler.h"

#include <panic.h>
#include <print.h>
#include <string.h>

/* The main HFP data struct */
hfp_task_data* theHfp;

/* Send an init cfm message to the application */
static void sendInitCfmToApp(hfp_init_status status)
{
    MAKE_HFP_MESSAGE(HFP_INIT_CFM);
    message->status = status;
    MessageSend(theHfp->clientTask, HFP_INIT_CFM, message);

    /* If the initialisation failed, free the allocated task */
    if (status > hfp_init_reinit_fail)
        free(theHfp);
}


/****************************************************************************
NAME
    HfpInit

DESCRIPTION
    

MESSAGE RETURNED
    HFP_INIT_CFM

RETURNS
    void
*/
void HfpInit(Task theAppTask, const hfp_init_params *config, const char* extra_indicators)
{
    if(theHfp)
    {
        sendInitCfmToApp(hfp_init_reinit_fail);
        return;
    }
    
    /* Allocate and zero our hfp_task_data */
    theHfp = PanicUnlessNew(hfp_task_data);
    memset(theHfp, 0, sizeof(hfp_task_data));
    
    PRINT(("HFP Task Data taking up %d words\n", sizeof(hfp_task_data)));
    PRINT(("%d Words for links\n", sizeof(hfp_link_data) * HFP_MAX_LINKS));
    PRINT(("%d Words for services\n", sizeof(hfp_service_data) * HFP_NUM_SERVICES));
    
    /* Check the app has passed in a valid pointer. */
    if (!config)
    {
        HFP_DEBUG(("Config parameters not passed in\n"));
    }
    else
    {
        uint8 index;
        hfp_link_data* link;
        hfp_profile profile = config->supported_profile;
        
        /* Set the handler function */
        theHfp->task.handler = hfpProfileHandler;
        
        /* Mask out unsupported features. */
        theHfp->hf_supported_features = (config->supported_features & ~HFP_ENHANCED_CALL_CONTROL);
        
        if(!supportedProfileIsHfp106(profile))
            theHfp->hf_supported_features &= ~HFP_CODEC_NEGOTIATION;
        
        /* Set the number of link loss reconnect attempts */
        theHfp->link_loss_time     = config->link_loss_time;
        theHfp->link_loss_interval = config->link_loss_interval;
        
        /* Connection related state updated in separate function */
        for_all_links(link, index)
        {
            /*hfpLinkReset(link, FALSE); - link already memset to 0 above */
            link->ag_supported_features = (AG_THREE_WAY_CALLING | AG_IN_BAND_RING);
        }
        
#if (HFP_MAX_LINKS > 1)
        /* Disable the second link if multipoint is disabled */
        if(!config->multipoint)
            theHfp->links[1].ag_slc_state = hfp_slc_disabled;
#endif
        
        /* Store the app task so we know where to return responses */
        theHfp->clientTask = theAppTask;

        index = 0;
        
        /* Make sure only one handsfree version is configured */
        if(supportedProfileIsHfp(profile))
        {
            hfp_profile handsfree_version = supportedProfileIsHfp106(profile) ? hfp_handsfree_106_profile : hfp_handsfree_profile;
            
            theHfp->services[index++].profile = handsfree_version;
            
#if (HFP_MAX_LINKS > 1)
            /* Register two services for multipoint */
            if(config->multipoint)
                theHfp->services[index++].profile = handsfree_version;
#endif
            
            /* Codec negotiation is supported */
            if(hfFeatureEnabled(HFP_CODEC_NEGOTIATION))
                hfpWbsEnable(config->supported_wbs_codecs);
        }

#if (HFP_SERVICES_PER_LINK > 1)
        /* Set up HSP service(s) */
        if(supportedProfileIsHsp(profile))
#else
        /* Set up HSP service(s) only if no HFP has been set up */
        if(supportedProfileIsHsp(profile) && !supportedProfileIsHfp(profile))
#endif
        {
            theHfp->services[index++].profile = hfp_headset_profile;
#if (HFP_MAX_LINKS > 1)
            /* Register two services for multipoint */
            if(config->multipoint)
                theHfp->services[index++].profile = hfp_headset_profile;
#endif
        }
#if (HFP_SERVICES_PER_LINK <= 1)
        else
        {
            /* App requested >1 service but we only have space for 1 */
            HFP_ASSERT_FAIL(("Could not initialise all profiles\n"));
        }
#endif
        if(index == 0)
            HFP_ASSERT_FAIL(("No supported profiles\n"));
        theHfp->num_supported_profiles = index;
        
        theHfp->extra_indicators = extra_indicators;
        
        theHfp->optional_indicators = config->optional_indicators;
        
        theHfp->disable_nrec = config->disable_nrec;
        
        theHfp->extended_errors = config->extended_errors;
        
        theHfp->csr_features = config->csr_features;
        
        /* We want sync connect notifications */
        ConnectionSyncRegister(&theHfp->task);
        
        /* Start registering RFCOMM channels */
        ConnectionRfcommAllocateChannel(&theHfp->task, HFP_DEFAULT_CHANNEL);
    }
}


/****************************************************************************
NAME
    hfpInitRfcommRegisterCfm

DESCRIPTION
    Rfcomm channel has been allocated.

RETURNS
    void
*/
void hfpInitRfcommRegisterCfm(const CL_RFCOMM_REGISTER_CFM_T *cfm)
{
    if(cfm->status == success)
    {
        /* Get pointer to service data with no channel */
        hfp_service_data* service = hfpGetServiceFromChannel(0);
        
        if(service)
        {
            /* Assign this channel to the unused service */
            service->rfc_server_channel = cfm->server_channel;
            /* Next RFC register if not all complete */
            if(hfpGetServiceFromChannel(0))
                ConnectionRfcommAllocateChannel(&theHfp->task, 0);
            /* Register SDP record for this service */
            hfpRegisterServiceRecord(service);
            return;
        }
    }
    
    /* Request failed or no free service */
    sendInitCfmToApp(hfp_init_rfc_chan_fail);
}


/****************************************************************************
NAME
    hfpInitSdpRegisterComplete

DESCRIPTION
    SDP registration has completed

RETURNS
    void
*/
void hfpInitSdpRegisterComplete(hfp_lib_status status)
{
    if(status == hfp_success)
    {
        /* Get the service we requested to register */
        hfp_service_data* service = hfpGetServiceFromChannel(theHfp->busy_channel);
        if(service)
        {
            if(service == HFP_SERVICE_TOP)
            {
                /* Last service successfully registered, done. */
                sendInitCfmToApp(hfp_init_success);
                theHfp->initialised = TRUE;
            }
            /* Make sure we clear the registering channel */
            theHfp->busy_channel = 0;
            return;
        }
    }
    /* Either request failed or couldn't find the right service */
    sendInitCfmToApp(hfp_init_sdp_reg_fail);
}


/****************************************************************************
NAME
    hfpInitSdpRegisterCfm

DESCRIPTION
    SDP register request from Connection library has returned

RETURNS
    void
*/
void hfpInitSdpRegisterCfm(const CL_SDP_REGISTER_CFM_T *cfm)
{
    hfp_lib_status status = (cfm->status == success) ? hfp_success : hfp_fail;
    if(status == hfp_success)
    {
        /* Get the service we requested to register */
        hfp_service_data* service = hfpGetServiceFromChannel(theHfp->busy_channel);
        /* Store the service handle */
        service->sdp_record_handle = cfm->service_handle;
    }
    hfpInitSdpRegisterComplete(status);
}
