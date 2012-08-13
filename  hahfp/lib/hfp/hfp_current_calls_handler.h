/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    hfp_current_calls_handler.h
    
DESCRIPTION
    
*/

#ifndef HFP_CURRENT_CALLS_HANDLER_H_
#define HFP_CURRENT_CALLS_HANDLER_H_


/****************************************************************************
NAME    
    hfpHandleCurrentCallsGetReq

DESCRIPTION
    Request current calls list from the AG.

RETURNS
    TRUE if successful (command supported by AG and HF), FALSE otherwise
*/
bool hfpHandleCurrentCallsGetReq(hfp_link_data* link);


#endif /* HFP_CURRENT_CALLS_HANDLER_H_ */
