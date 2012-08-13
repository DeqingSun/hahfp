/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    pbapc_messages.h
    
DESCRIPTION
	PhoneBook Access Profile Client Library - message send functions.

*/

#include <vm.h>
#include <print.h>
#include <pbap_common.h>

#include "pbapc.h"
#include "pbapc_private.h"

/*!
    @brief Add a new PBAP Devices with BT address.

	@param bdAddr The device with bt address to be added.
	
	if PBAPC_MAX_REMOTE_DEVICES devices have been connected, no new link is allowed; otherwise,
    if no device has been connected, put it to the first link;
*/


void pbapcMsgRegSDPCfm(pbapcState *state, pbapc_lib_status status)
{
	MAKE_PBAPC_MESSAGE(PBAPC_REG_SDP_CFM);
	message->device_id = state->device_id;
	message->status = status;
            
    MessageSend(pbapc->theAppTask, PBAPC_REG_SDP_CFM, message);
}

void pbapcMsgSendConnectCfm(pbapcState *state, pbapc_lib_status status, uint16 pktSize)
{
    if(status == pbapc_success)
    {
        pbapc->num_of_pbapc_connected++;
        state->connect_state  = pbapc_connected;
    }
    else
    {
        state->connect_state  = pbapc_disconnected;
    }
    
    {
	    MAKE_PBAPC_MESSAGE(PBAPC_CONNECT_CFM);
	    message->device_id = state->device_id;
        message->bdAddr    = state->bdAddr;
	    message->status    = status;
	    message->packetSize = pktSize;
            
        MessageSend(pbapc->theAppTask, PBAPC_CONNECT_CFM, message);
    }
}


void pbapcMsgSendDisConnectCfm(pbapcState *state, pbapc_lib_status status)
{
    state->connect_state  = pbapc_disconnected;
    
    if(pbapc->num_of_pbapc_connected) 
        pbapc->num_of_pbapc_connected--;
                                    
    {
	    MAKE_PBAPC_MESSAGE(PBAPC_DISCONNECT_IND);
	    message->device_id = state->device_id;
        message->num_of_pbapc_links = pbapc->num_of_pbapc_connected;
	    message->status = status;
            
        MessageSend(pbapc->theAppTask, PBAPC_DISCONNECT_IND, message);
    }
}

void pbapcMsgSendSetPhonebookCfm(pbapcState *state, pbapc_lib_status status)
{
	MAKE_PBAPC_MESSAGE(PBAPC_SET_PHONEBOOK_CFM);
	message->device_id = state->device_id;
	message->status = status;
            
    MessageSend(pbapc->theAppTask, PBAPC_SET_PHONEBOOK_CFM, message);
}

void pbapcMsgSendPullvCardListStartInd(pbapcState *state, Source src, uint16 pbook_size, 
										uint8 new_missed, uint32 totalLength, uint16 dataLen, 
										uint16 dataOffset, bool moreData)
{
	MAKE_PBAPC_MESSAGE(PBAPC_PULL_VCARD_LIST_START_IND);
	message->device_id = state->device_id;
    message->src = src;
	message->pbook_size = pbook_size;
	message->new_missed = new_missed;
	message->totalLength = totalLength; 	
	message->dataLen = dataLen;
	message->dataOffset = dataOffset;
    message->moreData = moreData;
            
    MessageSend(pbapc->theAppTask, PBAPC_PULL_VCARD_LIST_START_IND, message);
}

void pbapcMsgSendPullvCardListDataInd(pbapcState *state, Source src, uint16 dataLen, 
										uint16 dataOffset, bool moreData)
{
	MAKE_PBAPC_MESSAGE(PBAPC_PULL_VCARD_LIST_DATA_IND);
	message->device_id = state->device_id;
    message->src = src;
	message->dataLen = dataLen;
	message->dataOffset = dataOffset;
    message->moreData = moreData;
            
    MessageSend(pbapc->theAppTask, PBAPC_PULL_VCARD_LIST_DATA_IND, message);
}

void pbapcMsgSendPullvCardListCompleteInd(pbapcState *state, pbapc_lib_status status)
{
	MAKE_PBAPC_MESSAGE(PBAPC_PULL_VCARD_LIST_COMPLETE_IND);
	message->device_id = state->device_id;
	message->status = status;
            
    MessageSend(pbapc->theAppTask, PBAPC_PULL_VCARD_LIST_COMPLETE_IND, message);
}

void pbapcMsgSendPullvCardEntryStartInd(pbapcState *state, Source src, 
										uint32 totalLength, uint16 dataLen, 
										uint16 dataOffset, bool moreData)
{
	MAKE_PBAPC_MESSAGE(PBAPC_PULL_VCARD_ENTRY_START_IND);
	message->device_id = state->device_id;
    message->src = src;
	message->totalLength = totalLength; 	
	message->dataLen = dataLen;
	message->dataOffset = dataOffset;
    message->moreData = moreData;
            
    MessageSend(pbapc->theAppTask, PBAPC_PULL_VCARD_ENTRY_START_IND, message);
}

void pbapcMsgSendPullvCardEntryDataInd(pbapcState *state, Source src, uint16 dataLen, 
										uint16 dataOffset, bool moreData)
{
	MAKE_PBAPC_MESSAGE(PBAPC_PULL_VCARD_ENTRY_DATA_IND);
	message->device_id = state->device_id;
    message->src = src;
	message->dataLen = dataLen;
	message->dataOffset = dataOffset;
    message->moreData = moreData;
            
    MessageSend(pbapc->theAppTask, PBAPC_PULL_VCARD_ENTRY_DATA_IND, message);
}

void pbapcMsgSendPullvCardEntryCompleteInd(pbapcState *state, pbapc_lib_status status)
{
	MAKE_PBAPC_MESSAGE(PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND);
	message->device_id = state->device_id;
	message->status = status;
            
    MessageSend(pbapc->theAppTask, PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND, message);
}

void pbapcMsgSendPullPhonebookStartInd(pbapcState *state, Source src, uint16 pbook_size, 
										uint8 new_missed, uint32 totalLength, uint16 dataLen, 
										uint16 dataOffset, bool moreData)
{
	MAKE_PBAPC_MESSAGE(PBAPC_PULL_PHONEBOOK_START_IND);
	message->device_id = state->device_id;
    message->src = src;
	message->pbook_size = pbook_size;
	message->new_missed = new_missed;
	message->totalLength = totalLength; 	
	message->dataLen = dataLen;
	message->dataOffset = dataOffset;
    message->moreData = moreData;
            
    MessageSend(pbapc->theAppTask, PBAPC_PULL_PHONEBOOK_START_IND, message);
}

void pbapcMsgSendPullPhonebookDataInd(pbapcState *state, Source src, uint16 dataLen, 
										uint16 dataOffset, bool moreData)
{
	MAKE_PBAPC_MESSAGE(PBAPC_PULL_PHONEBOOK_DATA_IND);
	message->device_id = state->device_id;
    message->src = src;
	message->dataLen = dataLen;
	message->dataOffset = dataOffset;
    message->moreData = moreData;
            
    MessageSend(pbapc->theAppTask, PBAPC_PULL_PHONEBOOK_DATA_IND, message);
}

void pbapcMsgSendPullPhonebookCompleteInd(pbapcState *state, pbapc_lib_status status)
{
	MAKE_PBAPC_MESSAGE(PBAPC_PULL_PHONEBOOK_COMPLETE_IND);
	message->device_id = state->device_id;
	message->status = status;
            
    MessageSend(pbapc->theAppTask, PBAPC_PULL_PHONEBOOK_COMPLETE_IND, message);
}

void pbapcMsgSendGetServerPropertiesCompleteInd(pbapcState *state, pbapc_lib_status status, uint8 repositories)
{
	MAKE_PBAPC_MESSAGE(PBAPC_SERVER_PROPERTIES_COMPLETE_IND);
	message->device_id = state->device_id;
	message->status = status;
	message->repositories = repositories;
            
    MessageSend(pbapc->theAppTask, PBAPC_SERVER_PROPERTIES_COMPLETE_IND, message);
}

void pbapcMsgSendGetCompleteInd(pbapcState *state, pbapc_running_command command, pbapc_lib_status status)
{
	switch (command)
	{
	case pbapc_com_PullvCardList:
		pbapcMsgSendPullvCardListCompleteInd(state, status);
		break;
	case pbapc_com_PullvCard:
		pbapcMsgSendPullvCardEntryCompleteInd(state, status);
		break;
	case pbapc_com_PullPhonebook:
		pbapcMsgSendPullPhonebookCompleteInd(state, status);
		break;
	default:
		break;
	}
}
