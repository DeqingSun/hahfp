/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    pbapc_extract_headers.h
    
DESCRIPTION
	PhoneBook Access Profile Client Library - Extract App. Specific Parameters.

*/

#include <vm.h>
#include <print.h>
#include <pbap_common.h>
#include <goep.h>
#include <string.h>
#include <source.h>

#include <goep.h>
#include <goep_apphdrs.h>
#include "pbapc.h"
#include "pbapc_private.h"


void pbapcExtractApplicationParameters(pbapcState *state, GOEP_LOCAL_GET_START_HDRS_IND_T *msg)
{
	/* Set parameter default values */
	uint16 pbook_size = 0;
	uint8 new_missed = 0;
	const uint8 *src = SourceMap(msg->src);
	const uint8 *s;
	void *ws;
	uint16 param;
	uint8 size_param;
	pbap_goep_parameters paramID;
	bool fin;
	
	PRINT(("pbapcExtractApplicationParameters\n"));
	
	ws = Goep_apphdr_GetCreateWS(msg->src, msg->headerOffset, msg->headerLength);
	/* Can't fail since Goep_apphdr_GetCreateWS uses PanicUnlessMalloc */
	
	do
	{
		fin = Goep_apphdr_GetParameter(ws, (uint8*)&paramID, &param, &size_param);
		s = &src[param];
		switch (paramID)
		{
		case pbap_param_pbook_size:
			Goep_apphdr_GetUint16(pbook_size, s);
			PRINT(("    Phonebook Size : 0x%x\n",pbook_size));
			break;
		case pbap_param_missed_calls:
			Goep_apphdr_GetUint8(new_missed, s);
			PRINT(("    New Missed : 0x%x\n",new_missed));
			break;
		default:
			/* Don't support this parameter, ignore it */
			break;
		}
	}
	while (!fin);
			
	Goep_apphdr_GetDestroyWS(ws);
	
    switch (state->currCom)
    {
    case pbapc_com_PullvCardList:
	    {
            /* Send all the parameters as is from the source to the Lib */
			pbapcMsgSendPullvCardListStartInd(state, msg->src, pbook_size, 
                                              new_missed, msg->totalLength,
                                              msg->dataLength, msg->dataOffset,
                                              msg->moreData);
			break;
	    }
    case pbapc_com_PullPhonebook:
	    {
            /* Send all the parameters as is from the source to the Lib */
			pbapcMsgSendPullPhonebookStartInd(state, msg->src, pbook_size, 
                                              new_missed, msg->totalLength,
                                              msg->dataLength, msg->dataOffset,
                                              msg->moreData);
			break;
	    }
    default:
        break;
    }
}

