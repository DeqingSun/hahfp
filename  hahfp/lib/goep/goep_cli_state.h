/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    goep_cli_state.h
    
DESCRIPTION
	Client state machine header for the Generic Object Exchange Profile (GOEP) library

	This is the base profile for FTP Server, FTP Client and the Object Push 
	Profile (OPP) Libraries.
*/


#ifndef GEOP_CLI_STATE_H_
#define GEOP_CLI_STATE_H_

/* Client State Machine for the GOEP library */
void handleClientStates(goepState *state, Source src);

#endif /* GEOP_CLI_STATE_H_ */
