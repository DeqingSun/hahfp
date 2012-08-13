/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    a2dp_process_command.h
    
DESCRIPTION
    
	
*/

#ifndef A2DP_PROCESS_COMMAND_H_
#define A2DP_PROCESS_COMMAND_H_

#include "a2dp.h"
#include "a2dp_private.h"


uint16 a2dpProcessDiscoverCommand (remote_device *device, uint16 *payload_size);
uint16 a2dpProcessGetCapabilitiesCommand (remote_device *device, uint16 *payload_size);
uint16 a2dpProcessSetConfigurationCommand (remote_device *device, uint16 *payload_size);
uint16 a2dpProcessGetConfigurationCommand (remote_device *device, uint16 *payload_size);
uint16 a2dpProcessReconfigureCommand (remote_device *device, uint16 *payload_size);
uint16 a2dpProcessOpenCommand (remote_device *device);
uint16 a2dpProcessStartCommand (remote_device *device);
uint16 a2dpProcessCloseCommand(remote_device *device);
uint16 a2dpProcessSuspendCommand (remote_device *device);
bool a2dpProcessAbortCommand(remote_device *device);
bool a2dpProcessDiscoverResponse(remote_device *device);
uint16 a2dpProcessGetCapabilitiesResponse(remote_device *device);
bool a2dpSelectNextConfigurationSeid (remote_device *device);

#endif /* A2DP_PROCESS_COMMAND_H_ */
