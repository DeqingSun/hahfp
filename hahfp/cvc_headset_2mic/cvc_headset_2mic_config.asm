// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio plc 2007             http://www.csr.com
// %%version
//
// $Revision$ $Date$
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//    CVC static configuration file that includes tables of function pointers
//    and their corresponding data objects.
//
//    Customer modification to this file is NOT SUPPORTED.
//
//    CVC configuration should be handled from within the cvc_headset_2mic_config.h
//    header file.
//
// *****************************************************************************

.include "cvc_headset_2mic_library.h"
.include "cvc_headset_2mic_modules.h"
.include "cvc_headset_2mic_config.h"
.include "audio_proc_library.h"
.include "mtsf_library.h"

.define NUM_CVC_MUX_CHANNELS                    6
.define NUM_SAMPLES_PER_FRAME                   64

// Generate build error messages if necessary.
.if uses_ADF == 0
.if uses_NSVOLUME
   .error NDVC cannot be enabled without ADF
.endif
.endif
.define uses_RCV_FREQPROC  (uses_RCV_NS || uses_AEQ)
.define uses_RCV_VAD       (uses_RCV_AGC || uses_AEQ)

.if uses_NSVOLUME == 0
.if uses_VCAEC
   .error VCAEC cannot be enabled without NDVC
.endif
.endif

// Reserve buffers fo 2-channel fft128 filter_bank
.define FFT_BUFFER_SIZE                ($M.filter_bank.Parameters.FFT128_BUFFLEN * 2)
.define MAX_NUM_PEQ_STAGES (5)

.define Max_RefDelay          2

// Number of sample needed for reference delay buffer
.define  Max_RefDelay_Sample  (Max_RefDelay * $M.CVC.Num_Samples_Per_Frame)

   // Narrow band CVC configuration
   .CONST $M.DCB_ALFA                        0.00782322883606;
   .CONST $M.DCB_BETA                        0.99217677116394;
   .CONST $M.CVC.REF_DELAY                   0x000080;

   .CONST $M.CVC.Wideband_Flag               0;
   .CONST   $M.oms270.STATE_LENGTH     		 $M.oms270.narrow_band.STATE_LENGTH;
   .CONST   $M.oms270.SCRATCH_LENGTH   		 $M.oms270.narrow_band.SCRATCH_LENGTH;
   .CONST   $M.oms270.FFT_NUM_BIN      		 $M.CVC.Num_FFT_Freq_Bins;
   .define  $M.oms270.mode.object      		 $M.oms270.mode.narrow_band.object
   .CONST   $M.oms270.QUE_LENGTH             $M.oms270.MIN_SEARCH_LENGTH * 2;

   .define $M.filter_bank.Parameters.FFT_BUFFLEN    $M.filter_bank.Parameters.FFT128_BUFFLEN

   .CONST   $M.SET_MODE_GAIN.MODE_PTR                  0;
   .CONST   $M.SET_MODE_GAIN.ADC_MIXER_MANT_LEFT       1;
   .CONST   $M.SET_MODE_GAIN.ADC_MIXER_MANT_RIGHT      2;
   .CONST   $M.SET_MODE_GAIN.ADC_MIXER_EXP             3;
   .CONST   $M.SET_MODE_GAIN.SCO_GAIN_MANT             4;
   .CONST   $M.SET_MODE_GAIN.SCO_GAIN_EXP              5;
   .CONST   $M.SET_MODE_GAIN.STRUC_SIZE                6;

// System Configuration is saved in kap file.
.MODULE $M.CVC_MODULES_STAMP;
   .DATASEGMENT DM;
   .BLOCK ModulesStamp;
   .VAR  s1 = 0xfeeb;
   .VAR  s2 = 0xfeeb;
   .VAR  s3 = 0xfeeb;
   .VAR  CompConfig = CVC_HEADSET_2MIC_CONFIG_FLAG;
   .VAR  s4 = 0xfeeb;
   .VAR  s5 = 0xfeeb;
   .VAR  s6 = 0xfeeb;
.ENDBLOCK;
.ENDMODULE;

.MODULE $M.CVC.data;
   .DATASEGMENT DM;

   // These lines write module and version information to the kap file.
   .VAR kap_version_stamp = &$M.CVC_VERSION_STAMP.VersionStamp;
   .VAR kap_modules_stamp = &$M.CVC_MODULES_STAMP.ModulesStamp;

   // Shared Data for CVC modules.
   .VAR/DM2CIRC fft_circ[FFT_BUFFER_SIZE];
   // Temp Variable to handle disabled modules.
   .VAR  ZeroValue = 0;
   // Temp Variable.
   .VAR  OneValue = 1.0;

   .BLOCK/DM1  FFT_DM1;
      .VAR  fft_real[FFT_BUFFER_SIZE];
      .VAR  D_r_real[$M.CVC.Num_FFT_Freq_Bins];
      // buf_snd_harm is used for snd oms harmonicity calculations
      .VAR  buf_snd_harm[$M.CVC.Num_Samples_Per_Frame];
      .VAR  bufd_l_inp[$M.CVC.Num_FFT_Window];
      .VAR  bufd_aec_inp[$M.CVC.Num_FFT_Window];
      .VAR  bufd_outp[$M.CVC.Num_FFT_Window + $M.CVC.Num_Samples_Per_Frame];
      .VAR  D_rcv_real[$M.CVC.Num_FFT_Freq_Bins];
      .VAR  D_l_real[$M.CVC.Num_FFT_Freq_Bins];
      .VAR  bufd_rcv_inp[$M.CVC.Num_FFT_Window];
   .ENDBLOCK;
   .BLOCK/DM2 FFT_DM2;
      .VAR  fft_imag[FFT_BUFFER_SIZE];
      .VAR  D_l_imag[$M.CVC.Num_FFT_Freq_Bins];
      .VAR  D_r_imag[$M.CVC.Num_FFT_Freq_Bins];
      .VAR  bufd_r_inp[$M.CVC.Num_FFT_Window];
      .VAR  D_rcv_imag[$M.CVC.Num_FFT_Freq_Bins];
   .ENDBLOCK;


   // Default Block
   .VAR  DefaultParameters[$M.CVC_HEADSET_2MIC.PARAMETERS.STRUCT_SIZE] =
      0x002A0E,                        // OFFSET_HFK_CONFIG
      0x799999,                        // OFFSET_HFK_OMS_AGGR = 0.95
      0x799999,                        // OFFSET_ASR_OMS_AGGR = 0.95
      0x000001,                        // Harmonicity
      0x00000F,                        // OFFSET_ADCGAIN_LEFT
      0x00000F,                        // OFFSET_ADCGAIN_RIGHT
      0x7FFFFF,                        // OFFSET_NDVC_HYSTERESIS
      0x09D752,                        // OFFSET_NDVC_ATK_TC
      0x09D752,                        // OFFSET_NDVC_DEC_TC
      0x000006,                        // OFFSET_NDVC_NUMVOLSTEPS
      0xEEFF96,                        // OFFSET_NDVC_MAXNOISELVL // q8.16
      0xE6FFCA,                        // OFFSET_NDVC_MINNOISELVL // q8.16
      0x000001,                        // OFFSET_NDVC_LB                ADD NEW PARAMETERS
      0x000041,                        // OFFSET_NDVC_HB                FOR LOW/HIGH BINS
      0x000000,                        // OFFSET_SND_PEQ_NUMSTAGES
      0x000000,0x000000,0x000000,      // OFFSET_SND_PEQ
      0x000000,0x000000,0x000000,      // OFFSET_SND_PEQ
      0x000000,0x000000,0x000000,      // OFFSET_SND_PEQ
      0x000000,0x000000,0x000000,      // OFFSET_SND_PEQ
      0x000000,0x000000,0x000000,      // OFFSET_SND_PEQ
      0x000000,0x000000,0x000000,      // OFFSET_SND_PEQ
      0x000000,0x000000,0x000000,      // OFFSET_SND_PEQ
      0x000000,0x000000,0x000000,      // OFFSET_SND_PEQ
      0x000000,                        // OFFSET_SND_PEQ
      1,0,0,0,0,                       // OFFSET_SND_PEQ_SCALE
      0x000001,                        // OFFSET_RCV_PEQ_NUMSTAGES
      0x3BB509,0x8895ED,0x3BB509,0x37B3DF,0x88DFBA,   // OFFSET_RCV_PEQ
      0x000000,0x000000,0x000000,0x000000,0x000000,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      1,0,0,0,0,                       // OFFSET_RCV_PEQ_SCALE
      0x58E9FA,                        // Inverse of DAC gain q.15
      0x3B6CD4,                        // Inverse of DAC gain q.15
      0x2C9003,                        // Inverse of DAC gain q.15
      0x1DC87A,                        // Inverse of DAC gain q.15
      0x16558D,                        // Inverse of DAC gain q.15
      0x0EED4A,                        // Inverse of DAC gain q.15
      0x0B3190,                        // Inverse of DAC gain q.15
      0x077B2E,                        // Inverse of DAC gain q.15
      0x059C2F,                        // Inverse of DAC gain q.15
      0x03F8BD,                        // Inverse of DAC gain q.15
      0x02CFCC,                        // Inverse of DAC gain q.15
      0x01FD94,                        // Inverse of DAC gain q.15
      0x0168C1,                        // Inverse of DAC gain q.15
      0x00FF65,                        // Inverse of DAC gain q.15
      0x00B4CE,                        // Inverse of DAC gain q.15
      0x008000,                        // Inverse of DAC gain q.15

      //hard clipper
      0x7FFFFF,                        // OFFSET_CLIP_POINT
      0x23D70A,                        // OFFSET_SIDETONE_LIMIT 0x23D70A is 28%FS = -11 dBFS
      0x040000,                        // OFFSET_BOOST (Q5.18) 1/32 = 1.0
      0x721481,                        // OFFSET_BOOST_CLIP_POINT = -1 dBFS
      0x400000,                        // OFFSET_G_ALFA - not used.

      // Side tone filter parameter block
      0x7fffff,                        // OFFSET_ST_CLIP_POINT
      0x008000,                        // OFFSET_ST_ADJUST_LIMIT
      0,                               // OFFSET_STF_SWITCH
      0,                               // OFFSET_STF_NOISE_LOW_THRES
      0,                               // OFFSET_STF_NOISE_HIGH_THRES
      1,                               // OFFSET_STF_GAIN_EXP
      0x400000,                        // OFFSET_STF_GAIN_MANTISSA
      0x000003,                        // OFFSET_STF_PEQ_NUMSTAGES
      0x373CC9, 0x91866E, 0x373CC9, 0x2FAE26, 0x92BB03, // STF_PEQ_1
      0x373CC9, 0x91866E, 0x373CC9, 0x2FAE26, 0x92BB03, // STF_PEQ_2
      0x373CC9, 0x91866E, 0x373CC9, 0x2FAE26, 0x92BB03, // STF_PEQ_3
      1,1,1,                                            // STF_PEQ_SCALE

      0x400000,                        // OFFSET_SNDGAIN_MANTISSA
      0x000001,                        // OFFSET_SNDGAIN_EXPONENT
      0x400000,                        // OFFSET_RCVGAIN_MANTISSA
      0x000001,                        // OFFSET_RCVGAIN_EXPONENT
      0x400000,                        // OFFSET_PT_SNDGAIN_MANTISSA
      0x000001,                        // OFFSET_PT_SNDGAIN_EXPONENT
      0x400000,                        // OFFSET_PT_RCVGAIN_MANTISSA
      0x000001,                        // OFFSET_PT_RCVGAIN_EXPONENT

      0xFD95C1,                        // OFFSET_DMSS_PP_GAMMAP
      0x400000,                        // 0.5 - OFFSET_DMSS_PP_THRES
      0x000000,                        // OFFSET_DMSS_PP_CTRL_BIAS
      0x100000,                        // OFFSET_DMSS_PP_CTRL_TRANS
      0x000001,                        // OFFSET_DMSS_PP_RPTCNT   : Iterations-1 (0 or 1)
      0xF10E04,                        // OFFSET_MTSF_PWR_THRESH
      0x7FFFFF,                        // OFFSET_MTSF_TIER1_AGGR
      0x666666,                        // OFFSET_MTSF_TIER2_AGGR
      0x500000,                        // OFFSET_MTSF_MIC_DIST
      0x09D752,                        // OFFSET_MTSF_ATK_ALPHA
      0x010519,                        // OFFSET_MTSF_DEC_ALPHA
      0x000001,                        // OFFSET_MTSF_SWAP_ENABLE
      0x000003,                        // OFFSET_MTSF_WIN_LOW_IDX
      0x000004,                        // OFFSET_MTSF_WIN_CNT_IDX
      0x200000,                        // OFFSET_MTSF_WIN_THRESH
      0x0CCCCC,                        // OFFSET_MTSF_GAIN_THRESH
      0x000008,                        // OFFSET_MTSF_BIN_THRESH
      // Send AGC
      0x287A26,                        // OFFSET_SND_AGC_TARGET (-7dB)
      0x09D752,                        // OFFSET_SND_AGC_ATTACK_TC (0.1)
      0x0504DA,                        // OFFSET_SND_AGC_DECAY_TC (1.0)
      0x0342CE,                        // OFFSET_SND_AGC_A_90_PK (1.0)
      0x008A44,                        // OFFSET_SND_AGC_D_90_PK (1.0)
      0x0B3F30,                        // OFFSET_SND_AGC_G_MAX (15 dB)
      0x599999,                        // OFFSET_SND_AGC_COMP
      0x65AC8B,                        // OFFSET_SND_AGC_INP_THRESH
      0x180000,                        // OFFSET_SND_AGC_SP_ATTACK
      0x34CE07,                        // OFFSET_SND_AGC_AD_THRESH1
      0xDA9DF8,                        // OFFSET_SND_AGC_AD_THRESH2
      0x00A1E8,                        // OFFSET_SND_AGC_G_MIN

      // Receive VAD
      0x005743,                        // OFFSET_RCV_VAD_ATTACK_TC
      0x005743,                        // OFFSET_RCV_VAD_DECAY_TC
      0x1C5041,                        // OFFSET_RCV_VAD_ENVELOPE_TC
      0x000019,                        // OFFSET_RCV_VAD_INIT_FRAME_THRESH
      0x400000,                        // OFFSET_RCV_VAD_RATIO
      0x008000,                        // OFFSET_RCV_VAD_MIN_SIGNAL
      0xFD4000,                        // OFFSET_RCV_VAD_MIN_MAX_ENVELOPE
      0x005000,                        // OFFSET_RCV_VAD_DELTA_THRESHOLD
      0x000038,                        // OFFSET_RCV_VAD_COUNT_THRESHOLD

      // Receive AGC
      0x287A26,                        // OFFSET_RCV_AGC_TARGET (-7dB)
      0x09D752,                        // OFFSET_RCV_AGC_ATTACK_TC (0.1)
      0x0504DA,                        // OFFSET_RCV_AGC_DECAY_TC (1.0)
      0x0342CE,                        // OFFSET_RCV_AGC_A_90_PK (1.0)
      0x008A44,                        // OFFSET_RCV_AGC_D_90_PK (1.0)
      0x0B3F30,                        // OFFSET_RCV_AGC_G_MAX (15 dB)
      0x599999,                        // OFFSET_RCV_AGC_COMP
      0x65AC8B,                        // OFFSET_RCV_AGC_INP_THRESH
      0x180000,                        // OFFSET_RCV_AGC_SP_ATTACK
      0x34CE07,                        // OFFSET_RCV_AGC_AD_THRESH1
      0xDA9DF8,                        // OFFSET_RCV_AGC_AD_THRESH2
      0x00A1E8,                        // OFFSET_RCV_AGC_G_MIN


      0x000D1B,                        // OFFSET_MGDC_ALFAD
      0x005532,                        // OFFSET_MGDC_FRONTMICBIAS  Q8.16
      0x01FF2E,                        // OFFSET_MGDC_MAXCOMP       Q8.16
      0xA05417,                        // OFFSET_MGDC_ADAPT_THRESH  Q8.16
      0x000000,                        // OFFSET_AEC_RPT_ENABLE     (zero to disable /non-zero to enable)

      // Adaptive EQ
      0x004178,                        //OFFSET_AEQ_ATK_TC,
      0x7FBE87,                        //OFFSET_AEQ_ATK_1MTC,
      0x004178,                        //OFFSET_AEQ_DEC_TC,
      0x7FBE87,                        //OFFSET_AEQ_DEC_1MTC,
      0x035269,                        //OFFSET_AEQ_LO_GOAL_LOW,
      0x02534A,                        //OFFSET_AEQ_LO_GOAL_MID,
      0x02534A,                        //OFFSET_AEQ_LO_GOAL_HIGH,
      0x01A934,                        //OFFSET_AEQ_HI_GOAL_LOW,
      0x00FF1F,                        //OFFSET_AEQ_HI_GOAL_MID,
      0,                               //OFFSET_AEQ_HI_GOAL_HIGH,
      0x084E08,                        //OFFSET_AEQ_POWER_TH
      0xFC0381,                        //OFFSET_AEQ_MIN_GAIN
      0x03FC7F,                        //OFFSET_AEQ_MAX_GAIN
      1,                               //OFFSET_AEQ_VOL_STEP_UP_TH1
      2,                               //OFFSET_AEQ_VOL_STEP_UP_TH2
      0x00AA15,                        //OFFSET_AEQ_LOW_STEP
      0x018151,                        //OFFSET_AEQ_LOW_STEP_INV
      0x00550A,                        //OFFSET_AEQ_HIGH_STEP
      0x0302A8,                        //OFFSET_AEQ_HIGH_STEP_INV

      5,                               //OFFSET_AEQ_LOW_BAND_INDEX,
      8,                               //OFFSET_AEQ_LOW_BANDWIDTH,
      0x030000,                        //OFFSET_AEQ_LOG2_LOW_BANDWIDTH,
      0x000013,                        //OFFSET_AEQ_MID_BANDWIDTH,
      0x043F78,                        //OFFSET_AEQ_LOG2_MID_BANDWIDTH
      0x000018,                        //OFFSET_AEQ_HIGH_BANDWIDTH,
      0x0495C0,                        //OFFSET_AEQ_LOG2_HIGH_BANDWIDTH,
      0x00000E,                        //OFFSET_AEQ_MID1_BAND_INDEX,
      0x000021,                        //OFFSET_AEQ_MID2_BAND_INDEX,
      0x000039,                        //OFFSET_AEQ_HIGH_BAND_INDEX,

      // PLC parameter for tuning rate at which stats are displayed
      0x003E80,                        //OFFSET_PLC_STAT_INTERVAL,

      // Receive OMS
      0x666666,                        // OFFSET_HFK_OMS_AGGR
      0x000001,                        // Harmonicity

      0x000000,0x000000,0x000000,      // OFFSET_CUSTOM_PARAMETERS 0-2
      0x000000,0x000000,0x000000,      // OFFSET_CUSTOM_PARAMETERS 3-5
      0x000000,0x000000,0x000000,      // OFFSET_CUSTOM_PARAMETERS 6-8
      0,                               // OFFSET_CUSTOM_PARAMETERS 9
      0x00000B,                        // OFFSET_LVMODE_THRES
      0x000080,                        // OFFSET_REF_DELAY
      0x00000D,                        // OFFSET_ADCGAIN_ASR
      0x1009B9,                        // OFFSET_CNG_Q
      0x400000,                        // OFFSET_DTC_AGGR

      // Auxiliary gain
      0x7eca9c,                        // OFFSET_AUX_GAIN
      0x40000,                         // OFFSET_SCO_STREAM_MIX
      0x3f654e;                        // OFFSET_AUX_STREAM_MIX


   .VAR     CurParams[$M.CVC_HEADSET_2MIC.PARAMETERS.STRUCT_SIZE];
   // -------------------------------------------------------------------------------

   // DC Blocker Config Block - SPTBD - Replace with PEQs
.if uses_DCBLOCK
   // Filter format: b2,b1,b0,a2,a1
   .VAR/DM1CIRC dcblock_coeffs[5] =
      0.948607495176447/2, -1.897214990352894/2, 0.948607495176447/2,
      0.899857926182383/2, -1.894572054523406/2;
   .VAR/DM1       dcblock_scale = 1;
   .VAR/DM1       dcblock_gain_mts = 0.5;
   .VAR/DM1       dcblock_gain_exp = 0x000001;

   .VAR/DM2CIRC   in_l_dcblock_delaybuf_dm2[4]; // SIZE = (NUM_STAGES_FIELD+1)*2

   .VAR/DM2 in_l_dcblock_dm2[$audio_proc.peq.STRUC_SIZE] =
      0,                                        // PTR_INPUT_DATA_BUFF_FIELD
      0,                                        // INPUT_CIRCBUFF_SIZE_FIELD
      0,                                        // PTR_OUTPUT_DATA_BUFF_FIELD
      0,                                        // OUTPUT_CIRCBUFF_SIZE_FIELD
      &in_l_dcblock_delaybuf_dm2,               // PTR_DELAY_LINE_FIELD
      &dcblock_coeffs,                          // PTR_COEFS_BUFF_FIELD
      1,                                        // NUM_STAGES_FIELD
      0,                                        // DELAY_BUF_SIZE
      0,                                        // COEFF_BUF_SIZE
      64,                                       // BLOCK_SIZE_FIELD
      &dcblock_scale,                           // PTR_SCALE_BUFF_FIELD
      &dcblock_gain_exp,                        // INPUT_GAIN_EXPONENT_PTR
      &dcblock_gain_mts;                        // INPUT_GAIN_MANTISSA_PTR

   .VAR/DM2CIRC in_r_dcblock_delaybuf_dm2[4]; // SIZE = (NUM_STAGES_FIELD+1)*2

   .VAR/DM2 in_r_dcblock_dm2[$audio_proc.peq.STRUC_SIZE] =
      0,                                        // PTR_INPUT_DATA_BUFF_FIELD
      0,                                        // INPUT_CIRCBUFF_SIZE_FIELD
      0,                                        // PTR_OUTPUT_DATA_BUFF_FIELD
      0,                                        // OUTPUT_CIRCBUFF_SIZE_FIELD
      &in_r_dcblock_delaybuf_dm2,               // PTR_DELAY_LINE_FIELD
      &dcblock_coeffs,                          // PTR_COEFS_BUFF_FIELD
      1,                                        // NUM_STAGES_FIELD
      0,                                        // DELAY_BUF_SIZE
      0,                                        // COEFF_BUF_SIZE
      64,                                       // BLOCK_SIZE_FIELD
      &dcblock_scale,                           // PTR_SCALE_BUFF_FIELD
      &dcblock_gain_exp,                        // INPUT_GAIN_EXPONENT_PTR
      &dcblock_gain_mts;                        // INPUT_GAIN_MANTISSA_PTR

   .VAR/DM2CIRC   sco_dcblock_delaybuf_dm2[4]; // SIZE = (NUM_STAGES_FIELD+1)*2

   .VAR/DM2 sco_dcblock_dm2[$audio_proc.peq.STRUC_SIZE] =
      0,                                        // PTR_INPUT_DATA_BUFF_FIELD
      0,                                        // INPUT_CIRCBUFF_SIZE_FIELD
      0,                                        // PTR_OUTPUT_DATA_BUFF_FIELD
      0,                                        // OUTPUT_CIRCBUFF_SIZE_FIELD
      &sco_dcblock_delaybuf_dm2,                // PTR_DELAY_LINE_FIELD
      &dcblock_coeffs,                          // PTR_COEFS_BUFF_FIELD
      1,                                        // NUM_STAGES_FIELD
      0,                                        // DELAY_BUF_SIZE
      0,                                        // COEFF_BUF_SIZE
      64,                                       // BLOCK_SIZE_FIELD
      &dcblock_scale,                           // PTR_SCALE_BUFF_FIELD
      &dcblock_gain_exp,                        // INPUT_GAIN_EXPONENT_PTR
      &dcblock_gain_mts;                        // INPUT_GAIN_MANTISSA_PTR
.endif

   // FFT data object, common to all filter_bank cases
   // The three buffers in this object are temporary to FFT and could be shared
   .VAR fft_obj[$M.filter_bank.fft.STRUC_SIZE] =
      0,
      &fft_real,
      &fft_imag,
      &fft_circ,
      BITREVERSE(&fft_circ);

   // Analysis Filter Bank Config Block
   .VAR/DM1 AnalysisBank[$M.filter_bank.Parameters.TWO_CHNL_BLOCK_SIZE] =
      0,                                        // CH1_CONST_CBUF_LEN
      0,                                        // CH2_CONST_CBUF_LEN
      0,                                        // CH1_PTR_FRAME
      0,                                        // CH2_PTR_FRAME
      &bufd_l_inp,                              // OFFSET_CH1_PTR_HISTORY
      &bufd_r_inp,                              // OFFSET_CH2_PTR_HISTORY
      0,                                        // CH1_BEXP
      0,                                        // CH2_BEXP
      &D_l_real,                                // CH1_PTR_FFTREAL
      &D_l_imag,                                // CH1_PTR_FFTIMAG
      &D_r_real,                                // CH2_PTR_FFTREAL
      &D_r_imag;                                // CH2_PTR_FFTIMAG

   .VAR/DM1 AnalysisBank_AEC[$M.filter_bank.Parameters.ONE_CHNL_BLOCK_SIZE] =
      0,                               // CH1_CONST_CBUF_LEN
      0,                               // CH1_PTR_FRAME
      &bufd_aec_inp,                   // OFFSET_CH1_PTR_HISTORY
      0,                               // CH1_BEXP
      &D_rcv_real,                     // CH1_PTR_FFTREAL
      &D_rcv_imag,                     // CH1_PTR_FFTIMAG
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_REF_DELAY,  //OFFSET_DELAY_PTR
      &ref_delay_buffer,               // OFFSET_DELAY_BUF_PTR
      LENGTH(ref_delay_buffer);        // OFFSET_DELAY_BUF_LEN

   .VAR/DM1 AnalysisBank_ASR[$M.filter_bank.Parameters.ONE_CHNL_BLOCK_SIZE] =
      0,                               // CH1_CONST_CBUF_LEN
      0,                               // CH1_PTR_FRAME
      &bufd_l_inp,                     // OFFSET_CH1_PTR_HISTORY
      0,                               // CH1_BEXP
      &D_l_real,                       // CH1_PTR_FFTREAL
      &D_l_imag;                       // CH1_PTR_FFTIMAG

.if uses_RCV_FREQPROC
   .VAR/DM2  bufdr_outp[($M.CVC.Num_FFT_Window + $M.CVC.Num_Samples_Per_Frame)];
.endif

    // Analysis Filter Bank Config Block
   .VAR/DM1 AnalysisBank_rcv[$M.filter_bank.Parameters.ONE_CHNL_BLOCK_SIZE] =
      0,                               // CH1_CONST_CBUF_LEN
      0,                               // CH1_PTR_FRAME
      &bufd_rcv_inp,                   // OFFSET_CH1_PTR_HISTORY
      0,                               // CH1_BEXP
      &D_rcv_real,                     // CH1_PTR_FFTREAL
      &D_rcv_imag;                     // CH1_PTR_FFTIMAG

.if uses_RCV_FREQPROC
    // Synthesis Filter Bank Config Block
   .VAR/DM2 RcvSynthesisBank[$M.filter_bank.Parameters.ONE_CHNL_BLOCK_SIZE] =
      0,                               // OFFSET_CONST_CBUF_LEN
      0,                               // OFFSET_PTR_FRAME
      &bufdr_outp,                     // OFFSET_PTR_HISTORY
      &AnalysisBank_rcv + $M.filter_bank.Parameters.OFFSET_BEXP,
      &D_rcv_real,                         // OFFSET_PTR_FFTREAL
      &D_rcv_imag;                         // OFFSET_PTR_FFTIMAG
.endif



// NS
// The oms_scratch buffer reuses the AEC buffer to reduce
// the data memory usage.
.define oms_scratch $M.AEC_Headset.dm1.scratch1

.if uses_SND_NS
   // <start> of memory declared per instance of oms270
   .VAR/DM1CIRC sndLpX_queue[$M.oms270.QUE_LENGTH];
   .VAR sndoms_G[$M.oms270.FFT_NUM_BIN];
   .VAR sndoms_LpXnz[$M.oms270.FFT_NUM_BIN];
   .VAR sndoms_state[$M.oms270.STATE_LENGTH];

   .VAR oms270snd_obj[$M.oms270.STRUC_SIZE] =
        $M.oms270.mode.object,  //$M.oms270.PTR_MODE_FIELD
        0,                      // $M.oms270.CONTROL_WORD_FIELD
        $M.CVC_HEADSET_2MIC.CONFIG.SNDOMSENA,
                                // $M.oms270.ENABLE_BIT_MASK_FIELD
        1,                      // $M.oms270.MIN_SEARCH_ON_FIELD
        1,                      // $M.oms270.HARM_ON_FIELD
        1,                      // $M.oms270.MMSE_LSA_ON_FIELD
        &bufd_l_inp-2*$M.CVC.Num_Samples_Per_Frame,
                                // $M.oms270.PTR_INP_X_FIELD
        &D_r_real,              // $M.oms270.PTR_X_REAL_FIELD
        &D_r_imag,              // $M.oms270.PTR_X_IMAG_FIELD
        &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_BEXP,
                                // $M.oms270.PTR_BEXP_X_FIELD
        &D_l_real,              // $M.oms270.PTR_Y_REAL_FIELD
        &D_l_imag,              // $M.oms270.PTR_Y_IMAG_FIELD
        $M.oms270.MINUS1_Q8_16, // $M.oms270.INITIAL_POWER_FIELD
        &sndLpX_queue,          // $M.oms270.PTR_LPX_QUEUE_FIELD
        0,                      // $M.oms270.LPX_PREV_MIN_HHB_FIELD
        &sndoms_G,              // $M.oms270.G_FIELD;
        &sndoms_LpXnz,          // $M.oms270.LPXNZ_FIELD,
        &sndoms_state,          // $M.oms270.PTR_STATE_FIELD
        &oms_scratch,           // $M.oms270.PTR_SCRATCH_FIELD
        0.03921056084768,       // $M.oms270.ALFANZ_FIELD
        0xFF23A7,               // $M.oms270.LALFAS_FIELD
        0xFED889,               // $M.oms270.LALFAS1_FIELD
        0.45,                   // $M.oms270.HARMONICITY_THRESHOLD_FIELD
        0x733333,               // $M.oms270.VAD_THRESHOLD_FIELD
        0.95,                   // $M.oms270.AGRESSIVENESS_FIELD
        0;                      // $M.oms270.PTR_TONE_FLAG_FIELD
.endif

.if uses_RCV_NS
   // <start> of memory declared per instance of oms270
   .VAR/DM1CIRC rcvLpX_queue[$M.oms270.QUE_LENGTH];
   .VAR rcvoms_G[$M.oms270.FFT_NUM_BIN];
   .VAR rcvoms_LpXnz[$M.oms270.FFT_NUM_BIN];
   .VAR rcvoms_state[$M.oms270.STATE_LENGTH];

   .VAR oms270rcv_obj[$M.oms270.STRUC_SIZE] =
        $M.oms270.mode.object,  //$M.oms270.PTR_MODE_FIELD
        0,                      // $M.oms270.CONTROL_WORD_FIELD
        $M.CVC_HEADSET_2MIC.CONFIG.RCVOMSENA,
                                // $M.oms270.ENABLE_BIT_MASK_FIELD
        1,                      // $M.oms270.MIN_SEARCH_ON_FIELD
        1,                      // $M.oms270.HARM_ON_FIELD
        1,                      // $M.oms270.MMSE_LSA_ON_FIELD
        &bufd_rcv_inp-2*$M.CVC.Num_Samples_Per_Frame,
                                // $M.oms270.PTR_INP_X_FIELD
        &D_rcv_real,            // $M.oms270.PTR_X_REAL_FIELD
        &D_rcv_imag,            // $M.oms270.PTR_X_IMAG_FIELD
        &AnalysisBank_rcv + $M.filter_bank.Parameters.OFFSET_BEXP,
                                // $M.oms270.PTR_BEXP_X_FIELD
        &D_rcv_real,            // $M.oms270.PTR_Y_REAL_FIELD
        &D_rcv_imag,            // $M.oms270.PTR_Y_IMAG_FIELD
        $M.oms270.LDELTAP,      // $M.oms270.INITIAL_POWER_FIELD
        &rcvLpX_queue,          // $M.oms270.PTR_LPX_QUEUE_FIELD
        0,                      // $M.oms270.LPX_PREV_MIN_HHB_FIELD
        &rcvoms_G,              // $M.oms270.G_FIELD;
        &rcvoms_LpXnz,          // $M.oms270.LPXNZ_FIELD,
        &rcvoms_state,          // $M.oms270.PTR_STATE_FIELD
        &oms_scratch,           // $M.oms270.PTR_SCRATCH_FIELD
        0.03921056084768,       // $M.oms270.ALFANZ_FIELD
        0xFF23A7,               // $M.oms270.LALFAS_FIELD
        0xFED889,               // $M.oms270.LALFAS1_FIELD
        0.45,                   // $M.oms270.HARMONICITY_THRESHOLD_FIELD
        0x733333,               // $M.oms270.VAD_THRESHOLD_FIELD
        0.9,                    // $M.oms270.AGRESSIVENESS_FIELD
.if uses_AEQ
        &AEQ_DataObject + $M.AdapEq.AEQ_POWER_TEST_FIELD;
                                // $M.oms270.PTR_TONE_FLAG_FIELD
.else
        0;                      // $M.oms270.PTR_TONE_FLAG_FIELD
.endif

.endif
   .VAR/DM2CIRC ref_delay_buffer[Max_RefDelay_Sample];
.if uses_AEC
   .VAR/DM1 aec_dm1[$M.AEC_Headset.STRUCT_SIZE] =
      &D_r_real,                       // OFFSET_D_REAL_PTR
      &D_r_imag,                       // OFFSET_D_IMAG_PTR
      &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_PTR_BEXP,
      &D_rcv_real,                     // OFFSET_X_REAL_PTR
      &D_rcv_imag,                     // OFFSET_X_IMAG_PTR
      &AnalysisBank_AEC + $M.filter_bank.Parameters.OFFSET_PTR_BEXP,
      &sndoms_G,
      &sndoms_LpXnz,
      &sndoms_G,
      &fft_real,                        // OFFSET_E_REAL_PTR 9
      &fft_imag,                        // OFFSET_E_IMAG_PTR 10
      0.0,                            // OFFSET_OMS_AGGRESSIVENESS 11
      0x200000,                       // OFFSET_CNG_Q_ADJUST 12
      0x6Ed9eb,                       // OFFSET_CNG_G_ADJUST(unused) 13
      0.0,                            // OFFSET_DTC_AGRESSIVENESS 14
      0.5,                            // OFFSET_RER_WGT_L2PXR 15
      0.5,                            // OFFSET_RER_WGT_L2PDR 16
      0x00000E,                       // OFFSET_CONFIG
      0x000000;                       // OFFSET_ENABLE_REPEAT

.endif

.if uses_MTSF
   // disabled tier 2 by default
   .define MTSF_TIER2_ENABLED
   .VAR mtsf_gain_tier1[$M.CVC.Num_FFT_Freq_Bins];
   .VAR mtsf_phase_hb_tier1[3 * $M.CVC.Num_FFT_Freq_Bins];

.ifdef MTSF_TIER2_ENABLED
   .VAR mtsf_gain_tier2[$M.CVC.Num_FFT_Freq_Bins];
   .VAR mtsf_phase_hb_tier2[3 * $M.CVC.Num_FFT_Freq_Bins];
.endif
   // reuse oms/aec scratch = 3 * number of frequency bins
.define mtsf_scratch $M.AEC_Headset.dm1.scratch1

   .VAR mtsf_obj[$mtsf.STRUC_SIZE] =
      0,                 // CONFIG_FIELD
      $M.CVC_HEADSET_2MIC.CONFIG.BYPASS_MTSF,
                         // BYPASS_BIT_MASK_FIELD
      &D_l_real,         // PTR_X0_REAL_FIELD
      &D_l_imag,         // PTR_X0_IMAG_FIELD
      &D_r_real,         // PTR_X1_REAL_FIELD
      &D_r_imag,         // PTR_X1_IMAG_FIELD
      &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_BEXP,
                         // PTR_BEXP_X0_FIELD
      &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH2_BEXP,
                         // PTR_BEXP_X1_FIELD
      0x7fffff,          // TIER1_AGGR_FIELD
      0x666666,          // TIER2_AGGR_FIELD
.ifdef MTSF_TIER2_ENABLED
      1,                 // TIER2_ENABLE_FIELD
.else
      0,                 // TIER2_ENABLE_FIELD
.endif
      &mtsf_gain_tier1,  // PTR_TIER1_GAIN_FIELD
.ifdef MTSF_TIER2_ENABLED
      &mtsf_gain_tier2,  // PTR_TIER2_GAIN_FIELD
.else
      &ZeroValue,        // dummy pointer
.endif
      103872,            // SWAP_MARGIN_FIELD
      &mtsf_phase_hb_tier1,   // PTR_PHASE_HB_TIER1_FIELD
.ifdef MTSF_TIER2_ENABLED
      &mtsf_phase_hb_tier2,   // PTR_PHASE_HB_TIER2_FIELD
.else
      &ZeroValue,        // dummy pointer
.endif
      &mtsf_scratch,     // PTR_SCRATCH_FIELD
      $M.CVC.Num_FFT_Freq_Bins,
                         // NUM_FFT_BINS_FIELD
      0xF10E04,          // SILENCE_THRESH_FIELD
      5033165,           // COHERENCE_FIELD
      0,                 // WIND_DETECT_FIELD
      0x200000,          // WIND_THRESH_FIELD
      0,                 // LPDIFF_FIELD
      130053,            // LP_ALFA_FIELD
      0x9D752,           // COH_ATK_FIELD
      0x10519,           // COH_DEC_FIELD
      0x100000,          // D_HB_TIER1_FIELD (Q21)
      0,                 // D_LB_TIER1_FIELD (Q21)
      0x200000,          // D_TR_TIER1_FIELD (Q21)
      0x100000,          // D_HB_TIER2_FIELD (Q21)
      0x0B504F,          // D_LB_TIER2_FIELD (Q21)
      0x100000,          // D_TR_TIER2_FIELD (Q21)
      &D_r_real,         // PTR_X_REAL_TIER2_FIELD
      &D_r_imag,         // PTR_X_IMAG_TIER2_FIELD
      -3145728,          // PWR_FIELD
      0,                 // SWAP_LB_FIELD
      196608,            // SWAP_HB_FIELD
      0x500000,          // MIC_DIST_FIELD
      3,                 // WIN_LOW_IDX_FIELD
      4,                 // WIN_CNT_IDX_FIELD
      1,                 // SWAP_ENABLE_FIELD
      0,                 // G_MEAN_FIELD
      0x200000,          // SIN_HB_TIER1_FIELD (Q21)
      0,                 // SIN_LB_TIER1_FIELD (Q21)
      0x400000,          // SIN_TR_TIER1_FIELD (Q21)
      0x200000,          // SIN_HB_TIER2_FIELD (Q21)
      0x16A09E,          // SIN_LB_TIER2_FIELD (Q21)
      0x200000,          // SIN_TR_TIER2_FIELD (Q21)
      0x0CCCCC,          // GAIN_THRESH_FIELD
      0x000008;          // GAIN_THRESH_BINS_FIELD
.endif


.if uses_ADF
   .VAR/DM1 ADF_dm1[$M.adf_alg_1_0_0.STRUCT_SIZE] =
      &D_l_real,            // OFFSET_X0_REAL_PTR
      &D_l_imag,            // OFFSET_X0_IMAG_PTR
      &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_BEXP,            // OFFSET_X0_EXP_PTR
      &D_r_real,            // OFFSET_X1_REAL_PTR
      &D_r_imag,            // OFFSET_X1_IMAG_PTR
      &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH2_BEXP,            // OFFSET_X1_EXP_PTR

      0,                                              // OFFSET_OMS0_G_PTR
      0,                                              // OFFSET_OMS1_G_PTR
.if uses_SND_NS
      &sndoms_G,       // OFFSET_OMS2_G_PTR
.else
      0,
.endif
      &D_l_real,            // OFFSET_E_OMS_REAL_PTR
      &D_l_imag,            // OFFSET_E_OMS_IMAG_PTR
      &D_r_real,            // OFFSET_E_OUT_REAL_PTR
      &D_r_imag,            // OFFSET_E_OUT_IMAG_PTR
      $M.CVC_HEADSET_2MIC.CONFIG.BYPASS_ADF_PP_OMS12, // OFFSET_BYPASS_OMS12_MASK
      $M.CVC_HEADSET_2MIC.CONFIG.BYPASS_ADF_PP_OMS3,  // OFFSET_BYPASS_OMS3_MASK
      0x0003,              // OFFSET_CONTROL
      2,                   // OFFSET_PP_REPEAT
      0xFD95C1,            // OFFSET_PP_GAMMAP log2(gammaP/Np_sb), gammaP=0.75,Np_sb=4 : signed Q8.16
      0.5,                 // OFFSET_PP_THRES  1.0 - 0.65;  // threshold for high freq adapt decision
      1,                   // OFFSET_PP_VAD_DETECT
      0.35,                // OFFSET_PP_VAD_THRES
      0,                   // OFFSET_PP_CTRL_BIAS (0.0 dB)
      0x100000,            // OFFSET_PP_CTRL_TRANS (2.0 in Q19)
      0.000399920010665578,// OFFSET_MGDC_ALFAD
      0.00259525632413075, // OFFSET_MGDC_FRONTMICBIAS  Q8.16
      0.0155715379447845,  // OFFSET_MGDC_MAXCOMP       Q8.16
      0xA05417,            // OFFSET_MGDC_ADAPT_THRESH  Q8.16
      10,                  // OFFSET_MGDC_K_LB
      32,                  // OFFSET_MGDC_K_HB
      -4,                  // OFFSET_MGDC_MEAN_SCL_EXP
      0.727272727272727,   // OFFSET_MGDC_MEAN_SCL_MTS
.if uses_MTSF
      &mtsf_obj + $mtsf.WIND_DETECT_FIELD;
.else
      &ZeroValue;
.endif
                           // OFFSET_PTR_WIND_DETECT


   .VAR/DM1 aecpp_dm1[$M.AEC_Headset.STRUCT_SIZE] =
      &$M.adf_alg_1_0_0.dm2.V0_real,            // OFFSET_D_REAL_PTR
      &$M.adf_alg_1_0_0.dm1.V0_imag,            // OFFSET_D_IMAG_PTR
      &$M.adf_alg_1_0_0.dm1.BExp_X0,            // BExp
      0,                      // OFFSET_X_REAL_PTR
      0,                      // OFFSET_X_IMAG_PTR
      0,                      // BExp
      0,                      // OFFSET_OMS1_G_PTR
      0,                      // OFFSET_OMS1_D_NZ_PTR
      0,                      // OFFSET_OMS2_G_PTR
      &D_r_real,              // OFFSET_E_REAL_PTR 9
      &D_r_imag,              // OFFSET_E_IMAG_PTR 10
      0,                      // OFFSET_OMS_AGGRESSIVENESS 11
      0,                      // OFFSET_CNG_Q_ADJUST 12
      0,                      // OFFSET_CNG_G_ADJUST(unused) 13
      0,                      // OFFSET_DTC_AGRESSIVENESS 14
      0,                      // OFFSET_RER_WGT_L2PXR 15
      0,                      // OFFSET_RER_WGT_L2PDR 16
      0,                      // OFFSET_CONFIG
      0;                      // OFFSET_ENABLE_REPEAT (unused)

.endif

   // Synthesis Filter Bank Config Block
   .VAR/DM2 SynthesisBank[$M.filter_bank.Parameters.ONE_CHNL_BLOCK_SIZE] =
      0,                                        // OFFSET_CONST_CBUF_LEN
      0,                                        // OFFSET_PTR_FRAME
      &bufd_outp,                               // OFFSET_PTR_HISTORY
      &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_BEXP, // OFFSET_PTR_BEXP
      &D_r_real,                                // OFFSET_PTR_FFTREAL
      &D_r_imag;                                // OFFSET_PTR_FFTIMAG

.if uses_SND_PEQ
   // Parameteric EQ
   .VAR/DM2CIRC   snd_peq_delaybuf_dm2[2 * (MAX_NUM_PEQ_STAGES + 1)]; // SIZE = (NUM_STAGES_FIELD+1)*2
   .VAR/DM1CIRC   snd_peq_coeffs[5 * MAX_NUM_PEQ_STAGES];      // SIZE = 5*NUM_STAGES_FIELD
   .VAR snd_peq_scale[5];
   // This PEQ has a gain of 1, so it can be put before the AGC.
   .VAR/DM2 snd_peq_dm2[$audio_proc.peq.STRUC_SIZE] =
      0,                                        // PTR_INPUT_DATA_BUFF_FIELD
      0,                                        // INPUT_CIRCBUFF_SIZE_FIELD
      0,                                        // PTR_OUTPUT_DATA_BUFF_FIELD
      0,                                        // OUTPUT_CIRCBUFF_SIZE_FIELD
      &snd_peq_delaybuf_dm2,                    // PTR_DELAY_LINE_FIELD
      &snd_peq_coeffs,                          // PTR_COEFS_BUFF_FIELD
      0,                                        // NUM_STAGES_FIELD
      0,                                        // DELAY_BUF_SIZE
      0,                                        // COEFF_BUF_SIZE
      64,                                       // BLOCK_SIZE_FIELD
      &snd_peq_scale,                           // PTR_SCALE_BUFF_FIELD
      &ZeroValue,
      &OneValue;
.endif

   //SND AGC Pre-Gain stage
   .VAR/DM1 out_gain_dm1[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,    // OFFSET_INPUT_PTR
      0,    // OFFSET_INPUT_LEN
      0,    // OFFSET_OUTPUT_PTR
      0,    // OFFSET_OUTPUT_LEN
      64,      // NUM_SAMPLES
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SNDGAIN_MANTISSA, // OFFSET_PTR_MANTISSA
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SNDGAIN_EXPONENT; // OFFSET_PTR_EXPONENT
      // SND_AGC module


   .VAR/DM1 mute_cntrl_dm1[$M.MUTE_CONTROL.STRUC_SIZE] =
      0,                                        // OFFSET_INPUT_PTR
      0,                                        // OFFSET_INPUT_LEN
      64,                                       // OFFSET_NUM_SAMPLES
      &$M.CVC_HEADSET_2MIC.CurCallState,        // OFFSET_PTR_STATE
      $M.CVC.CALLST.MUTE;                       // OFFSET_MUTE_VAL

.if uses_SND_AGC
      // Send AGC
   .VAR/DM snd_agc400_dm[$M.agc400.STRUC_SIZE] =
      0,                   //OFFSET_SYS_CON_WORD_FIELD
      $M.CVC_HEADSET_2MIC.CONFIG.BYPASS_SNDAGC,
                           //OFFSET_BYPASS_BIT_MASK_FIELD
      0,                   //OFFSET_PTR_INPUT_FIELD
      64,                  //OFFSET_INPUT_CBUF_LEN_FIELD
      0,                   //OFFSET_PTR_OUTPUT_FIELD
      64,                  //OFFSET_OUTPUT_CBUF_LEN_FIELD
      64,                  //OFFSET_FRAME_SIZE_FIELD
.if uses_ADF
      &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_PP_VAD_DETECT,
.else
      0,
.endif
                           //OFFSET_PTR_VAD_VALUE_FIELD
      0x392CED,            //OFFSET_AGC_TARGET_FIELD (-7dB)
      0,                   //OFFSET_INPUT_LEVEL_FIELD  (initialized in AGC)
      0,                   //OFFSET_INPUT_LEVEL_MIN_FIELD (initialized in AGC)
      0x2081D,             //OFFSET_ATTACK_TC_FIELD
      0,                   //OFFSET_ONE_M_ATTACK_TC_FIELD   (initialized in AGC)
      0x10519,             //OFFSET_DECAY_TC_FIELD
      0,                   //OFFSET_ONE_M_DECAY_TC_FIELD (initialized in AGC)
      0x504DA,             //OFFSET_ALPHA_A_90_FIELD
      0,                   //OFFSET_ONE_M_ALPHA_A_90_FIELD  (initialized in AGC)
      0x914F,              //OFFSET_ALPHA_D_90_FIELD
      0,                   //OFFSET_ONE_M_ALPHA_D_90_FIELD  (initialized in AGC)
      0x0B3F30,            //OFFSET_G_MAX_FIELD (15 dB)
      0x020000,            //OFFSET_G_MIN_FIELD (0 dB)
      0x020000,            //OFFSET_G_FIELD
      0x020000,            //OFFSET_G_REAL_FIELD
      0x65AC8C,            //OFFSET_INPUT_THRESHOLD_FIELD
      0xF9E4C,             //OFFSET_ADAPT_THRESHOLD1_FIELD
      0xF21482,            //OFFSET_ADAPT_THRESHOLD2_FIELD
      0x180000,            //OFFSET_ATTACK_SPEED_FIELD
      0x599999,            //OFFSET_COMP_FIELD
      0,                   //OFFSET_ONE_M_COMP_FIELD  (initialized in AGC)
      0x7FFFFF,            //OFFSET_HARD_LIMIT_FIELD
      0,                   //OFFSET_PTR_TONE_FLAG_FIELD
      0x020000,            //OFFSET_INITIAL_GAIN_FIELD (Q17 format)
      0x28000;             //OFFSET_PWR_SCALE_FIELD (Q16 format)
.endif

.if uses_ADF && uses_SND_NS && uses_SND_AGC
   .VAR oms_vad_recalc[$M.CVC.oms_vad_recalc.STRUC_SIZE] =
      &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_PP_VAD_DETECT,
                           // PTR_VAD_VALUE_FIELD
      &vad400_dm2 + $M.vad400.FLAG_FIELD,
                           // PTR_RCV_VAD_FIELD
      0,                   // HOLD_COUNTER_FIELD
      4,                   // HOLD_VALUE_FIELD
      &sndoms_G,
                           // PTR_G_OMS_FIELD
      0.3,                 // MEAN_OMS_THRESH_FIELD
.if uses_MTSF
      &mtsf_obj + $mtsf.WIND_DETECT_FIELD;
.else
      &ZeroValue;
.endif

.endif

.if uses_RCV_PEQ
   .VAR/DM2CIRC   rcv_peq_delaybuf_dm2[2 * (MAX_NUM_PEQ_STAGES + 1)];// SIZE = (NUM_STAGES_FIELD+1)*2
   .VAR/DM1CIRC   rcv_peq_coeffs[5 * MAX_NUM_PEQ_STAGES];            // SIZE = 5*NUM_STAGES_FIELD
   .VAR rcv_peq_scale[5];
   .VAR/DM2 rcv_peq_dm2[$audio_proc.peq.STRUC_SIZE] =
      0,                                        // PTR_INPUT_DATA_BUFF_FIELD
      0,                                        // INPUT_CIRCBUFF_SIZE_FIELD
      0,                                        // PTR_OUTPUT_DATA_BUFF_FIELD
      0,                                        // OUTPUT_CIRCBUFF_SIZE_FIELD
      &rcv_peq_delaybuf_dm2,                    // PTR_DELAY_LINE_FIELD
      &rcv_peq_coeffs,                          // PTR_COEFS_BUFF_FIELD
      0,                                        // NUM_STAGES_FIELD
      0,                                        // DELAY_BUF_SIZE
      0,                                        // COEFF_BUF_SIZE
      64,                                       // BLOCK_SIZE_FIELD
      &rcv_peq_scale,                           // PTR_SCALE_BUFF_FIELD
      &ZeroValue,                               // INPUT_GAIN_EXPONENT_PTR
      &OneValue;                                // INPUT_GAIN_MANTISA_PTR
.endif



   // RCV fixed gain stage before AGC
   .VAR/DM1 rcv_pregain_dm1[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,    // OFFSET_INPUT_PTR
      0,    // OFFSET_INPUT_LEN
      0,    // OFFSET_OUTPUT_PTR
      0,    // OFFSET_OUTPUT_LEN
      64,   // NUM_SAMPLES
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCVGAIN_MANTISSA, // OFFSET_PTR_MANTISSA
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCVGAIN_EXPONENT; // OFFSET_PTR_EXPONENT

.if uses_RCV_VAD
   // Filter format: b2,b1,b0,a2,a1
   .VAR/DM1CIRC  rcv_vad_peq_coeffs[15] =
      3658586, -7303920, 3662890, 3363562, -7470041,
      3874204, -7787540, 4194304, 3702500, -7573428,
      4101184, -7581562, 4194304, 4082490, -7559795;

   .VAR/DM1      rcv_vad_peq_scale[3] = 1,1,1;  // SIZE = NUM_STAGES_FIELD
   .VAR/DMCIRC   rcv_vad_delaybuf[8];           // SIZE = (NUM_STAGES_FIELD+1)*2
   .VAR/DMCIRC   rcv_vad_peq_output[64];

   .VAR/DM rcv_vad_peq[$audio_proc.peq.STRUC_SIZE] =
      0,                                        // PTR_INPUT_DATA_BUFF_FIELD
      0,                                        // INPUT_CIRCBUFF_SIZE_FIELD
      &rcv_vad_peq_output,                      // PTR_OUTPUT_DATA_BUFF_FIELD
      64,                                       // OUTPUT_CIRCBUFF_SIZE_FIELD
      &rcv_vad_delaybuf,                        // PTR_DELAY_LINE_FIELD
      &rcv_vad_peq_coeffs,                      // PTR_COEFS_BUFF_FIELD
      3,                                        // NUM_STAGES_FIELD
      0,                                        // DELAY_BUF_SIZE
      0,                                        // COEFF_BUF_SIZE
      64,                                       // BLOCK_SIZE_FIELD
      &rcv_vad_peq_scale,                       // PTR_SCALE_BUFF_FIELD
      &ZeroValue,                               // INPUT_GAIN_EXPONENT_PTR
      &OneValue;                                // INPUT_GAIN_MANTISSA_PTR

   // RCV VAD
   .VAR/DM1 vad400_dm1[$M.vad400.DM1_OBJECT_SIZE_FIELD] =
      &rcv_vad_peq_output, // INPUT_PTR_FIELD
      64,                  // INPUT_LENGTH_FIELD
      0x1C5042,            // ENVELOPE_TC_FIELD
      0xFF6000,            // E_FILTER_FIELD
      0,                   // E_FILTER_MAX_FIELD
      0x5744,              // DECAY_TC_FIELD
      0x7FA8BC,            // ONEMINUS_DECAY_TC_FIELD
      0x5744,              // ATTACK_TC_FIELD
      0x7FA8BC,            // ONEMINUS_ATTACK_TC_FIELD
      0x400000,            // RATIO_FIELD
      0x8000,              // MIN_SIGNAL_FIELD
      0,                   // COUNTER_DELTA_FIELD
      56;                  // COUNT_THRESHOLD_FIELD

   .VAR/DM2 vad400_dm2[$M.vad400.DM2_OBJECT_SIZE_FIELD] =
      0,                   // E_FILTER_MIN_FIELD
      0x63AFBE,            // ONEMINUS_ENVELOPE_TC_FIELD
      0,                   // COUNTER_FIELD
      25,                  // INIT_FRAME_THRESH_FIELD
      0xFD4000,            // MIN_MAX_ENVELOPE_FIELD
      0x5000,              // DELTA_THRESHOLD
      0,                   // FLAG_FIELD
      64;                  // BLOCK_SIZE_FIELD

.endif

.if uses_RCV_AGC
   // RCV AGC
   .VAR/DM rcv_agc400_dm[$M.agc400.STRUC_SIZE] =
      0,                   //OFFSET_SYS_CON_WORD_FIELD
      $M.CVC_HEADSET_2MIC.CONFIG.BYPASS_RCVAGC,
                           //OFFSET_BYPASS_BIT_MASK_FIELD
      0,                   //OFFSET_PTR_INPUT_FIELD
      64,                  //OFFSET_INPUT_CBUF_LEN_FIELD
      0,                   //OFFSET_PTR_OUTPUT_FIELD
      64,                  //OFFSET_OUTPUT_CBUF_LEN_FIELD
      64,                  //OFFSET_FRAME_SIZE_FIELD
      &vad400_dm2 + $M.vad400.FLAG_FIELD,
                           //OFFSET_PTR_VAD_VALUE_FIELD
      0x392CED,            //OFFSET_AGC_TARGET_FIELD (-7dB)
      0,                   //OFFSET_INPUT_LEVEL_FIELD  (initialized in AGC)
      0,                   //OFFSET_INPUT_LEVEL_MIN_FIELD (initialized in AGC)
      0x2081D,             //OFFSET_ATTACK_TC_FIELD
      0,                   //OFFSET_ONE_M_ATTACK_TC_FIELD   (initialized in AGC)
      0x10519,             //OFFSET_DECAY_TC_FIELD
      0,                   //OFFSET_ONE_M_DECAY_TC_FIELD (initialized in AGC)
      0x504DA,             //OFFSET_ALPHA_A_90_FIELD
      0,                   //OFFSET_ONE_M_ALPHA_A_90_FIELD  (initialized in AGC)
      0x914F,              //OFFSET_ALPHA_D_90_FIELD
      0,                   //OFFSET_ONE_M_ALPHA_D_90_FIELD  (initialized in AGC)
      0x0B3F30,            //OFFSET_G_MAX_FIELD
      0x020000,            //OFFSET_G_MIN_FIELD
      0x020000,            //OFFSET_G_FIELD
      0x020000,            //OFFSET_G_REAL_FIELD
      0x65AC8C,            //OFFSET_INPUT_THRESHOLD_FIELD
      0xF9E4C,             //OFFSET_ADAPT_THRESHOLD1_FIELD
      0xF21482,            //OFFSET_ADAPT_THRESHOLD2_FIELD
      0x180000,            //OFFSET_ATTACK_SPEED_FIELD
      0x599999,            //OFFSET_COMP_FIELD
      0,                   //OFFSET_ONE_M_COMP_FIELD  (initialized in AGC)
      0x7FFFFF,            //OFFSET_HARD_LIMIT_FIELD
.if uses_AEQ
      &AEQ_DataObject + $M.AdapEq.AEQ_POWER_TEST_FIELD,
                           //OFFSET_PTR_TONE_FLAG_FIELD
.else
      0,
.endif
      0x040000,            //OFFSET_INITIAL_GAIN_FIELD (Q17 format)
      0x28000;             //OFFSET_PWR_SCALE_FIELD (Q16 format)
.endif

   // Hard Clipper
   .VAR/DM1 rcv_hc_dm1[$M.HC_Alg_1_0_0.Parameters.OBJECT_BLOCK_SIZE] =
      0,                         // OFFSET_PTR_IN_FRAME
      0,                         // OFFSET_CONST_CBUF_LEN_IN_FRAME
      0,                         // OFFSET_PTR_OUT_FRAME
      0,                         // OFFSET_CONST_CBUF_LEN_OUT_FRAME
      64,                        // OFFSET_FRAME_SIZE
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_INV_DAC_GAIN_TABLE, // OFFSET_GAIN_TABLE_PTR (0dB)
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_STF_GAIN_MANTISSA,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_STF_GAIN_EXP,
      0,                            // OFFSET_CLIP_POINT
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_BOOST_CLIP_POINT,
      0x23D70A,                        // OFFSET_SIDETONE_LIMIT 0x23D70A is 28%FS = -11 dBFS
      &$M.CVC_HEADSET_2MIC.current_side_tone_gain, // OFFSET_PTR_CURRENT_SIDETONE_GAIN {modified sidetone gain}
      1.0,                          // OFFSET_BOOST
      &$M.CVC_HEADSET_2MIC.PeakAux,      // OFFSET_PTR_PEAK_AUXVAL
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_CLIP_POINT, // OFFSET_ST_CLIP_POINT
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_ADJUST_LIMIT, // OFFSET_ST_ADJUST_LIMIT
      &$M.CVC_HEADSET_2MIC.PeakSideTone;                             // OFFSET_PTR_PEAK_ST

.if uses_NSVOLUME
   // <start> of memory declared per instance of oms270
   .VAR/DM1CIRC ndvcLpX_queue[$M.oms270.QUE_LENGTH];
   .VAR ndvcoms_G[$M.oms270.FFT_NUM_BIN];
   .VAR ndvcoms_LpXnz[$M.oms270.FFT_NUM_BIN];
   .VAR ndvcoms_state[$M.oms270.STATE_LENGTH];

   .VAR oms270NDVC_obj[$M.oms270.STRUC_SIZE] =
        $M.oms270.mode.object,  // $M.oms270.PTR_MODE_FIELD
        0,                      // $M.oms270.CONTROL_WORD_FIELD
        $M.CVC_HEADSET_2MIC.CONFIG.BYPASS_NDVC,
                                // $M.oms270.ENABLE_BIT_MASK_FIELD
        1,                      // $M.oms270.MIN_SEARCH_ON_FIELD
        0,                      // $M.oms270.HARM_ON_FIELD
        0,                      // $M.oms270.MMSE_LSA_ON_FIELD
        &buf_snd_harm,          // $M.oms270.PTR_INP_X_FIELD
        &D_l_real,              // $M.oms270.PTR_X_REAL_FIELD
        &D_l_imag,              // $M.oms270.PTR_X_IMAG_FIELD
        &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_BEXP,
                                // $M.oms270.PTR_BEXP_X_FIELD
        &D_l_real,              // $M.oms270.PTR_Y_REAL_FIELD
        &D_l_imag,              // $M.oms270.PTR_Y_IMAG_FIELD
        $M.oms270.MINUS1_Q8_16, // $M.oms270.INITIAL_POWER_FIELD
        &ndvcLpX_queue,         // $M.oms270.PTR_LPX_QUEUE_FIELD
        0,                      // $M.oms270.LPX_PREV_MIN_HHB_FIELD
        &ndvcoms_G,             // $M.oms270.G_FIELD;
        &ndvcoms_LpXnz,         // $M.oms270.LPXNZ_FIELD,
        &ndvcoms_state,         // $M.oms270.PTR_STATE_FIELD
        &oms_scratch,           // $M.oms270.PTR_SCRATCH_FIELD
        0.03921056084768,       // $M.oms270.ALFANZ_FIELD
        0xFF23A7,               // $M.oms270.LALFAS_FIELD
        0xFED889,               // $M.oms270.LALFAS1_FIELD
        0.45,                   // $M.oms270.HARMONICITY_THRESHOLD_FIELD
        0x733333,               // $M.oms270.VAD_THRESHOLD_FIELD
        0.95,                   // $M.oms270.AGRESSIVENESS_FIELD
        0;                      // $M.oms270.PTR_TONE_FLAG_FIELD

   // NDVC - Noise Controlled Volume
   .VAR/DM1 ndvc_noise_elv_tbl[8];              // Length equals OFFSET_NUMVOLSTEPS
   .VAR/DM1 ndvc_dm1[$M.NDVC_Alg1_0_0.Parameters.BLOCK_SIZE] =
      0,                         //OFFSET_CONTROL_WORD
      $M.CVC_HEADSET_2MIC.CONFIG.BYPASS_NDVC,                    //OFFSET_BITMASK_BYPASS
      &ndvcoms_LpXnz,  // OFFSET_PTR_LPDNZ
      2, /* 100hz */                   // OFFSET_LPDNZ_LB
      40, /* 2500hz */                 // OFFSET_LPDNZ_HB
      &ndvc_noise_elv_tbl,             // OFFSET_PTR_NOISELVLTBL
      250; /* 2s */                    // OFFSET_TIMER_RESET
.endif

.if uses_AEQ
   .VAR/DM aeq_band_pX[$M.AdapEq.Num_AEQ_Bands];
   .VAR/DM goal_low[$M.AdapEq.Num_AEQ_Bands] = 217706, 152394, 152394;
   .VAR/DM goal_high[$M.AdapEq.Num_AEQ_Bands] = 108853, 65312, 0;
   .VAR/DM AEQ_DataObject[$M.AdapEq.STRUC_SIZE] =
      0,
      $M.CVC_HEADSET_2MIC.CONFIG.AEQENA,
      &D_rcv_real,                                  // PTR_X_REAL_FIELD             2
      &D_rcv_imag,                                  // PTR_X_IMAG_FIELD             3
      &AnalysisBank_rcv + $M.filter_bank.Parameters.OFFSET_BEXP,   // PTR_BEXP_X_FIELD             4
      &D_rcv_real,                                  // PTR_Z_REAL_FIELD             5
      &D_rcv_imag,                                  // PTR_Z_IMAG_FIELD             6
      6-1,                                      // LOW_INDEX_FIELD              7
      8,                                        // LOW_BW_FIELD                 8
      8388608/8,                                // LOW_INV_INDEX_DIF_FIELD      9
      19,                                       // MID_BW_FIELD                 10
      (8388608/19),                             // MID_INV_INDEX_DIF_FIELD      11
      24,                                       // HIGH_BW_FIELD                12
      (8388608/24),                             // HIGH_INV_INDEX_DIF_FIELD     13
      0,                                        // AEQ_EQ_COUNTER_FIELD         14
      250,                                      // AEQ_EQ_INIT_FRAME_FIELD      15
      0,                                        // AEQ_GAIN_LOW_FIELD           16
      0,                                        // AEQ_GAIN_HIGH_FIELD          17
      &vad400_dm2 + $M.vad400.FLAG_FIELD,   	// VAD_AGC_FIELD                18
      0.00199794769287,                         // ALFA_A_FIELD                 19
      1-0.00199794769287,                       // ONE_MINUS_ALFA_A_FIELD       20
      0.00199794769287,                         // ALFA_D_FIELD                 21
      1-0.00199794769287,                       // ONE_MINUS_ALFA_D_FIELD       22
      0.03920936584473,                         // ALFA_ENV_FIELD               23
      1-0.03920936584473,                       // ONE_MINUS_ALFA_ENV_FIELD     24
      &aeq_band_pX,                             // PTR_AEQ_BAND_PX_FIELD        25
      0,                                        // STATE_FIELD                  26
      &$M.CVC_HEADSET_2MIC.SysDACadjust,             // PTR_VOL_STEP_UP_FIELD        27
      1,                                        // VOL_STEP_UP_TH1_FIELD        28
      2,                                        // VOL_STEP_UP_TH2_FIELD        29
      &goal_low,                                // PTR_GOAL_LOW_FIELD           30
      &goal_high,                               // PTR_GOAL_HIGH_FIELD          31
      14,                                       // MID1_INDEX_FIELD             32
      33,                                       // MID2_INDEX_FIELD             33
      57,                                       // HIGH_INDEX_FIELD             34
      98642,                                    // INV_AEQ_PASS_LOW_FIELD       35
      197283,                                   // INV_AEQ_PASS_HIGH_FIELD      36
      43541,                                    // AEQ_PASS_LOW_FIELD Q8.16     37
      21771,                                    // AEQ_PASS_HIGH_FIELD Q8.16    38
      544265,                                   // AEQ_POWER_TH_FIELD Q8.16     39
      0,                                        // AEQ_TONE_POWER_FIELD Q8.16   40
      -326559,                                  // AEQ_MIN_GAIN_TH_FIELD Q8.16  41
      326559,                                   // AEQ_MAX_GAIN_TH_FIELD Q8.16  42
      0;                                        // AEQ_POWER_TEST_FIELD         43
.endif
   // --------------------------------------------------------------------------
   // CVC sidetone filter block
   // --------------------------------------------------------------------------

   .define   SIDETONE_PEQ_STAGES   3
   .define   STF_PEQ_CTRL_STAGE1   1   // 1st stage is under control by PrmMgr
   .define   STF_PEQ_CTRL_STAGE2   2   // 2nd stage is under control by PrmMgr
   .define   STF_PEQ_CTRL_STAGE3   3   // 3rd stage is under control by PrmMgr

   // sidetone PEQ
   .VAR/DM2CIRC sidetone_peq_delaybuf_dm2[2 * (SIDETONE_PEQ_STAGES + 1)];
   .VAR/DM1CIRC sidetone_peq_coeffs[5 * SIDETONE_PEQ_STAGES] =
        0x373CC9, 0x91866D, 0x373CC9, 0x2FAE28, 0x92BB02,
        0x373CC9, 0x91866D, 0x373CC9, 0x2FAE28, 0x92BB02,
        0x373CC9, 0x91866D, 0x373CC9, 0x2FAE28, 0x92BB02;
   .VAR/DM1     sidetone_peq_scale[SIDETONE_PEQ_STAGES] = 1, 1, 1;
   .VAR/DM2	    sidetone_filter_object_dm2[$CVC.sidetone_filter.STRUC_SIZE] =
      0,                               // SWITCH_FIELD
      0,                               // NOISE_LOW_THRES_FIELD
      0,                               // NOISE_HIGH_THRES_FIELD
.if uses_NSVOLUME	                   // NOISE_LEVEL_PTR_FIELD: from NDVC FILTSUMLPDNZ
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_FILTSUMLPDNZ,
.else
      &ZeroValue,
.endif
      0,                               // Config
      // Below starts sidetone PEQ fields
      0,                               // PTR_INPUT_DATA_BUFF_FIELD
      0,                               // INPUT_CIRCBUFF_SIZE_FIELD
      0,                               // PTR_OUTPUT_DATA_BUFF_FIELD
      0,                               // OUTPUT_CIRCBUFF_SIZE_FIELD
      &sidetone_peq_delaybuf_dm2,      // PTR_DELAY_LINE_FIELD
      &sidetone_peq_coeffs,            // PTR_COEFS_BUFF_FIELD
      SIDETONE_PEQ_STAGES,             // NUM_STAGES_FIELD
      0,                               // DELAYLINE_SIZE_FIELD
      0,                               // COEFS_SIZE_FIELD
      0,                               // BLOCK_SIZE_FIELD
      &sidetone_peq_scale,
      // Gain: Exponent
      &ZeroValue,
      // Gain: Mantissa
      &OneValue;

   // This gain is used in ASR mode when PEQ and AGC do not get called.
   .VAR/DM1 out_gain_asr[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,    // OFFSET_INPUT_PTR
      0,    // OFFSET_INPUT_LEN
      0,    // OFFSET_OUTPUT_PTR
      0,    // OFFSET_OUTPUT_LEN
      64,      // NUM_SAMPLES
      &OneValue, // OFFSET_PTR_MANTISSA
      &ZeroValue; // OFFSET_PTR_EXPONENT


   .VAR     mic_in_l_pk_dtct[] =
      0,                         // PTR_INPUT_BUFFER_FIELD
      0,                         // INPUT_BUFF_SIZE_FIELD
      64,                        // NUM_SAMPLES_FEILD
      &$M.CVC_HEADSET_2MIC.PeakMicLeft;   // PEAK_LEVEL_PTR

   .VAR     mic_in_r_pk_dtct[] =
      0,                         // PTR_INPUT_BUFFER_FIELD
      0,                         // INPUT_BUFF_SIZE_FIELD
      64,                        // NUM_SAMPLES_FEILD
      &$M.CVC_HEADSET_2MIC.PeakMicRight;  // PEAK_LEVEL_PTR


   .VAR     sco_in_pk_dtct[] =
      0,                         // PTR_INPUT_BUFFER_FIELD
      0,                         // INPUT_BUFF_SIZE_FIELD
      64,                        // NUM_SAMPLES_FEILD
      &$M.CVC_HEADSET_2MIC.PeakScoIn;  // PEAK_LEVEL_PTR

   .VAR     sco_out_pk_dtct[] =
      0,                         // PTR_INPUT_BUFFER_FIELD
      0,                         // INPUT_BUFF_SIZE_FIELD
      64,                        // NUM_SAMPLES_FEILD
      &$M.CVC_HEADSET_2MIC.PeakScoOut;// PEAK_LEVEL_PTR

   .VAR     spkr_out_pk_dtct[] =
      0,                         // PTR_INPUT_BUFFER_FIELD
      0,                         // INPUT_BUFF_SIZE_FIELD
      64,                        // NUM_SAMPLES_FEILD
      &$M.CVC_HEADSET_2MIC.PeakSpkr;   // PEAK_LEVEL_PTR


   .VAR     ModeControl[$M.SET_MODE_GAIN.STRUC_SIZE] =
      &$M.CVC_HEADSET_2MIC.cur_mode;

   .VAR     adc_mixer[$M.audio_proc.stream_mixer.STRUC_SIZE] =
      0,                         // $M.audio_proc.stream_mixer.OFFSET_INPUT_CH1_PTR
      0,                         // $M.audio_proc.stream_mixer.OFFSET_INPUT_CH1_LEN
      0,                         // $M.audio_proc.stream_mixer.OFFSET_INPUT_CH2_PTR
      0,                         // $M.audio_proc.stream_mixer.OFFSET_INPUT_CH2_LEN
      0,                         // $M.audio_proc.stream_mixer.OFFSET_OUTPUT_PTR
      0,                         // $M.audio_proc.stream_mixer.OFFSET_OUTPUT_LEN
      64,                        // $M.audio_proc.stream_mixer.NUM_SAMPLES
      &ModeControl + $M.SET_MODE_GAIN.ADC_MIXER_MANT_LEFT,   // $M.audio_proc.stream_mixer.OFFSET_PTR_CH1_MANTISSA
      &ModeControl + $M.SET_MODE_GAIN.ADC_MIXER_MANT_RIGHT,  // $M.audio_proc.stream_mixer.OFFSET_PTR_CH2_MANTISSA
      &ModeControl + $M.SET_MODE_GAIN.ADC_MIXER_EXP;         // $M.audio_proc.stream_mixer.OFFSET_PTR_EXPONENT

   .VAR     sco_gain[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,
      0,
      0,
      0,
      64,
      &ModeControl + $M.SET_MODE_GAIN.SCO_GAIN_MANT, // OFFSET_PTR_MANTISSA
      &ModeControl + $M.SET_MODE_GAIN.SCO_GAIN_EXP;  // OFFSET_PTR_EXPONENT

   // -------------------------------------------------------------------------------
   // Table of functions for current mode
   .VAR  ModeProcTable[$M.CVC.SYSMODE.MAX_MODES] =
      &PsThru_proc_funcs,      // undefined state
      &hfk_proc_funcs,         // hfk mode
      &asr_proc_funcs,         // asr mode
      &PsThru_proc_funcs,      // pass-thru left mode
      &PsThru_proc_funcs,      // pass-thru right mode
      &LpBack_proc_funcs,      // loop-back mode
      &PsThru_proc_funcs;      // standby-mode

   // -------Stream Distribution and Update tables ---------------------
   // sndin stream map
   .VAR     stream_map_left_adc[] =

.if uses_DCBLOCK
      &in_l_dcblock_dm2 + $audio_proc.peq.INPUT_ADDR_FIELD,
      &in_l_dcblock_dm2 + $audio_proc.peq.INPUT_SIZE_FIELD,
      &in_l_dcblock_dm2 + $audio_proc.peq.OUTPUT_ADDR_FIELD,
      &in_l_dcblock_dm2 + $audio_proc.peq.OUTPUT_SIZE_FIELD,
.endif

      &mic_in_l_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &mic_in_l_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD,
      &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_PTR_FRAME,
      &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_CONST_CBUF_LEN,
      &AnalysisBank_ASR + $M.filter_bank.Parameters.OFFSET_PTR_FRAME,
      &AnalysisBank_ASR + $M.filter_bank.Parameters.OFFSET_CONST_CBUF_LEN,
      &adc_mixer + $M.audio_proc.stream_mixer.OFFSET_INPUT_CH1_PTR,
      &adc_mixer + $M.audio_proc.stream_mixer.OFFSET_INPUT_CH1_LEN;
   // sndin stream map
   .VAR     stream_map_right_adc[] =

.if uses_DCBLOCK
      &in_r_dcblock_dm2 + $audio_proc.peq.INPUT_ADDR_FIELD,
      &in_r_dcblock_dm2 + $audio_proc.peq.INPUT_SIZE_FIELD,
      &in_r_dcblock_dm2 + $audio_proc.peq.OUTPUT_ADDR_FIELD,
      &in_r_dcblock_dm2 + $audio_proc.peq.OUTPUT_SIZE_FIELD,
.endif

      &mic_in_r_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &mic_in_r_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD,
      &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH2_PTR_FRAME,
      &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH2_CONST_CBUF_LEN,
      &adc_mixer + $M.audio_proc.stream_mixer.OFFSET_INPUT_CH2_PTR,
      &adc_mixer + $M.audio_proc.stream_mixer.OFFSET_INPUT_CH2_LEN;
   // rcvin stream map
   .VAR     stream_map_sco_in_hfk_asr[] =

.if uses_DCBLOCK
      &sco_dcblock_dm2 + $audio_proc.peq.INPUT_ADDR_FIELD,
      &sco_dcblock_dm2 + $audio_proc.peq.INPUT_SIZE_FIELD,
      &sco_dcblock_dm2 + $audio_proc.peq.OUTPUT_ADDR_FIELD,
      &sco_dcblock_dm2 + $audio_proc.peq.OUTPUT_SIZE_FIELD,
.endif

      &sco_in_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &sco_in_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD,

.if uses_RCV_PEQ
      &rcv_peq_dm2 + $audio_proc.peq.INPUT_ADDR_FIELD,
      &rcv_peq_dm2 + $audio_proc.peq.INPUT_SIZE_FIELD,
      &rcv_peq_dm2 + $audio_proc.peq.OUTPUT_ADDR_FIELD,
      &rcv_peq_dm2 + $audio_proc.peq.OUTPUT_SIZE_FIELD,
.endif

      &rcv_pregain_dm1 + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &rcv_pregain_dm1 + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN,
      &rcv_pregain_dm1 + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &rcv_pregain_dm1 + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN,

.if uses_RCV_VAD
      &rcv_vad_peq + $audio_proc.peq.INPUT_ADDR_FIELD,
      &rcv_vad_peq + $audio_proc.peq.INPUT_SIZE_FIELD,
      // Output of PEQ is input to VAD
      // Input to PEQ and VAD output is then input to AGC
.endif
.if uses_RCV_AGC
      &rcv_agc400_dm + $M.agc400.OFFSET_PTR_INPUT_FIELD,
      &rcv_agc400_dm + $M.agc400.OFFSET_INPUT_CBUF_LEN_FIELD,
      &rcv_agc400_dm + $M.agc400.OFFSET_PTR_OUTPUT_FIELD,
      &rcv_agc400_dm + $M.agc400.OFFSET_OUTPUT_CBUF_LEN_FIELD,
.endif

.if uses_RCV_FREQPROC
      &AnalysisBank_rcv + $M.filter_bank.Parameters.OFFSET_PTR_FRAME,
      &AnalysisBank_rcv + $M.filter_bank.Parameters.OFFSET_CONST_CBUF_LEN,
      &RcvSynthesisBank + $M.filter_bank.Parameters.OFFSET_PTR_FRAME,
      &RcvSynthesisBank + $M.filter_bank.Parameters.OFFSET_CONST_CBUF_LEN,
.endif

      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_PTR_IN_FRAME,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CONST_CBUF_LEN_IN_FRAME;

   // sndout stream map
   .VAR     stream_map_sco_out[] =
      &SynthesisBank + $M.filter_bank.Parameters.OFFSET_PTR_FRAME,
      &SynthesisBank + $M.filter_bank.Parameters.OFFSET_CONST_CBUF_LEN,
      &out_gain_asr + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &out_gain_asr + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN,
      &out_gain_asr + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &out_gain_asr + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN,
      &out_gain_dm1 + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &out_gain_dm1 + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN,
      &out_gain_dm1 + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &out_gain_dm1 + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN,

.if uses_SND_AGC
      &snd_agc400_dm + $M.agc400.OFFSET_PTR_INPUT_FIELD,
      &snd_agc400_dm + $M.agc400.OFFSET_INPUT_CBUF_LEN_FIELD,
      &snd_agc400_dm + $M.agc400.OFFSET_PTR_OUTPUT_FIELD,
      &snd_agc400_dm + $M.agc400.OFFSET_OUTPUT_CBUF_LEN_FIELD,
.endif

.if uses_SND_PEQ
      &snd_peq_dm2 + $audio_proc.peq.OUTPUT_ADDR_FIELD,
      &snd_peq_dm2 + $audio_proc.peq.OUTPUT_SIZE_FIELD,
      &snd_peq_dm2 + $audio_proc.peq.INPUT_ADDR_FIELD,
      &snd_peq_dm2 + $audio_proc.peq.INPUT_SIZE_FIELD,
.endif

      &mute_cntrl_dm1 + $M.MUTE_CONTROL.OFFSET_INPUT_PTR,
      &mute_cntrl_dm1 + $M.MUTE_CONTROL.OFFSET_INPUT_LEN,
      &sco_out_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &sco_out_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD,
      &adc_mixer + $M.audio_proc.stream_mixer.OFFSET_OUTPUT_PTR,
      &adc_mixer + $M.audio_proc.stream_mixer.OFFSET_OUTPUT_LEN;
   // rcvout stream map
   .VAR    stream_map_dac_hfk_asr[] =
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_PTR_OUT_FRAME,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CONST_CBUF_LEN_OUT_FRAME,
.if uses_AEC
      &AnalysisBank_AEC + $M.filter_bank.Parameters.OFFSET_PTR_FRAME,
      &AnalysisBank_AEC + $M.filter_bank.Parameters.OFFSET_CONST_CBUF_LEN,
.endif
      &spkr_out_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &spkr_out_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD;

   // -------------------------------------------------------------------------------
   // rcvin stream map
   .VAR     stream_map_sco_in_passthr_lpbk[] =
      &sco_in_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &sco_in_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD,
      &sco_gain + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &sco_gain + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN;
   // rcvout stream map
   .VAR    stream_map_dac_passthr[] =
      &sco_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &sco_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_PTR_IN_FRAME,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CONST_CBUF_LEN_IN_FRAME,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_PTR_OUT_FRAME,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CONST_CBUF_LEN_OUT_FRAME,
      &spkr_out_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &spkr_out_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD;
   // -------------------------------------------------------------------------------
   // sndout stream map
   .VAR     stream_map_sco_out_lpbk[] =
      &sco_out_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &sco_out_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD,
      &sco_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &sco_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN;
   .VAR    stream_map_dac_lpbk[] =
      &adc_mixer + $M.audio_proc.stream_mixer.OFFSET_OUTPUT_PTR,
      &adc_mixer + $M.audio_proc.stream_mixer.OFFSET_OUTPUT_LEN,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_PTR_IN_FRAME,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CONST_CBUF_LEN_IN_FRAME,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_PTR_OUT_FRAME,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CONST_CBUF_LEN_OUT_FRAME,
      &spkr_out_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &spkr_out_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD;

   // Parameter to Module Map
   .VAR/DM2 ParameterMap[] =

   // sco gain
   &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SCO_STREAM_MIX,
   &$M.App.AuxilaryAudio.Tone.ScoGain,

   // aux gain
   &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AUX_STREAM_MIX,
   &$M.App.AuxilaryAudio.Tone.AuxGain,
.if uses_AEC
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_OMS_AGGR,    &aec_dm1 + $M.AEC_Headset.OFFSET_OMS_AGGRESSIVENESS,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_CNG_Q,           &aec_dm1 + $M.AEC_Headset.OFFSET_CNG_Q_ADJUST,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_DTC_AGGR,        &aec_dm1 + $M.AEC_Headset.OFFSET_DTC_AGRESSIVENESS,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,      &aec_dm1 + $M.AEC_Headset.OFFSET_CONFIG,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEC_RPT_ENABLE,  &aec_dm1 + $M.AEC_Headset.OFFSET_ENABLE_REPEAT,
.endif

.if uses_ADF
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,      &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_CONTROL,
.endif

.if uses_SND_NS
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,      &oms270snd_obj + $M.oms270.CONTROL_WORD_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_OMS_AGGR,    &oms270snd_obj + $M.oms270.AGRESSIVENESS_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_OMS_HARMONICITY, &oms270snd_obj + $M.oms270.HARM_ON_FIELD,
.endif

.if uses_RCV_NS
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,      &oms270rcv_obj + $M.oms270.CONTROL_WORD_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_OMS_HFK_AGGR,&oms270rcv_obj + $M.oms270.AGRESSIVENESS_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_OMS_HI_RES_MODE, &oms270rcv_obj + $M.oms270.HARM_ON_FIELD,
.endif
.if uses_RCV_VAD
      // RCV VAD parameters
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_VAD_ATTACK_TC,         &vad400_dm1 + $M.vad400.ATTACK_TC_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_VAD_DECAY_TC,          &vad400_dm1 + $M.vad400.DECAY_TC_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_VAD_ENVELOPE_TC,       &vad400_dm1 + $M.vad400.ENVELOPE_TC_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_VAD_INIT_FRAME_THRESH, &vad400_dm2 + $M.vad400.INIT_FRAME_THRESH_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_VAD_RATIO,             &vad400_dm1 + $M.vad400.RATIO_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_VAD_MIN_SIGNAL,        &vad400_dm1 + $M.vad400.MIN_SIGNAL_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_VAD_MIN_MAX_ENVELOPE,  &vad400_dm2 + $M.vad400.MIN_MAX_ENVELOPE_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_VAD_DELTA_THRESHOLD,   &vad400_dm2 + $M.vad400.DELTA_THRESHOLD_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_VAD_COUNT_THRESHOLD,   &vad400_dm1 + $M.vad400.COUNT_THRESHOLD_FIELD,
.endif

.if uses_RCV_AGC
      // RCV AGC parameters
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,         &rcv_agc400_dm + $M.agc400.OFFSET_SYS_CON_WORD_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_TARGET,     &rcv_agc400_dm + $M.agc400.OFFSET_AGC_TARGET_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_ATTACK_TC,  &rcv_agc400_dm + $M.agc400.OFFSET_ATTACK_TC_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_DECAY_TC,   &rcv_agc400_dm + $M.agc400.OFFSET_DECAY_TC_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_A_90_PK,    &rcv_agc400_dm + $M.agc400.OFFSET_ALPHA_A_90_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_D_90_PK,    &rcv_agc400_dm + $M.agc400.OFFSET_ALPHA_D_90_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_G_MAX,      &rcv_agc400_dm + $M.agc400.OFFSET_G_MAX_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_COMP,       &rcv_agc400_dm + $M.agc400.OFFSET_COMP_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_INP_THRESH, &rcv_agc400_dm + $M.agc400.OFFSET_INPUT_THRESHOLD_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_SP_ATTACK,  &rcv_agc400_dm + $M.agc400.OFFSET_ATTACK_SPEED_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_AD_THRESH1, &rcv_agc400_dm + $M.agc400.OFFSET_ADAPT_THRESHOLD1_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_AD_THRESH2, &rcv_agc400_dm + $M.agc400.OFFSET_ADAPT_THRESHOLD2_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_AGC_G_MIN,      &rcv_agc400_dm + $M.agc400.OFFSET_G_MIN_FIELD,
.endif


.if uses_SND_AGC
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,         &snd_agc400_dm + $M.agc400.OFFSET_SYS_CON_WORD_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_TARGET,     &snd_agc400_dm + $M.agc400.OFFSET_AGC_TARGET_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_ATTACK_TC,  &snd_agc400_dm + $M.agc400.OFFSET_ATTACK_TC_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_DECAY_TC,   &snd_agc400_dm + $M.agc400.OFFSET_DECAY_TC_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_A_90_PK,    &snd_agc400_dm + $M.agc400.OFFSET_ALPHA_A_90_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_D_90_PK,    &snd_agc400_dm + $M.agc400.OFFSET_ALPHA_D_90_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_G_MAX,      &snd_agc400_dm + $M.agc400.OFFSET_G_MAX_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_COMP,       &snd_agc400_dm + $M.agc400.OFFSET_COMP_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_INP_THRESH, &snd_agc400_dm + $M.agc400.OFFSET_INPUT_THRESHOLD_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_SP_ATTACK,  &snd_agc400_dm + $M.agc400.OFFSET_ATTACK_SPEED_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_AD_THRESH1, &snd_agc400_dm + $M.agc400.OFFSET_ADAPT_THRESHOLD1_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_AD_THRESH2, &snd_agc400_dm + $M.agc400.OFFSET_ADAPT_THRESHOLD2_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_AGC_G_MIN,      &snd_agc400_dm + $M.agc400.OFFSET_G_MIN_FIELD,
.endif

.if uses_NSVOLUME
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_NDVC_HYSTERESIS,    &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_HYSTERESIS,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_NDVC_ATK_TC,        &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_ATK_TC,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_NDVC_DEC_TC,        &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_DEC_TC,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_NDVC_NUMVOLSTEPS,   &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_NUMVOLSTEPS,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_NDVC_MAXNOISELVL,   &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_MAXNOISELVL,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_NDVC_MINNOISELVL,   &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_MINNOISELVL,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_NDVC_LB,            &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_LPDNZ_LB,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_NDVC_HB,            &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_LPDNZ_HB,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,         &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_CONTROL_WORD,   //control word for bypass
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,         &oms270NDVC_obj + $M.oms270.CONTROL_WORD_FIELD,
.endif

.if uses_SND_PEQ
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_NUMSTAGES,  &snd_peq_dm2 + $audio_proc.peq.NUM_STAGES_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE1_B2,  &snd_peq_coeffs,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE1_B1,  &snd_peq_coeffs + 1,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE1_B0,  &snd_peq_coeffs + 2,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE1_A2,  &snd_peq_coeffs + 3,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE1_A1,  &snd_peq_coeffs + 4,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE2_B2,  &snd_peq_coeffs + 5,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE2_B1,  &snd_peq_coeffs + 6,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE2_B0,  &snd_peq_coeffs + 7,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE2_A2,  &snd_peq_coeffs + 8,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE2_A1,  &snd_peq_coeffs + 9,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE3_B2,  &snd_peq_coeffs + 10,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE3_B1,  &snd_peq_coeffs + 11,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE3_B0,  &snd_peq_coeffs + 12,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE3_A2,  &snd_peq_coeffs + 13,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE3_A1,  &snd_peq_coeffs + 14,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE4_B2,  &snd_peq_coeffs + 15,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE4_B1,  &snd_peq_coeffs + 16,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE4_B0,  &snd_peq_coeffs + 17,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE4_A2,  &snd_peq_coeffs + 18,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE4_A1,  &snd_peq_coeffs + 19,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE5_B2,  &snd_peq_coeffs + 20,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE5_B1,  &snd_peq_coeffs + 21,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE5_B0,  &snd_peq_coeffs + 22,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE5_A2,  &snd_peq_coeffs + 23,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_STAGE5_A1,  &snd_peq_coeffs + 24,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_SCALE1,     &snd_peq_scale,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_SCALE2,     &snd_peq_scale + 1,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_SCALE3,     &snd_peq_scale + 2,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_SCALE4,     &snd_peq_scale + 3,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SND_PEQ_SCALE5,     &snd_peq_scale + 4,
.endif

.if uses_RCV_PEQ
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_NUMSTAGES,  &rcv_peq_dm2+$audio_proc.peq.NUM_STAGES_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE1_B2,  &rcv_peq_coeffs,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE1_B1,  &rcv_peq_coeffs + 1,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE1_B0,  &rcv_peq_coeffs + 2,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE1_A2,  &rcv_peq_coeffs + 3,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE1_A1,  &rcv_peq_coeffs + 4,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE2_B2,  &rcv_peq_coeffs + 5,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE2_B1,  &rcv_peq_coeffs + 6,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE2_B0,  &rcv_peq_coeffs + 7,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE2_A2,  &rcv_peq_coeffs + 8,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE2_A1,  &rcv_peq_coeffs + 9,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE3_B2,  &rcv_peq_coeffs + 10,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE3_B1,  &rcv_peq_coeffs + 11,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE3_B0,  &rcv_peq_coeffs + 12,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE3_A2,  &rcv_peq_coeffs + 13,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE3_A1,  &rcv_peq_coeffs + 14,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE4_B2,  &rcv_peq_coeffs + 15,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE4_B1,  &rcv_peq_coeffs + 16,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE4_B0,  &rcv_peq_coeffs + 17,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE4_A2,  &rcv_peq_coeffs + 18,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE4_A1,  &rcv_peq_coeffs + 19,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE5_B2,  &rcv_peq_coeffs + 20,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE5_B1,  &rcv_peq_coeffs + 21,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE5_B0,  &rcv_peq_coeffs + 22,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE5_A2,  &rcv_peq_coeffs + 23,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_STAGE5_A1,  &rcv_peq_coeffs + 24,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_SCALE1,     &rcv_peq_scale,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_SCALE2,     &rcv_peq_scale + 1,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_SCALE3,     &rcv_peq_scale + 2,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_SCALE4,     &rcv_peq_scale + 3,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_RCV_PEQ_SCALE5,     &rcv_peq_scale + 4,
.endif

      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_CLIP_POINT,      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CLIP_POINT,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_BOOST_CLIP_POINT,&rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_BOOST_CLIP_POINT,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_SIDETONE_LIMIT,  &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_SIDETONE_LIMIT,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_BOOST,           &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_BOOST,

      // sidetone filter control
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_STF_SWITCH,
      &sidetone_filter_object_dm2 + $CVC.sidetone_filter.SWITCH_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_STF_NOISE_LOW_THRES,
      &sidetone_filter_object_dm2 + $CVC.sidetone_filter.NOISE_LOW_THRES_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_STF_NOISE_HIGH_THRES,
      &sidetone_filter_object_dm2 + $CVC.sidetone_filter.NOISE_HIGH_THRES_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_NUMSTAGES,
      &sidetone_filter_object_dm2 + $CVC.sidetone_filter.PEQ_START_FIELD + $audio_proc.peq.NUM_STAGES_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE1_B2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE1 - 1)*5 + 0,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE1_B1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE1 - 1)*5 + 1,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE1_B0,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE1 - 1)*5 + 2,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE1_A2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE1 - 1)*5 + 3,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE1_A1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE1 - 1)*5 + 4,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_SCALE1,         &sidetone_peq_scale + (STF_PEQ_CTRL_STAGE1 - 1),
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE2_B2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE2 - 1)*5 + 0,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE2_B1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE2 - 1)*5 + 1,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE2_B0,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE2 - 1)*5 + 2,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE2_A2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE2 - 1)*5 + 3,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE2_A1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE2 - 1)*5 + 4,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_SCALE2,         &sidetone_peq_scale + (STF_PEQ_CTRL_STAGE2 - 1),
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE3_B2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE3 - 1)*5 + 0,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE3_B1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE3 - 1)*5 + 1,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE3_B0,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE3 - 1)*5 + 2,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE3_A2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE3 - 1)*5 + 3,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_STAGE3_A1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE3 - 1)*5 + 4,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ST_PEQ_SCALE3,         &sidetone_peq_scale + (STF_PEQ_CTRL_STAGE3 - 1),

      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,
      &sidetone_filter_object_dm2 + $CVC.sidetone_filter.OFFSET_ST_CONFIG,

.if uses_ADF
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,             &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_CONTROL,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_DMSS_PP_RPTCNT,         &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_PP_REPEAT,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_DMSS_PP_GAMMAP,         &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_PP_GAMMAP,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_DMSS_PP_THRES,          &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_PP_THRES,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_DMSS_PP_CTRL_BIAS,      &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_PP_CTRL_BIAS,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_DMSS_PP_CTRL_TRANS,     &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_PP_CTRL_TRANS,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MGDC_ALFAD,             &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_MGDC_ALFAD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MGDC_FRONTMICBIAS,      &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_MGDC_FRONTMICBIAS,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MGDC_MAXCOMP,           &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_MGDC_MAXCOMP,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MGDC_ADAPT_THRESH,      &ADF_dm1 + $M.adf_alg_1_0_0.OFFSET_MGDC_ADAPT_THRESH,
.endif

.if uses_MTSF
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,             &mtsf_obj + $mtsf.CONFIG_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_PWR_THRESH,        &mtsf_obj + $mtsf.SILENCE_THRESH_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_TIER1_AGGR,        &mtsf_obj + $mtsf.TIER1_AGGR_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_TIER2_AGGR,        &mtsf_obj + $mtsf.TIER2_AGGR_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_MIC_DIST,          &mtsf_obj + $mtsf.MIC_DIST_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_ATK_ALPHA,         &mtsf_obj + $mtsf.COH_ATK_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_DEC_ALPHA,         &mtsf_obj + $mtsf.COH_DEC_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_SWAP_ENABLE,       &mtsf_obj + $mtsf.SWAP_ENABLE_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_WIN_LOW_IDX,       &mtsf_obj + $mtsf.WIN_LOW_IDX_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_WIN_CNT_IDX,       &mtsf_obj + $mtsf.WIN_CNT_IDX_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_WIN_THRESH,        &mtsf_obj + $mtsf.WIND_THRESH_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_GAIN_THRESH,       &mtsf_obj + $mtsf.GAIN_THRESH_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_MTSF_BIN_THRESH,        &mtsf_obj + $mtsf.GAIN_THRESH_BINS_FIELD,
.endif

.if uses_AEQ
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,             &AEQ_DataObject + $M.AdapEq.CONTROL_WORD_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_ATK_TC,             &AEQ_DataObject + $M.AdapEq.ALFA_A_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_ATK_1MTC,           &AEQ_DataObject + $M.AdapEq.ONE_MINUS_ALFA_A_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_DEC_TC,             &AEQ_DataObject + $M.AdapEq.ALFA_D_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_DEC_1MTC,           &AEQ_DataObject + $M.AdapEq.ONE_MINUS_ALFA_D_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_LO_GOAL_LOW,        &goal_low,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_LO_GOAL_MID,        &goal_low + 1,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_LO_GOAL_HIGH,       &goal_low + 2,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_HI_GOAL_LOW,        &goal_high,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_HI_GOAL_MID,        &goal_high + 1,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_HI_GOAL_HIGH,       &goal_high + 2,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_POWER_TH,           &AEQ_DataObject + $M.AdapEq.AEQ_POWER_TH_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_MIN_GAIN,           &AEQ_DataObject + $M.AdapEq.AEQ_MIN_GAIN_TH_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_MAX_GAIN,           &AEQ_DataObject + $M.AdapEq.AEQ_MAX_GAIN_TH_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_VOL_STEP_UP_TH1,    &AEQ_DataObject + $M.AdapEq.VOL_STEP_UP_TH1_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_VOL_STEP_UP_TH2,    &AEQ_DataObject + $M.AdapEq.VOL_STEP_UP_TH2_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_LOW_STEP,           &AEQ_DataObject + $M.AdapEq.AEQ_PASS_LOW_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_LOW_STEP_INV,       &AEQ_DataObject + $M.AdapEq.INV_AEQ_PASS_LOW_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_HIGH_STEP,          &AEQ_DataObject + $M.AdapEq.AEQ_PASS_HIGH_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_HIGH_STEP_INV,      &AEQ_DataObject + $M.AdapEq.INV_AEQ_PASS_HIGH_FIELD,

      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_LOW_BAND_INDEX,     &AEQ_DataObject + $M.AdapEq.LOW_INDEX_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_LOW_BANDWIDTH,      &AEQ_DataObject + $M.AdapEq.LOW_BW_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_LOG2_LOW_BANDWIDTH, &AEQ_DataObject + $M.AdapEq.LOG2_LOW_INDEX_DIF_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_MID_BANDWIDTH,      &AEQ_DataObject + $M.AdapEq.MID_BW_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_LOG2_MID_BANDWIDTH, &AEQ_DataObject + $M.AdapEq.LOG2_MID_INDEX_DIF_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_HIGH_BANDWIDTH,     &AEQ_DataObject + $M.AdapEq.HIGH_BW_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_LOG2_HIGH_BANDWIDTH,&AEQ_DataObject + $M.AdapEq.LOG2_HIGH_INDEX_DIF_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_MID1_BAND_INDEX,    &AEQ_DataObject + $M.AdapEq.MID1_INDEX_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_MID2_BAND_INDEX,    &AEQ_DataObject + $M.AdapEq.MID2_INDEX_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_AEQ_HIGH_BAND_INDEX,    &AEQ_DataObject + $M.AdapEq.HIGH_INDEX_FIELD,
.endif

.if uses_PLC
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_PLC_STAT_INTERVAL,      &$sco_in.decoder_obj + $sco_pkt_handler.STAT_LIMIT_FIELD,
      &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_HFK_CONFIG,             &$sco_in.decoder_obj + $sco_pkt_handler.CONFIG_FIELD,
.endif

      0;

   // Statistics from Modules sent via SPI
   // -------------------------------------------------------------------------------
   .VAR  StatisticsPtrs[$M.CVC_HEADSET_2MIC.STATUS.BLOCK_SIZE] =
      &$M.CVC_HEADSET_2MIC.cur_mode,                             // $M.CVC_HEADSET_2MIC.STATUS.CUR_MODE_OFFSET
      &$M.CVC_HEADSET_2MIC.CurCallState,                         // $M.CVC_HEADSET_2MIC.STATUS.CALL_STATE_OFFSET
      &$M.CVC_HEADSET_2MIC.SysControl,                           // $M.CVC_HEADSET_2MIC.STATUS.SYS_CONTROL_OFFSET
      &$M.CVC_HEADSET_2MIC.CurDAC,                               // $M.CVC_HEADSET_2MIC.STATUS.CUR_DAC_OFFSET
      &$M.CVC_HEADSET_2MIC.Last_PsKey,                           // $M.CVC_HEADSET_2MIC.STATUS.PRIM_PSKEY_OFFSET
      &$M.CVC_HEADSET_2MIC.SecStatus,                            // $M.CVC_HEADSET_2MIC.STATUS.SEC_STAT_OFFSET
      &$M.CVC_HEADSET_2MIC.PeakSpkr,                             // $M.CVC_HEADSET_2MIC.STATUS.PEAK_DAC_OFFSET
      &$M.CVC_HEADSET_2MIC.PeakMicLeft,                          // $M.CVC_HEADSET_2MIC.STATUS.PEAK_ADC_LEFT_OFFSET
      &$M.CVC_HEADSET_2MIC.PeakMicRight,                         // $M.CVC_HEADSET_2MIC.STATUS.PEAK_ADC_RIGHT_OFFSET
      &$M.CVC_HEADSET_2MIC.PeakScoIn,                            // $M.CVC_HEADSET_2MIC.STATUS.PEAK_SCO_IN_OFFSET
      &$M.CVC_HEADSET_2MIC.PeakScoOut,                           // $M.CVC_HEADSET_2MIC.STATUS.PEAK_SCO_OUT_OFFSET
      &$M.CVC_HEADSET_2MIC.PeakMips,                             // $M.CVC_HEADSET_2MIC.STATUS.PEAK_MIPS_OFFSET

.if uses_NSVOLUME
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_FILTSUMLPDNZ,
                                                                 // $M.CVC_HEADSET_2MIC.STATUS.NDVC_NOISE_EST_OFFSET
.else
      &ZeroValue,                                                // $M.CVC_HEADSET_2MIC.STATUS.NDVC_NOISE_EST_OFFSET
.endif
      &$M.CVC_HEADSET_2MIC.PeakAux,                              // $M.CVC_HEADSET_2MIC.STATUS.PEAK_AUX_OFFSET
      &$M.CVC_MODULES_STAMP.CompConfig,                          // $M.CVC_HEADSET_2MIC.STATUS.COMPILED_CONFIG
      &$M.CVC_HEADSET_2MIC.ConnectStatus,                        // $M.CVC_HEADSET_2MIC.STATUS.CONNSTAT
      &$M.CVC_HEADSET_2MIC.current_side_tone_gain,               // $M.CVC_HEADSET_2MIC.STATUS.SIDETONE_GAIN
      &$M.CVC_HEADSET_2MIC.SysDACadjust,
.if uses_PLC
      &$sco_in.decoder_obj + $sco_pkt_handler.PACKET_LOSS_FIELD,    // PLC Loss Rate
.else
      &ZeroValue,
.endif
.if uses_AEQ
      &AEQ_DataObject + $M.AdapEq.AEQ_GAIN_LOW_FIELD,          // AEQ Gain Low
      &AEQ_DataObject + $M.AdapEq.AEQ_GAIN_HIGH_FIELD,         // AEQ Gain High
      &AEQ_DataObject + $M.AdapEq.STATE_FIELD,                 // AEQ State
      &AEQ_DataObject + $M.AdapEq.AEQ_POWER_TEST_FIELD,        // AEQ Tone Detection
      &AEQ_DataObject + $M.AdapEq.AEQ_TONE_POWER_FIELD,        // AEQ Tone Power
.else
      &ZeroValue,
      &ZeroValue,
      &ZeroValue,
      &ZeroValue,
      &ZeroValue,
.endif
      &$M.CVC_HEADSET_2MIC.PeakSideTone,
.if uses_MTSF
      &mtsf_obj + $mtsf.PWR_FIELD,
      &mtsf_obj + $mtsf.G_MEAN_FIELD;
.else
      &ZeroValue,
      &ZeroValue;
.endif
   // Processing Tables
   // -------------------------------------------------------------------------------
   .VAR ReInitializeTable[] =
      // Function                               r7                   r8

.if uses_DCBLOCK
      $audio_proc.peq.initialize,         &in_l_dcblock_dm2,        0,
      $audio_proc.peq.initialize,         &in_r_dcblock_dm2,        0,
      $audio_proc.peq.initialize,         &sco_dcblock_dm2,         0,
.endif

      $M.filter_bank.one_channel.frame64_proto128_fft128.analysis.Initialize.func,  &fft_obj, &AnalysisBank_ASR,
      $M.filter_bank.two_channel.frame64_proto128_fft128.analysis.Initialize.func,  &fft_obj, &AnalysisBank,
      $M.filter_bank.one_channel.frame64_proto128_fft128.synthesis.Initialize.func, &fft_obj, &SynthesisBank,

.if uses_AEC
      $M.filter_bank.one_channel.frame64_proto128_fft128.analysis.Initialize.func,  &fft_obj, &AnalysisBank_AEC,
      $M.AEC_Headset.Initialize.func,      0,                   &aec_dm1,
.endif

.if uses_MTSF
      $mtsf.initialize,                    0,                   &mtsf_obj,
.endif

.if uses_ADF
      $M.adf_alg_1_0_0.Initialize.func,        &ADF_dm1,           &aecpp_dm1,
      .if uses_NSVOLUME
          $M.oms270.initialize.func,           &oms270NDVC_obj,         	0,
      .endif
.endif

.if uses_RCV_FREQPROC
      $M.filter_bank.one_channel.frame64_proto128_fft128.analysis.Initialize.func,  &fft_obj, &AnalysisBank_rcv,
      $M.filter_bank.one_channel.frame64_proto128_fft128.synthesis.Initialize.func,  &fft_obj, &RcvSynthesisBank,
.endif

.if uses_RCV_NS
      $M.oms270.initialize.func,           &oms270rcv_obj,         	0,
.endif

.if uses_SND_NS
      $M.oms270.initialize.func,           &oms270snd_obj,         	0,
.endif

.if uses_SND_PEQ
      $audio_proc.peq.initialize,         &snd_peq_dm2,        0,
.endif

.if uses_RCV_VAD
      $audio_proc.peq.initialize,         &rcv_vad_peq,        0,
      $M.vad400.initialize.func,                &vad400_dm1,         &vad400_dm2,
.endif
.if uses_RCV_AGC
      $M.agc400.initialize.func,                0,                   &rcv_agc400_dm,
.endif

.if uses_SND_AGC
      $M.agc400.initialize.func,                0,                   &snd_agc400_dm,
.endif

.if uses_AEQ
      $M.AdapEq.initialize.func,                0,                 &AEQ_DataObject,
.endif
.if uses_RCV_PEQ
      $audio_proc.peq.initialize,         &rcv_peq_dm2,        0,
.endif


.if uses_NSVOLUME
      $M.NDVC_alg1_0_0.Initialize.func,         &ndvc_dm1,           0,
.endif

.if uses_PLC
      $frame.sco_initialize,               &$sco_in.decoder_obj,         0,
.endif

      $CVC.sidetone_filter.Initialize,     &sidetone_filter_object_dm2,  &rcv_hc_dm1,
      0;                                    // END OF TABLE


   // -------------------------------------------------------------------------------
   .VAR hfk_proc_funcs[] =
      // Function                               r7                            r8
      $frame.distribute_streams_rm,                &stream_setup_table_hfk_asr,  &$cvc_cbuffers,

.if uses_DCBLOCK
      $audio_proc.peq.process,            &in_l_dcblock_dm2,            0,
      $audio_proc.peq.process,            &in_r_dcblock_dm2,            0,
      $audio_proc.peq.process,            &sco_dcblock_dm2,             0,
.endif

      $M.audio_proc.peak_monitor.Process.func,           &mic_in_l_pk_dtct,            0,
      $M.audio_proc.peak_monitor.Process.func,           &mic_in_r_pk_dtct,            0,
      $M.audio_proc.peak_monitor.Process.func,           &sco_in_pk_dtct,              0,

.if uses_RCV_VAD
      $audio_proc.peq.process,            &rcv_vad_peq,                 0,
      $M.vad400.process.func,             &vad400_dm1,                  &vad400_dm2,
.endif

// This processing should be before the AEC module, so the changes on the
// receive side are taken into account for echo cancellation.
.if uses_RCV_FREQPROC
   .if uses_RCV_NS
      $M.CVC.data.copyInpBuffer2HarmBuffer,     0, &oms270rcv_obj + $M.oms270.PTR_INP_X_FIELD,
   .endif
   $M.filter_bank.one_channel.frame64_proto128_fft128.analysis.Process.func, &fft_obj, &AnalysisBank_rcv,
   .if uses_AEQ
      $M.AdapEq.process.tone_detect,            0,                   &AEQ_DataObject,
   .endif

   .if uses_RCV_NS
      $M.oms270.process.func,       			&oms270rcv_obj,        	0,
      $M.oms270.apply_gain.func,                &oms270rcv_obj,        	0,
   .endif

   .if uses_AEQ
      $M.AdapEq.process.func,                   0,                   &AEQ_DataObject,
   .endif
.endif

.if uses_RCV_FREQPROC
      $M.CVC.Zero_DC_Nyquist.func,              &$M.CVC.data.D_rcv_real,        &$M.CVC.data.D_rcv_imag,
      $M.filter_bank.one_channel.frame64_proto128_fft128.synthesis.Process.func, &fft_obj, &RcvSynthesisBank,
.endif
.if uses_RCV_PEQ
      $audio_proc.peq.process,            &rcv_peq_dm2,                 0,
.endif

      // This is pre RCVAGC gain stage
      $M.audio_proc.stream_gain.Process.func,              &rcv_pregain_dm1,             0,

.if uses_RCV_AGC
      $M.agc400.process.func,                   0,                            &rcv_agc400_dm,
.endif

      $M.HC_Alg_1_0_0.Process.func,             0,                            &rcv_hc_dm1,

.if uses_AEC
      $M.filter_bank.one_channel.frame64_proto128_fft128.analysis.Process.func, &fft_obj, &AnalysisBank_AEC,
.endif

.if uses_SND_NS
      $M.CVC.data.copyInpBuffer2HarmBuffer,     0, &oms270snd_obj + $M.oms270.PTR_INP_X_FIELD,
.endif
      $M.filter_bank.two_channel.frame64_proto128_fft128.analysis.Process.func, &fft_obj, &AnalysisBank,
.if uses_SND_NS
      $M.CVC.data.setOMSBExp,                   &oms270snd_obj,               &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_PTR_BEXP,
.endif
      $M.CVC.data.setSynthBExp,                 &SynthesisBank,               &AnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_PTR_BEXP,

.if uses_ADF
    .if uses_NSVOLUME
          $M.oms270.process.func,       		&oms270NDVC_obj,        	  0,
    .endif
.endif

.if uses_MTSF
      $mtsf.process,                            0,                            &mtsf_obj,
.endif

.if uses_ADF
      $M.adf_alg_1_0_0.micGainDiffComp.func,    &ADF_dm1,                     0,
      $M.adf_alg_1_0_0.Process.func,            0,                            0,
.if uses_ADF_PP
      $M.adf_alg_1_0_0.PostProcess.func,        0,                            0,
.else
      $M.adf_alg_1_0_0.NoPostProcess.func,      0,                            0,
.endif
.else
      $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_l_real,      &$M.CVC.data.D_r_real,
      $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_l_imag,      &$M.CVC.data.D_r_imag,
.endif

.if uses_MTSF
.ifdef MTSF_TIER2_ENABLED
      $mtsf.apply_tier2,                        0,                          &mtsf_obj,
.endif
.endif

.if uses_SND_NS
      $M.oms270.process.func,       			&oms270snd_obj,        	0,
      $M.oms270.apply_gain.func,             &oms270snd_obj,        	0,
.endif

.if uses_AEC
    .if uses_SND_NS
    // If the AEC is disabled during run-time, the CNG is not applying the OMS gain. Vector D_l_real has the
    // OMS processed signal, so we have to copy it to fft_real
       $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_l_real,      &$M.CVC.data.fft_real,
       $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_l_imag,      &$M.CVC.data.fft_imag,
    .else
      // Copy data from ADF output to AEC output in case the AEC is deactivated during run-time
       $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_r_real,      &$M.CVC.data.fft_real,
       $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_r_imag,      &$M.CVC.data.fft_imag,
    .endif
   .if uses_VCAEC
      // Call fnlms process with pointer to curvolume and VCAEC_THR as arguments
      $M.AEC_Headset.fnmls_process.func,        &$M.CVC_HEADSET_2MIC.CurDAC,
                                                &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_LVMODE_THRES,
      // Call cng process with pointer to curvolume and VCAEC_THR as arguments
      $M.AEC_Headset.comfort_noise_generator.func, &$M.CVC_HEADSET_2MIC.CurDAC,
                                                   &CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_LVMODE_THRES,
   .else
      $M.AEC_Headset.fnmls_process.func,              0,                   0,
      $M.AEC_Headset.comfort_noise_generator.func,    0,                   0,
   .endif
      // Copy output of AEC to input of Synthesis filter.
      $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.fft_real,      &$M.CVC.data.D_r_real,
      $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.fft_imag,      &$M.CVC.data.D_r_imag,
.else
    .if uses_SND_NS
    // Since the AEC is not used, the CNG is not applying the OMS gain. Vector D_l_real has the
    // OMS processed signal, so we have to copy it to D_r_real
          $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_l_real,      &$M.CVC.data.D_r_real,
          $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_l_imag,      &$M.CVC.data.D_r_imag,
    .endif
.endif

.if uses_ADF && uses_SND_NS && uses_SND_AGC
      $M.CVC.oms_vad_recalc.func,               0,                            &oms_vad_recalc,
.endif

      $M.CVC.Zero_DC_Nyquist.func,              &$M.CVC.data.D_r_real,        &$M.CVC.data.D_r_imag,
      $M.filter_bank.one_channel.frame64_proto128_fft128.synthesis.Process.func, &fft_obj, &SynthesisBank,


.if uses_SND_PEQ
      $audio_proc.peq.process,            &snd_peq_dm2,                 0,
.endif

      $M.audio_proc.stream_gain.Process.func,              &out_gain_dm1,                0,

.if uses_SND_AGC
      $M.agc400.process.func,                   0,                            &snd_agc400_dm,
.endif

      $M.MUTE_CONTROL.Process.func,             &mute_cntrl_dm1,              0,


.if uses_NSVOLUME
      $M.NDVC_alg1_0_0.Process.func,            &ndvc_dm1,                    0,
.endif

      $M.audio_proc.peak_monitor.Process.func,           &sco_out_pk_dtct,             0,
      $M.audio_proc.peak_monitor.Process.func,           &spkr_out_pk_dtct,            0,

      $frame.update_streams_rm,                    &stream_setup_table_hfk_asr,  &$cvc_cbuffers,

      0;

   // -------------------------------------------------------------------------------
   .VAR asr_proc_funcs[] =
      // Function                               r7                            r8
      $frame.distribute_streams_rm,                &stream_setup_table_hfk_asr,  &$cvc_cbuffers,

.if uses_DCBLOCK
      $audio_proc.peq.process,            &in_l_dcblock_dm2,            0,
      $audio_proc.peq.process,            &in_r_dcblock_dm2,            0,
      $audio_proc.peq.process,            &sco_dcblock_dm2,             0,
.endif

      $M.audio_proc.peak_monitor.Process.func,           &mic_in_l_pk_dtct,            0,
      $M.audio_proc.peak_monitor.Process.func,           &mic_in_r_pk_dtct,            0,
      $M.audio_proc.peak_monitor.Process.func,           &sco_in_pk_dtct,              0,

    // This is pre RCVAGC gain stage
      $M.audio_proc.stream_gain.Process.func,            &rcv_pregain_dm1,             0,

.if uses_RCV_VAD
      $audio_proc.peq.process,                  &rcv_vad_peq,          0,
      $M.vad400.process.func,                   &vad400_dm1,           &vad400_dm2,
.endif
.if uses_RCV_FREQPROC
   .if uses_RCV_NS
      $M.CVC.data.copyInpBuffer2HarmBuffer,     0, &oms270rcv_obj + $M.oms270.PTR_INP_X_FIELD,
   .endif
   $M.filter_bank.one_channel.frame64_proto128_fft128.analysis.Process.func, &fft_obj, &AnalysisBank_rcv,
   .if uses_AEQ
      $M.AdapEq.process.tone_detect,            0,                   &AEQ_DataObject,
   .endif

   .if uses_RCV_NS
      $M.oms270.process.func,       			&oms270rcv_obj,        	0,
      $M.oms270.apply_gain.func,                &oms270rcv_obj,        	0,
   .endif

   .if uses_AEQ
      $M.AdapEq.process.func,                   0,                   &AEQ_DataObject,
   .endif
.endif

.if uses_RCV_FREQPROC
      $M.CVC.Zero_DC_Nyquist.func,              &$M.CVC.data.D_rcv_real,        &$M.CVC.data.D_rcv_imag,
      $M.filter_bank.one_channel.frame64_proto128_fft128.synthesis.Process.func, &fft_obj, &RcvSynthesisBank,
.endif
.if uses_RCV_PEQ
      $audio_proc.peq.process,            &rcv_peq_dm2,                 0,
.endif

    // This is pre RCVAGC gain stage
      $M.audio_proc.stream_gain.Process.func,            &rcv_pregain_dm1,             0,

   .if uses_RCV_AGC
      $M.agc400.process.func,                      0,                  &rcv_agc400_dm,
   .endif

      $M.HC_Alg_1_0_0.Process.func,                0,                  &rcv_hc_dm1,
      $M.CVC.sidetone_filter.DisableSideTone,      0,                  &sidetone_filter_object_dm2,
.if uses_SND_NS
      $M.CVC.data.copyInpBuffer2HarmBuffer,     0, &oms270snd_obj + $M.oms270.PTR_INP_X_FIELD,
.endif
      $M.filter_bank.one_channel.frame64_proto128_fft128.analysis.Process.func, &fft_obj, &AnalysisBank_ASR,
.if uses_SND_NS
      $M.CVC.data.setOMSBExp,                   &oms270snd_obj,               &AnalysisBank_ASR + $M.filter_bank.Parameters.OFFSET_PTR_BEXP,
.endif
      $M.CVC.data.setSynthBExp,                 &SynthesisBank,               &AnalysisBank_ASR + $M.filter_bank.Parameters.OFFSET_PTR_BEXP,
      $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_l_real,        &$M.CVC.data.D_r_real,
      $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_l_imag,        &$M.CVC.data.D_r_imag,
.if uses_SND_NS
      $M.CVC.data.copyASRaggr2OMSaggr,          0,                      0,
      $M.oms270.process.func,       			&oms270snd_obj,        	0,
      $M.oms270.apply_gain.func,                &oms270snd_obj,        	0,
    // Vector D_l_real has the OMS processed signal, so we have to copy it to D_r_real
      $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_l_real,      &$M.CVC.data.D_r_real,
      $M.adf_alg_1_0_0.CopyADFVector.func,      &$M.CVC.data.D_l_imag,      &$M.CVC.data.D_r_imag,
.endif
    .if uses_NSVOLUME
          $M.oms270.process.func,       		&oms270NDVC_obj,        	  0,
    .endif


      $M.CVC.Zero_DC_Nyquist.func,              &$M.CVC.data.D_r_real,        &$M.CVC.data.D_r_imag,
      $M.filter_bank.one_channel.frame64_proto128_fft128.synthesis.Process.func, &fft_obj, &SynthesisBank,

      $M.audio_proc.stream_gain.Process.func,   &out_gain_dm1,          0,
      $M.MUTE_CONTROL.Process.func,             &mute_cntrl_dm1,        0,


.if uses_NSVOLUME
      $M.NDVC_alg1_0_0.Process.func,            &ndvc_dm1,              0,
.endif
      $M.audio_proc.stream_gain.Process.func,   &out_gain_asr,          0,

      $M.audio_proc.peak_monitor.Process.func,  &sco_out_pk_dtct,       0,
      $M.audio_proc.peak_monitor.Process.func,  &spkr_out_pk_dtct,      0,
      $frame.update_streams_rm,                    &stream_setup_table_hfk_asr,  &$cvc_cbuffers,
      0;

   // -------------------------------------------------------------------------------
   .VAR PsThru_proc_funcs[] =
      // Function                               r7                            r8
      $frame.distribute_streams_rm,             &stream_setup_table_passthr,  &$cvc_cbuffers,
      $M.set_mode_gain.func,                    &ModeControl,                 0,
      $M.audio_proc.peak_monitor.Process.func,  &mic_in_l_pk_dtct,            0,
      $M.audio_proc.peak_monitor.Process.func,  &mic_in_r_pk_dtct,            0,
      $M.audio_proc.peak_monitor.Process.func,  &sco_in_pk_dtct,              0,
      $M.audio_proc.stream_mixer.Process.func,  &adc_mixer,                   0,
      $M.audio_proc.stream_gain.Process.func,   &sco_gain,                    0,
      $M.HC_Alg_1_0_0.Process.func,             0,                            &rcv_hc_dm1,
      $M.CVC.sidetone_filter.DisableSideTone,   0,                  &sidetone_filter_object_dm2,
      $M.audio_proc.peak_monitor.Process.func,  &sco_out_pk_dtct,             0,
      $M.audio_proc.peak_monitor.Process.func,  &spkr_out_pk_dtct,            0,
      $frame.update_streams_rm,                 &stream_setup_table_passthr,  &$cvc_cbuffers,
      0;

   // -------------------------------------------------------------------------------
   .VAR LpBack_proc_funcs[] =
      // Function                               r7                         r8
      $frame.distribute_streams_rm,             &stream_setup_table_lpbk,  &$cvc_cbuffers,
      $M.set_mode_gain.func,                    &ModeControl,              0,
      $M.audio_proc.peak_monitor.Process.func,  &mic_in_l_pk_dtct,         0,
      $M.audio_proc.peak_monitor.Process.func,  &mic_in_r_pk_dtct,         0,
      $M.audio_proc.peak_monitor.Process.func,  &sco_in_pk_dtct,           0,
      $M.audio_proc.stream_mixer.Process.func,  &adc_mixer,                0,
      $M.audio_proc.stream_gain.Process.func,   &sco_gain,                 0,
      $M.HC_Alg_1_0_0.Process.func,             0,                         &rcv_hc_dm1,
      $M.CVC.sidetone_filter.DisableSideTone,   0,                  &sidetone_filter_object_dm2,
      $M.audio_proc.peak_monitor.Process.func,  &sco_out_pk_dtct,          0,
      $M.audio_proc.peak_monitor.Process.func,  &spkr_out_pk_dtct,         0,
      $frame.update_streams_rm,                 &stream_setup_table_lpbk,  &$cvc_cbuffers,
      0;

   // -------------------------------------------------------------------------------
   // stream distribution table
   // -------------------------------------------------------------------------------

   .VAR    stream_setup_table_hfk_asr[] =
      // Frame Properties
      0.125,     // Frame Jitter

      // SCO_IN_BUFFER_FIELD
      &stream_map_sco_in_hfk_asr, LENGTH(stream_map_sco_in_hfk_asr)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      NUM_SAMPLES_PER_FRAME,0,0,0,0,

      // ADC_IN_BUFFER_FIELD
      &stream_map_left_adc,       LENGTH(stream_map_left_adc)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      NUM_SAMPLES_PER_FRAME,0,0,0,0,

      // ADC_IN_BUFFER_FIELD
      &stream_map_right_adc,      LENGTH(stream_map_right_adc)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      NUM_SAMPLES_PER_FRAME,0,0,0,0,

      // SCO_OUT_BUFFER_FIELD
      &stream_map_sco_out,        LENGTH(stream_map_sco_out)/2,
      $frame.distribute_output_stream_rm, $frame.update_output_streams_rm,
      NUM_SAMPLES_PER_FRAME,0,0,0,0,

      // DAC_OUT_BUFFER_FIELD
      &stream_map_dac_hfk_asr,    LENGTH(stream_map_dac_hfk_asr)/2,
      $frame.distribute_output_stream_rm, $frame.update_output_streams_rm,
      NUM_SAMPLES_PER_FRAME,0,0,0,0,
      0;
   // stream distribution table

   .VAR    stream_setup_table_lpbk[] =
      // Frame Properties
      0.125,     // Frame Jitter
      // SCO_IN_BUFFER_FIELD
      &stream_map_sco_in_passthr_lpbk, LENGTH(stream_map_sco_in_passthr_lpbk)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      NUM_SAMPLES_PER_FRAME, 0,0,0,0,

      // ADC_IN_BUFFER_FIELD
      &stream_map_left_adc,            LENGTH(stream_map_left_adc)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      NUM_SAMPLES_PER_FRAME, 0,0,0,0,

      // ADC_IN_BUFFER_FIELD
      &stream_map_right_adc,           LENGTH(stream_map_right_adc)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      NUM_SAMPLES_PER_FRAME, 0,0,0,0,

      // SCO_OUT_BUFFER_FIELD
      &stream_map_sco_out_lpbk,        LENGTH(stream_map_sco_out_lpbk)/2,
      $frame.distribute_output_stream_rm, $frame.update_output_streams_rm,
      NUM_SAMPLES_PER_FRAME, 0,0,0,0,

      // DAC_OUT_BUFFER_FIELD
      &stream_map_dac_lpbk,            LENGTH(stream_map_dac_lpbk)/2,
      $frame.distribute_output_stream_rm, $frame.update_output_streams_rm,
      NUM_SAMPLES_PER_FRAME, 0,0,0,0,
      0;

   .VAR    stream_setup_table_passthr[] =
      // Frame Properties
      0.125,     // Frame Jitter
      // SCO_IN_BUFFER_FIELD
      &stream_map_sco_in_passthr_lpbk, LENGTH(stream_map_sco_in_passthr_lpbk)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      NUM_SAMPLES_PER_FRAME, 0,0,0,0,

      // ADC_IN_BUFFER_FIELD
      &stream_map_left_adc,            LENGTH(stream_map_left_adc)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      NUM_SAMPLES_PER_FRAME, 0,0,0,0,

      // ADC_IN_BUFFER_FIELD
      &stream_map_right_adc,           LENGTH(stream_map_right_adc)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      NUM_SAMPLES_PER_FRAME, 0,0,0,0,

      // SCO_OUT_BUFFER_FIELD
      &stream_map_sco_out,             LENGTH(stream_map_sco_out)/2,
      $frame.distribute_output_stream_rm, $frame.update_output_streams_rm,
      NUM_SAMPLES_PER_FRAME, 0,0,0,0,

      // DAC_OUT_BUFFER_FIELD
      &stream_map_dac_passthr,         LENGTH(stream_map_dac_passthr)/2,
      $frame.distribute_output_stream_rm, $frame.update_output_streams_rm,
      NUM_SAMPLES_PER_FRAME, 0,0,0,0,
      0;


// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio plc 2006             http://www.csr.com
//
// $Revision:$  $Date:$
//
// MODULE:
//          $M.CVC.data.setOMSBExp
//          $M.CVC.data.setSynthBExp
//DESCRIPTION:
//       Sets the BExp pointer for OMS and Synthesis to the value located in r8.
//       It is used by the ASR mode to avoid creating new structures due to the
//       one channel filterbank.
// INPUTS:
//      r7 - Pointer to the data structure
//      r8 - Pointer to the desired BExp
// OUTPUT:
//      None
// MODIFIED REGISTERS:
//
// *****************************************************************************
.ifdef BLD_PRIVATE
   .PRIVATE;
.endif

   .CODESEGMENT PM;

setOMSBExp:
   M[r7 + $M.oms270.PTR_BEXP_X_FIELD] = r8;
   rts;

setSynthBExp:
   M[r7 + $M.filter_bank.Parameters.OFFSET_PTR_BEXP] = r8;
   rts;

.if uses_SND_NS
   .CODESEGMENT PM;

copyASRaggr2OMSaggr:
   r0 = M[&CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_ASR_OMS_AGGR];
   M[&oms270snd_obj + $M.oms270.AGRESSIVENESS_FIELD] = r0;
   rts;

// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio plc 2006             http://www.csr.com
//
// $Revision:$  $Date:$
//
// MODULE:
//          $M.CVC.data.copyInpBuffer2HarmBuffer
//
//DESCRIPTION:
//       Copies data from history buffer to the Harmonicity buffer
//
// INPUTS:
//      r8 - Pointer to the harmonicity buffer.
// OUTPUT:
//      None
// MODIFIED REGISTERS:
//
// *****************************************************************************
copyInpBuffer2HarmBuffer:
    M1 = 1;

    r0 = M[r8];
    I1 = r0 + $M.CVC.Num_Samples_Per_Frame;
    I4 = I1 + $M.CVC.Num_Samples_Per_Frame;

    r10 = $M.CVC.Num_Samples_Per_Frame;
    r1 = M[I4,M1];
    do copyHarmBufferLoop;
        M[I1,M1] = r1, r1 = M[I4,M1];
    copyHarmBufferLoop:

    rts;
.endif
.ENDMODULE;


// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio plc 2006             http://www.csr.com
//
// $Revision:$  $Date:$
//
// MODULE:
//          $M.set_mode_gain.func
//DESCRIPTION:
//      Control the gains based on mode
// INPUTS:
//      r7 - Pointer to the  data structure
// *****************************************************************************

.MODULE $M.set_mode_gain;

    .CODESEGMENT PM;
    .DATASEGMENT DM;
    .VAR FunctionTable[] = &StandByMode,   // invalid
                           &StandByMode,   // hfk
                           &StandByMode,   // asr
                           &PassThrLeft,   // Pass-Through Left
                           &PassThrRight,  // Pass-Through Right
                           &LpBk,          // Loop-Back
                           &StandByMode;   // Stand-By

   .VAR  StandByMode[] = 0,0,1,0,1;
   .VAR  PassThrLeft[] = 0.5,0,1,0.5,1;
   .VAR  PassThrRight[] = 0,0.5,1,0.5,1;
   .VAR  LpBk[] = 0.5,0,1,0.5,1;

func:

   // load ADC mantissa and exponent from user parameters
   r0 = M[$M.CVC.data.CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_PT_SNDGAIN_MANTISSA];
   r1 = M[$M.CVC.data.CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_PT_SNDGAIN_EXPONENT];
   M[PassThrLeft] = r0;
   M[PassThrLeft + 2] = r1;
   M[PassThrRight + 1] = r0;
   M[PassThrRight + 2] = r1;

   r0 = M[$M.CVC.data.CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_PT_RCVGAIN_MANTISSA];
   r1 = M[$M.CVC.data.CurParams + $M.CVC_HEADSET_2MIC.PARAMETERS.OFFSET_PT_RCVGAIN_EXPONENT];
   M[PassThrLeft + 3] = r0;
   M[PassThrLeft + 4] = r1;
   M[PassThrRight + 3] = r0;
   M[PassThrRight + 4] = r1;

   // Get Mode
   r0 = M[r7 + $M.SET_MODE_GAIN.MODE_PTR];
   r0 = M[r0];
   r1 = M[FunctionTable + r0];
   I0 = r1;
   r10 = 5;
   // Set Gains
   I4 = r7 + $M.SET_MODE_GAIN.ADC_MIXER_MANT_LEFT;
   do lp_copy;
      r0 = M[I0,1];
      M[I4,1] = r0;
lp_copy:
   rts;
.ENDMODULE;

