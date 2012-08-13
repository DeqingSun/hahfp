/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    dm_info_remote_version.c        

DESCRIPTION
    This file contains the management entity responsible for arbitrating 
    access to functionality in BlueStack that provides information on
    the setup of the local device or about the link to the remote device.

NOTES

*/


/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"
#include "sink.h"

/*****************************************************************************/
void ConnectionReadRemoteVersion(Task theAppTask, Sink sink)
{
     /* All requests are sent through the internal state handler */    
    bdaddr addr;
     MAKE_CL_MESSAGE(CL_INTERNAL_DM_READ_REMOTE_VERSION_REQ);
     message->theAppTask = theAppTask;
     
    /* Get bdaddr from sink */
    if (!SinkGetBdAddr(sink, &addr))
    {
        message->bd_addr.lap = 0;
        message->bd_addr.nap = 0;
        message->bd_addr.uap = 0;
    }
    else
    {
        message->bd_addr = addr;
    }
    
     MessageSend(connectionGetCmTask(), CL_INTERNAL_DM_READ_REMOTE_VERSION_REQ, message);
}

void ConnectionReadRemoteVersionBdaddr(Task theAppTask, const bdaddr *addr)
{
    /* All requests are sent through the internal state handler */    
    MAKE_CL_MESSAGE(CL_INTERNAL_DM_READ_REMOTE_VERSION_REQ);
    message->theAppTask = theAppTask;
    message->bd_addr = *addr;
    MessageSend(connectionGetCmTask(), CL_INTERNAL_DM_READ_REMOTE_VERSION_REQ, message);
}

