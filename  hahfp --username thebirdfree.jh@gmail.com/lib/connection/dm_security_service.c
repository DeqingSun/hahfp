/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    dm_security_service.c        

DESCRIPTION
    This file contains the management entity responsible for device security

NOTES

*/


/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"

#include    <message.h>
#include    <string.h>
#include    <vm.h>


/*****************************************************************************/
void ConnectionSmRegisterIncomingService(dm_protocol_id protocol_id, uint32 channel, dm_security_in security)
{
    /* Check params are within allowed values - debug build only */
#ifdef CONNECTION_DEBUG_LIB
    if ((protocol_id != protocol_l2cap) && (protocol_id != protocol_rfcomm))
    {
        CL_DEBUG(("Out of range protocol id 0x%x\n", protocol_id));
    }

    if((protocol_id == protocol_rfcomm) && ((channel < RFCOMM_SERVER_CHANNEL_MIN) || (channel > RFCOMM_SERVER_CHANNEL_MAX)))
    {
        CL_DEBUG(("Out of range RFCOMM server channel 0x%lx\n", channel));
    }
    
#endif

    {
    MAKE_CL_MESSAGE(CL_INTERNAL_SM_REGISTER_REQ);
    message->protocol_id = protocol_id;
    message->channel = channel;
    message->outgoing_ok = FALSE;   
    message->security_level = security;
    message->psm = 0;
    MessageSend(connectionGetCmTask(), CL_INTERNAL_SM_REGISTER_REQ, message);
    }
}


/*****************************************************************************/
void ConnectionSmUnRegisterIncomingService(dm_protocol_id protocol_id, uint32 channel)
{
#ifdef CONNECTION_DEBUG_LIB
    if ((protocol_id != protocol_l2cap) && (protocol_id != protocol_rfcomm))
    {
        CL_DEBUG(("Out of range protocol id 0x%x\n", protocol_id));
    }

    if((protocol_id == protocol_rfcomm) && ((channel < RFCOMM_SERVER_CHANNEL_MIN) || (channel > RFCOMM_SERVER_CHANNEL_MAX)))
    {
        CL_DEBUG(("Out of range RFCOMM server channel 0x%lx\n", channel));
    }
#endif

    {
    MAKE_CL_MESSAGE(CL_INTERNAL_SM_UNREGISTER_REQ);
    message->protocol_id = protocol_id;
    message->channel = channel;
    MessageSend(connectionGetCmTask(), CL_INTERNAL_SM_UNREGISTER_REQ, message);
    }
}



