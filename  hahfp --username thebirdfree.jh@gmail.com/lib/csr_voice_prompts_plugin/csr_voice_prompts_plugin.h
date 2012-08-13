/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_voice_prompts_plugin.h

DESCRIPTION
    
    
NOTES
   
*/
/*!
@file   csr_voice_prompts_plugin.h

@brief  Header file for the csr plugin which allows simple 8 bit pcm Text to speech
    phrases to be connected directly to the PCM ports
        
    This can connect the simple TTS phrases using the internal codecs on 
    either DAC channel A only or DAC channel A & B
    
    Tone playback is not supported in this plugin as the AudioPlayTone() functionality is 
    used for the TTS playback
     
     
     How to use:
     
     AudioConnect() the csr_simple_text_to_speech_plugin using the audio library
     
     call AudioPlayTone() with the digit you wish to be played.
     
     Note the TTS digits must be stored in the filesystem onchip in the folder tts/
     
     The digits used are of the formeat 0.raw (digit 0)
     
     The file format for the TTS phrases is 8 bit mono PCM with no header information.
     
     i.e similar to wav but with any wav file format header information removed
     
     If the digits are not located in the filesystem at the correct place, then the 
     application will panic.
      
*/



#ifndef _CSR_VOICE_PROMPTS_PLUGIN_H_
#define _CSR_VOICE_PROMPTS_PLUGIN_H_

#include <message.h>

extern const TaskData csr_voice_prompts_plugin ;
 
#endif

