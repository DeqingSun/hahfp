/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    avrcp_metadata_transfer.c

DESCRIPTION
    This file defines internal functions for handling Metadata  
    commands and Responses    

NOTES

*/


/****************************************************************************
    Header files
*/
#include <Source.h>
#include <Stream.h>
#include <Source.h>
#include <Panic.h>
#include <memory.h>

#include "avrcp_metadata_transfer.h"
#include "avrcp_caps_handler.h"
#include "avrcp_player_app_settings_handler.h"
#include "avrcp_notification_handler.h"
#include "avrcp_element_attributes_handler.h"
#include "avrcp_continuation_handler.h"
#include "avrcp_absolute_volume.h"
#include "avrcp_mediaplayer.h"
#include "avrcp_browsing_handler.h"
#include "avrcp_init.h"

/****************************************************************************
 *NAME    
 *    avrcpGetCommandType    
 *
 *DESCRIPTION
 *   This static function returns a type of AVRCP command from the PDU ID.
 *
 *PARAMETERS
 * pdu          -       command PDU ID. 
 ****************************************************************************/
static uint8 avrcpGetCommandType(uint8 id)
{
    uint8 ctype;

    switch(id)
    {
        case AVRCP_REGISTER_NOTIFICATION_PDU_ID:
            ctype = AVRCP0_CTYPE_NOTIFY;
            break;

        case AVRCP_SET_ABSOLUTE_VOLUME_PDU_ID:
        case AVRCP_SET_APP_VALUE_PDU_ID:            /* fall through */
        case AVRCP_INFORM_BATTERY_STATUS_PDU_ID:    /* fall through */
        case AVRCP_INFORM_CHARACTER_SET_PDU_ID:     /* fall through */
        case AVRCP_SET_ADDRESSED_PLAYER_PDU_ID:     /* fall through */
        case AVRCP_PLAY_ITEM_PDU_ID:                /* fall through */
        case AVRCP_ADD_TO_NOW_PLAYING_PDU_ID:        /* fall through */
            ctype = AVRCP0_CTYPE_CONTROL;
            break;

        default: /* applicable to all other commands */
           ctype = AVRCP0_CTYPE_STATUS;
           break;
    }    

    return ctype;
}

#ifndef AVRCP_TG_ONLY_LIB /* Disable CT for TG only lib */

static avrcp_packet_type avrcpGetMetadataPacketType( AVRCP* avrcp,
                                   avrcp_packet_type meta_packet_type)
{
    avrcp_packet_type pkt_type;

    if( avrcp->fragment == meta_packet_type ) return meta_packet_type;
 
    pkt_type = avrcp->fragment | meta_packet_type;

    if( (pkt_type == avrcp_packet_type_end ) &&
        !(meta_packet_type == avrcp_packet_type_single || 
         avrcp->fragment == avrcp_packet_type_single ) )
    {
        pkt_type = avrcp_packet_type_continue;
    }

    return pkt_type;
} 


/****************************************************************************
 *NAME    
 *    convertPduToMsgId    
 *
 *DESCRIPTION
 *  This static function converts the PDU ID to a confirmation message
 *  for sending common Metadata confirmation messages to the application. 
 *  Get Capabilities and Get Play Status confirmation messages uses different 
 *  format, hence  AVRCP_GET_CAPS_PDU_ID and AVRCP_GET_PLAY_STATUS_PDU_ID are 
 *  not handled here.
 *
 *PARAMETERS
 * pdu          -       command PDU ID. 
 ****************************************************************************/
static uint16 convertPduToMsgId(uint16 pdu_id)
{
    uint16 msg_id=0;

    switch(pdu_id)
    {
        case AVRCP_LIST_APP_ATTRIBUTES_PDU_ID:
             msg_id = AVRCP_LIST_APP_ATTRIBUTE_CFM;
             break;

        case AVRCP_LIST_APP_VALUE_PDU_ID: 
            msg_id = AVRCP_LIST_APP_VALUE_CFM;
            break;

        case AVRCP_GET_APP_VALUE_PDU_ID:
            msg_id = AVRCP_GET_APP_VALUE_CFM;
            break;

        case AVRCP_GET_APP_ATTRIBUTE_TEXT_PDU_ID:
            msg_id = AVRCP_GET_APP_ATTRIBUTE_TEXT_CFM;
            break;

        case AVRCP_GET_APP_VALUE_TEXT_PDU_ID:
            msg_id = AVRCP_GET_APP_VALUE_TEXT_CFM;
            break;

        case AVRCP_GET_ELEMENT_ATTRIBUTES_PDU_ID:
            msg_id = AVRCP_GET_ELEMENT_ATTRIBUTES_CFM;
            break;

        case AVRCP_ABORT_CONTINUING_RESPONSE_PDU_ID:
            msg_id = AVRCP_ABORT_CONTINUING_RESPONSE_CFM;
            break;

        case AVRCP_INFORM_BATTERY_STATUS_PDU_ID:
            msg_id = AVRCP_INFORM_BATTERY_STATUS_CFM;
            break;

        case AVRCP_INFORM_CHARACTER_SET_PDU_ID:
            msg_id = AVRCP_INFORM_CHARACTER_SET_CFM;
            break;
    
        case AVRCP_SET_APP_VALUE_PDU_ID:
            msg_id = AVRCP_SET_APP_VALUE_CFM;
            break;

        case AVRCP_SET_ADDRESSED_PLAYER_PDU_ID:
            msg_id = AVRCP_SET_ADDRESSED_PLAYER_CFM;
            break;

        case AVRCP_PLAY_ITEM_PDU_ID:
            msg_id = AVRCP_PLAY_ITEM_CFM;
            break;

        case AVRCP_ADD_TO_NOW_PLAYING_PDU_ID:
            msg_id = AVRCP_ADD_TO_NOW_PLAYING_CFM;
            break;

        default:
            /* Error. Should not reach here */
            AVRCP_DEBUG(("Unknown PDU:0x%X ", pdu_id));
            break;
    }

    return msg_id;
}

/****************************************************************************
* NAME    
*  avrcpSendMetadataFailCfmToClient    
*
* DESCRIPTION
*  Send a confirmation message to the application depending on the Metadata
*  command sent.
*
* PARAMETERS
*   avrcp       - AVRCP instance
*   status      - Status to Application. 
****************************************************************************/
void avrcpSendMetadataFailCfmToClient(AVRCP *avrcp, avrcp_status_code status)
{
    switch (avrcp->pending)
    {
        case avrcp_get_caps:
            avrcpSendGetCapsCfm(avrcp, status, 0, 0, 0, 0, 0);
            break;
        case avrcp_list_app_attributes:
            avrcpSendCommonFragmentedMetadataCfm(avrcp, status,
                                 AVRCP_LIST_APP_ATTRIBUTE_CFM, 0, 0,  0);
            break;
        case avrcp_list_app_values:
            avrcpSendCommonFragmentedMetadataCfm(avrcp, status,
                                 AVRCP_LIST_APP_VALUE_CFM, 0, 0, 0);
            break;
        case avrcp_get_app_values:
            avrcpSendCommonFragmentedMetadataCfm(avrcp, status,
                                 AVRCP_GET_APP_VALUE_CFM, 0, 0, 0);
            break;
        case avrcp_set_app_values:
            avrcpSendCommonMetadataCfm(avrcp, status, AVRCP_SET_APP_VALUE_CFM);
            break;
        case avrcp_get_app_attribute_text:
            avrcpSendCommonFragmentedMetadataCfm(avrcp, status,
                                 AVRCP_GET_APP_ATTRIBUTE_TEXT_CFM, 0, 0, 0);
            break;
        case avrcp_get_app_value_text:
            avrcpSendCommonFragmentedMetadataCfm(avrcp, status,
                                 AVRCP_GET_APP_VALUE_TEXT_CFM, 0, 0, 0);
            break;
        case avrcp_get_element_attributes:
            avrcpSendCommonFragmentedMetadataCfm(avrcp, status,
                                 AVRCP_GET_ELEMENT_ATTRIBUTES_CFM, 0, 0,  0);
            break;
        case avrcp_get_play_status:
            avrcpSendGetPlayStatusCfm(avrcp, status, 0, 0, 0, 0);
            break;
        case avrcp_playback_status:
        case avrcp_track_changed:
        case avrcp_track_reached_end:
        case avrcp_track_reached_start:
        case avrcp_playback_pos_changed:
        case avrcp_batt_status_changed:
        case avrcp_system_status_changed:
        case avrcp_player_setting_changed:
            avrcpSendRegisterNotificationFailCfm(avrcp, status, 
                            avrcp->pending - avrcp_events_start_dummy);
            break;
        case avrcp_next_group:
           avrcpSendCommonMetadataCfm(avrcp, status, AVRCP_NEXT_GROUP_CFM);
           break;
        case avrcp_previous_group:
           avrcpSendCommonMetadataCfm(avrcp, status, AVRCP_PREVIOUS_GROUP_CFM);
            break;
        case avrcp_abort_continuation:
            avrcpSendCommonMetadataCfm(avrcp, status,
                             AVRCP_ABORT_CONTINUING_RESPONSE_CFM);
            break;
        case avrcp_battery_information:
            avrcpSendCommonMetadataCfm(avrcp, status,   
                             AVRCP_INFORM_BATTERY_STATUS_CFM);
            break;
        case avrcp_character_set:
            avrcpSendCommonMetadataCfm(avrcp, status,
                             AVRCP_INFORM_CHARACTER_SET_CFM);
            break;
        case avrcp_absolute_volume:
            avrcpSendAbsoluteVolumeCfm(avrcp, status, 0);
            break;

        case avrcp_set_addressed_player:
            avrcpSendAddressedPlayerCfm(avrcp, status);
            break;

        case avrcp_play_item:
        case avrcp_add_to_now_playing:  /* Fall through */
            avrcpSendCommonStatusCfm(avrcp, status, 
                                     convertPduToMsgId(avrcp->pending));
            break;

        default:
            break;
    }

    avrcp->pending = avrcp_none;
}


/****************************************************************************
 *NAME    
 *    avrcpHandleMetadataResponse    
 *
 *DESCRIPTION
 * This function handles the incoming Meta Data Command
 *
 *PARAMETERS
 * avrcp        -       AVRCP Entity
 * ptr          -       pointer to Meta Data response data
 * packet_size  -       size of the response packet
 *    
 *  Octet 0-5 are vendor dependent
 *                  -----------------------------------------------
 *                  | MSB |     |      |    |   |    |   |    LSB |
 *                  |-----------------------------------------------|
 *  octet 6         |      PDU ID                                   |
 *                  |-----------------------------------------------|
 *  octet 7         |      Reserved                     |  PT       |
 *                  |-----------------------------------------------|  
 *    octet 8 -9    |              Parameter Length                 |
 *                  |-----------------------------------------------|
 *  octet 10 - n    |               Parameters                      |
 *                  |-----------------------------------------------| 
 *
 *MESSAGE RETURNED
*****************************************************************************/
void avrcpHandleMetadataResponse(   AVRCP       *avrcp, 
                                    const uint8 *ptr, 
                                    uint16      packet_size)
{
    uint16 pdu_id = ptr[AVRCP_AVC_PDU_OFFSET];
    uint16 meta_packet_type = ptr[AVRCP_AVC_PT_OFFSET];
    uint8 transaction = (avrcp->av_msg[AVCTP_HEADER_START_OFFSET]  & 
                        AVCTP_TRANSACTION_MASK) >> AVCTP0_TRANSACTION_SHIFT;
    uint16 param_len = ptr[AVRCP_AVC_LEN_OFFSET] << 8 |
                       ptr[AVRCP_AVC_LEN_OFFSET+1];
    avrcp_status_code status;
    avrcp_response_type  response =  ptr[AVRCP_CTYPE_OFFSET];
    const uint8 *data=NULL;
    uint16 pdu_len = param_len + 
                    AVRCP_VENDOR_HEADER_SIZE+METADATA_HEADER_SIZE;

    if( packet_size < AVRCP_VENDOR_HEADER_SIZE+METADATA_HEADER_SIZE )
    {
        avrcpSourceProcessed(avrcp, TRUE);
        /* Corrupted data received */
        return;
    }

    /* More than one response PDU will be in the packet . Only process
       the first one and leave the rest for next iteration */
    if(packet_size > pdu_len)
    {
        avrcp->av_msg_len    -= (packet_size - pdu_len);
    }
   

    if( packet_size < pdu_len ) 
    {
        if( avrcp->fragment == avrcp_packet_type_single)
        {
            avrcpSourceProcessed(avrcp, TRUE);
            /* Corrupted data received */
            return;
        }

        packet_size -= (AVRCP_VENDOR_HEADER_SIZE + METADATA_HEADER_SIZE);
        param_len -= packet_size;
        avrcp->av_msg[AVRCP_AVC_LEN_START_OFFSET] = (param_len >> 8) & 0xFF;
        avrcp->av_msg[AVRCP_AVC_LEN_START_OFFSET+1] = param_len & 0x00FF;
    }
    else
    {
        packet_size = param_len;
    }



    if(packet_size)
    {
        if(avrcp->fragment == avrcp_packet_type_single)
        {
            data = &ptr[AVRCP_AVC_PARAM_OFFSET];
        }
        else
        {
            data = SourceMap(StreamSourceFromSink(avrcp->sink));
        }
    }

   if((pdu_id !=  AVRCP_REGISTER_NOTIFICATION_PDU_ID) &&
      (avrcp->pending != pdu_id) && 
      (avrcp->continuation_pdu != pdu_id))
   {
      avrcpSourceProcessed(avrcp, TRUE);
      /* response error */
      return;
   }

   status = convertResponseToStatus(response);
   meta_packet_type = avrcpGetMetadataPacketType (avrcp, meta_packet_type); 

   /* If Status is avrcp_rejected, data[0] contains the 
      Error status code */
   if((status == avrcp_rejected) && data) 
   {
        status = data[0] | AVRCP_ERROR_STATUS_BASE;
   }
    
                
    switch (pdu_id)
    {
    case AVRCP_GET_CAPS_PDU_ID:
        {
           avrcpHandleGetCapsResponse(avrcp, response, 
                                      meta_packet_type, data, 
                                      packet_size);
        }
        break;

    case AVRCP_LIST_APP_ATTRIBUTES_PDU_ID:
    case AVRCP_LIST_APP_VALUE_PDU_ID:           /* Fall through */
    case AVRCP_GET_APP_VALUE_PDU_ID:            /* Fall through */
    case AVRCP_GET_APP_ATTRIBUTE_TEXT_PDU_ID:   /* Fall through */
    case AVRCP_GET_APP_VALUE_TEXT_PDU_ID:       /* Fall through */
    case AVRCP_GET_ELEMENT_ATTRIBUTES_PDU_ID:   /* Fall through */
        {
            /* Only process the response if it was expected. */
            avrcpSendCommonFragmentedMetadataCfm(avrcp, 
                                            status, 
                                            convertPduToMsgId(pdu_id), 
                                            meta_packet_type, 
                                            packet_size, 
                                            data);
        }
        break;

    case AVRCP_GET_PLAY_STATUS_PDU_ID: 
        {
            avrcpHandleGetPlayStatusResponse(avrcp, status,
                                             transaction, 
                                             data, packet_size);
        }
        break;

    case AVRCP_REGISTER_NOTIFICATION_PDU_ID:
        {
            avrcpSendNotification(avrcp,
                                  response,data, packet_size);   
        }
        break;


    case AVRCP_ABORT_CONTINUING_RESPONSE_PDU_ID:
    case AVRCP_INFORM_BATTERY_STATUS_PDU_ID:     /* Fall through */
    case AVRCP_INFORM_CHARACTER_SET_PDU_ID:      /* Fall through */
    case AVRCP_SET_APP_VALUE_PDU_ID:             /* Fall through */
        {
            avrcpSendCommonMetadataCfm(avrcp,status,convertPduToMsgId(pdu_id));
        }
        break;

    case AVRCP_SET_ADDRESSED_PLAYER_PDU_ID: 
    case AVRCP_PLAY_ITEM_PDU_ID:             /* Fall through */
    case AVRCP_ADD_TO_NOW_PLAYING_PDU_ID:     /* Fall through */
        {
            avrcpSendCommonStatusCfm(avrcp, status, 
                                     convertPduToMsgId(pdu_id));
        }
        break;    
    case AVRCP_SET_ABSOLUTE_VOLUME_PDU_ID:
        {
            avrcpSendAbsoluteVolumeCfm(avrcp, status, data);
        }
        break;

    default: /* Other PDU IDs */
         /* Ignore any PDUs that aren't recognised. */
         avrcpSourceProcessed(avrcp, TRUE);
         return;
    }

    /* For AVRCP_REGISTER_NOTIFICATION_PDU_ID, pending flag was 
       handled already in avrcpSendNotification() function */
    if(pdu_id != AVRCP_REGISTER_NOTIFICATION_PDU_ID)
    {
        (void) MessageCancelAll(&avrcp->task, AVRCP_INTERNAL_WATCHDOG_TIMEOUT);

        if(response == avctp_response_interim)
        {
            /* Set a watchdog timeout start waiting for final response. Do not
               start the timer for event notifications since it may take more
               time.*/
              MessageSendLater( &avrcp->task, 
                          AVRCP_INTERNAL_WATCHDOG_TIMEOUT, 
                          0, AVCTP_WATCHDOG_TIMEOUT );
        }
        else
        {
            /* No longer waiting */
            avrcp->pending = avrcp_none;
        }
    }

    if ((meta_packet_type != avrcp_packet_type_single) && 
        (meta_packet_type != avrcp_packet_type_end))
    {
        /* Store the PDU ID of the fragmented data. */
        avrcp->continuation_pdu = pdu_id;
    }
    else
    {
        avrcp->continuation_pdu = 0;
    }


    /* Drop the source here if everyone has finished with it. */
    avrcpSourceProcessed(avrcp, TRUE);
}

/****************************************************************************
* NAME    
*  avrcpCreateMetadataTransferCmd   
*
* DESCRIPTION
*   Allocates the PDU and frames the Metadata transfer PDU header.
*
* PARAMETERS
*   pdu_id       - PDU ID 
*   param_length-  Length to be specified in Metadata header.
*   extra_data  -  Data to be copied followed by the Metadata header.
*   extra_data_length - length of extra_data.
*   pkt_type -     AVCTP packet type for framing the header. 
****************************************************************************/
uint8 *avrcpCreateMetadataTransferCmd(uint8     pdu_id,
                                      uint16    param_length, 
                                      uint8     *extra_data,
                                      uint16    extra_data_length,
                                      uint8     pkt_type)
{
    uint8 *pdu;
    uint16 length  = METADATA_HEADER_SIZE + extra_data_length;

    /* Allocate the PDU */
    pdu = (uint8 *) malloc(length);

    if (pdu)
    {
        /* Set the PDU ID - one octet */ 
        pdu[0] = pdu_id & 0xff;

        /* Pkt type - single, start, continue, end */
        pdu[1] = pkt_type;

        /* Parameter length */
        pdu[2] = (param_length >> 8) & 0xff;
        pdu[3] = param_length & 0xff;

        /* Copy the passed in params into the end of the pdu */
        if (extra_data)
            memmove(pdu+4, extra_data, extra_data_length);
    }

    return pdu;
}

/****************************************************************************
*NAME    
*    avrcpSendMetadataCommand    
*
*DESCRIPTION
*  Meta Data Control API functions, calls this function to prepare and send 
*  Metadata vendor dependent commands to TG. 
*    
*PARAMETERS
*   avrcp                   - Task
*   id                      - Command Identifier
*   pending                 - waiting for this response.
*   extra_param_len         - Length of extra_param in bytes. 
*                              Typically less than 10.
*   extra_param             - Pointer to extra_param
*   data_size               - Number of bytes in data.
*   data                    - Data source.
*
*RETURN
*****************************************************************************/
avrcp_status_code avrcpSendMetadataCommand(AVRCP*       avrcp, 
                                           uint16       id, 
                                           avrcpPending pending, 
                                           uint16       extra_param_len,
                                           uint8*       extra_params, 
                                           uint16       size_params, 
                                           Source       source_params)
{
    avrcp_packet_type metadata_packet_type = avrcp_packet_type_single;
    uint16 total_data = size_params + extra_param_len; 
    uint8* extra_data = NULL;

    /* Must have metadata supported at this end to be able to send 
       this command. Maybe should look at remote supported features also.  
    */ 
    if(!isAvrcpMetadataEnabled(avrcpGetDeviceTask()))
    {
        return avrcp_unsupported;
    }


    /* AVRCP library is not expecting any AVC command packet of size 
       more than 512 Bytes. If the Packet size is more than 512 bytes, 
       Library return error. This is as per the specification and it is not
       the limitation. Refer Specifiaction Errata 3155 */
    if(total_data  > AVRCP_AVC_MAX_DATA_SIZE)
    {
        /* AVRCP Library does not expect any AV/C Commands with more than 
           512 Bytes. Continution PDUs are defined in the specification 
           only for responses and not for commands. So reject it */
        return avrcp_rejected_invalid_content;
    }
     
    /*
     * Create enough space to hold the Source data passed in, as well as the
     * extra  fields required (in this case the number of attributes sent).
     */
    extra_data = avrcpCreateMetadataTransferCmd(id, 
                                                total_data, 
                                                extra_params, 
                                                extra_param_len,
                                                metadata_packet_type); 
    if(extra_data )
    {
     /* Metadata transfer PDUs use the Vendordependent message to
       construct the right message and send it from here */
        MAKE_AVRCP_MESSAGE(AVRCP_INTERNAL_VENDORDEPENDENT_REQ);
        message->company_id = AVRCP_BT_COMPANY_ID;
        message->subunit_type = subunit_panel;
        message->subunit_id = 0x00;
        message->ctype = avrcpGetCommandType(id);
        message->data = source_params;
        message->data_length = size_params;
        message->extra_data = extra_data;
        message->extra_data_len =  METADATA_HEADER_SIZE + extra_param_len;
        message->pending = pending;
        MessageSendConditionally(&avrcp->task, 
                                 AVRCP_INTERNAL_VENDORDEPENDENT_REQ, 
                                 message,(uint16*)&avrcp->pending);       

    }
    else
    {
        return avrcp_no_resource;
    }

    return avrcp_success;

}
    
#endif /* !AVRCP_TG_ONLY_LIB */

#ifndef AVRCP_CT_ONLY_LIB /* Disable TG for CT only lib */
/****************************************************************************
 *NAME    
 *    convertPduToEnum    
 *
 *DESCRIPTION
 *   This static function returns a corresponding enum value from a 
 *   Metadata PDU IDs other than Continuation PDUs, after validating 
 *   the Metadata PDUs. It returns avrcp_vendor if it is an unknown 
 *   PDU or Continuation PDU.
 *
 *PARAMETERS
 * pdu          -       command PDU ID. 
 ****************************************************************************/
static avrcpPending convertPduToEnum(uint16 pdu_id)
{
    switch (pdu_id)
    {
    case AVRCP_GET_CAPS_PDU_ID:             
    case AVRCP_LIST_APP_ATTRIBUTES_PDU_ID:   /* fall through */
    case AVRCP_LIST_APP_VALUE_PDU_ID:        /* fall through */            
    case AVRCP_GET_APP_VALUE_PDU_ID:         /* fall through */        
    case AVRCP_SET_APP_VALUE_PDU_ID:         /* fall through */    
    case AVRCP_GET_APP_ATTRIBUTE_TEXT_PDU_ID:/* fall through */    
    case AVRCP_GET_APP_VALUE_TEXT_PDU_ID:    /* fall through */    
    case AVRCP_GET_ELEMENT_ATTRIBUTES_PDU_ID:/* fall through */    
    case AVRCP_GET_PLAY_STATUS_PDU_ID:       /* fall through */    
    case AVRCP_INFORM_BATTERY_STATUS_PDU_ID: /* fall through */    
    case AVRCP_INFORM_CHARACTER_SET_PDU_ID:  /* fall through */    
    case AVRCP_SET_ABSOLUTE_VOLUME_PDU_ID:   /* fall through */    
    case AVRCP_SET_ADDRESSED_PLAYER_PDU_ID:  /* fall through */
    case AVRCP_PLAY_ITEM_PDU_ID:             /* fall through */
    case AVRCP_ADD_TO_NOW_PLAYING_PDU_ID:     /* fall through */
        return (avrcpPending)pdu_id;
        break;
    default:
        return avrcp_vendor;
        break;
    }
}


/****************************************************************************
* NAME    
*   avrcpFrameAVCResponseHeader   
*
* DESCRIPTION
*  Frame the Vendor dependent response header for AVC responses.
*
* PARAMETERS
*    ptr                - A valid pointer to response header
*    trans_id           - Transaction ID
*    pdu_id             - PDU ID
*    avrcp_packet_type  - Metadata packet type
*    uint16             - parameter length    
*
* RETURNS
*    void
****************************************************************************/
static void avrcpFrameAVCResponseHeader(uint8*            ptr, 
                                        uint8             trans_id,
                                        uint8             pdu_id,
                                        avrcp_packet_type metadata_packet_type,
                                        uint16            param_length)
                     
{
    /* Frame the AVCTP Start header. Frame it as a Start packet, 
       avrcpSendAvcResponse() will calculate the packet len correctly 
       while sending it. ptr[1] is for number of packets. This will
       be calculated later */
    ptr[0] = (AVCTP0_PACKET_TYPE_START | AVCTP0_CR_RESPONSE) | 
             (trans_id <<  AVCTP0_TRANSACTION_SHIFT);
    ptr[2] = AVCTP1_PROFILE_AVRCP_HIGH;
    ptr[3] = AVCTP2_PROFILE_AVRCP_REMOTECONTROL;

    /* Frame the Vendor Dependent Header */
    ptr[4] = 0; /* Packet type. fill later */
    ptr[5] = ((subunit_panel << AVRCP1_SUBUNIT_TYPE_SHIFT) & 
               AVRCP1_SUBUNIT_TYPE_MASK) | (0x00 & AVRCP1_SUBUNIT_ID_MASK);
    ptr[6] = AVRCP2_VENDORDEPENDENT;
    ptr[7] = (AVRCP_BT_COMPANY_ID & 0xFF0000) >> 16;
    ptr[8] = (AVRCP_BT_COMPANY_ID & 0x00FF00) >> 8;
    ptr[9] = (AVRCP_BT_COMPANY_ID & 0x0000FF);

    /* Set the AV/C Header */
    ptr[10] = pdu_id & 0xFF;
    ptr[11] = metadata_packet_type;
    ptr[12] = (param_length >> 8) & 0xFF;
    ptr[13] = param_length & 0x00FF;
}


/****************************************************************************
* NAME    
*   prepareMetadataControlResponse    
*
* DESCRIPTION
*   Prepare and send AVC Control Command response
*
* PARAMETERS
*   avrcp       - AVRCP instance
*   response    - Application Response , standardised by this function
*   id          - response identifier.
****************************************************************************/
static void prepareMetadataControlResponse(AVRCP *avrcp, 
                                    avrcp_response_type response,
                                    uint16 id)
{
    if((id == AVRCP_INFORM_BATTERY_STATUS_PDU_ID) ||
       (id == AVRCP_INFORM_CHARACTER_SET_PDU_ID))
    {
        avrcpSendMetadataResponse(avrcp, response, id, 0, 
                                  avrcp_packet_type_single, 
                                  0, 0,NULL);
    }
    else
    {
       uint8 data[AVRCP_ERROR_CODE_SIZE] = {AVRCP_STATUS_SUCCESS};
       avrcpSendMetadataResponse(avrcp, response, id, 0, 
                                 avrcp_packet_type_single, 
                                 AVRCP_ERROR_CODE_SIZE, 
                                 AVRCP_ERROR_CODE_SIZE,
                                 data);

        /* Reject Any Registered Notifications on Addressed Player Change */
       if(id == AVRCP_SET_ADDRESSED_PLAYER_PDU_ID)
       {
        /* Reject all Events associated with the Current Player and clear that
           Notifications.  All Associated events are defined in
           Table 6.43 of AVRCP 1.4 specification  */
           avrcpRejectRegisterNotifications(avrcp, 
                    AVRCP_PLAYER_ID_ASSOCIATED_EVENTS,
                    avrcp_response_rejected_addressed_player_changed);
       }
    }
}
/****************************************************************************
* NAME    
*  avrcpSendRejectMetadataResponse    
*
* DESCRIPTION
*  Reject a Metadata command. 
*
* PARAMETERS
*   avrcp       - AVRCP instance
*   response    - Application Response. 
*   id          - PDU ID.
****************************************************************************/
void avrcpSendRejectMetadataResponse(AVRCP *avrcp,
                                     avrcp_response_type response, 
                                     uint16 id)
{
    MAKE_AVRCP_MESSAGE(AVRCP_INTERNAL_REJECT_METADATA_RES);

    if(!avrcp->block_received_data)
    {
        /* Unknown Command . Treat it as avrcp_vendor and 
           block receiving of any data until sending a reject response
           for this PDU  */
        avrcp->block_received_data = avrcp_vendor;
    }

    message->pdu_id = id;
    message->response = response;
    MessageSend(&avrcp->task, 
                   AVRCP_INTERNAL_REJECT_METADATA_RES, message);

}

/****************************************************************************
* NAME    
*  avrcpHandleInternalRejectMetadataResponse    
*
* DESCRIPTION
*  Send a Metadata reject response.
*
* PARAMETERS
*   avrcp       - AVRCP instance
*   response    - Application Response. 
*   id          - PDU ID.
****************************************************************************/
void avrcpHandleInternalRejectMetadataResponse(AVRCP *avrcp,
                                               avrcp_response_type response, 
                                               uint16 id)
{
    uint8 mandatory_data[1]; /* Just error code for failure */
    uint16 data_size = 0;

   /* Get the error status code */
    mandatory_data[0] = avrcpGetErrorStatusCode(&response,
                                                avrcpGetCommandType(id));

    if(response == avctp_response_rejected)
    {
        data_size = AVRCP_ERROR_CODE_SIZE;
    }    
    
    /* Send a response to this PDU now. */
    avrcpSendMetadataResponse(avrcp, response, id, 0, 
                              avrcp_packet_type_single, 
                              data_size , data_size, 
                              mandatory_data);

}

/****************************************************************************
 *NAME    
 *    avrcpHandleMetadataCommand    
 *
 *DESCRIPTION
 * This function handles the incoming Meta Data Command
 *
 *PARAMETERS
 * avrcp        -       AVRCP Entity
 * ptr          -       pointer to Meta Data Commad data
 * packet_size  -       size of the request packet
 *    
 *  octet 0-5 are vendor dependent header
 *                  -----------------------------------------------
 *                  | MSB |      |     |      |    |    |     | LSB |
 *                  |-----------------------------------------------|
 *  octet 6         |      PDU ID                                   |
 *                  |-----------------------------------------------|
 *  octet 7         |      Reserved                     |  PT       |
 *                  |-----------------------------------------------|  
 *  octet 8 -9      |              Parameter Length                 |
 *                  |-----------------------------------------------|
 *  octet 10 - n    |               Parameters                      |
 *                  |-----------------------------------------------| 
 *
 *MESSAGE RETURNED
*****************************************************************************/
void avrcpHandleMetadataCommand(AVRCP       *avrcp, 
                                const uint8 *ptr, 
                                uint16      packet_size)
{
    uint16 pdu_id =  ptr[AVRCP_AVC_PDU_OFFSET];
    uint16 meta_packet_type = ptr[AVRCP_AVC_PT_OFFSET];
    uint16 param_len = ptr[AVRCP_AVC_LEN_OFFSET] << 8 | 
                       ptr[AVRCP_AVC_LEN_OFFSET+1];
    const uint8 *data = NULL;
    uint8 start_data = 0;
    uint16 pdu_len = param_len + AVRCP_VENDOR_HEADER_SIZE+
                                  METADATA_HEADER_SIZE;
    
    /* Metadata must be enabled at this end to handle these commands. */
    if(!isAvrcpMetadataEnabled(avrcpGetDeviceTask()))
    {
        avrcpSendRejectMetadataResponse(avrcp, 
                                        avctp_response_not_implemented, 
                                        pdu_id);
        return;
    }

    if(packet_size > pdu_len)
    {
        /* Received Multiple commands. Handle only one */
        avrcp->av_msg_len    -= (packet_size - pdu_len);
    }
    else  if( avrcp->fragment == avrcp_packet_type_start )  
    {
        pdu_len = packet_size;
        param_len = pdu_len -  (AVRCP_VENDOR_HEADER_SIZE+
                                METADATA_HEADER_SIZE);
    
    }


    /* Library is not expecting any Fragmented Metadata command packets */
    if((packet_size < pdu_len) ||
       (meta_packet_type != avrcp_packet_type_single))
    {
        /* Corrupted data received */
        avrcpSendRejectMetadataResponse(avrcp, 
                                        avrcp_response_rejected_invalid_pdu,
                                        pdu_id);
        return;
    }


    /* Discard any continuation packets if the CT sent a new command */
    if((avrcp->continuation_pdu) && 
        (pdu_id != AVRCP_REQUEST_CONTINUING_RESPONSE_PDU_ID) &&
        (pdu_id != AVRCP_ABORT_CONTINUING_RESPONSE_PDU_ID))
    {
        abortContinuation(avrcp);
    }

    /* Get the data if there are parameters in the packet */ 
    if(param_len)
    { 
        if( avrcp->fragment == avrcp_packet_type_single)
        { 
            data = &ptr[AVRCP_AVC_PARAM_OFFSET];
        }
        else
        {
            data = SourceMap(StreamSourceFromSink(avrcp->sink));
        }
        start_data = data[0];
    }

     
    /* Stop processing anymore data at this end until a response is
       sent. Store the capability ID if the command is 
       AVRCP_GET_CAPS_PDU_ID. This is required to send a failure 
       response on response timeout.*/
    if (pdu_id == AVRCP_GET_CAPS_PDU_ID)
    {
       avrcpBlockReceivedData( avrcp, 
                               convertPduToEnum(pdu_id), 
                               start_data);
    }
    else if(pdu_id == AVRCP_REGISTER_NOTIFICATION_PDU_ID)
    {
        avrcpBlockReceivedData(avrcp, 
                 (avrcp_events_start_dummy + ptr[AVRCP_AVC_PARAM_OFFSET]),
                  0);
    }
    else
    {
        avrcpBlockReceivedData( avrcp, 
                                convertPduToEnum(pdu_id), 
                                0);
    }
    

    /* Copy the last Transaction ID. This is required to frame the response
       packet if the application interleaved the response with a notify 
       event */
    avrcp->rsp_transaction_label =(avrcp->av_msg[AVCTP_HEADER_START_OFFSET] >> 
                            AVCTP0_TRANSACTION_SHIFT);

    switch (pdu_id)
    {
    case AVRCP_GET_CAPS_PDU_ID:
        {
            avrcpHandleGetCapsCommand(avrcp, start_data);
        }
        break;

    case AVRCP_LIST_APP_ATTRIBUTES_PDU_ID:
        {
            avrcpHandleListAppAttributesCommand(avrcp);
        }
        break;

    case AVRCP_LIST_APP_VALUE_PDU_ID:
        {
            avrcpHandleListAppValuesCommand(avrcp, start_data);
        }
        break;
    case AVRCP_GET_APP_VALUE_PDU_ID:
        {
            avrcpHandleGetAppValuesCommand(avrcp,
                                           data, param_len);
        }
        break;
    case AVRCP_SET_APP_VALUE_PDU_ID:
        {
            avrcpHandleSetAppValuesCommand(avrcp,
                                           data, param_len);
        }
        break;
    case AVRCP_GET_APP_ATTRIBUTE_TEXT_PDU_ID:
        {
            avrcpHandleGetAppAttributeTextCommand(avrcp,
                                                 data, param_len);

        }
        break;
    case AVRCP_GET_APP_VALUE_TEXT_PDU_ID:
        {
            avrcpHandleGetAppValueTextCommand(avrcp, 
                                              data, param_len);
        }
        break;

    case AVRCP_GET_ELEMENT_ATTRIBUTES_PDU_ID:
        {
            avrcpHandleGetElementAttributesCommand(avrcp,
                                                  data, param_len);
        }
        break;
    case AVRCP_GET_PLAY_STATUS_PDU_ID:
        {
            avrcpSendCommonMetadataInd(avrcp, AVRCP_GET_PLAY_STATUS_IND);
        }
        break;
    case AVRCP_REGISTER_NOTIFICATION_PDU_ID:
        {
            avrcpHandleRegisterNotificationCommand(avrcp, data);
        }
        break;
    case AVRCP_REQUEST_CONTINUING_RESPONSE_PDU_ID:
        {
            avrcpHandleRequestContinuingCommand(avrcp, start_data);
        }
        break;
    case AVRCP_ABORT_CONTINUING_RESPONSE_PDU_ID:
        {
            avrcpHandleAbortContinuingCommand(avrcp, start_data);
        }
        break;
    case AVRCP_INFORM_BATTERY_STATUS_PDU_ID:
        {
            avrcpHandleInformBatteryStatusCommand(avrcp, start_data);
        }
        break;
    case AVRCP_INFORM_CHARACTER_SET_PDU_ID:
        {
            avrcpHandleInformCharSetCommand(avrcp,
                                            data, param_len);
        }
        break;
    case AVRCP_SET_ABSOLUTE_VOLUME_PDU_ID:
        {
            if (param_len == 1) /* check to pass qualification test TC_TG_VLH_BI_01_C */
                avrcpHandleSetAbsoluteVolumeCommand(avrcp, start_data);           
            else
                avrcpSendRejectMetadataResponse(avrcp,
                                                avrcp_response_rejected_invalid_content,
                                                pdu_id);
        }
        break;
    case AVRCP_SET_ADDRESSED_PLAYER_PDU_ID:
        {
            avrcpHandleAddressedPlayerCommand(avrcp, data, param_len);
        }
        break;
    case AVRCP_PLAY_ITEM_PDU_ID:            
    case AVRCP_ADD_TO_NOW_PLAYING_PDU_ID:    /* fall through */
        {
            avrcpHandleNowPlayingCommand(avrcp, data, param_len,pdu_id);
        }
        break;
    default:
        avrcpSendRejectMetadataResponse(avrcp,
                                        avrcp_response_rejected_invalid_pdu,
                                        pdu_id);
        break;
    }
}



/****************************************************************************
* NAME    
*   avrcpHandleCommonMetadataControlResponse    
*
* DESCRIPTION
*  Handle AVRCP_INTERNAL_COMMON_METADATA_CONTROL_RES message
*
* PARAMETERS
*   avrcp       - AVRCP instance
*   message     - Metadata Control response. 
****************************************************************************/
void avrcpHandleCommonMetadataControlResponse(AVRCP         *avrcp,
                AVRCP_INTERNAL_COMMON_METADATA_CONTROL_RES_T *message )
{
    if(message->response != avctp_response_accepted)
    {
        avrcpHandleInternalRejectMetadataResponse(avrcp,
                                                  message->response, 
                                                  message->pdu_id);
    }
    else
    {
        prepareMetadataControlResponse(avrcp,
                                       message->response, 
                                       message->pdu_id);
    }
}

/****************************************************************************
* NAME    
*  prepareMetadataStatusResponse    
*
* DESCRIPTION
*  Prepare the Metadata response for a command type of STATUS.
*
* PARAMETERS
*   avrcp       - AVRCP instance
*   response    - response code.  
*   id          - PDU ID 
*   param_length- Sum of size_mandatory_data and length of data in the Source.
*   data_list   - Source contains the application data
*   size_mandatory_data - Size of mandatory_data.
*   mandatory_data - Non-application data.
****************************************************************************/
void prepareMetadataStatusResponse( AVRCP               *avrcp, 
                                    avrcp_response_type response, 
                                    uint16              id, 
                                    uint16              param_length, 
                                    Source              data_list, 
                                    uint16              size_mandatory_data, 
                                    uint8*              mandatory_data)
{
    avrcp_packet_type  metadata_packet_type =  avrcp_packet_type_single;

    /* Check if Metadata packet has to be fragmented to 
        fit into 512 AV/C frame size restriction. */
    if (param_length > AVRCP_AVC_MAX_DATA_SIZE)
    {
        /* Metadata fragmentation will occur. */

        /* Store the continuation packets until the CT
             specifically requests them. */
        avrcpStoreNextContinuationPacket(avrcp, data_list, 
                                         param_length-AVRCP_AVC_MAX_DATA_SIZE, 
                                         id, response);

        param_length = AVRCP_AVC_MAX_DATA_SIZE;
        
        metadata_packet_type = avrcp_packet_type_start;

    }

    /* Send the Start or Single packet immediately. */
    avrcpSendMetadataResponse(avrcp, response, id, data_list, 
                              metadata_packet_type, param_length, 
                              size_mandatory_data, mandatory_data);

}

/****************************************************************************
 *NAME    
 *   avrcpSendMetadataResponse
 *
 *DESCRIPTION
 * This function prepare the metadata response and send it. This function 
 * should be called only to send Metadata response for the corresponding
 * blocking command unless it is a final Event Notification response.
 *
 *PARAMETERS
 * avrcp        -       AVRCP Entity
 * ptr          -       pointer to Meta Data response data
 * packet_size  -       size of the response packet
 *    
 *  Octet 0-5 are vendor dependent
 *                   -----------------------------------------------
 *                  | MSB |   |   |    |   |        |      |    LSB |
 *                  |-----------------------------------------------|
 *  octet 6         |      PDU ID                                   |
 *                  |-----------------------------------------------|
 *  octet 7         |      Reserved                     |  PT       |
 *                  |-----------------------------------------------|  
 *    octet 8 -9    |              Parameter Length                  |
 *                  |-----------------------------------------------|
 *  octet 10 - n    |               Parameters                      |
 *                  |-----------------------------------------------| 
 *
 *MESSAGE RETURNED
*****************************************************************************/

void avrcpSendMetadataResponse(AVRCP              *avrcp, 
                               avrcp_response_type response, 
                               uint8               pdu_id, 
                               Source              caps_list, 
                               avrcp_packet_type   metadata_packet_type, 
                               uint16              param_length, 
                               uint16              size_mandatory_data, 
                               uint8              *mandatory_data)
{
    uint8  *temp       = NULL;
    uint8   trans_id   = 0;
    uint16  temp_len   = 0;

    /* 
     * For all metadata responses, blocking command must match with the 
     * response. All Metadata response APIs already check this. 
     * Non interim Event Register Notification responses are the only 
     * exception. Check the response match with the blocking command 
     * before sending an event notification. 
     */ 
    if(pdu_id == AVRCP_REGISTER_NOTIFICATION_PDU_ID)
    {
        uint8 event_id = (response == avctp_response_rejected)?
                           mandatory_data[1] :  mandatory_data[0]; 

        /* extract the right transaction ID */
        trans_id = avrcp->notify_transaction_label[event_id-1]; 

        if((avrcp_events_start_dummy + event_id) == avrcp->block_received_data)
        {
            /* Responding to the last received data. Release the Command */
            avrcpSourceDrop(avrcp);
        }
        else
        {
            /* Not responding to the last received command. Do not 
                process the Source since application may be still 
                accessing it. Take a back up*/
            temp = avrcp->av_msg;
            temp_len = avrcp->av_msg_len;
        }
    }
    else
    {
        /* Responding to the last command.
           transaction ID of response shall match with the command */
        trans_id = avrcp->rsp_transaction_label;

       /* Release the command packet */
       avrcpSourceDrop(avrcp);
    }
   
    /* Allocate memory for Metadata Response */
    avrcp->av_msg = PanicUnlessMalloc(AVRCP_AVC_START_HEADER_SIZE 
                                     + size_mandatory_data);

    /* Allocated data is greater that AVRCP_AVC_START_HEADER_SIZE, always
       set the length as AVRCP_AVC_START_HEADER_SIZE. */
    avrcp->av_msg_len = AVRCP_AVC_START_HEADER_SIZE;
 
    /* Frame the Response PDU */
    avrcpFrameAVCResponseHeader( avrcp->av_msg, trans_id, pdu_id, 
                                 metadata_packet_type, 
                                 param_length);
    memmove( &avrcp->av_msg[AVRCP_AVC_START_HEADER_SIZE], mandatory_data,
             size_mandatory_data);

    /* Send the Response Packet */
    avrcpSendAvcResponse(avrcp, 
                         AVRCP_VENDOR_HEADER_SIZE +METADATA_HEADER_SIZE +
                         size_mandatory_data,
                         response,
                          param_length - size_mandatory_data, caps_list);

    /* Now restore the blocking command if exists. */
    if(temp)
    {
        /* restore the old command */
        free(avrcp->av_msg);
        avrcp->av_msg = temp;
    }
    else if((pdu_id == AVRCP_REGISTER_NOTIFICATION_PDU_ID) || 
           (response != avctp_response_interim)) 
    {
        /* Notification Responses, Do not block the next command to come
           even if it  is INTERIM response. */
        avrcpUnblockReceivedData(avrcp);
    }

    if (metadata_packet_type == avrcp_packet_type_end ||
        metadata_packet_type == avrcp_packet_type_single)
    {
        avrcp->continuation_data = 0;
        SourceEmpty(caps_list);
    }
}

#endif /* !AVRCP_CT_ONLY_LIB*/

