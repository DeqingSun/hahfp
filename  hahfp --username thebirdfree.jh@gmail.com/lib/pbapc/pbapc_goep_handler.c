/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    pbapc_goep_handler.h
    
DESCRIPTION
	PhoneBook Access Profile Client Library - handler functions for GOEP messages.

*/

#include <vm.h>
#include <bdaddr.h>
#include <print.h>

#include <pbap_common.h>
#include <goep.h>
#include <connection.h>
#include <app/bluestack/types.h>
#include <app/bluestack/bluetooth.h>
#include <app/bluestack/sdc_prim.h>
#include <service.h>
#include <region.h>
#include <memory.h>
#include <sdp_parse.h>

#include "pbapc.h"
#include "pbapc_private.h"

#include "goep_apphdrs.h"

static pbapc_lib_status convert_error(goep_lib_status status);

static void handleGoepInitCfm(pbapcState *state, const GOEP_INIT_CFM_T *msg);
static void handleGoepChannelInd(pbapcState *state, const GOEP_CHANNEL_IND_T *msg);
static void handleGoepAuthRequestInd(pbapcState *state, GOEP_AUTH_REQUEST_IND_T *msg);
static void handleGoepConnectCfm(pbapcState *state, const GOEP_CONNECT_CFM_T *msg);
static void handleGoepDisconnectInd(pbapcState *state, const GOEP_DISCONNECT_IND_T *msg);

static void handleGoepLocalGetStartInd(pbapcState *state, GOEP_LOCAL_GET_START_IND_T *msg);
static void handleGoepLocalGetDataInd(pbapcState *state, GOEP_LOCAL_GET_DATA_IND_T *msg);
static void handleGoepLocalGetCompleteInd(pbapcState *state, GOEP_LOCAL_GET_COMPLETE_IND_T *msg);

static void handleGoepGetAppHeadersInd(pbapcState *state, GOEP_GET_APP_HEADERS_IND_T *msg);
static void handleGoepGetStartHeadersInd(pbapcState *state, GOEP_LOCAL_GET_START_HDRS_IND_T *msg);

static void handleGoepSetPathCfm(pbapcState *state, GOEP_SET_PATH_CFM_T *msg);

/* Handle SDP Register Cfm */
static void handleSDPRegisterCfm(pbapcState *state, CL_SDP_REGISTER_CFM_T *msg);
static void handleSDPServSrchAttrCfm(pbapcState *state, CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *msg);


/****************************************************************************
NAME	
    pbabcGoepHandler

DESCRIPTION
    Handler for messages received by the PBABC Task from GOEP.
*/
void pbapcGoepHandler(pbapcState *state, MessageId id, Message message)
{
	switch (id)
	{
	case GOEP_INIT_CFM:
		handleGoepInitCfm(state, (GOEP_INIT_CFM_T *)message);
		break;
	case GOEP_CHANNEL_IND:
		handleGoepChannelInd(state, (GOEP_CHANNEL_IND_T *)message);
		break;
	case GOEP_CONNECT_CFM:
		handleGoepConnectCfm(state, (GOEP_CONNECT_CFM_T *)message);		
		break;
	case GOEP_AUTH_REQUEST_IND:
		handleGoepAuthRequestInd(state, (GOEP_AUTH_REQUEST_IND_T *)message);		
		break;
	case GOEP_DISCONNECT_IND:
		handleGoepDisconnectInd(state, (GOEP_DISCONNECT_IND_T *)message);
		break;
		
	case GOEP_LOCAL_GET_START_IND:
		handleGoepLocalGetStartInd(state, (GOEP_LOCAL_GET_START_IND_T*)message);
		break;
	case GOEP_LOCAL_GET_DATA_IND:
		handleGoepLocalGetDataInd(state, (GOEP_LOCAL_GET_DATA_IND_T*)message);
		break;
	case GOEP_LOCAL_GET_COMPLETE_IND:
		handleGoepLocalGetCompleteInd(state, (GOEP_LOCAL_GET_COMPLETE_IND_T*)message);
		break;
		
	case GOEP_GET_APP_HEADERS_IND:
		handleGoepGetAppHeadersInd(state, (GOEP_GET_APP_HEADERS_IND_T *)message);
		break;		
	case GOEP_LOCAL_GET_START_HDRS_IND:
		handleGoepGetStartHeadersInd(state, (GOEP_LOCAL_GET_START_HDRS_IND_T *)message);
		break;
		
	case GOEP_SET_PATH_CFM:
		handleGoepSetPathCfm(state, (GOEP_SET_PATH_CFM_T *)message);
		break;
		
		/* Messages from the connection library */
	case CL_SDP_REGISTER_CFM:
		handleSDPRegisterCfm(state, (CL_SDP_REGISTER_CFM_T*)message);
		break;
	case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
		handleSDPServSrchAttrCfm(state, (CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T*)message);
		break;
		
	default:
		PRINT(("PAPC - GOEP Unhandled message : 0x%X\n",id));
		break;
	};
}


/* Error value conversion */
static pbapc_lib_status convert_error(goep_lib_status status)
{
	pbapc_lib_status ret = pbapc_success;
	
	switch (status)
	{
	case goep_success:
		ret = pbapc_success;
		break;
	case goep_remote_disconnect:
		ret = pbapc_remote_disconnect;
		break;
	case goep_host_abort:
	case goep_local_abort: /* Deliberate Fallthrough */
		ret = pbapc_aborted;
		break;
		
	case goep_setpath_notfound:
		ret = pbapc_spb_not_found;
		break;
	case goep_setpath_unauthorised:
		ret = pbapc_spb_unauthorised;
		break;

    case goep_invalid_command:      
		ret = pbapc_not_idle;
		break;
    case goep_invalid_parameters:   
	case goep_server_unsupported:   
	case goep_server_invalid_serv:  
    case goep_connection_refused:   
    case goep_get_badrequest:       
    case goep_get_forbidden:        
    case goep_get_notfound:         	
	case goep_transport_failure:
	default:
		ret = pbapc_failure;
		break;
	}
	
	return ret;
}

static void handleSDPRegisterCfm(pbapcState *state, CL_SDP_REGISTER_CFM_T *msg)
{
	PRINT(("handleSDPRegisterCfm\n"));
	
	if (msg->status == success)
	{
		pbapcMsgRegSDPCfm(state, pbapc_success);
	}
	else
	{
		pbapcMsgRegSDPCfm(state, pbapc_sdp_failure_bluestack);
	}
}

static void handleSDPServSrchAttrCfm(pbapcState *state, CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *msg)
{
    PRINT(("handleSDPServSrchAttrCfm :\n"));
	
	if (msg->status==success)
	{
		uint8 *ptr;
		uint8 *end_ptr;
		uint8 repos;
		
		ptr = msg->attributes;
		end_ptr = msg->attributes+msg->size_attributes;
		
		SdpParseGetPbapRepos(end_ptr-ptr, ptr, &repos);
		PRINT(("    Repository Mask : %d\n",repos));
		
		state->svr_repos = repos;
		pbapcMsgSendGetServerPropertiesCompleteInd(state, pbapc_success, repos);
	}
	else
	{
		PRINT(("    Error : %d\n",msg->status));
		
		pbapcMsgSendGetServerPropertiesCompleteInd(state, pbapc_prop_sdp_error, 0);
	}
	
	state->currCom = pbapc_com_None;
}


static void handleGoepInitCfm(pbapcState *state, const GOEP_INIT_CFM_T *msg)
{
    PRINT(("handleGoepInitCfm\n"));
    
    if (msg->status != goep_success)
    { /* Couldn't start registration, clean up */
		PRINT(("GOEP_INIT_CFM:   GOEP Status %d\n", msg->status));
		pbapcMsgSendConnectCfm(state, pbapc_failure, 0);
    }
    else
    { /* Registered with GOEP, start connection */
        state->handle = msg->goep;
		
		if (state->currCom == pbapc_com_RegSdp)
		{ /* Trying to register an SDP Record, do that next */
			pbapcRegisterSdpRecord(state);
            PRINT(("GOEP_INIT_CFM: Register SDP record\n"));
		}
		else
		{
			if (state->rfcChan == 0)
			{
                /* We want a RFCOMM Channel, so go get it */
				GoepGetChannel(state->handle);
                PRINT(("GOEP_INIT_CFM: Get a RFCOMM Channel\n"));
            }
			else /* Already have a RFCOMM channel, send connect request */
			{
				const uint8 *targ;
				uint16 len = 0;
                PRINT(("GOEP_INIT_CFM: Already have a RFCOMM channel, send connect request\n"));
	
				targ = PbapcoGetTargetString(&len);
				GoepConnect(state->handle, &state->bdAddr, state->rfcChan, state->packetSize ,len, targ);
			}
		}
    }
}

static void handleGoepChannelInd(pbapcState *state, const GOEP_CHANNEL_IND_T *msg)
{
    PRINT(("handleGoepChannelInd\n"));
    
    if (msg->status != goep_success)
    { /* Couldn't start registration, clean up */
		PRINT(("GOEP_CHANNEL_IND: GOEP Status %d\n", msg->status));
		pbapcMsgSendConnectCfm(state, pbapc_failure, 0);
    }
    else
	{
		const uint8 *targ;
		uint16 len = 0;
		state->rfcChan = msg->channel;
        
		PRINT(("GOEP_CHANNEL_IND: Get channel success, try to connect it, Channel:[%x], packetsize:[%x]\n", state->rfcChan, state->packetSize));
        
		targ = PbapcoGetTargetString(&len);
		GoepConnect(state->handle, &state->bdAddr, state->rfcChan, state->packetSize ,len, targ);
    }
}

static void handleGoepConnectCfm(pbapcState *state, const GOEP_CONNECT_CFM_T *msg)
{
	pbapc_lib_status status = pbapc_success;
	uint16 packetSize = 0;
	
    PRINT(("handleGoepConnectCfm\n"));
	
    if (msg->status != goep_success)
    { /* Couldn't connect */
		PRINT(("     GOEP Status %d\n", msg->status));
        status = convert_error(msg->status);
    }
    else
    { /* GOEP has connected with the server, inform App. */
        /* Store connection data */
        state->packetSize = msg->maxPacketLen;
        
        packetSize = state->packetSize;
    }
	
	state->currCom= pbapc_com_None;
	
	state->curr_repository = pbap_r_unknown;
	state->curr_phonebook = pbap_b_unknown;
	state->target_repository = pbap_r_unknown;
	state->target_phonebook = pbap_b_unknown;
	
	pbapcMsgSendConnectCfm(state, status, packetSize);
}

static void handleGoepAuthRequestInd(pbapcState *state, GOEP_AUTH_REQUEST_IND_T *msg)
{
	PBAPC_AUTH_REQUEST_IND_T *message = (PBAPC_AUTH_REQUEST_IND_T *)
										PanicUnlessMalloc(sizeof(PBAPC_AUTH_REQUEST_IND_T) + msg->size_realm);
    PRINT(("handleGoepAuthRequestInd\n"));
	
	memmove(message, msg, sizeof(PBAPC_AUTH_REQUEST_IND_T) + msg->size_realm);
	message->device_id = state->device_id;
	
    MessageSend(pbapc->theAppTask, PBAPC_AUTH_REQUEST_IND, message);
}

static void handleGoepDisconnectInd(pbapcState *state, const GOEP_DISCONNECT_IND_T *msg)
{
	pbapc_lib_status status = pbapc_success;
    PRINT(("handleGoepDisconnectInd\n"));
    
    if (msg->status != goep_success)
    { /* Send error message */
        
        status= convert_error(msg->status);
    }
	state->currCom= pbapc_com_None;
	
	/* Null bdAddr pointer since we no longer have a need for it */
    BdaddrSetZero(&state->bdAddr);
	state->svr_repos = 0;

	pbapcMsgSendDisConnectCfm(state, status);
}

static void handleGoepSetPathCfm(pbapcState *state, GOEP_SET_PATH_CFM_T *msg)
{
    PRINT(("handleGoepSetPathCfm\n"));
    
    if (msg->status != goep_success)
    { /* Error, try to return to our old location */
		PRINT(("     Error 0x%x\n", msg->status));
		
		state->setPBError = convert_error(msg->status);
		if ((state->setPBState == pbapc_spb_GotoRepository) && (state->setPBError == pbapc_spb_not_found))
			state->setPBError = pbapc_spb_no_repository;
		
		state->setPBState = pbapc_spb_GotoRoot;
		state->target_repository = state->curr_repository;
		state->target_phonebook = state->curr_phonebook;
		
		pbapc_spb_startState(state);
    }
	else
	{
		if (pbapc_spb_completeState(state))
		{ /* State machine complete, clean up state and inform the app. */
			state->currCom= pbapc_com_None;
			if (state->target_repository == pbap_current)
			{
				if (state->curr_repository == pbap_r_unknown)
					state->curr_repository = pbap_local;
			}
			else
				state->curr_repository = state->target_repository;
			state->curr_phonebook = state->target_phonebook;

			pbapcMsgSendSetPhonebookCfm(state, state->setPBError);
			state->setPBError = pbapc_success;
		}
	}
}


static void handleGoepGetAppHeadersInd(pbapcState *state, GOEP_GET_APP_HEADERS_IND_T *msg)
{
	uint16 lenUsed = 0;
	
	PRINT(("handleGoepGetAppHeadersInd\n"));
	
	switch (state->currCom)
	{
	case pbapc_com_PullvCardList:
		lenUsed = pbapcAddvCardListHeaders(state, msg->sink);
		break;
	case pbapc_com_PullvCard:
		lenUsed = pbapcAddvCardEntryHeaders(state, msg->sink);
		break;
	case pbapc_com_PullPhonebook:
		lenUsed = pbapcAddPhonebookHeaders(state, msg->sink);
		break;
	default:
		PBAPC_DEBUG(("PBAC - Get App. Headers - Using a command that does not support app headers\n"));
		break;
	};
	
	GoepSendAppSpecificPacket(state->handle, lenUsed);
	free(state->params);
	state->params = NULL;
}

static void handleGoepGetStartHeadersInd(pbapcState *state, GOEP_LOCAL_GET_START_HDRS_IND_T *msg)
{
    PRINT(("handleGoepGetStartHeadersInd : "));
    
    switch (state->currCom)
    {
    case pbapc_com_PullvCardList:
	    {
	        PRINT(("pbapc_com_PullvCardList\n"));
			pbapcExtractApplicationParameters(state, msg);
			break;
	    }
    case pbapc_com_PullPhonebook:
	    {
	        PRINT(("pbapc_com_PullPhonebook\n"));
			pbapcExtractApplicationParameters(state, msg);
			break;
	    }
    default:
        break;
    }
}

static void handleGoepLocalGetStartInd(pbapcState *state, GOEP_LOCAL_GET_START_IND_T *msg)
{
    PRINT(("handleGoepLocalGetStartInd :\n"));
    
    switch (state->currCom)
    {
    case pbapc_com_PullvCardList:
	    {
	        PRINT(("    pbapc_com_PullvCardList\n"));
			pbapcMsgSendPullvCardListStartInd(state, msg->src, 0, 
										0, msg->totalLength, msg->dataLength, msg->dataOffset, msg->moreData);
			break;
	    }
    case pbapc_com_PullvCard:
	    {
	        PRINT(("    pbapc_com_PullvCard\n"));
			pbapcMsgSendPullvCardEntryStartInd(state, msg->src, msg->totalLength, msg->dataLength, 
											   			msg->dataOffset, msg->moreData);
			if (state->name)
			{
				free(state->name);
				state->name = NULL;
			}
			break;
	    }
    case pbapc_com_PullPhonebook:
	    {
	        PRINT(("    pbapc_com_PullPhonebook\n"));
			pbapcMsgSendPullPhonebookStartInd(state, msg->src, 0, 
										0, msg->totalLength, msg->dataLength, msg->dataOffset, msg->moreData);
			if (state->name)
			{
				free(state->name);
				state->name = NULL;
			}
			break;
	    }
    default:
        break;
    }
}

static void handleGoepLocalGetDataInd(pbapcState *state, GOEP_LOCAL_GET_DATA_IND_T *msg)
{
    PRINT(("handleGoepLocalGetDataInd :\n"));
    
    switch (state->currCom)
    {
    case pbapc_com_PullvCardList:
	    {
	        PRINT(("    pbapc_com_PullvCardList\n"));
			pbapcMsgSendPullvCardListDataInd(state, msg->src, msg->dataLength, msg->dataOffset, msg->moreData);
			break;
	    }
    case pbapc_com_PullvCard:
	    {
	        PRINT(("    pbapc_com_PullvCard\n"));
			pbapcMsgSendPullvCardEntryDataInd(state, msg->src, msg->dataLength, msg->dataOffset, msg->moreData);
			if (state->name)
			{
				free(state->name);
				state->name = NULL;
			}
			break;
	    }
    case pbapc_com_PullPhonebook:
	    {
	        PRINT(("    pbapc_com_PullPhonebook\n"));
			pbapcMsgSendPullPhonebookDataInd(state, msg->src, msg->dataLength, msg->dataOffset, msg->moreData);
			if (state->name)
			{
				free(state->name);
				state->name = NULL;
			}
			break;
	    }
    default:
        break;
    }
}

static void handleGoepLocalGetCompleteInd(pbapcState *state, GOEP_LOCAL_GET_COMPLETE_IND_T *msg)
{
	pbapc_lib_status res = convert_error(msg->status);
	
    PRINT(("handleGoepLocalGetCompleteInd :\n"));
	
    switch (state->currCom)
    {
    case pbapc_com_PullvCardList:
	    {
	        PRINT(("    pbapc_com_PullvCardList\n"));
			pbapcMsgSendPullvCardListCompleteInd(state, res);
			break;
	    }
    case pbapc_com_PullvCard:
	    {
	        PRINT(("    pbapc_com_PullvCard\n"));
			pbapcMsgSendPullvCardEntryCompleteInd(state, res);
			break;
	    }
    case pbapc_com_PullPhonebook:
	    {
	        PRINT(("    pbapc_com_PullPhonebook\n"));
			pbapcMsgSendPullPhonebookCompleteInd(state, res);
			break;
	    }
    default:
        break;
    }
	
	state->currCom = pbapc_com_None;
}

