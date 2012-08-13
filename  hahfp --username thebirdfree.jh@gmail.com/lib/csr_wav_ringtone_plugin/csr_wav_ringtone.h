/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_wav_ringtone_plugin.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _CSR_WAV_RINGTONE_H_
#define _CSR_WAV_RINGTONE_H_


/*play the wav ringtone indicated*/
void CsrWavRingtonePluginPlayTone ( ringtone_note * tone , Task codec_task , uint16 tone_volume , bool stereo ) ;

/*stop the wav ring tone*/
void CsrWavRingtonePluginStopTone ( void ) ;

/*tidy up after the wav ringtone has completed*/
void CsrWavRingtonePluginToneComplete( void );

#endif

