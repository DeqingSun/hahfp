/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010
Part of HeadsetSDK-Stereo R110.0
*/


#include "aghfp_common.h"

#include <panic.h>


/*****************************************************************************/
void aghfpSetState(AGHFP *aghfp, aghfp_state state)
{
	aghfp->state = state;
}


/*****************************************************************************
	Create a common cfm message (many AGHFP defined messages sent to the app
	have the form of the message below and a common function can be used to
	allocate them). Send the message not forgetting to set the correct 
	message id.
*/
void aghfpSendCommonCfmMessageToApp(uint16 message_id, AGHFP *aghfp, aghfp_lib_status status)
{
	MAKE_AGHFP_MESSAGE(AGHFP_COMMON_CFM_MESSAGE);
	message->status = status;
	message->aghfp = aghfp;
	MessageSend(aghfp->client_task, message_id, message);
}


/*****************************************************************************/
bool supportedProfileIsHfp(aghfp_profile profile)
{
    if ((profile == aghfp_handsfree_profile) || (profile == aghfp_handsfree_15_profile))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/*****************************************************************************/
bool supportedProfileIsHfp15(aghfp_profile profile)
{
    if (profile == aghfp_handsfree_15_profile)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/*****************************************************************************/
bool supportedProfileIsHsp(aghfp_profile profile)
{
    if (profile == aghfp_headset_profile)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

