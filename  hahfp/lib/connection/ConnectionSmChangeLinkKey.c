/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    ConnectionSmChangeLinkKey.c        

DESCRIPTION
    This file contains the management entity responsible for device security

NOTES

*/


/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"
#include "dm_security_auth.h"

#include    <message.h>
#include    <string.h>
#include    <vm.h>


/*****************************************************************************/
void ConnectionSmChangeLinkKey(Sink sink)
{   
    MAKE_CL_MESSAGE(CL_INTERNAL_SM_CHANGE_LINK_KEY_REQ);
    message->sink = sink;
    MessageSend(connectionGetCmTask(), CL_INTERNAL_SM_CHANGE_LINK_KEY_REQ, message);
}

