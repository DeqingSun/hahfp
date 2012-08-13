/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    pbapc_private.h
    
DESCRIPTION
	PhoneBook Access Profile Client Library - private header.

*/

#ifndef	PBAPC_PRIVATE_H_
#define	PBAPC_PRIVATE_H_

#include <stdlib.h>
#include <panic.h>
#include <message.h>

#include "goep.h"

/* Macros for creating messages */
#define MAKE_PBAPC_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);

#define PBAPC_NO_PAYLOAD      (0x0)

#ifdef PBAPC_LIBRARY_DEBUG
	#include <panic.h>
	#include <stdio.h>
	#define PBAPC_DEBUG(x) {printf x; Panic();}
	#define PBAPC_ASSERT(c, x) { if (!(c)) { printf x; Panic();} }
#else
	#define PBAPC_DEBUG(x)
	#define PBAPC_ASSERT(c, x)
#endif

/* Currently running PBAP Client Command */
typedef enum
{
    pbapc_com_None,
	pbapc_com_RegSdp,
    pbapc_com_Connect,
    pbapc_com_ObexPassword,
    pbapc_com_Disconnect,
	
    pbapc_com_SetPhonebook,
	
	pbapc_com_PullvCardList,
	pbapc_com_PullvCard,
	pbapc_com_PullPhonebook,
	
	pbapc_com_GetProperties,
	
	pbapc_com_EOL
} pbapc_running_command;

/* Set Phonebook state machine */
typedef enum
{
    pbapc_spb_None,
    pbapc_spb_GotoRoot,
    pbapc_spb_GotoParent,
    pbapc_spb_GotoRepository,
    pbapc_spb_GotoTelecom,
    pbapc_spb_GotoPhonebook,
	
	pbapc_spb_EOL
} pbapc_set_phonebook_state;

/*!
    @brief Pbap link priority is used to identify different pbapc links to
    AG devices using the order in which the devices were connected.
*/
typedef enum
{
    /*! The link that was connected first. */
    pbapc_primary_link,
    /*! The link that was connected second. */
    pbapc_secondary_link,    
    /*! Invalid Link. */
    pbapc_invalid_link
} pbapc_link_priority;

/*!
    @brief Pbap connection state.
*/
typedef enum
{
    /*! The link was disconnected. */
    pbapc_disconnected,
    /*! The link was connecting. */
    pbapc_connecting,    
    /*! The link was connected. */
    pbapc_connected,
    /*! The link was disconnecting. */
    pbapc_disconnecting
} pbapc_connect_state;

typedef struct 
{
	/* Task associated with this session */
   	TaskData  task;
    /* Server Bluetooth address */
    bdaddr    bdAddr;
    /* Device id for this pbapc connection */
    unsigned device_id : 3;
    unsigned connect_state : 3;

    unsigned unused: 2;
	unsigned svr_repos: 8;    
	/* Maximum packet length for this session */
	uint16 packetSize;
    /* Currently running PBAPC command */
    pbapc_running_command currCom;
    /* Goep Handle */
    GOEP *handle;
	/* Server channel */
	unsigned int rfcChan:8;
	/* Features supported by this client */
	unsigned int features:8;
	
	pbap_phone_repository curr_repository;
	pbap_phone_book curr_phonebook;
	pbap_phone_repository target_repository;
	pbap_phone_book target_phonebook;
	
	pbapc_set_phonebook_state setPBState;
	pbapc_lib_status setPBError;
	
	void *params;
	uint8 *name;
} pbapcState;

/*!
    @brief Pbapc link data is used to store the pbapc link related data.
*/
#define PBAPC_MAX_REMOTE_DEVICES    2       /* Maximum number of separate remote devices supported */

struct __PBAPC
{
    /* Task associated with this session */
	Task      theAppTask;

    pbapcState pbapc_link[PBAPC_MAX_REMOTE_DEVICES];

    unsigned num_of_pbapc_connected:3;
};

typedef struct __PBAPC pbapcClient;

/* Provide a reference for all A2DP lib modules - should not be exposed at API level */
extern pbapcClient *pbapc;

enum
{
	PBAPC_INT_DUMMY_FIRST,
	PBAPC_INT_REG_SDP,

	PBAPC_INT_CONNECT,
	PBAPC_INT_AUTH_RESP,
	PBAPC_INT_DISCONNECT,
	PBAPC_INT_ABORT,
	
	PBAPC_INT_SET_PHONEBOOK,
	
	PBAPC_INT_GET_VCARD_LIST_START,
	PBAPC_INT_GET_VCARD_START,
	PBAPC_INT_GET_PHONEBOOK_START,
	
	PBAPS_PULL_NEXT,
	
	PBAPC_INT_GET_SERVER_PROPERTIES,
			
    PBAPC_INT_ENDOFLIST
};

/* Internal Message Structures */
typedef struct
{
    bdaddr	bdAddr;
    uint16	maxPacketSize;    
} PBAPC_INT_CONNECT_T;

typedef struct
{
	const uint8 *digest;
	uint16 size_userid;
	const uint8 *userid;
	const uint8 *nonce;
} PBAPC_INT_AUTH_RESP_T;


typedef struct
{
    uint8	feature;    
} PBAPC_INT_REG_SDP_T;

typedef struct
{
	pbap_phone_repository repository;
	pbap_phone_book phonebook;
} PBAPC_INT_SET_PHONEBOOK_T;

typedef struct
{
	pbap_order_values order;
	pbap_phone_book pbook;
	const uint8 *srchVal;
	uint16 size_srchVal;
	pbap_search_values srchAttr;
	uint16 maxList;
	uint16 listStart;
} PBAPC_INT_GET_VCARD_LIST_START_T;

typedef struct
{
	uint16 entry;
	uint32 filter_lo;
	uint32 filter_hi;
	pbap_format_values format;
} PBAPC_INT_GET_VCARD_START_T;

typedef struct
{
	pbap_phone_repository repository;
	pbap_phone_book phonebook;
	uint32 filter_lo;
	uint32 filter_hi;
	pbap_format_values format;
	uint16 maxList;
	uint16 listStart;
} PBAPC_INT_GET_PHONEBOOK_START_T;

typedef struct
{
	pbapc_running_command command;
} PBAPS_PULL_NEXT_T;

/* Structure to store the parameters for a PullvCardEntry function */
typedef struct
{
	uint32 filter_lo;
	uint32 filter_hi;
	pbap_format_values format;
} PBAPC_INT_GET_VCARD_START_PARAMS_T;

typedef struct
{
	uint32 filter_lo;
	uint32 filter_hi;
	pbap_format_values format;
	uint16 maxList;
	uint16 listStart;
} PBAPC_INT_GET_PHONEBOOK_START_PARAMS_T;

/****************************************************************************
NAME	
    pbapcIntHandler

DESCRIPTION
    Handler for messages received by the PBABC Task.
*/
void pbapcIntHandler(Task task, MessageId id, Message message);


/****************************************************************************
NAME	
    pbabcGoepHandler

DESCRIPTION
    Handler for messages received by the PBABC Task from GOEP.
*/
void pbapcGoepHandler(pbapcState *state, MessageId id, Message message);

/* Start a state for Set Phonebook
   	Will make the relevant set path call to GOEP
*/
void pbapc_spb_startState(pbapcState *state);

/* Complete a state for Set Phonebook, and start the next one
   
   returns TRUE on complete
*/
bool pbapc_spb_completeState(pbapcState *state);

/* Register the Client Side SDP Record with Bluestack */
void pbapcRegisterSdpRecord(pbapcState *state);

/* Message send functions */
void pbapcMsgRegSDPCfm(pbapcState *state, pbapc_lib_status status);
void pbapcMsgSendConnectCfm(pbapcState *state, pbapc_lib_status status, uint16 pktSize);
void pbapcMsgSendDisConnectCfm(pbapcState *state, pbapc_lib_status status);
void pbapcMsgSendSetPhonebookCfm(pbapcState *state, pbapc_lib_status status);

void pbapcMsgSendPullvCardListStartInd(pbapcState *state, Source src, uint16 pbook_size, 
										uint8 new_missed, uint32 totalLength, uint16 dataLen, 
										uint16 dataOffset, bool moreData);
void pbapcMsgSendPullvCardListDataInd(pbapcState *state, Source src, uint16 dataLen, 
										uint16 dataOffset, bool moreData);
void pbapcMsgSendPullvCardListCompleteInd(pbapcState *state, pbapc_lib_status status);

void pbapcMsgSendPullvCardEntryStartInd(pbapcState *state, Source src, 
										uint32 totalLength, uint16 dataLen, 
										uint16 dataOffset, bool moreData);
void pbapcMsgSendPullvCardEntryDataInd(pbapcState *state, Source src, uint16 dataLen, 
										uint16 dataOffset, bool moreData);
void pbapcMsgSendPullvCardEntryCompleteInd(pbapcState *state, pbapc_lib_status status);

void pbapcMsgSendPullPhonebookStartInd(pbapcState *state, Source src, uint16 pbook_size, 
										uint8 new_missed, uint32 totalLength, uint16 dataLen, 
										uint16 dataOffset, bool moreData);
void pbapcMsgSendPullPhonebookDataInd(pbapcState *state, Source src, uint16 dataLen, 
										uint16 dataOffset, bool moreData);
void pbapcMsgSendPullPhonebookCompleteInd(pbapcState *state, pbapc_lib_status status);

void pbapcMsgSendGetCompleteInd(pbapcState *state, pbapc_running_command command, pbapc_lib_status status);

void pbapcMsgSendGetServerPropertiesCompleteInd(pbapcState *state, pbapc_lib_status status, uint8 repositories);

/* Functions to manipulate Application specific paramters to a GEOP packet */
uint16 pbapcAddvCardListHeaders(pbapcState *state, Sink sink);
uint16 pbapcAddvCardEntryHeaders(pbapcState *state, Sink sink);
uint16 pbapcAddPhonebookHeaders(pbapcState *state, Sink sink);
void pbapcExtractApplicationParameters(pbapcState *state, GOEP_LOCAL_GET_START_HDRS_IND_T *msg);

#endif /* PBAPC_PRIVATE_H_ */

