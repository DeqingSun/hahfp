/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    pbapc_int_handler.c
    
DESCRIPTION
	PhoneBook Access Profile Client Library - handler functions for internal messages.

*/

#include <vm.h>
#include <pbap_common.h>
#include <print.h>
#include <goep.h>
#include <memory.h>
#include <connection.h>
#include <stdio.h>

#include "pbapc.h"
#include "pbapc_private.h"

static const uint8 vCardExt[] = {0,'.',0,'v',0,'c',0,'f',0,0};

/* Service request array used to find the channel number */
static const uint8 pbapServiceRequest [] =
{
    0x35, /* type = DataElSeq */
    0x05, /* size ...4 bytes in DataElSeq */
    0x1a, 0x00, 0x00, ((goep_PBAP_PSE>>8)&0xFF),(goep_PBAP_PSE&0xFF) /* Class ID required */
};

/* Request the supported features and repositories of the PBAP PSE */
static const uint8 serverPropertiesAttributeRequest[] =
{
    0x35,               /* 0b00110 101 type=DataElSeq */
    0x03,               /* size = 6 bytes in DataElSeq */        
    0x09, ((PBAP_REPOS>>8)&0xFF),(PBAP_REPOS&0xFF)    /* 2 byte UINT attrID - SupportedRepositories */
};


static void handleIntRegisterSDP(pbapcState *state, PBAPC_INT_REG_SDP_T *msg);
static void handleIntConnect(pbapcState *state, const PBAPC_INT_CONNECT_T *msg);
static void handleIntAuthResp(pbapcState *state, PBAPC_INT_AUTH_RESP_T *msg);
static void handleIntDisconnect(pbapcState *state);
static void handleIntAbort(pbapcState *state);
static void handleIntSetPhonebook(pbapcState *state, PBAPC_INT_SET_PHONEBOOK_T *msg);
static void handleIntPullvCardList(pbapcState *state, PBAPC_INT_GET_VCARD_LIST_START_T *msg);
static void handleIntPullvCard(pbapcState *state, PBAPC_INT_GET_VCARD_START_T *msg);
static void handleIntPullPhonebook(pbapcState *state, PBAPC_INT_GET_PHONEBOOK_START_T *msg);
static void handleIntPullNext(pbapcState *state, PBAPS_PULL_NEXT_T *msg);
static void handleIntGetServerProperties(pbapcState *state);

/****************************************************************************
NAME	
    pbapcIntHandler

DESCRIPTION
    Handler for messages received by the PBABC Task.
*/
void pbapcIntHandler(Task task, MessageId id, Message message)
{
	/* Get task control block */
	pbapcState *state = (pbapcState*)task;
	
	if ((id >= PBAPC_INT_DUMMY_FIRST) && (id <= PBAPC_INT_ENDOFLIST))
	{
		switch (id)
		{
		case PBAPC_INT_REG_SDP:
			handleIntRegisterSDP(state, (PBAPC_INT_REG_SDP_T *)message);
			break;
		case PBAPC_INT_CONNECT:
			handleIntConnect(state, (PBAPC_INT_CONNECT_T *)message);
			break;
		case PBAPC_INT_AUTH_RESP:
			handleIntAuthResp(state, (PBAPC_INT_AUTH_RESP_T *)message);
			break;
		case PBAPC_INT_DISCONNECT:
			handleIntDisconnect(state);
			break;
		case PBAPC_INT_ABORT:
			handleIntAbort(state);
			break;
			
		case PBAPC_INT_SET_PHONEBOOK:
			handleIntSetPhonebook(state, (PBAPC_INT_SET_PHONEBOOK_T *)message);
			break;
			
		case PBAPC_INT_GET_VCARD_LIST_START:
			handleIntPullvCardList(state, (PBAPC_INT_GET_VCARD_LIST_START_T *)message);
			break;
		case PBAPC_INT_GET_VCARD_START:
			handleIntPullvCard(state, (PBAPC_INT_GET_VCARD_START_T *)message);
			break;
		case PBAPC_INT_GET_PHONEBOOK_START:
			handleIntPullPhonebook(state, (PBAPC_INT_GET_PHONEBOOK_START_T *)message);
			break;
			
		case PBAPS_PULL_NEXT:
			handleIntPullNext(state, (PBAPS_PULL_NEXT_T*)message);
			break;
			
		case PBAPC_INT_GET_SERVER_PROPERTIES:
			handleIntGetServerProperties(state);
			break;
			
		default:
			PRINT(("PAPC Unhandled message : 0x%X\n",id));
			break;
		};
	}
	else
	{
		pbapcGoepHandler(state, id, message);
	}
}


static void handleIntRegisterSDP(pbapcState *state, PBAPC_INT_REG_SDP_T *msg)
{
    PRINT(("handleIntRegisterSDP\n"));
	
    state->currCom = pbapc_com_RegSdp;
	state->features = msg->feature;
        
	if (state->handle)
	{ /* Goep library already initialised for this client */
		pbapcRegisterSdpRecord(state);
	}
	else
	{
	    /* Initialise GOEP stating which type of device we want to connect to */
		GoepInit(&state->task, goep_Client, goep_PBAP_PSE);
	}
}

static void handleIntConnect(pbapcState *state, const PBAPC_INT_CONNECT_T *msg)
{
    PRINT(("handleIntConnect\n"));
	
	/* Store requested session parameters */
    state->packetSize = msg->maxPacketSize;
   	state->bdAddr = msg->bdAddr;
	
    state->currCom= pbapc_com_Connect;
        
	if (state->handle)
	{ /* Goep library already initialised for this client */
		if (state->rfcChan == 0)
			GoepGetChannel(state->handle);
		else /* Already have a RFCOMM channel, send connect request */
		{
			const uint8 *targ;
			uint16 len = 0;
			
			targ = PbapcoGetTargetString(&len);
			GoepConnect(state->handle, &state->bdAddr, state->rfcChan, state->packetSize ,len, targ);
		}
	}
	else
	{
	    /* Initialise GOEP stating which type of device we want to connect to */
		GoepInit(&state->task, goep_Client, goep_PBAP_PSE);
	}
}

static void handleIntAuthResp(pbapcState *state, PBAPC_INT_AUTH_RESP_T *msg)
{
    PRINT(("handleIntAuthResp\n"));
	
	if (state->currCom!= pbapc_com_Connect)
    { /* Not idle, Send Error Message */
		pbapcMsgSendConnectCfm(state, pbapc_wrong_state, 0);
    }
    else
    {
		GoepConnectAuthResponse(state->handle, msg->digest, msg->size_userid, msg->userid, msg->nonce);
	}
}

static void handleIntDisconnect(pbapcState *state)
{
    PRINT(("handleIntDisconnect\n"));
    
    if (state->currCom!= pbapc_com_None)
    { /* Not idle, Send Error Message */
		pbapcMsgSendDisConnectCfm(state, pbapc_not_idle);
    }
    else
    {        
        /* Send disconnect */
        GoepDisconnect(state->handle);
        /* Set command to close session */
        state->currCom= pbapc_com_Disconnect;
    }
}

static void handleIntAbort(pbapcState *state)
{
    PRINT(("handleIntAbort\n"));
    
	GoepAbort(state->handle);
}

static void handleIntSetPhonebook(pbapcState *state, PBAPC_INT_SET_PHONEBOOK_T *msg)
{
    PRINT(("handleIntSetPhonebook\n"));
	
    if (state->currCom != pbapc_com_None)
    { /* Not idle, Send Error Message */
		pbapcMsgSendSetPhonebookCfm(state, pbapc_not_idle);
    }
	else if ((state->target_repository == msg->repository) && (state->target_phonebook == msg->phonebook))
	{ /* Already in the target folder, just send the confirm */
		PRINT(("    Already at target\n"));
		pbapcMsgSendSetPhonebookCfm(state, pbapc_success);
	}
    else
    {
		if (state->svr_repos != 0)
		{ /* Repositories supported by the server is known, check them */
			if (((msg->repository == pbap_sim1) && (!(state->svr_repos & PBAP_REP_SIM1))) || 
					((msg->repository == pbap_local) && (!(state->svr_repos & PBAP_REP_LOCAL))))
			{
				pbapcMsgSendSetPhonebookCfm(state, pbapc_spb_no_repository);
				return ;
			}
		}
		state->currCom = pbapc_com_SetPhonebook;
		/* Assume it will succeed for now */
		state->setPBError = pbapc_success;
		
		state->target_repository = msg->repository;
		state->target_phonebook = msg->phonebook;
		
		if (state->curr_repository == pbap_r_unknown)
		{ /* Current repository is unknown, so it must be root */
			switch (state->target_repository)
			{
			case pbap_current:
			case pbap_local:
				state->setPBState = pbapc_spb_GotoTelecom;
				break;
			case pbap_sim1:
				state->setPBState = pbapc_spb_GotoRepository;
				break;
			default:
				PBAPC_DEBUG(("PBAC - Set Phonebook - invalid repository\n"));
				break;
			}
		}
		else if (state->curr_repository != state->target_repository)
		{ /* Target repository is different from the current repository */
			if (state->target_repository == pbap_current)
				state->setPBState = pbapc_spb_GotoParent;
			else
				state->setPBState = pbapc_spb_GotoRoot;
		}
		else
		{ /* Target and current repositories the same, goto parent */
			state->setPBState = pbapc_spb_GotoParent;
		}
	
		pbapc_spb_startState(state);
    }
}

static void handleIntPullvCardList(pbapcState *state, PBAPC_INT_GET_VCARD_LIST_START_T *msg)
{
	PRINT(("handleIntPullvCardList\n"));
    if (state->currCom != pbapc_com_None)
    { /* Not idle, Send Error Message */
		pbapcMsgSendPullvCardListCompleteInd(state, pbapc_not_idle);
    }
    else
    {
		uint16 typeLen;
		const uint8 *type = PbapcoGetvCardListingMimeType(&typeLen);
		
		uint16 nameLen = 1;/* errata suggests not to use nameLen = 0 */
		const uint8 *name = NULL;
		
		if (msg->pbook != pbap_b_unknown)
		{
			if ((state->curr_phonebook >= pbap_pb) && (state->curr_phonebook <= pbap_cch))
			{
				pbapcMsgSendPullvCardListCompleteInd(state, pbapc_vcl_no_pbook_folder);
				return;
			}
			name = PbapcoGetBookNameFromID(msg->pbook, &nameLen);
			if (!name)
			{
				pbapcMsgSendPullvCardListCompleteInd(state, pbapc_vcl_invalid_pbook);
				return;
			}
		}
		
		/* Store parameters */
		state->params = malloc(sizeof(PBAPC_INT_GET_VCARD_LIST_START_T));
		if (!state->params)
		{ /* Can't allocate memory, send error to app. */
			pbapcMsgSendPullvCardListCompleteInd(state, pbapc_vcl_no_param_resources);
		}
		else
		{
			state->currCom = pbapc_com_PullvCardList;
			
			memmove(state->params, msg, sizeof(PBAPC_INT_GET_VCARD_LIST_START_T));
	
			GoepLocalGetFirstPacketHeaders(state->handle, nameLen, name, typeLen, type);
		}
	}
}

static void handleIntPullvCard(pbapcState *state, PBAPC_INT_GET_VCARD_START_T *msg)
{
	PRINT(("handleIntPullvCard\n"));
    if (state->currCom != pbapc_com_None)
    { /* Not idle, Send Error Message */
		pbapcMsgSendPullvCardEntryCompleteInd(state, pbapc_not_idle);
    }
    else
    {
		uint16 size_type = 0;
		const uint8 *type = PbapcoGetvCardMimeType(&size_type);
		uint16 size_name = 0;
		uint8 *name = NULL;
		char pbap_digits[10];
		uint16 num_dig = 0;
		uint16 src_idx, dst_idx;
		
		if ((msg->entry == 0) && (state->curr_phonebook != pbap_pb))
		{
			pbapcMsgSendPullvCardEntryCompleteInd(state, pbapc_vce_invalid_entry);
			return;
		}

		/* Generate vCard name */
		sprintf(pbap_digits, "%X", msg->entry);
		num_dig = strlen(pbap_digits);
		size_name = (num_dig * 2) + sizeof(vCardExt);
		
		name = (uint8*)malloc(size_name * sizeof(uint8));
		if (!name)
		{ /* Send error to the application */
			pbapcMsgSendPullvCardEntryCompleteInd(state, pbapc_vce_no_name_resources);
			return;
		}

		memmove(&name[num_dig*2], vCardExt, sizeof(vCardExt));
		
		for (src_idx = 0, dst_idx = 0; src_idx < num_dig; src_idx++, dst_idx += 2)
		{
			name[dst_idx] = 0;
			name[dst_idx+1] = pbap_digits[src_idx];
		}
		state->name = name;
        
#if 0
        /*
         * Sending request without application specific parameters is known
         * to cause interoperability problems with some devices so it is
         * disabled by default.
         */
		if (((msg->format == pbap_format_def) || (msg->format == pbap_format_21)) && (msg->filter_lo == 0))
		{ /* Send request without app. specific parameters */
			GoepLocalGetFirstPacket(state->handle, size_name, name, size_type, type);
		}
		else
#endif
		{ /* Send request with app. specific parameters */
			/* Store Parameters */
			state->params = malloc(sizeof(PBAPC_INT_GET_VCARD_START_PARAMS_T));
			if (!state->params)
			{ /* Can't allocate memory, send error to app. */
				pbapcMsgSendPullvCardListCompleteInd(state, pbapc_vce_no_param_resources);
				free(state->name);
				state->name = NULL;
				return;
			}
			else
			{ /* Send Request */
				PBAPC_INT_GET_VCARD_START_PARAMS_T *params = (PBAPC_INT_GET_VCARD_START_PARAMS_T*)state->params;
				params->format = msg->format;
				params->filter_lo = msg->filter_lo;
				params->filter_hi = msg->filter_hi;
				GoepLocalGetFirstPacketHeaders(state->handle, size_name, name, size_type, type);
			}
		}
		state->currCom = pbapc_com_PullvCard;
	}
}

static void handleIntPullPhonebook(pbapcState *state, PBAPC_INT_GET_PHONEBOOK_START_T *msg)
{
	PRINT(("handleIntPullPhonebook\n"));
	
    if (state->currCom != pbapc_com_None)
    { /* Not idle, Send Error Message */
		pbapcMsgSendPullPhonebookCompleteInd(state, pbapc_not_idle);
    }
    else
    {
		/* If Repositories supported by the server is known, check them */
			if ((state->svr_repos != 0) && (((msg->repository == pbap_sim1) && (!(state->svr_repos & PBAP_REP_SIM1))) || 
					((msg->repository == pbap_local) && (!(state->svr_repos & PBAP_REP_LOCAL)))))
		{
			pbapcMsgSendPullPhonebookCompleteInd(state, pbapc_ppb_no_repository);
		}
		else
		{
			PBAPC_INT_GET_PHONEBOOK_START_PARAMS_T *params;
			uint16 size_type = 0;
			const uint8 *type = PbapcoGetPhonebookMimeType(&size_type);
			uint16 size_name = 0;
			uint8 *name = NULL;
		
			/* store parameters */
			state->params = malloc(sizeof(PBAPC_INT_GET_PHONEBOOK_START_PARAMS_T));
			if (!state->params)
			{ /* Can't allocate memory, send error to app. */
				pbapcMsgSendPullPhonebookCompleteInd(state, pbapc_ppb_no_param_resources);
				return;
			}
			params = (PBAPC_INT_GET_PHONEBOOK_START_PARAMS_T*)state->params;
			params->filter_lo = msg->filter_lo;
			params->filter_hi = msg->filter_hi;
			params->format = msg->format;
			params->maxList = msg->maxList;
			params->listStart = msg->listStart;

			if (msg->phonebook != pbap_b_unknown)
			{
				uint8 *pos = NULL;
				const uint8 *rep = NULL, *pb = NULL;
				uint16 rep_size = 0, pb_size = 0, tel_size = 0;
				const uint8 *tel = PbapcoGetBookNameFromID(pbap_telecom, &tel_size);
			
				/* Generate length of name */
				pb = PbapcoGetBookNameFromID(msg->phonebook, &pb_size);
				if (msg->repository >= pbap_sim1)
				{
					rep = PbapcoGetRepositoryNameFromID(msg->repository, &rep_size);
					size_name += rep_size;
				}
				size_name += tel_size;
				size_name += pb_size;
				size_name += sizeof(vCardExt);
				/* Returned strings contain a NULL terminator which isn't needed, so there is need to add the length
					of the folder seperator.  Also, the length is 2 bytes too long (one of the NULL terminators),
					so this must be deducted */
				size_name -= 2;
		
				/* Allocate memory */
				name = malloc(size_name * sizeof(uint8));
				if (!name)
				{
					pbapcMsgSendPullPhonebookCompleteInd(state, pbapc_ppb_no_name_resources);
					return;
				}
			
				/* build name */
				pos = name;
				if (msg->repository >= pbap_sim1)
				{
					memmove(pos, rep, rep_size);
					pos += rep_size;
					/* Insert folder seperator */
					*(pos-1) = '/';
				}
				memmove(pos, tel, tel_size);
				pos += tel_size;
				/* Insert folder seperator */
				*(pos-1) = '/';
				memmove(pos, pb, pb_size);
				pos += pb_size-2;
				memmove(pos, vCardExt, sizeof(vCardExt));
			
				state->name = name;
			}
		
			/* start get */
			GoepLocalGetFirstPacketHeaders(state->handle, size_name, name, size_type, type);
		
			state->currCom = pbapc_com_PullPhonebook;
		}
	}
}

static void handleIntPullNext(pbapcState *state, PBAPS_PULL_NEXT_T *msg)
{
	PRINT(("handleIntPullNext\n"));
    if (state->currCom != msg->command)
    {
		PRINT(("       Error\n"));
		pbapcMsgSendGetCompleteInd(state, msg->command, pbapc_wrong_state);
    }
    else
    {
		GoepLocalGetAck(state->handle);
	}
}

static void handleIntGetServerProperties(pbapcState *state)
{
	PRINT(("handleIntGetServerProperties\n"));
	
    if (state->currCom != pbapc_com_None)
    { /* Not idle, Send Error Message */
		pbapcMsgSendGetServerPropertiesCompleteInd(state, pbapc_wrong_state, 0);
    }
    else
    {
		state->currCom = pbapc_com_GetProperties;
		ConnectionSdpServiceSearchAttributeRequest(&state->task, &state->bdAddr, 0x32, sizeof(pbapServiceRequest),
							(uint8 *) pbapServiceRequest, sizeof(serverPropertiesAttributeRequest), serverPropertiesAttributeRequest);
	}
}
