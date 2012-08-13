/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_simple_text_to_speech_plugin.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _CSR_SIMPLE_TESXT_TO_SPEECH_H_
#define _CSR_SIMPLE_TESXT_TO_SPEECH_H_

void CsrVoicePromptsPluginInit(uint16 no_prompts, const voice_prompts_index* header_location, uint16 no_languages);

#ifdef INSTALL_DSP_VP_SUPPORT
void CsrVoicePromptPluginPlayDsp(void);
#endif

void CsrVoicePromptsPluginPlayBack(void);

void CsrVoicePromptsPluginPlayPhrase(uint16 id , uint8 * data , uint16 size_data , uint16 language, Task codec_task , uint16 tone_volume , bool stereo);

void CsrVoicePromptsPluginStopPhrase(void);

#endif

