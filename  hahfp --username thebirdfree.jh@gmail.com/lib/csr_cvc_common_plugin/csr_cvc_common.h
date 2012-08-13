/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_cvc_common.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _CSR_CVC_COMMON_H_
#define _CSR_CVC_COMMON_H_

/*plugin functions*/
void CsrCvcPluginConnect( CvcPluginTaskdata *task, 
                          Sink audio_sink , 
						  AUDIO_SINK_T sink_type, 
						  Task codec_task , 
						  uint16 volume , 
						  uint32 rate , 					  
						  bool stereo , 					  
						  bool mic_switch,						  
						  AUDIO_MODE_T mode,
						  AUDIO_POWER_T power);
void CsrCvcPluginDisconnect(CvcPluginTaskdata *task) ;
void CsrCvcPluginSetVolume( CvcPluginTaskdata *task, uint16 volume ) ;
void CsrCvcPluginSetMode ( CvcPluginTaskdata *task, AUDIO_MODE_T mode , const void * params ) ;
void CsrCvcPluginSetModeNoDsp ( AUDIO_MODE_T mode , const void * params ) ;
void CsrCvcPluginPlayTone (CvcPluginTaskdata *task, ringtone_note * tone , uint16 tone_volume, bool stereo) ;
void CsrCvcPluginStopTone ( CvcPluginTaskdata *task ) ;
void CsrCvcPluginMicSwitch ( CvcPluginTaskdata *task ) ;
void CsrCvcPluginMicSwitchNoDsp ( void );
void CsrCvcPluginToneForceCompleteNoDsp( CvcPluginTaskdata *task );
void CsrCvcPluginSetPower( CvcPluginTaskdata *task,  AUDIO_POWER_T power );

/*internal plugin message functions*/
void CsrCvcPluginInternalMessage( CvcPluginTaskdata *task ,uint16 id , Message message ) ;

void CsrCvcPluginToneComplete ( CvcPluginTaskdata *task ) ;

/* The power level that the plugin will switch to low power mode */
#define LPIBM_THRESHOLD		POWER_BATT_LEVEL0

#endif

