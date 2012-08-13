/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2010
Part of HeadsetSDK-Stereo R110.0 

FILE NAME
    obex_common.c

DESCRIPTION
    This file defines all API function common for Client and Server. All these
    functions are utility functions and it immediately returns a result. 
*/

#include <source.h>
#include <stdlib.h>
#include <panic.h>
#include <stream.h>
#include "obex_extern.h"
#include "obex_private.h"

/**********************************************************************
 * NAME
 *  ObexAuthReqResponse
 *
 * DESCRIPTION 
 *  API to send the OBEX authentication challenge to the remote device
 *
 * PARAMETERS
 *  Refer obex.h
 ************************************************************************/
void ObexAuthReqResponse( Obex session,
                          const uint8* nonce,
                          ObexAuthOptions options,
                          uint16 realmLen,
                          const uint8* realm )
{
    uint8* digest;
    uint16 len = OBEX_TOTAL_DIGEST_SIZE;
    OBEX_ASSERT( session );
  
    if( !IsObexConnecting( session ) ) return;
    if( !nonce )
    {
        obexSendAuthPacket( session,  OBEX_AUTH_CHALLENGE, 0, 0 );
        return;               
    }


    if( realmLen > OBEX_MAX_VAL_SIZE) realmLen =  OBEX_MAX_VAL_SIZE; 
    if( options ) len +=  OBEX_TOTAL_OPTIONS_SIZE;
    if( realm )   len +=  realmLen + OBEX_TLV_HDR_SIZE;

    digest = PanicUnlessMalloc( len );

    len = obexFrameTLVHeader( digest, OBEX_REQ_NONCE_TAG, 
                                 OBEX_SIZE_DIGEST, nonce );

    if(options)
    {
        len += obexFrameTLVHeader( digest+len, OBEX_REQ_OPTIONS_TAG,
                                     1, (const uint8*)&options); 
    }

    if(realm)
    {
        len += obexFrameTLVHeader( digest+len, OBEX_REQ_REALM_TAG,
                                    realmLen, realm ); 
    }    
   
    obexSendAuthPacket( session,
                        OBEX_AUTH_CHALLENGE, len, 
                        StreamRegionSource( digest, len ) ); 

    free( digest );
    
}

/**********************************************************************
 * NAME
 *  ObexAuthClgResponse
 *
 * DESCRIPTION 
 *  API to send the OBEX authentication response to the remote device
 *
 * PARAMETERS
 *  Refer obex.h
 ************************************************************************/
void ObexAuthClgResponse( Obex session,
                          const uint8* reqDigest, 
                          uint8 userIdLen,
                          const uint8* userId )
{
    uint8* digest;
    uint16 len = OBEX_TOTAL_DIGEST_SIZE;
    OBEX_ASSERT( session );
    OBEX_ASSERT( reqDigest );

    if( !IsObexConnecting( session ) ) return;
    if( userIdLen > OBEX_MAX_VAL_SIZE) userIdLen =  OBEX_MAX_VAL_SIZE; 
    if( userId ) len += userIdLen + OBEX_TLV_HDR_SIZE;

    digest = PanicUnlessMalloc( len );

    len = obexFrameTLVHeader( digest, OBEX_RSP_DIGEST_TAG, 
                                 OBEX_SIZE_DIGEST, reqDigest );

    if( userId )
    {
        len += obexFrameTLVHeader( digest+len, OBEX_RSP_USRID_TAG,
                                   userIdLen, userId ); 
    }

    obexSendAuthPacket( session,
                        OBEX_AUTH_RESPONSE, len, 
                        StreamRegionSource( digest, len ) ); 

    free( digest );
}

/**********************************************************************
 * NAME
 *  ObexDeleteSessionResponse 
 *
 * DESCRIPTION 
 *  API to delete the OBEX Session. This must be the final API with this
 *  session handle. 
 *
 * PARAMETERS
 *  Refer obex.h
 ************************************************************************/
void ObexDeleteSessionResponse( Obex session ) 
{
    OBEX_ASSERT( session );
    obexDeleteSessionTask( session );
}
/**********************************************************************
 * NAME
 *  ObexGetSink 
 *
 * DESCRIPTION 
 *  Get a valid sink 
 *
 * PARAMETERS
 *  Refer obex.h
 ************************************************************************/
Sink ObexGetSink( Obex session )
{
    if(!session) return 0;

    return session->sink;
}

/**********************************************************************
 * NAME
 *  ObexGetChannel
 *
 * DESCRIPTION 
 *  Get the associated server channel
 *
 * PARAMETERS
 *  Refer obex.h
 ************************************************************************/
uint8 ObexGetChannel( Obex session )
{
    if(!session) return OBEX_INVALID_CHANNEL;
    return session->rfcChannel;
}
/*************************************************************************
 * NAME
 *  ObexObjNew
 *
 * DESCRIPTION 
 *  This Utility API frames the OBEX Packet header and creates space for 
 *  the headers. It returns 0 on error. On success it returns the 
 *  the available length for the OBEX headers.
 *
 * PARAMETERS
 *  Refer obex.h
 *************************************************************************/
uint16 ObexObjNew( Obex session )
{
    if (!session ) return OBEX_INVALID_LEN;

    /* Assuming this packet request is for PUT. It can change later
       while sending the request */
    return obexNewPacket( session , OBEX_EMPTY_BODY_SIZE, OBEX_PUT ); 
}

/***********************************************************************
 * NAME
 *  ObexObjDelete
 *
 * DESCRIPTION
 *  API to delete the Object if the application wants to abort the PUT or 
 *  GET Operation while framing the packet 
 *
 * PARAMETERS
 *  Refer obex.h
 *************************************************************************/
void  ObexObjDelete( Obex session)
{
    OBEX_ASSERT( session );
    obexObjFlush( session , FALSE );
}

/***********************************************************************
 * NAME
 *  ObexObjAddVariableHdr
 *
 * DESCRIPTION
 *  Add a byte sequence or Unicode header to the packet 
 *
 * PARAMETERS
 *  Refer obex.h 
 *************************************************************************/
bool ObexObjAddVariableHdr( Obex session,  
                            ObexSeqHeaderId hdrId,
                            uint16  hdrLen,
                            Source  src )
{
    if( !session ) return FALSE;

    if(!src) return obexAddEmptyHeader( session, hdrId, hdrLen );

    if( IsByteSeqHdr(hdrId) ) 
    {
        return obexAddSeqHeader( session, hdrId, hdrLen, src ); 
    }
    else if (IsUnicodeHdr(hdrId) )
    {
        return obexAddUnicodeHeader( session, hdrId, hdrLen, src );
    }
    
    return FALSE;
}
/***********************************************************************
 * NAME
 *  ObexObjAddByteHdr
 *
 * DESCRIPTION
 *  Add a Unsigned 4 byte header to the object.
 *
 * PARAMETERS
 *  Refer obex.h 
 *************************************************************************/
bool ObexObjAddByteHdr( Obex session, ObexByteHeaderId hdrId, uint32 hdrVal)
{
    if( !session ) return FALSE;
    return obexAddUint32Header( session, hdrId, hdrVal );
}

/***********************************************************************
 * NAME
 *  ObexObjExtractByteHdrValue
 *
 * DESCRIPTION
 *  Extract 4 byte value header.
 *
 * PARAMETERS
 *  Refer obex.h 
 *************************************************************************/
uint32 ObexObjExtractByteHdrValue( ObexByteHeaderId hdrId,
                                   uint16  srcLen,
                                   Source  src )
{
    const uint8* pkt = SourceMap( src );

    if(pkt == NULL ) return OBEX_INVALID_UINT32;
    return obexGetUint32Header( pkt, srcLen, hdrId );
}
 
/***********************************************************************
 * NAME
 *  ObexObjMapHdrValue
 *
 * DESCRIPTION
 *  Point to to start of the Header value in the Source 
 *
 * PARAMETERS
 *  Refer obex.h 
 *************************************************************************/
const uint8* ObexObjMapHdrValue( ObexSeqHeaderId hdrId,
                                 uint16  srcLen,
                                 Source  src,
                                 uint16 *hdrLen )
{
    const uint8* pkt = SourceMap( src );
    if( !pkt) return NULL;
    *hdrLen = srcLen;

    return obexGetSeqHeader( pkt, hdrLen, hdrId );
}

/***********************************************************************
 * NAME
 *  ObexObjFilterHdrValue
 *
 * DESCRIPTION
 * Drop any preceding headers before this headers.
 *
 * PARAMETERS
 *  Refer obex.h 
 *************************************************************************/
const uint8* ObexObjFilterHdrValue( ObexSeqHeaderId  hdrId,
                                       uint16  srcLen,
                                       Source  src,
                                       uint16 *hdrLen )
{
    const uint8* pkt = SourceMap ( src );
    uint16 offset;
    *hdrLen = srcLen;

    if( !pkt) return NULL;

    offset = obexFetchHeader( pkt, srcLen , hdrId );
    if( offset == OBEX_INVALID_UINT16 ) return NULL;

    obexGetSeqHeader( pkt, hdrLen , hdrId );
    SourceDrop( src, offset+OBEX_SEQ_HDR_SIZE );

    return SourceMap( src );

}

/***********************************************************************
 * NAME
 *  ObexSourceDrop
 *
 * DESCRIPTION
 *  Drop the Source buffer 
 *
 * PARAMETERS
 *  Refer obex.h 
 *************************************************************************/
void ObexSourceDrop( Obex session )
{
    OBEX_ASSERT( session );
    obexSourceEmpty( session );
}

