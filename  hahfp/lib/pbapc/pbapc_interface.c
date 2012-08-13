/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    pbapc_interface.h
    
DESCRIPTION
	PhoneBook Access Profile Client Library - public interface.

*/

#include <vm.h>
#include <pbap_common.h>
#include <memory.h>
#include <print.h>
#include <bdaddr.h>
#include "pbapc.h"
#include "pbapc_private.h"

pbapcClient *pbapc = NULL;


/*!
    @brief Check the number of connected PBAP Devices.
*/
static uint8 GetNumberOfPbapcConnectedDevices(void)
{
    uint8 NoOfDevices = 0;
    
    if((pbapc->pbapc_link[pbapc_primary_link]).connect_state == pbapc_connected)
        NoOfDevices++;
    if((pbapc->pbapc_link[pbapc_secondary_link]).connect_state == pbapc_connected)
        NoOfDevices++;
   
    return NoOfDevices;
}

/*!
    @brief Add a new PBAP Devices with BT address.

	@param bdAddr The device with bt address to be added.
	
	if PBAPC_MAX_REMOTE_DEVICES devices have been connected, no new link is allowed; otherwise,
    if no device has been connected, put it to the first link;
*/

static uint8 AddPbapDevice (const bdaddr *bdAddr)
{
    pbapc_link_priority priority  = pbapc_invalid_link;
    pbapc->num_of_pbapc_connected = GetNumberOfPbapcConnectedDevices(); 
    
    if(!BdaddrIsZero(bdAddr) && pbapc->num_of_pbapc_connected < 2)
    {
        /* check whether the device has been connected or connecting */
        priority = PbapcLinkFrmAddr(bdAddr);
        
        if(priority == pbapc_invalid_link)
        {
            uint8 device_id = 0;
        
            for(device_id = 0; device_id < PBAPC_MAX_REMOTE_DEVICES; device_id++)
            {
                 if( !(pbapc->pbapc_link[device_id].connect_state == pbapc_connected || 
                       pbapc->pbapc_link[device_id].connect_state == pbapc_connecting) )
                 {
                    priority = (pbapc_link_priority)device_id;
                    return priority;
                }
            }            
        }
        else if(pbapc->pbapc_link[priority].connect_state == pbapc_connected || 
                pbapc->pbapc_link[priority].connect_state == pbapc_connecting)
        {
            /* This device has been connected or connecting, don't try to reconnect it */
            priority  = pbapc_invalid_link;
        }
    }
    
    return priority;
}

/*!
	@brief Initialise the PBAP Client Library.

	@param theAppTask The current application task.
	
	@param priority The profile instance low power mode priority. For a set of
	profile instances with connections on a given ACL the instance with the
	highest priority value wins and the low power mode on the ACL is set
	according to it's power table.
	
	PBAPC_INIT_CFM message will be received by the application. 
*/
void PbapcInit(Task theAppTask, uint16 priority)
{
	MAKE_PBAPC_MESSAGE(PBAPC_INIT_CFM);
	
    if( !pbapc )
    {
        uint8 device_id = 0;
	    pbapc = PanicUnlessNew(pbapcClient);
	    memset(pbapc, 0, sizeof(pbapcClient));	
	    
        PRINT(("PBAPC Init\n"));

        for(device_id=0; device_id < PBAPC_MAX_REMOTE_DEVICES; device_id++)
        {
            pbapcState *pbapc_link = &(pbapc->pbapc_link[device_id]);
            
	        pbapc_link->task.handler = pbapcIntHandler;
	        pbapc_link->curr_repository = pbap_r_unknown;
	        pbapc_link->curr_phonebook  = pbap_b_unknown;
	        pbapc_link->target_repository = pbap_r_unknown;
	        pbapc_link->target_phonebook  = pbap_b_unknown;
            
            pbapc_link->device_id         = device_id;
            pbapc_link->connect_state     = pbapc_disconnected;
        }
	        
        pbapc->theAppTask             = theAppTask;
        pbapc->num_of_pbapc_connected = 0;
        
        message->status = pbapc_success;
    }	
    else
    {
         message->status = pbapc_failure;
    }
    
	MessageSend(theAppTask, PBAPC_INIT_CFM, message);
}

/*!
	@brief Register the client side SDP Record
	@param pbapc PBAPC Session Handle.
	@param feature Features supported by the client.  Mask values
	defined in pbap_common.h.
	
	PBAPC_REG_SDP_CFM message will be received by the application. 
*/
void PbapcRegisterSDP(uint16 device_id, uint8 feature)
{
	Task state = (Task)( &((pbapc->pbapc_link[device_id]).task));
	
	/* Send an internal message */
	{
		MAKE_PBAPC_MESSAGE(PBAPC_INT_REG_SDP);
        
		message->feature = feature;        
		MessageSend(state, PBAPC_INT_REG_SDP, message);
	}
}

/*!
	@brief Open an PBAPC Connection with a server.
	@param pbapc PBAPC Session Handle.
	@param bd_addr The Bluetooth address of the device being replied to.
	@param maxPacketSize Maximum packet size supported by this client.
	
	This will make a Bluetooth connection with the server.
*/
void PbapcConnect(const bdaddr *bdAddr, const uint16 maxPacketSize)
{
	Task state;
	pbapc_link_priority device_id;
    
	PBAPC_ASSERT((bdAddr != NULL), ("PBAPC - Null Bluetooth address pointer\n")); 

    device_id = (pbapc_link_priority) AddPbapDevice(bdAddr);
    
    if(device_id != pbapc_invalid_link)
    {
        pbapcState *pbapc_link = &(pbapc->pbapc_link[device_id]);
        
        state = (Task)( &(pbapc_link->task) );
        
	    pbapc_link->connect_state   = pbapc_connecting;

        /* Send an internal message */
	    {
		    MAKE_PBAPC_MESSAGE(PBAPC_INT_CONNECT);        
		    message->bdAddr = *bdAddr;
		    message->maxPacketSize=maxPacketSize;        
		    MessageSend(state, PBAPC_INT_CONNECT, message);
	    }
    }
}

/*!
	@brief Disconnect from an server.  
	@param session PBAPC Session Handle.
*/
void PbapcDisconnect( uint16 device_id )
{
    Task state;
    pbapcState *pbapc_link = &(pbapc->pbapc_link[device_id]);
    
    if(!(pbapc_link->connect_state == pbapc_disconnected || pbapc_link->connect_state == pbapc_disconnecting))
    {
        state = (Task)( &(pbapc_link->task) );
        
        pbapc_link->connect_state   = pbapc_disconnecting;
        
	    /* Send an internal message */
	    MessageSend(state, PBAPC_INT_DISCONNECT, PBAPC_NO_PAYLOAD);
    }
}

/*
	@brief Respond to an authentication challenge during connect.
	@param pbapc PBAPC Session Handle.
	@param digest MD5 Digest response. Must be a 16byte string.
	@param size_userid Length of the optional User Id string.
	@param userid Option User Id string.
	@param nonce Optional MD5 Digest Nonce as sent in the Challenge. Must be a 16byte string or NULL.
	
	This function can be called by a GOEP Client on receipt of a GOEP_AUTH_REQUEST_IND message to reply
	to an authentication challenge.
	A GOEP_CONNECT_CFM message will be received to indicate that the connection process
	has completed.
*/
void PbapcConnectAuthResponse(uint16 device_id, const uint8 *digest, uint16 size_userid, const uint8 *userid, const uint8 *nonce)
{
	Task state;
	
    state = (Task)( &((pbapc->pbapc_link[device_id]).task) );
	
	/* Send an internal message */
	{
		MAKE_PBAPC_MESSAGE(PBAPC_INT_AUTH_RESP);
		message->digest = digest;
		message->size_userid = size_userid;
		message->userid = userid;
		message->nonce = nonce;
		MessageSend(state, PBAPC_INT_AUTH_RESP, message);
	}
}

/*!
	@brief Abort the current multi-packet operation.
	@param pbapc PBAPC Session Handle.
	
	The relevant COMPLETE_IND message will be received with the status code of pbapc_aborted on success.
*/
void PbapcAbort(uint16 device_id)
{
    Task state;
    
    state = (Task)( &((pbapc->pbapc_link[device_id]).task) );
    
	/* Send an internal message */
	MessageSend(state, PBAPC_INT_ABORT, PBAPC_NO_PAYLOAD);
}

/*!
	@brief Change the current active phonebook.  
	@param pbapc PBAPC Session Handle.
	@param repository Repository containing the new phonebook.
	@param phonebook The new phonebook.
	
	To change to a different phonebook in the same repository, use pbap_current as the
	repository.
	
	PBAPC_SET_PHONEBOOK_CFM message will be received by the application. 
*/
void PbapcSetPhonebook(uint16 device_id, pbap_phone_repository repository, pbap_phone_book phonebook)
{
	Task state;
	
	PBAPC_ASSERT(((repository >= pbap_current) && (repository < pbap_r_unknown)), ("PBAPC - Invalid Repository ID\n")); 
	PBAPC_ASSERT(((phonebook >= pbap_telecom) && (phonebook < pbap_b_unknown)), ("PBAPC - Invalid Phonebook ID\n")); 

    state = (Task)( &((pbapc->pbapc_link[device_id]).task) );
	
	/* Send an internal message */
	{
		MAKE_PBAPC_MESSAGE(PBAPC_INT_SET_PHONEBOOK);        
		message->repository = repository;
		message->phonebook = phonebook;
		MessageSend(state, PBAPC_INT_SET_PHONEBOOK, message);
	}
}

/*!
	@brief The packet received has been processed and is no longer needed.
	@param pbapc PBAPC Session Handle (as returned in PBAPC_INIT_CFM).
	
	Every packet send to the client that contains a source must be declared
	complete before the next function is called.  e.g. When a
	PBAPC_PULL_VCARD_LIST_START_IND has been received, PbapcPacketComplete must be
	called before calling PbapcPullvCardListingNext.
	
	No message is received on completion.
*/
void PbapcPacketComplete(uint16 device_id)
{
	GoepPacketComplete( (pbapc->pbapc_link[device_id]).handle );
}

/*!
	@brief Start to get the vCard listing for the current phonebook.
	@param pbapc PBAPC Session Handle.
	@param order Result sort order. Use 'pbap_order_default' for default sort order.
	@param pbook Phonebook folder to retrieve. Use 'pbap_b_unknown' for the current folder.
	@param srchVal Value to search for.
	@param size_srchVal Length of the search value.
	@param srchAttr Attribute to search. Use 'pbap_a_unknown' for default search attribute.
	@param maxList Maximum List Count.
	@param listStart List Start Offset.
*/
void PbapcPullvCardListingStart(uint16 device_id, pbap_order_values order, pbap_phone_book pbook, const uint8 *srchVal,
								uint16 size_srchVal, pbap_search_values srchAttr, uint16 maxList, uint16 listStart)
{
	Task state;
	
	PBAPC_ASSERT(((order >= pbap_order_idx) && (order <= pbap_order_default)), ("PBAPC - Invalid Order ID\n")); 
	PBAPC_ASSERT(((srchAttr >= pbap_search_name) && (srchAttr <= pbap_a_unknown)), ("PBAPC - Invalid Search Attribute\n")); 

    state = (Task)( &((pbapc->pbapc_link[device_id]).task) );
	
	/* Send an internal message */
	{
		MAKE_PBAPC_MESSAGE(PBAPC_INT_GET_VCARD_LIST_START);
		message->order = order;
		message->pbook = pbook;
		message->srchVal = srchVal;
		message->size_srchVal = size_srchVal;
		message->srchAttr = srchAttr;
		message->maxList = maxList;
		message->listStart = listStart;
		MessageSend(state, PBAPC_INT_GET_VCARD_LIST_START, message);
	}
}

/*!
	@brief Get the next vCard listing packet for the current phonebook.
	@param pbapc PBAPC Session Handle.
*/
void PbapcPullvCardListingNext(uint16 device_id)
{
	Task state;
	
    state = (Task)( &((pbapc->pbapc_link[device_id]).task) );
	
	/* Send an internal message */
	{
		MAKE_PBAPC_MESSAGE(PBAPS_PULL_NEXT);
		message->command = pbapc_com_PullvCardList;
		MessageSend(state, PBAPS_PULL_NEXT, message);
	}
}

/*!
	@brief Start to get a vCard entry from the current phonebook.
	@param pbapc PBAPC Session Handle.
	@param entry Entry number of the vCard to download.
	@param filter_lo PBAP Parameter filter mask low 32 bits.  Use zero to not sent the parameter.
	@param filter_hi PBAP Parameter filter mask high 32 bits.  Use zero to not sent the parameter.
	@param format PBAP Parameter format.
	
	If the filter does not include the manditory fields as required by the specified format, these
	will be added.
	
	A PBAPC_PULL_VCARD_START_IND message will be received by the appliction when the first packet
	arrives.  A PBAPC_PULL_VCARD_COMPLETE_IND message will be received after the last packet is
	received.
*/
void PbapcPullvCardEntryStart(uint16 device_id, uint16 entry, uint32 filter_lo, uint32 filter_hi, pbap_format_values format)
{
	Task state;
	
	PBAPC_ASSERT(((format >= pbap_format_21) && (format <= pbap_format_def)), ("PBAPC - Invalid Format\n")); 

    state = (Task)( &((pbapc->pbapc_link[device_id]).task) );
	
	/* Send an internal message */
	{
		MAKE_PBAPC_MESSAGE(PBAPC_INT_GET_VCARD_START);
		message->entry = entry;
		message->filter_lo = filter_lo;
		message->filter_hi = filter_hi;
		message->format = format;
		MessageSend(state, PBAPC_INT_GET_VCARD_START, message);
	}
}

/*!
	@brief Get the next vCard entry packet from the current phonebook.
	@param pbapc PBAPC Session Handle.
	
	A PBAPC_PULL_VCARD_ENTRY_DATA_IND message will be received by the appliction for each packet after
	the first.  A PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND message will be received after the last packet is
	received.
*/
void PbapcPullvCardEntryNext(uint16 device_id)
{
	Task state;
	
    state = (Task)( &((pbapc->pbapc_link[device_id]).task) );
	
	/* Send an internal message */
	{
		MAKE_PBAPC_MESSAGE(PBAPS_PULL_NEXT);
		message->command = pbapc_com_PullvCard;
		MessageSend(state, PBAPS_PULL_NEXT, message);
	}
}

/*!
	@brief Start a PullPhonebook Function
	@param pbapc PBAPC Session Handle.
	@param repository Repository containing the phonebook to PULL
	@param phonebook Phonebook to PULL
	@param filter_lo PBAP Parameter filter mask low 32 bits.  Use zero to not sent the parameter.
	@param filter_hi PBAP Parameter filter mask high 32 bits.  Use zero to not sent the parameter.
	@param format PBAP Parameter format.
	@param maxList Maximum List Count.
	@param listStart List Start Offset.
*/
void PbapcPullPhonebookStart(uint16 device_id, pbap_phone_repository repository, pbap_phone_book phonebook,
							 	uint32 filter_lo, uint32 filter_hi, pbap_format_values format, uint16 maxList, uint16 listStart)
{
	Task state;
	pbapcState *pbapc_link = &(pbapc->pbapc_link[device_id]);
    
	PBAPC_ASSERT(((format >= pbap_format_21) && (format <= pbap_format_def)), ("PBAPC - Invalid Format\n")); 
	PBAPC_ASSERT(((phonebook >= pbap_pb) && (phonebook <= pbap_b_unknown)), ("PBAPC - Invalid Phonebook\n")); 

    state = (Task)( &(pbapc_link->task) );
	
	/* Check that the repository is valid */

	if ((pbapc_link->curr_phonebook == pbap_b_unknown) && (phonebook == pbap_b_unknown))
	{ /* Also check if browsing supported by server */
		pbapcMsgSendPullPhonebookCompleteInd(pbapc_link, pbapc_ppb_no_required_name);
		return;
	}
	/* Send an internal message */
	{
		MAKE_PBAPC_MESSAGE(PBAPC_INT_GET_PHONEBOOK_START);
		message->repository = repository;
		message->phonebook = phonebook;
		message->filter_lo = filter_lo;
		message->filter_hi = filter_hi;
		message->format = format;
		message->maxList = maxList;
		message->listStart = listStart;
		MessageSend(state, PBAPC_INT_GET_PHONEBOOK_START, message);
	}
}

/*!
	@brief Get the next Phonebook packet from the current phonebook.
	@param pbapc PBAPC Session Handle.
	
	A PBAPC_PULL_VCARD_ENTRY_DATA_IND message will be received by the appliction for each packet after
	the first.  A PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND message will be received after the last packet is
	received.
*/
void PbapcPullPhonebookNext(uint16 device_id)
{
	Task state;
	
    state = (Task)( &((pbapc->pbapc_link[device_id]).task) );
	
	/* Send an internal message */
	{
		MAKE_PBAPC_MESSAGE(PBAPS_PULL_NEXT);
		message->command = pbapc_com_PullPhonebook;
		MessageSend(state, PBAPS_PULL_NEXT, message);
	}
}

/*!
	@brief Obtain the properties of the remote server.
	@param pbapc PBAPC Session Handle.
	
	Perform an SDP search of the server to obtain the supported repositories mask.
	
	Completes with a PBAPC_PROPERTIES_IND.
*/
void PbapcGetServerProperties(uint16 device_id)
{
	Task state;
	
    state = (Task)( &((pbapc->pbapc_link[device_id]).task) );
	
	/* Check to ensure we are connected and idle */
	
	/* Send an internal message */
	MessageSend(state, PBAPC_INT_GET_SERVER_PROPERTIES, PBAPC_NO_PAYLOAD);
}

/*!
	@brief Obtain the device_id of Pbap Link from BT address.
	
	Completes with a device_id is returned.
*/
uint16 PbapcLinkFrmAddr(const bdaddr *bdAddr)
{
    uint16 device_id = pbapc_invalid_link;
    
    for(device_id = 0; device_id < PBAPC_MAX_REMOTE_DEVICES; device_id++)
    {
       if(BdaddrIsSame(&(pbapc->pbapc_link[device_id].bdAddr), bdAddr))
           return(device_id);            
    }
    
    return(device_id);
}
