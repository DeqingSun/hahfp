/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    pbapc_add_app_hdrs.h
    
DESCRIPTION
	PhoneBook Access Profile Client Library - Handles the addition of App. Specific. Parameters.

*/

#include <vm.h>
#include <bdaddr.h>
#include <print.h>
#include <sink.h>

#include <pbap_common.h>
#include <goep.h>

#include "pbapc.h"
#include "pbapc_private.h"

#include "goep_apphdrs.h"


uint16 pbapcAddvCardListHeaders(pbapcState *state, Sink sink)
{
	PBAPC_INT_GET_VCARD_LIST_START_T *param = (PBAPC_INT_GET_VCARD_LIST_START_T*)state->params;
	uint16 lenUsed = 0;
	uint16 len;
	
    PRINT(("pbapcAddvCardListHeaders\n"));
	
	/* Application specific parameters.  Only add if different from the default value */
	if ((param->order != pbap_param_order_def) && (param->order != pbap_order_default))
	{
		len = Goep_apphdr_AddUint8(sink, pbap_param_order, param->order);
		PBAPC_ASSERT((len != 0), ("PBAPC - Could not add Order\n"));
		lenUsed+=len;
	}
	if (param->srchVal && (param->size_srchVal>0))
	{
		len = Goep_apphdr_AddBuffer(sink, pbap_param_srch_val, param->srchVal, param->size_srchVal);
		PBAPC_ASSERT((len != 0), ("PBAPC - Could not add Search Order\n"));
		lenUsed+=len;
	}
	if ((param->srchAttr != pbap_param_srch_attr_def) && (param->srchAttr != pbap_a_unknown))
	{
		len = Goep_apphdr_AddUint8(sink, pbap_param_srch_attr, param->srchAttr);
		PBAPC_ASSERT((len != 0), ("PBAPC - Could not add Search Attribute\n"));
		lenUsed+=len;
	}
	/* Max List count needs to be sent mandatorily */
	{
		len = Goep_apphdr_AddUint16(sink, pbap_param_max_list, param->maxList);
		PBAPC_ASSERT((len != 0), ("PBAPC - Could not add Max List\n"));
		lenUsed+=len;
	}
	if (param->listStart != pbap_param_strt_offset_def)
	{
		len = Goep_apphdr_AddUint16(sink, pbap_param_strt_offset, param->listStart);
		PBAPC_ASSERT((len != 0), ("PBAPC - Could not add List Start\n"));
		lenUsed+=len;
	}
	
	return lenUsed;
}

uint16 pbapcAddvCardEntryHeaders(pbapcState *state, Sink sink)
{
	PBAPC_INT_GET_VCARD_START_PARAMS_T *param = (PBAPC_INT_GET_VCARD_START_PARAMS_T*)state->params;
	uint16 lenUsed = 0;
	uint16 len;
	
    PRINT(("pbapcAddvCardEntryHeaders\n"));
	
	/* Application specific parameters.  Only add if different from the default value */
	if ((param->format != pbap_param_format_def) && (param->format != pbap_format_def))
	{
		len = Goep_apphdr_AddUint8(sink, pbap_param_format, param->format);
		PBAPC_ASSERT((len != 0), ("PBAPC - Could not add Format\n"));
		lenUsed+=len;
	}
	if (param->filter_lo != 0)
	{
		/* Mask in manditory filter mask to ensure they are set */
		if (param->format == pbap_format_30)
			param->filter_lo |= pbap_filter_vcard30;
		else
			param->filter_lo |= pbap_filter_vcard21;
		
		len = Goep_apphdr_AddUint64(sink, pbap_param_filter, param->filter_lo, param->filter_hi);
		PBAPC_ASSERT((len != 0), ("PBAPC - Could not add Filter\n"));
		lenUsed+=len;
	}
	
	if (state->name)
	{ /* Entry name must have been added by this point, free it since it's not needed anymore */
		free(state->name);
		state->name = NULL;
	}

	return lenUsed;
}

uint16 pbapcAddPhonebookHeaders(pbapcState *state, Sink sink)
{
	PBAPC_INT_GET_PHONEBOOK_START_PARAMS_T *param = (PBAPC_INT_GET_PHONEBOOK_START_PARAMS_T*)state->params;
	uint16 lenUsed = 0;
	uint16 len;
	
    PRINT(("pbapcAddPhonebookHeaders\n"));
	
	/* Application specific parameters.  Only add if different from the default value */
	if ((param->format != pbap_param_format_def) && (param->format != pbap_format_def))
	{
		len = Goep_apphdr_AddUint8(sink, pbap_param_format, param->format);
		PBAPC_ASSERT((len != 0), ("PBAPC - Could not add Format\n"));
		lenUsed+=len;
	}
	if (param->filter_lo != 0)
	{
		/* Mask in manditory filter mask to ensure they are set */
		if (param->format == pbap_format_30)
			param->filter_lo |= pbap_filter_vcard30;
		else
			param->filter_lo |= pbap_filter_vcard21;
		
		len = Goep_apphdr_AddUint64(sink, pbap_param_filter, param->filter_lo, param->filter_hi);
		PBAPC_ASSERT((len != 0), ("PBAPC - Could not add Filter\n"));
		lenUsed+=len;
	}
	
	if (state->name)
	{ /* Entry name must have been added by this point, free it since it's not needed anymore */
		free(state->name);
		state->name = NULL;
	}
	
	/* Always add the max. List Count parameter */
	len = Goep_apphdr_AddUint16(sink, pbap_param_max_list, param->maxList);
	PBAPC_ASSERT((len != 0), ("PBAPC - Could not add Max List\n"));
	lenUsed+=len;
	
	if (param->listStart != pbap_param_strt_offset_def)
	{
		len = Goep_apphdr_AddUint16(sink, pbap_param_strt_offset, param->listStart);
		PBAPC_ASSERT((len != 0), ("PBAPC - Could not add List Start\n"));
		lenUsed+=len;
	}

	return lenUsed;
}
