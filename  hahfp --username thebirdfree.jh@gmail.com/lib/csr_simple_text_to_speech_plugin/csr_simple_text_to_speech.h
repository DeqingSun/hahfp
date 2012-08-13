/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_simple_text_to_speech_plugin.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _CSR_SIMPLE_TESXT_TO_SPEECH_H_
#define _CSR_SIMPLE_TESXT_TO_SPEECH_H_

void CsrSimpleTextToSpeechPluginPlayBack(void);

void CsrSimpleTextToSpeechPluginPlayDigit ( ringtone_note * tone, Task codec_task, uint16 tone_volume , bool stereo); 

void CsrSimpleTextToSpeechPluginPlayPhrase ( uint16 id , uint8 * data , uint16 size_data , uint16 language , Task codec_task , uint16 tts_volume , bool stereo ) ;

void CsrSimpleTextToSpeechPluginStopPhrase ( void ) ;

#endif

