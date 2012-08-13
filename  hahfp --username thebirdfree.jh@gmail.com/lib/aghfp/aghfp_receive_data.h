/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010
Part of HeadsetSDK-Stereo R110.0

*/

#ifndef AGHFP_RECEIVE_DATA_H_
#define AGHFP_RECEIVE_DATA_H_


/****************************************************************************
	Called when we get an indication from the firmware that there's more data 
	received and waiting in the RFCOMM buffer. Parse it.
*/
void aghfpHandleReceivedData(AGHFP *aghfp, Source source);


#endif /* AGHFP_RECEIVE_DATA_H_ */
