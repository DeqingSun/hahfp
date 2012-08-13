/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_simple_text_to_speech_plugin.h

DESCRIPTION
    
    
NOTES
   
*/
/*!
@file   csr_simple_text_to_speech_plugin.h

@brief  Header file for the csr plugin which allows simple 8 bit pcm Text to speech
    phrases to be connected directly to the PCM ports
        
	This can connect the simple TTS phrases using the internal codecs on 
    either DAC channel A only or DAC channel A & B
    
    Tone playback is not supported in this plugin as the AudioPlayTone() functionality is 
    used for the TTS playback
     
     
     How to use:
     
     AudioConnect() the csr_simple_text_to_speech_plugin using the audio library
     
     call AudioPlayTone() ith the digit you wish to be played.
     
     Note the TTS digits must be stored in the filesystem onchip in the folder tts/
     
     The digits used are of the formeat 0.raw (digit 0)
     
     The file format for the TTS phrases is 8 bit mono PCM with no header information.
     
     i.e similar to wav but with any wav file format header information removed
     
     If the digits are not located in the filesystem at the correct place, then the 
     application will panic.
      
*/



#ifndef _CSR_SIMPLE_TEXT_TO_SPEECH_PLUGIN_H_
#define _CSR_SIMPLE_TEXT_TO_SPEECH_PLUGIN_H_

#include <message.h>

extern const TaskData csr_simple_text_to_speech_plugin ;


/*! 
	This is an audio plugin that can be used with the audio library
	The plugin provides TTS playback whilst no other audio is routed
	
	The enum for the digits is passed into the AudioPlayTone() function instead 
    of the pointer to a tone. This will then cause the TTS phrase from the filesystem 
    to be played back according to the rest of the parameters int the tone API. 
*/
typedef enum {
    DIGIT_0 = 0 ,
    DIGIT_1 ,
    DIGIT_2 ,
    DIGIT_3 ,
    DIGIT_4 ,
    DIGIT_5 ,
    DIGIT_6 ,
    DIGIT_7 ,
    DIGIT_8 ,
    DIGIT_9 ,
    SIMPLE_TEXT_TO_SPEECH_PHRASES_TOP            
} SIMPLE_TEXT_TO_SPEECH_PHRASES_T ;
 
#endif

