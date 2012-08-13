/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    pbapc_int_handler.h
    
DESCRIPTION
	PhoneBook Access Profile Client Library - handler functions for internal messages.

*/

#include <vm.h>
#include <pbap_common.h>
#include <print.h>
#include <goep.h>

#include "pbapc.h"
#include "pbapc_private.h"

#ifdef DEBUG_PRINT_ENABLED
static void printPBString(const uint8* str, uint16 len)
{
	uint16 c;
	
	PRINT(("         "));
	for (c=0; c<len; c++)
	{
		if (str[c]>=' ')
		{
			PRINT(("%c", str[c]));
		}
		else
		{
			PRINT(("."));
		}
	}
	PRINT(("\n"));
}
#else
	#define printPBString(x,y)
#endif /* DEBUG_PRINT_ENABLED */


void pbapc_spb_startState(pbapcState *state)
{
	const uint8 *folder = NULL;
	uint16 len = 0, flags = GOEP_PATH_NOCREATE;
	
    PRINT(("pbapc_spb_startState\n"));
	
	switch (state->setPBState)
	{
	case pbapc_spb_GotoRoot:
		/* No need to do anything for this case */
		PRINT(("    Goto Root\n"));
		break;
	case pbapc_spb_GotoParent:
		PRINT(("    Goto Parent\n"));
		flags = flags | GOEP_PATH_PARENT;
		break;
	case pbapc_spb_GotoRepository:
		PRINT(("    Goto Repository\n"));
		folder = PbapcoGetRepositoryNameFromID(state->target_repository, &len);
		printPBString(folder, len);
		break;
	case pbapc_spb_GotoTelecom:
		PRINT(("    Goto Telecom\n"));
		folder = PbapcoGetBookNameFromID(pbap_telecom, &len);
		break;
	case pbapc_spb_GotoPhonebook:
		PRINT(("    Goto Phonebook\n"));
		folder = PbapcoGetBookNameFromID(state->target_phonebook, &len);
		printPBString(folder, len);
		break;
	default:
		PBAPC_DEBUG(("PBAC - Set Phonebook - invalid start state\n"));
		break;
	}
	
	GoepSetPath(state->handle, flags, len, folder);
}

bool pbapc_spb_completeState(pbapcState *state)
{
	bool ret = FALSE;
    PRINT(("pbapc_spb_completeState\n"));
	
	switch (state->setPBState)
	{
	case pbapc_spb_GotoRoot:
		if (state->target_repository <= pbap_local)
			state->setPBState = pbapc_spb_GotoTelecom;
		else
			state->setPBState = pbapc_spb_GotoRepository;
		break;
	case pbapc_spb_GotoParent:
		state->setPBState = pbapc_spb_GotoPhonebook;
		break;
	case pbapc_spb_GotoRepository:
		state->setPBState = pbapc_spb_GotoTelecom;
		break;
	case pbapc_spb_GotoTelecom:
		state->setPBState = pbapc_spb_GotoPhonebook;
		break;
	case pbapc_spb_GotoPhonebook:
		ret = TRUE;
		break;
	default:
		PBAPC_DEBUG(("PBAC - Set Phonebook - invalid complete state\n"));
		break;
	}
	
	if (!ret)
		pbapc_spb_startState(state);
	
	return ret;
}

