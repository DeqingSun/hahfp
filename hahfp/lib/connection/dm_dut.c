/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    dm_dut.c        

DESCRIPTION    

NOTES

*/


/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"


/*****************************************************************************/
void ConnectionEnterDutMode(void)
{    
    /* All requests are sent through the internal state handler */    
    MessageSend(connectionGetCmTask(), CL_INTERNAL_DM_DUT_REQ, 0);
}

