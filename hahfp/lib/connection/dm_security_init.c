/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    dm_security_init.c        

DESCRIPTION
    This file contains the functions to initialise the SM component of 
    the connection library    

NOTES

*/


/****************************************************************************
    Header files
*/
#include    "connection.h"
#include    "connection_private.h"
#include    "init.h"
#include    "dm_security_init.h"

#include    <vm.h>


/****************************************************************************
NAME
    connectionSmInit

DESCRIPTION
    This Function is called to initialise SM. The config option is not 
    currently used but may be in future.

RETURNS
    Nothing.
*/
void connectionSmInit(cl_dm_bt_version version, connectionSmState *smState)
{
    MAKE_CL_MESSAGE(CL_INTERNAL_SM_INIT_REQ);

    message->options            =  DM_SM_INIT_SECURITY_MODE | 
                                   DM_SM_INIT_MODE3_ENC;

    message->config             =  0;
    message->write_auth_enable  =  cl_sm_wae_acl_owner_none;

    if(version >= bluetooth2_1)
    {
        message->options            |=  DM_SM_INIT_WRITE_AUTH_ENABLE;

        message->security_mode      =   sec_mode4_ssp;
        message->mode3_enc          =   hci_enc_mode_pt_to_pt_and_bcast;

    }    
    else
    {
        message->security_mode      =   sec_mode2_service;
        message->mode3_enc          =   hci_enc_mode_pt_to_pt;
    }

    smState->security_mode = message->security_mode;
    smState->enc_mode = message->mode3_enc;
    
    MessageSend(connectionGetCmTask(), CL_INTERNAL_SM_INIT_REQ, message);
}
