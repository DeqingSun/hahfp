/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_cvc_common_plugin.h

DESCRIPTION
    
    
NOTES
   
*/
#ifndef _CSR_CVC_COMMON_PLUGIN_H_
#define _CSR_CVC_COMMON_PLUGIN_H_

#include <message.h> 

/*!  CSR_CVC_COMMON plugin

	This is an cVc plugin that can be used with the cVc DSP library.
*/

typedef struct
{
	TaskData	data;
	unsigned	cvc_plugin_variant:4 ;	/* Selects the CVC plugin variant */
    unsigned	sco_encoder:3 ;			/* Sets if its CVSd, Auri or SBC */
    unsigned	two_mic:1;				/* Set the bit if using 2mic plugin */
    unsigned	sco_config:3;			/* Value to send in Kalimba MESSAGE_SCO_CONFIG message */
    unsigned   adc_dac_16kHz:1;     /* Set ADC/DAC sample rates to 16kHz */
    unsigned	reserved:4 ;			/* Set the reserved bits to zero */
}CvcPluginTaskdata;

extern const CvcPluginTaskdata csr_cvsd_cvc_1mic_handsfree_plugin ;
extern const CvcPluginTaskdata csr_cvsd_cvc_1mic_headset_plugin ;
extern const CvcPluginTaskdata csr_cvsd_cvc_2mic_headset_plugin ;


extern const CvcPluginTaskdata csr_cvsd_no_dsp_plugin;

/* internal message ids */
#define MESSAGE_FORCE_TONE_COMPLETE 	0x0001





/* Additional modes not explicitly defined by the audio manager */
#define AUDIO_MODE_CVC_MSG_BASE     (0x1000)

enum 
{
   AUDIO_MODE_CVC_BASE = AUDIO_MODE_CVC_MSG_BASE,
   AUDIO_MODE_SPEECH_REC
};

#endif

