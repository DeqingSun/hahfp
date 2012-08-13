/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005

FILE NAME
    csr_common_example_if.h

DESCRIPTION
   
*/

#ifndef _CSR_COMMON_EXAMPLE_INTERFACE_H_
#define _CSR_COMMON_EXAMPLE_INTERFACE_H_
  
#define MESSAGE_SETMODE         0x0004 

#define MESSAGE_REM_BT_ADDRESS  0x2001
 
#define SYSMODE_PSTHRGH         0

#define MINUS_45dB              0x0      /* value used with SetOutputGainNow VM trap */

#define CODEC_MUTE              MINUS_45dB

#define BYTES_PER_MSBC_FRAME    60

/* dsp message structure*/
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;

/* Values for the selecting the plugin variant in the ExamplePluginTaskdata structure  */
typedef enum
{
   CVSD_8K_1_MIC  =  0,

   CVSD_8K_2_MIC  =  1,

   SBC_1_MIC      =  2,

   SBC_2_MIC      =  3

}EXAMPLE_PLUGIN_TYPE_T;


#endif

