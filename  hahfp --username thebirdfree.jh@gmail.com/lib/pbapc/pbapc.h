/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    pbapc.h
    
DESCRIPTION
	PhoneBook Access Profile Client Library.

*/
/*!
@file pbapc.h
@brief PhoneBook Access Profile Client Library.


Library Dependecies : connection, region, service, goep, goep_apphdrs, bdaddr, pbap_common
		
Library variants:-
		pbapc - pbapc with no debug
		pbapc_debug - pbapc with debug checks
		pbapc_debug_print - pbapc with debug checks and print output
*/

#ifndef	PBAP_CLIENT_H_
#define	PBAP_CLIENT_H_

#include <message.h>
#include <bdaddr_.h>
#include <pbap_common.h>
#include <goep.h>
#include <library.h>

/*!
	@brief PBAP Client status. 
*/
typedef enum 
{
	/*! Last operation was successful. */
	pbapc_success,					
	/*! Last operation failed. */
    pbapc_failure,		
	/*! Last operation was aborted. */
	pbapc_aborted,
	/*! Client is not idle, so cannot perform the current operation. */
	pbapc_not_idle,
	/*! Operation failed due to being in the wrong state.*/
	pbapc_wrong_state,
	/*! Unable to register the SDP record due to a lack of resources */
	pbapc_sdp_failure_resource,
	/*! Unable to register the SDP record due to Bluestack */
	pbapc_sdp_failure_bluestack,
	/*! Remote host has disconnected or the link has been lost. */
	pbapc_remote_disconnect,
	
	/*! Not authorised to access this phonebook */
	pbapc_spb_unauthorised = 0x10,
	/*! The server does not contain this repository */
	pbapc_spb_no_repository,
	/*! Phonebook does not exist */
	pbapc_spb_not_found,
	
	/*! No resources to generate application specific parameters header for PullvCardList. */
	pbapc_vcl_no_param_resources = 0x20,
	/*! A phonebook folder was specified for PullvCardList where there are no sub-folders (i.e. in pb). */
	pbapc_vcl_no_pbook_folder,
	/*! A phonebook folder was specified for PullvCardList which is invalid */
	pbapc_vcl_invalid_pbook,
	
	/*! No resources to generate application specific parameters header for PullvCardEntry. */
	pbapc_vce_no_param_resources = 0x30,
	/*! No resources to generate the vCard entry name for PullvCardEntry. */
	pbapc_vce_no_name_resources,
	/*! Invalid entry for this phonebook for PullvCardEntry.  Only folder 'pb' can contain an entry 0. */
	pbapc_vce_invalid_entry,
	
	/*! No resources to generate application specific parameters header for PullPhonebook. */
	pbapc_ppb_no_param_resources = 0x40,
	/*! No resources to generate the phonebook name for PullPhonebook. */
	pbapc_ppb_no_name_resources,
	/*! No name for PullPhonebook when it is required.  e.g. server is not in a phonebook directory */
	pbapc_ppb_no_required_name,
	/*! The server does not contain this repository */
	pbapc_ppb_no_repository,
	
	/*! Request to get the server properties failed due to an SDP error */
	pbapc_prop_sdp_error,

	pbapc_end_of_status_list
} pbapc_lib_status;

/*
   @brief Upstream Messages for the PBAP Client Library   
*/

/*
	Do not document this enum.
*/
#ifndef DO_NOT_DOCUMENT
typedef enum
{
	/* Session Control */
	PBAPC_INIT_CFM = PBAPC_MESSAGE_BASE,
	PBAPC_REG_SDP_CFM,
	
	PBAPC_CONNECT_CFM,
	PBAPC_AUTH_REQUEST_IND,
	PBAPC_DISCONNECT_IND,
	
	PBAPC_SET_PHONEBOOK_CFM,
	
	PBAPC_PULL_VCARD_LIST_START_IND,
	PBAPC_PULL_VCARD_LIST_DATA_IND,
	PBAPC_PULL_VCARD_LIST_COMPLETE_IND,
	
	PBAPC_PULL_VCARD_ENTRY_START_IND,
	PBAPC_PULL_VCARD_ENTRY_DATA_IND,
	PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND,
	
	PBAPC_PULL_PHONEBOOK_START_IND,
	PBAPC_PULL_PHONEBOOK_DATA_IND,
	PBAPC_PULL_PHONEBOOK_COMPLETE_IND,
	
	PBAPC_SERVER_PROPERTIES_COMPLETE_IND,
	
	PBAPC_MESSAGE_TOP
} PbapcMessageId;
#endif

/*!
	@brief This message returns the result of an PbapcInit attempt.
*/
typedef struct
{
	/*!< The current status of the PBAPC library. */
    pbapc_lib_status status;
} PBAPC_INIT_CFM_T;

/*!
	@brief This message returns the result of an attempt to register the SDP Record.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< The current status of the PBAPC library. */
    pbapc_lib_status status;
} PBAPC_REG_SDP_CFM_T;

/*!
	@brief This message returns the result of an PbapcConnect attempt.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
    /* Server Bluetooth address */
    bdaddr bdAddr;
	/*! The current status of the PBAPC library. */
    pbapc_lib_status status;         
	/*! Maximum size of packet transferable during this session. Equals
	  min(client_packet_size , server_packet_size). */
	uint16          packetSize;		
} PBAPC_CONNECT_CFM_T;


/*!
	@brief This message is received when the remote server requests authentication during connection.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;	    
	uint8 nonce[GOEP_SIZE_DIGEST];
	uint8 options;
	uint16 size_realm;
	uint8 realm[1];
} PBAPC_AUTH_REQUEST_IND_T;

/*!
	@brief This message returns the result of an PbapcDisconnect attempt.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
    /*!< number of pbapc links after this disconnection */
    uint16 num_of_pbapc_links;
    /*!< The current status of the PBAPC library. */
    pbapc_lib_status status;
} PBAPC_DISCONNECT_IND_T;

/*!
	@brief This message returns the result of an PbapcSetPhonebook attempt.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< The current status of the PBAPC library. */
    pbapc_lib_status status;
} PBAPC_SET_PHONEBOOK_CFM_T;

/*!
	@brief This message returns the result of an PbapcPullvCardListingStart attempt.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< Source containing the data. */
    Source  src;
	/*!< Size of the phonebook. */
	uint16	pbook_size;
	/*!< New Missed Calls. */
	uint8	new_missed;
	/*! Total length of the listing if known. */
	uint32  totalLength; 	
	/*!< Data packet for the folder contents. */
	uint16  dataLen;
	/*!< Offset into the stream of the packet start. */
	uint16  dataOffset;
	/*!< Is there any more data. */
    bool    moreData;
} PBAPC_PULL_VCARD_LIST_START_IND_T;

/*!
	@brief This message returns the result of an PbapcPullvCardListingNext attempt.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< Source containing the data. */
    Source  src;
	/*!< Data packet for the folder contents. */
	uint16  dataLen;
	/*!< Offset into the stream of the packet start. */
	uint16  dataOffset;
	/*!< Is there any more data. */
    bool    moreData;
} PBAPC_PULL_VCARD_LIST_DATA_IND_T;

/*!
	@brief This message is sent when a PullvCardList function completes.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< The current status of the PBAPC library. */
    pbapc_lib_status status;
} PBAPC_PULL_VCARD_LIST_COMPLETE_IND_T;

/*!
	@brief This message returns the result of an PbapcPullvCardEntryStart attempt.
*/
typedef struct
{
	/*!< PBAPC Session Handle.*/
    uint16 device_id;
	/*!< Source containing the data. */
    Source  src;
	/*! Total length of the listing if known. */
	uint32  totalLength; 	
	/*!< Data packet for the folder contents. */
	uint16  dataLen;
	/*!< Offset into the stream of the packet start. */
	uint16  dataOffset;
	/*!< Is there any more data. */
    bool    moreData;
} PBAPC_PULL_VCARD_ENTRY_START_IND_T;

/*!
	@brief This message returns the result of an PbapcPullvCardEntryNext attempt.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< Source containing the data. */
    Source  src;
	/*!< Data packet for the folder contents. */
	uint16  dataLen;
	/*!< Offset into the stream of the packet start. */
	uint16  dataOffset;
	/*!< Is there any more data. */
    bool    moreData;
} PBAPC_PULL_VCARD_ENTRY_DATA_IND_T;

/*!
	@brief This message is sent when a PullvCardEntry function completes.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< The current status of the PBAPC library. */
    pbapc_lib_status status;
} PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND_T;

/*!
	@brief This message returns the result of an PbapcPullPhonebookStart attempt.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< Source containing the data. */
    Source  src;
	/*!< Size of the phonebook. */
	uint16	pbook_size;
	/*!< New Missed Calls. */
	uint8	new_missed;
	/*! Total length of the listing if known. */
	uint32  totalLength; 	
	/*!< Data packet for the folder contents. */
	uint16  dataLen;
	/*!< Offset into the stream of the packet start. */
	uint16  dataOffset;
	/*!< Is there any more data. */
    bool    moreData;
} PBAPC_PULL_PHONEBOOK_START_IND_T;

/*!
	@brief This message returns the result of an PbapcPullvPhonebookNext attempt.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< Source containing the data. */
    Source  src;
	/*!< Data packet for the folder contents. */
	uint16  dataLen;
	/*!< Offset into the stream of the packet start. */
	uint16  dataOffset;
	/*!< Is there any more data. */
    bool    moreData;
} PBAPC_PULL_PHONEBOOK_DATA_IND_T;

/*!
	@brief This message is sent when a PullPhonebook function completes.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< The current status of the PBAPC library. */
    pbapc_lib_status status;
} PBAPC_PULL_PHONEBOOK_COMPLETE_IND_T;

/*!
	@brief This message is received when a PbapcGetServerProperties request completes.
*/
typedef struct
{
	/*!< PBAPC Server ID.*/
    uint16 device_id;
	/*!< The current status of the PBAPC library. */
    pbapc_lib_status status;
	/*! Repositories supported by the server */
	uint8 repositories;
} PBAPC_SERVER_PROPERTIES_COMPLETE_IND_T;

/*   Downstream API for the PBAP Client Library   */

/*!
	@brief Initialise the PBAP Client Library.
	@param theAppTask The current application task.
	@param priority The profile instance low power mode priority. For a set of
	profile instances with connections on a given ACL the instance with the
	highest priority value wins and the low power mode on the ACL is set
	according to it's power table.
	
	PBAPC_INIT_CFM message will be received by the application. 
*/
void PbapcInit(Task theAppTask, uint16 priority);

/*!
	@brief Register the client side SDP Record
	@param device_id PBAPC Session Handle.
	@param feature Features supported by the client.  Mask values
	defined in pbap_common.h.
	
	PBAPC_REG_SDP_CFM message will be received by the application. 
*/
void PbapcRegisterSDP(uint16 device_id, uint8 feature);	

/*!
	@brief Open an PBAPC Connection with a server.
	@param bd_addr The Bluetooth address of the device being replied to.
	@param maxPacketSize Maximum packet size supported by this client.
	
	This will make a Bluetooth connection with the server.
	
	PBAPC_CONNECT_CFM message will be received by the application.
*/
void PbapcConnect(const bdaddr *bdAddr, const uint16 maxPacketSize);

/*!
	@brief Disconnect from an server.  
	@param device_id PBAPC Session Handle.
	
	PBAPC_DISCONNECT_CFM message will be received by the application. 
*/
void PbapcDisconnect(uint16 device_id);

/*
	@brief Respond to an authentication challenge during connect.
	@param device_id PBAPC Session Handle.
	@param digest MD5 Digest response. Must be a 16byte string.
	@param size_userid Length of the optional User Id string.
	@param userid Option User Id string.
	@param nonce Optional MD5 Digest Nonce as sent in the Challenge. Must be a 16byte string or NULL.
	
	This function can be called by a PBAP Client on receipt of a PBAPC_AUTH_REQUEST_IND message to reply
	to an authentication challenge.
	A PBAPC_CONNECT_CFM message will be received to indicate that the connection process
	has completed.
*/
void PbapcConnectAuthResponse(uint16 device_id, const uint8 *digest, uint16 size_userid, const uint8 *userid, const uint8 *nonce);

/*!
	@brief Abort the current multi-packet operation.
	@param device_id PBAPC Session Handle.
	
	The relevant COMPLETE_IND message will be received with the status code of pbapc_aborted on success.
*/
void PbapcAbort(uint16 device_id);

/*!
	@brief Change the current active phonebook.  
	@param device_id PBAPC Session Handle.
	@param repository Repository containing the new phonebook.
	@param phonebook The new phonebook.
	
	To change to a different phonebook in the same repository, use pbap_current as the
	repository.
	
	PBAPC_SET_PHONEBOOK_CFM message will be received by the application. 
*/
void PbapcSetPhonebook(uint16 device_id, pbap_phone_repository repository, pbap_phone_book phonebook);

/*!
	@brief The packet received has been processed and is no longer needed.
	@param device_id PBAPC Session Handle (as returned in PBAPC_INIT_CFM).
	
	Every packet send to the client that contains a source must be declared
	complete before the next function is called.  e.g. When a
	PBAPC_PULL_VCARD_LIST_START_IND has been received, PbapcPacketComplete must be
	called before calling PbapcPullvCardListingNext.
	
	No message is received on completion.
*/
void PbapcPacketComplete(uint16 device_id);

/*!
	@brief Start to get the vCard listing for the current phonebook.
	@param device_id PBAPC Session Handle.
	@param order Result sort order. Use 'pbap_order_default' for default sort order.
	@param pbook Phonebook folder to retrieve. Use 'pbap_b_unknown' for the current folder.
	@param srchVal Value to search for.
	@param size_srchVal Length of the search value.
	@param srchAttr Attribute to search. Use 'pbap_a_unknown' for default search attribute.
	@param maxList Maximum List Count.
	@param listStart List Start Offset.
	
	The ownership of the pointer 'srchVal' remains with the caller and must remain valid until a 
	PBAPC_PULL_VCARD_LIST_START_IND message has been received.
	
	A PBAPC_PULL_VCARD_LIST_START_IND message will be received by the appliction when the first packet
	arrives.  A PBAPC_PULL_VCARD_LIST_COMPLETE_IND message will be received after the last packet is
	received.
*/
void PbapcPullvCardListingStart(uint16 device_id, pbap_order_values order, pbap_phone_book pbook, const uint8 *srchVal,
								uint16 size_srchVal, pbap_search_values srchAttr, uint16 maxList, uint16 listStart);

/*!
	@brief Get the next vCard listing packet for the current phonebook.
	@param device_id PBAPC Session Handle.
	
	A PBAPC_PULL_VCARD_LIST_DATA_IND message will be received by the appliction for each packet after
	the first.  A PBAPC_PULL_VCARD_LIST_COMPLETE_IND message will be received after the last packet is
	received.
*/
void PbapcPullvCardListingNext(uint16 device_id);

/*!
	@brief Start to get a vCard entry from the current phonebook.
	@param device_id PBAPC Session Handle.
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
void PbapcPullvCardEntryStart(uint16 device_id, uint16 entry, uint32 filter_lo, uint32 filter_hi, pbap_format_values format);

/*!
	@brief Get the next vCard entry packet from the current phonebook.
	@param device_id PBAPC Session Handle.
	
	A PBAPC_PULL_VCARD_ENTRY_DATA_IND message will be received by the appliction for each packet after
	the first.  A PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND message will be received after the last packet is
	received.
*/
void PbapcPullvCardEntryNext(uint16 device_id);

/*!
	@brief Start a PullPhonebook Function
	@param device_id PBAPC Session Handle.
	@param repository Repository containing the phonebook to PULL
	@param phonebook Phonebook to PULL
	@param filter_lo PBAP Parameter filter mask low 32 bits.  Use zero to not sent the parameter.
	@param filter_hi PBAP Parameter filter mask high 32 bits.  Use zero to not sent the parameter.
	@param format PBAP Parameter format.
	@param maxList Maximum List Count.
	@param listStart List Start Offset.
	
	If the filter does not include the manditory fields as required by the specified format, these
	will be added.
	
	A PBAPC_PULL_PHONEBOOK_START_IND message will be received by the appliction when the first packet
	arrives.  A PBAPC_PULL_PHONEBOOK_COMPLETE_IND message will be received after the last packet is
	received.
*/
void PbapcPullPhonebookStart(uint16 device_id, pbap_phone_repository repository, pbap_phone_book phonebook,
							 	uint32 filter_lo, uint32 filter_hi, pbap_format_values format, uint16 maxList, uint16 listStart);

/*!
	@brief Get the next Phonebook packet from the current phonebook.
	@param device_id PBAPC Session Handle.
	
	A PBAPC_PULL_VCARD_ENTRY_DATA_IND message will be received by the appliction for each packet after
	the first.  A PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND message will be received after the last packet is
	received.
*/
void PbapcPullPhonebookNext(uint16 device_id);

/*!
	@brief Obtain the properties of the remote server.
	@param device_id PBAPC Session Handle.
	
	Perform an SDP search of the server to obtain the supported repositories mask.
	
	Completes with a PBAPC_SERVER_PROPERTIES_COMPLETE_IND.
*/
void PbapcGetServerProperties(uint16 device_id);

/*!
	@brief Obtain the device_id of Pbap Link from BT address.
	
	Completes with a device_id is returned.
*/
uint16 PbapcLinkFrmAddr(const bdaddr *bdAddr);


#endif /* PBAP_CLIENT_H_ */

