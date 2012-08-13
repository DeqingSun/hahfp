/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2010
Part of HeadsetSDK-Stereo R110.0 

FILE NAME
    obex_client.c
    
DESCRIPTION
    This file defines all API functions for a OBEX client session.
    The library allows creation of multiple OBEX session to multiple 
    devices. It is the applications responsibility to limit the number of 
    sessions for optimal resource usage.
*/

#include "obex_extern.h"
#include <stdlib.h>

/****************************************************************************
 * NAME
 *  ObexConnectResponse
 * 
 * DESCRIPTION
 *  API to establish a OBEX server session 
 *
 * PARAMETERS
 *  Refer obex.h
 *
 * RETURNS
 **************************************************************************/
Task ObexConnectResponse( Sink						sink,
                          uint8                 rfcChannel,
                          bool                  accept,
                          const ObexConnParams* connParams)  
{
    Obex sessionTask;

    if( accept && connParams )
    {    
        /* Create OBEX session Task */
        sessionTask = obexCreateSession( connParams->connTaskData,
                                         connParams->sizeConnTask,
                                         obex_server,
                                         connParams->sizeTarget,
                                         connParams->target );

        return obexSessionResp( sessionTask, TRUE, 
                                connParams->auth, sink, 
                                rfcChannel );
    }
    else
    {
        return obexSessionResp( NULL, FALSE, FALSE, sink, rfcChannel );
    }    
}

/**************************************************************************
 * NAME
 *  ObexPutResponse
 * 
 * DESCRIPTION
 *  API to send a PUT response
 *
 * PARAMETERS
 *  Refer obex.h
 **************************************************************************/
void ObexPutResponse( Obex session, ObexResponse response )
{
    OBEX_ASSERT( session );
    obexSendResponse( session, response );
}

/**************************************************************************
 * NAME
 *  ObexGetResponse
 * 
 * DESCRIPTION
 *  API to send a GET response
 *
 * PARAMETERS
 *  Refer obex.h
 **************************************************************************/
void ObexGetResponse( Obex session, ObexResponse response )
{
    OBEX_ASSERT( session );
    obexSendResponse( session, response );
}

/**************************************************************************
 * NAME
 *  ObexSetPathResponse
 * 
 * DESCRIPTION
 *  API to send a SETPATH response
 *
 * PARAMETERS
 *  Refer obex.h
 **************************************************************************/
void ObexSetPathResponse( Obex session, ObexResponse response )
{
    OBEX_ASSERT( session );
    if(response == obex_continue ) response = obex_remote_success;

    obexSendResponse( session, response );
}
