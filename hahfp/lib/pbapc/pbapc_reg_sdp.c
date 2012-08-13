/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    pbapc_reg_sdp.c
    
DESCRIPTION
	PhoneBook Access Profile Client Library - handles the registration of the client side SDP Record.

*/

#include <vm.h>
#include <pbap_common.h>
#include <print.h>
#include <goep.h>
#include <string.h>

#include <app/bluestack/types.h>
#include <app/bluestack/bluetooth.h>
#include <app/bluestack/sdc_prim.h>
#include <service.h>
#include <connection.h>

#include "pbapc.h"
#include "pbapc_private.h"

static const uint8 serviceRecordPBAP[] =
    {			
        /* Service class ID list */
        0x09,0x00,0x01,		/* AttrID , ServiceClassIDList */
        0x35,0x03,			/* 3 bytes in total DataElSeq */
        0x19,((goep_PBAP_PCE>>8)&0xFF),(goep_PBAP_PCE&0xFF),
							/* 2 byte UUID, Service class = Phonebook Access Server */
		/* profile descriptor list */
        0x09,0x00,0x09,		/* AttrId, ProfileDescriptorList */
		0x35,0x08,			/* DataElSeq wrapper */
        0x35,0x06,			/* 6 bytes in total DataElSeq */
        0x19,((goep_PBAP_PCE>>8)&0xFF),(goep_PBAP_PCE&0xFF),
							/* 2 byte UUID, Service class = OBEXPhonebookAccess */
        0x09,0x01,0x00,		/* 2 byte uint, version = 100 */

		/* service name */
        0x09,0x01,0x00,		/* AttrId - Service Name */
        0x25,0x10,			/* 16 byte string - OBEX PBAP Client */
        'O','B','E','X',' ','P','B','A','P',' ','C','l','i','e','n','t',
        
	    /* Supported features */    
	    0x09, 0x03, 0x11,   /* AttrId - Supported Features */
    	0x08, 0x00    		/* 1 byte UINT - supports phonebook browsing.  Passed in by the App. */
    };

/* Insert the Supported Features into a service record */
static bool insertFeatures(uint8 *ptr, uint8 *end, uint8 features);

/* Insert the Supported Features into a service record */
static bool insertFeatures(uint8 *ptr, uint8 *end, uint8 features)
{
    Region value;
    ServiceDataType type;
    Region record;
	
    record.begin = ptr;
    record.end   = end;
	if (ServiceFindAttribute(&record, saSupportedFeatures, &type, &value))
	{
		if((type == sdtUnsignedInteger) && RegionSize(&value))
		{
			RegionWriteUnsigned(&value, features);
			return TRUE;
		}
	}
	
    return FALSE;
}


/* Register the Client Side SDP Record with Bluestack */
void pbapcRegisterSdpRecord(pbapcState *state)
{
	uint8* sdp;
				
	sdp = (uint8 *)PanicUnlessMalloc(sizeof(serviceRecordPBAP));
	memmove(sdp, serviceRecordPBAP, sizeof(serviceRecordPBAP));

	if (!insertFeatures(sdp, sdp + sizeof(serviceRecordPBAP), state->features))
	{
		pbapcMsgRegSDPCfm(state, pbapc_sdp_failure_resource);
		free(sdp);
	}
	else
	{
		/* Send the service record to the connection lib to be registered with BlueStack */
		ConnectionRegisterServiceRecord(&state->task, sizeof(serviceRecordPBAP),sdp);
	}
}

