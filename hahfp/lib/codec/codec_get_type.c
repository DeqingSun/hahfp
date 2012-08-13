/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    codec_get_type.c
    
DESCRIPTION
	
*/

#include "codec.h"
#include "codec_private.h"
#include "codec_wm8731_message_handler.h"
#include "codec_csr_internal_message_handler.h"

#include <stdlib.h>


/****************************************************************************
NAME	
	CodecGetCodecType

DESCRIPTION
   Get the codec type in use.
*/
codec_type CodecGetCodecType(Task codecTask)
{
	if (codecTask->handler == csrInternalMessageHandler)
    {
		return codec_csr_internal;
    }
    else if (codecTask->handler == wolfsonMessageHandler)
    {
        return codec_wm8731;
    }
	else
	{
		return codec_none;
	}
}




