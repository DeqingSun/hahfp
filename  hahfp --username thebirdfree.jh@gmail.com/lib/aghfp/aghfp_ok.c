/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010
Part of HeadsetSDK-Stereo R110.0
*/


#include "aghfp_ok.h"
#include "aghfp_private.h"
#include "aghfp_send_data.h"

#include <panic.h>
#include <string.h>


/****************************************************************************/
void AghfpSendOk(AGHFP *aghfp)
{
    MessageSend(&aghfp->task, AGHFP_INTERNAL_SEND_OK_REQ, 0);
}


/****************************************************************************/
void aghfpSendOk(AGHFP *aghfp)
{
	if (aghfp->rfcomm_sink)
	{
	    aghfpSendAtCmd(aghfp, "OK");
	}
	else
	{
		AGHFP_DEBUG_PANIC(("Couldn't send OK because there is no RFCOMM sink"));
	}
}


/****************************************************************************/
void AghfpSendError(AGHFP *aghfp)
{
    MessageSend(&aghfp->task, AGHFP_INTERNAL_SEND_ERROR_REQ, 0);
}


/****************************************************************************/
void aghfpSendError(AGHFP *aghfp)
{
	if (aghfp->rfcomm_sink)
	{
		aghfpSendAtCmd(aghfp, "ERROR");
	}
	else
	{
		AGHFP_DEBUG_PANIC(("Couldn't send ERROR because there is no RFCOMM sink"));
	}
}
