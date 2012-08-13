/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_wav_ringtone_plugin.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _CSR_WAV_RINGTONE_PLUGIN_H_
#define _CSR_WAV_RINGTONE_PLUGIN_H_

#include <message.h>

/*!  audio plugin

	This is an audio plugin that can be used with the audio library
	The plugin provides tone playback whilst no other audio is routed
*/
extern const TaskData csr_wav_ringtone_plugin ;

    /*these are the ring tones supported by the simple wav ringtone plugin */
typedef enum {
    RINGTONE_0 = 0 ,
    RINGTONE_1 ,
    WAV_RINGTONES_TOP            
} WAV_RINGTONES_T ;
 

#endif

