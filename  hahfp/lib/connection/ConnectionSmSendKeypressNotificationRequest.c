/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    ConnectionSmSendKeypressNotificationRequest.c        

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
void ConnectionSmSendKeypressNotificationRequest(const bdaddr* bd_addr, cl_sm_keypress_type type)
{
    MAKE_CL_MESSAGE(CL_INTERNAL_SM_SEND_KEYPRESS_NOTIFICATION_REQ);
    message->bd_addr = *bd_addr;
    message->type = type;
    MessageSend(connectionGetCmTask(), CL_INTERNAL_SM_SEND_KEYPRESS_NOTIFICATION_REQ, message);
}
