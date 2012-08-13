/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    a2dp_api.h
    
DESCRIPTION

*/

#ifndef A2DP_API_H_
#define A2DP_API_H_

void a2dpSignallingConnectInd (remote_device *device);
void a2dpSignallingConnectCfm (remote_device *device, a2dp_status_code status);
void a2dpSignallingDisconnectInd (remote_device *device, a2dp_status_code status);
void a2dpSignallingLinklossInd (remote_device *device);

void a2dpMediaOpenInd (remote_device *device);
void a2dpMediaOpenCfm (remote_device *device, media_channel *media, a2dp_status_code status);
void a2dpMediaCloseInd (remote_device *device, media_channel *media, a2dp_status_code status);
void a2dpMediaStartInd (remote_device *device, media_channel *media);
void a2dpMediaStartCfm (remote_device *device, media_channel *media, a2dp_status_code status);
void a2dpMediaSuspendInd (remote_device *device, media_channel *media);
void a2dpMediaSuspendCfm (remote_device *device, media_channel *media, a2dp_status_code status);
void a2dpMediaCodecConfigureInd (remote_device *device, media_channel *media);
#endif
