/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    ConnectionReadRemoteName.c        

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
void ConnectionReadRemoteName(Task theAppTask, const bdaddr *bd_addr)
{
   MAKE_CL_MESSAGE(CL_INTERNAL_DM_READ_REMOTE_NAME_REQ);
   message->theAppTask = theAppTask;
   message->bd_addr = *bd_addr;
   MessageSend(connectionGetCmTask(), CL_INTERNAL_DM_READ_REMOTE_NAME_REQ, message);
}
