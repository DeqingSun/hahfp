/****************************************************************************

FILE NAME
    hearing_aid_sim.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _HEARING_AID_SIM_H_
#define _HEARING_AID_SIM_H_

/*plugin functions*/
void HearingAidSimPluginConnect( Task task, 
                          Sink audio_sink , 
						  AUDIO_SINK_T sink_type, 
						  Task codec_task , 
						  uint16 volume , 
						  uint32 rate , 					  
						  bool stereo , 					  
						  bool mic_switch,						  
						  AUDIO_MODE_T mode,
						  AUDIO_POWER_T power);
void HearingAidSimPluginDisconnect(Task task) ;
void HearingAidSimPluginSetVolume( Task task, uint16 volume ) ;
void HearingAidSimPluginPlayTone ( Task task, ringtone_note * tone , uint16 tone_volume) ;
void HearingAidSimPluginStopTone ( Task task ) ;

void HearingAidSimPluginToneComplete ( Task task ) ;

#endif

