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
//    CVC configuration should be handled from within the cvc_headset_config.h
//    header file.
//
// *****************************************************************************

.include "audio_proc_library.h"
.include "cvc_headset_library.h"
.include "cvc_headset_modules.h"
.include "cvc_headset_config.h"
.include "flash.h"
.include "stack.h"

// Generate build error messages if necessary.
.if uses_SND_NS == 0
.if uses_AEC
   .error AEC cannot be enabled without OMS
.endif
.if uses_NSVOLUME
   .error NDVC cannot be enabled without OMS
.endif
.endif

.define uses_RCV_FREQPROC  (uses_RCV_NS || uses_AEQ)
.define uses_RCV_VAD       (uses_RCV_AGC || uses_AEQ)

   // Narrow band CVC configuration
   .CONST $M.CVC.REF_DELAY                   0x000080;

   .CONST $M.CVC.Wideband_Flag               0;
   .CONST   $M.oms270.STATE_LENGTH     		 $M.oms270.narrow_band.STATE_LENGTH;
   .CONST   $M.oms270.SCRATCH_LENGTH   		 $M.oms270.narrow_band.SCRATCH_LENGTH;
   .CONST   $M.oms270.FFT_NUM_BIN      		 $M.CVC.Num_FFT_Freq_Bins;
   .define  $M.oms270.mode.object      		 $M.oms270.mode.narrow_band.object
   .CONST   $M.oms270.QUE_LENGTH              $M.oms270.MIN_SEARCH_LENGTH * 2;

   .define wnr_params                         CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_WNR_AGGR

   .define $M.filter_bank.Parameters.FFT_BUFFLEN                  $M.filter_bank.Parameters.FFT128_BUFFLEN

   .define $M.filter_bank.two_channel.analysis.Initialize.func    $M.filter_bank.two_channel.frame64_proto128_fft128.analysis.Initialize.func
   .define $M.filter_bank.two_channel.analysis.Process.func       $M.filter_bank.two_channel.frame64_proto128_fft128.analysis.Process.func

   .define $M.filter_bank.one_channel.synthesis.Initialize.func   $M.filter_bank.one_channel.frame64_proto128_fft128.synthesis.Initialize.func
   .define $M.filter_bank.one_channel.synthesis.Process.func      $M.filter_bank.one_channel.frame64_proto128_fft128.synthesis.Process.func

   .define $M.filter_bank.one_channel.analysis.Initialize.func    $M.filter_bank.one_channel.frame64_proto128_fft128.analysis.Initialize.func
   .define $M.filter_bank.one_channel.analysis.Process.func       $M.filter_bank.one_channel.frame64_proto128_fft128.analysis.Process.func

   .define Max_RefDelay          8

// Reserve buffers fo 2-channel fft128 filter_bank
.define FFT_BUFFER_SIZE                ($M.filter_bank.Parameters.FFT_BUFFLEN * 2)

.define MAX_NUM_PEQ_STAGES             (5)

// Number of sample needed for reference delay buffer
.define  Max_RefDelay_Sample  (Max_RefDelay * $M.CVC.Num_Samples_Per_Frame)

   .CONST   $M.SET_MODE_GAIN.MODE_PTR                  0;
   .CONST   $M.SET_MODE_GAIN.ADC_MANT                  1;
   .CONST   $M.SET_MODE_GAIN.ADC_EXP                   2;
   .CONST   $M.SET_MODE_GAIN.SCO_IN_MANT               3;
   .CONST   $M.SET_MODE_GAIN.SCO_IN_EXP                4;
   .CONST   $M.SET_MODE_GAIN.STRUC_SIZE                5;

// System Configuration is saved in kap file.
.MODULE $M.CVC_MODULES_STAMP;
   .DATASEGMENT DM;
   .BLOCK ModulesStamp;
      .VAR  s1 = 0xfeeb;
      .VAR  s2 = 0xfeeb;
      .VAR  s3 = 0xfeeb;
      .VAR  CompConfig = CVC_HEADSET_CONFIG_FLAG;
      .VAR  s4 = 0xfeeb;
      .VAR  s5 = 0xfeeb;
      .VAR  s6 = 0xfeeb;
   .ENDBLOCK;
.ENDMODULE;

.MODULE $M.CVC.data;
   .DATASEGMENT DM;

   // Temp Variable to handle disabled modules.
   .VAR  ZeroValue = 0;
   .VAR  OneValue = 1.0;

   // These lines write module and version information to the kap file.
   .VAR kap_version_stamp = &$M.CVC_VERSION_STAMP.VersionStamp;
   .VAR kap_modules_stamp = &$M.CVC_MODULES_STAMP.ModulesStamp;

   // Shared Data for CVC modules.
   .VAR/DM2CIRC fft_circ[FFT_BUFFER_SIZE];

   // The following two scratch buffers for the fft_object
   // reuses the scratch buffer from the AEC module.  This allows
   // reduction in the requirement of data memory for the overall system.
   // To be noted: The same AEC scratch memory is also reused for the
   // OMS270 scratch.
   .define fft_real $M.AEC_Headset.dm1.W.ri
   .define fft_imag $M.AEC_Headset.dm1.W.ri + FFT_BUFFER_SIZE

   .BLOCK/DM1   FFT_DM1;
      // The following two buffers have been reused as noted above
      .VAR  D_real[$M.CVC.Num_FFT_Freq_Bins];
      .VAR  X_real[$M.CVC.Num_FFT_Freq_Bins];
.if (uses_SND_NS || uses_RCV_FREQPROC)
      .VAR  E_real[$M.CVC.Num_FFT_Freq_Bins]; // shared E and snd_harm buffer[1]
.endif
      .VAR  bufd_inp[$M.CVC.Num_FFT_Window];
      .VAR  bufd_outp[($M.CVC.Num_FFT_Window + $M.CVC.Num_Samples_Per_Frame)];
   .ENDBLOCK;

   .BLOCK/DM2 FFT_DM2;
      .VAR  D_imag[$M.CVC.Num_FFT_Freq_Bins];
      .VAR  X_imag[$M.CVC.Num_FFT_Freq_Bins];
.if (uses_SND_NS || uses_RCV_FREQPROC)
      .VAR  E_imag[$M.CVC.Num_FFT_Freq_Bins]; // shared E and rcv_harm buffer[1]
.endif
.if uses_RCV_FREQPROC
      .VAR  bufdr_inp[$M.CVC.Num_FFT_Window];
.endif
      .VAR  bufx_inp[$M.CVC.Num_FFT_Window];
   .ENDBLOCK;


   // Default Block
   .VAR  DefaultParameters[$M.CVC_HEADSET.PARAMETERS.STRUCT_SIZE] =
      0x00E202,                        // OFFSET_HFK_CONFIG
      0x799999,                        // OFFSET_HFK_OMS_AGGR
      0x799999,                        // OFFSET_ASR_OMS_AGGR
      0x000001,                        // Harmonicity
      0.5,                             // OFFSET_WNR_AGGR
      -40,                             // OFFSET_WNR_POWER_THRES
      1,                               // OFFSET_WNR_HOLD
      0x1009B9,                        // OFFSET_CNG_Q
      0x400000,                        // OFFSET_DTC_AGGR
      0,                               // OFFSET_ENABLE_AEC_REUSE
      0x00000F,                        // OFFSET_ADCGAIN
      0x7FFFFF,                        // OFFSET_NDVC_HYSTERESIS
      0x09D752,                        // OFFSET_NDVC_ATK_TC
      0x09D752,                        // OFFSET_NDVC_DEC_TC
      0x000006,                        // OFFSET_NDVC_NUMVOLSTEPS
      0xEEFF96,                        // OFFSET_NDVC_MAXNOISELVL // q8.16
      0xE6FFCA,                        // OFFSET_NDVC_MINNOISELVL // q8.16
      0x000001,                        // OFFSET_NDVC_LB
      0x000041,                        // OFFSET_NDVC_HB
      0x000000,                        // OFFSET_SND_PEQ_NUMSTAGES
      0x000000,0x000000,0x000000,0x000000,0x000000,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      1,0,0,0,0,                       // OFFSET_SND_PEQ_SCALE
      1,                               // OFFSET_RCV_PEQ_NUMSTAGES
      0x3BB507,0x8895F2,0x3BB507,0x37B3DA,0x88DFBE,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      0x000000,0x000000,0x000000,0x000000,0x000000,
      1,0,0,0,0,                       // OFFSET_RCV_PEQ_SCALE
      8,                               // OFFSET_LVMODE_THRES
      0.694640395,                     // Inverse of DAC gain q.15
      0.464258683,
      0.348144898,
      0.232680525,
      0.174485778,
      0.116616509,
      0.087450044,
      0.058446705,
      0.043828846,
      0.031028447,
      0.021966458,
      0.015551061,
      0.011009308,
      0.007793993,
      0.005517725,
      0.00390625,
      //hard clipper
      0x7FFFFF,                        // OFFSET_CLIP_POINT
      0x23D70A,                        // OFFSET_SIDETONE_LIMIT 28%FS = -11 dBFS
      0.03125,                         // OFFSET_BOOST (Q5.18) 1/32 = 1.0
      0x721481,                        // OFFSET_BOOST_CLIP_POINT = -1 dBFS
      0.5,                             // OFFSET_G_ALFA - not used.

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

      $M.CVC.REF_DELAY,                // OFFSET_REF_DELAY
      0x00000F,                        // OFFSET_ADCGAIN_ASR

 	  // Send VAD
      0x005743,                        //OFFSET_SND_VAD_ATTACK_TC
      0x005743,                        //OFFSET_SND_VAD_DECAY_TC
      0x1C5041,                        //OFFSET_SND_VAD_ENVELOPE_TC
      0x000019,                        //OFFSET_SND_VAD_INIT_FRAME_THRESH
      0x400000,                        //OFFSET_SND_VAD_RATIO
      0x008000,                        //OFFSET_SND_VAD_MIN_SIGNAL
      0xFD4000,                        //OFFSET_SND_VAD_MIN_MAX_ENVELOPE
      0x005000,                        //OFFSET_SND_VAD_DELTA_THRESHOLD
      0x000038,                        //OFFSET_SND_VAD_COUNT_THRESHOLD

      // Send AGC
      0x287A26,                        //OFFSET_SND_AGC_TARGET (-7dB)
      0x09D752,                        //OFFSET_SND_AGC_ATTACK_TC (0.1)
      0x0504DA,                        //OFFSET_SND_AGC_DECAY_TC (1.0)
      0x0342CE,                        //OFFSET_SND_AGC_A_90_PK (1.0)
      0x008A44,                        //OFFSET_SND_AGC_D_90_PK (1.0)
      0x0B3F30,                        //OFFSET_SND_AGC_G_MAX (15 dB)
      0x599999,                        //OFFSET_SND_AGC_COMP
      0x65AC8B,                        //OFFSET_SND_AGC_INP_THRESH
      0x180000,                        //OFFSET_SND_AGC_SP_ATTACK
      0x34CE07,                        //OFFSET_SND_AGC_AD_THRESH1
      0xDA9DF8,                        //OFFSET_SND_AGC_AD_THRESH2
      0x00A1E8,                        //OFFSET_SND_AGC_G_MIN

      // Receive VAD
      0x005743,                        //OFFSET_RCV_VAD_ATTACK_TC
      0x005743,                        //OFFSET_RCV_VAD_DECAY_TC
      0x1C5041,                        //OFFSET_RCV_VAD_ENVELOPE_TC
      0x000019,                        //OFFSET_RCV_VAD_INIT_FRAME_THRESH
      0x400000,                        //OFFSET_RCV_VAD_RATIO
      0x008000,                        //OFFSET_RCV_VAD_MIN_SIGNAL
      0xFD4000,                        //OFFSET_RCV_VAD_MIN_MAX_ENVELOPE
      0x005000,                        //OFFSET_RCV_VAD_DELTA_THRESHOLD
      0x000038,                        //OFFSET_RCV_VAD_COUNT_THRESHOLD

      // Receive AGC
      0x287A26,                        //OFFSET_RCV_AGC_TARGET (-7dB)
      0x09D752,                        //OFFSET_RCV_AGC_ATTACK_TC (0.1)
      0x0504DA,                        //OFFSET_RCV_AGC_DECAY_TC (1.0)
      0x0342CE,                        //OFFSET_RCV_AGC_A_90_PK (1.0)
      0x008A44,                        //OFFSET_RCV_AGC_D_90_PK (1.0)
      0x0B3F30,                        //OFFSET_RCV_AGC_G_MAX (15 dB)
      0x599999,                        //OFFSET_RCV_AGC_COMP
      0x65AC8B,                        //OFFSET_RCV_AGC_INP_THRESH
      0x180000,                        //OFFSET_RCV_AGC_SP_ATTACK
      0x34CE07,                        //OFFSET_RCV_AGC_AD_THRESH1
      0xDA9DF8,                        //OFFSET_RCV_AGC_AD_THRESH2
      0x00A1E8,                        //OFFSET_RCV_AGC_G_MIN

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

      // Auxiliary gain
      0x7eca9c,                        // OFFSET_AUX_GAIN
      0x40000,                         // OFFSET_SCO_STREAM_MIX
      0x3f654e,                        // OFFSET_AUX_STREAM_MIX

      // DSP USER block
      0,0,0,0,0,                       // OFFSET_CUSTOM_PARAMETERS_0-4
      0,0,                             // OFFSET_CUSTOM_PARAMETERS 5-6
                                       //
      0,0,0;                           // OFFSET_CUSTOM_PARAMETERS_7-9



   .VAR  CurParams[$M.CVC_HEADSET.PARAMETERS.STRUCT_SIZE];
   // -----------------------------------------------------------------------------

   // FFT data object, common to all filter_bank cases
   // The three buffers in this object are temporary to FFT and could be shared
   .VAR fft_obj[$M.filter_bank.fft.STRUC_SIZE] =
      0,
      &fft_real,
      &fft_imag,
      &fft_circ,
      BITREVERSE(&fft_circ);


   .VAR/DM2CIRC ref_delay_buffer[Max_RefDelay_Sample];

.if uses_RCV_FREQPROC
   .VAR/DM2  bufdr_outp[($M.CVC.Num_FFT_Window + $M.CVC.Num_Samples_Per_Frame)];

.endif

   // Analysis Filter Bank Config Block
   .VAR/DM1 AecAnalysisBank[$M.filter_bank.Parameters.TWO_CHNL_BLOCK_SIZE] =
      0,                               // CH1_CONST_CBUF_LEN
      0,                               // CH2_CONST_CBUF_LEN
      0,                               // CH1_PTR_FRAME
      0,                               // CH2_PTR_FRAME
      &bufd_inp,                       // OFFSET_CH1_PTR_HISTORY
      &bufx_inp,                       // OFFSET_CH2_PTR_HISTORY
      0,                               // CH1_BEXP
      0,                               // CH2_BEXP
      &D_real,                         // CH1_PTR_FFTREAL
      &D_imag,                         // CH1_PTR_FFTIMAG
      &X_real,                         // CH2_PTR_FFTREAL
      &X_imag,                         // CH2_PTR_FFTIMAG
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_REF_DELAY,  //OFFSET_CH2_DELAY_PTR
      &ref_delay_buffer,               // OFFSET_CH2_DELAY_BUF_PTR
      LENGTH(ref_delay_buffer);        // OFFSET_CH2_DELAY_BUF_LEN

   // Syntheseis Filter Bank Config Block
   .VAR/DM2 SndSynthesisBank[$M.filter_bank.Parameters.ONE_CHNL_BLOCK_SIZE] =
      0,                               // OFFSET_CONST_CBUF_LEN
      0,                               // OFFSET_PTR_FRAME
      &bufd_outp,                      // OFFSET_PTR_HISTORY
      &AecAnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_BEXP,
.if uses_SND_NS
      &E_real,                         // OFFSET_PTR_FFTREAL
      &E_imag;                         // OFFSET_PTR_FFTIMAG
.else
      &D_real,                         // OFFSET_PTR_FFTREAL
      &D_imag;                         // OFFSET_PTR_FFTIMAG
.endif


.if uses_RCV_FREQPROC
    // Analysis Filter Bank Config Block
   .VAR/DM1 RcvAnalysisBank[$M.filter_bank.Parameters.ONE_CHNL_BLOCK_SIZE] =
      0,                               // CH1_CONST_CBUF_LEN
      0,                               // CH1_PTR_FRAME
      &bufdr_inp,                      // OFFSET_CH1_PTR_HISTORY
      0,                               // CH1_BEXP
      &D_real,                         // CH1_PTR_FFTREAL
      &D_imag,                         // CH1_PTR_FFTIMAG
      0,                               // No Channel Delay
      0,
      0;

    // Syntheseis Filter Bank Config Block
   .VAR/DM2 RcvSynthesisBank[$M.filter_bank.Parameters.ONE_CHNL_BLOCK_SIZE] =
      0,                               // OFFSET_CONST_CBUF_LEN
      0,                               // OFFSET_PTR_FRAME
      &bufdr_outp,                     // OFFSET_PTR_HISTORY
      &RcvAnalysisBank + $M.filter_bank.Parameters.OFFSET_BEXP,
      &E_real,                         // OFFSET_PTR_FFTREAL
      &E_imag;                         // OFFSET_PTR_FFTIMAG
.endif



// NS
// The oms_scratch buffer reuses the AEC buffer to reduce
// the data memory usage.
.define oms_scratch $M.AEC_Headset.dm1.W.ri

.if uses_SND_NS
   // <start> of memory declared per instance of oms270
   .VAR/DM1CIRC sndLpX_queue[$M.oms270.QUE_LENGTH];
   .VAR sndoms_G[$M.oms270.FFT_NUM_BIN];
   .VAR sndoms_LpXnz[$M.oms270.FFT_NUM_BIN];
   .VAR sndoms_state[$M.oms270.STATE_LENGTH];

   .VAR oms270snd_obj[$M.oms270.STRUC_SIZE] =
        $M.oms270.mode.object,  //$M.oms270.PTR_MODE_FIELD
        0,                      // $M.oms270.CONTROL_WORD_FIELD
        $M.CVC_HEADSET.CONFIG.SNDOMSENA,
                                // $M.oms270.ENABLE_BIT_MASK_FIELD
        1,                      // $M.oms270.MIN_SEARCH_ON_FIELD
        1,                      // $M.oms270.HARM_ON_FIELD
        1,                      // $M.oms270.MMSE_LSA_ON_FIELD
        &bufd_inp-2*$M.CVC.Num_Samples_Per_Frame, // $M.oms270.PTR_INP_X_FIELD
        &D_real,                // $M.oms270.PTR_X_REAL_FIELD
        &D_imag,                // $M.oms270.PTR_X_IMAG_FIELD
        &AecAnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_BEXP,
                                // $M.oms270.PTR_BEXP_X_FIELD
        &E_real,                // $M.oms270.PTR_Y_REAL_FIELD
        &E_imag,                // $M.oms270.PTR_Y_IMAG_FIELD
        $M.oms270.MINUS1_Q8_16,    // $M.oms270.INITIAL_POWER_FIELD
        &sndLpX_queue,          // $M.oms270.PTR_LPX_QUEUE_FIELD
        0,                      // $M.oms270.LPX_PREV_MIN_HHB_FIELD
        &sndoms_G,                 // $M.oms270.G_FIELD;
        &sndoms_LpXnz,             // $M.oms270.LPXNZ_FIELD,
        &sndoms_state,             // $M.oms270.PTR_STATE_FIELD
        &oms_scratch,           // $M.oms270.PTR_SCRATCH_FIELD
        0.03921056084768,       // $M.oms270.ALFANZ_FIELD
        0xFF23A7,               // $M.oms270.LALFAS_FIELD
        0xFED889,               // $M.oms270.LALFAS1_FIELD
        0.45,                   // $M.oms270.HARMONICITY_THRESHOLD_FIELD
        0x733333,               // $M.oms270.VAD_THRESHOLD_FIELD
        1.0,                    // $M.oms270.AGRESSIVENESS_FIELD
        0,                      // $M.oms270.PTR_TONE_FLAG_FIELD
        &vad400_dm2 + $M.vad400.FLAG_FIELD, // $M.oms270.PTR_RCVVAD_FLAG_FIELD
        &snd_vad400_dm2 + $M.vad400.FLAG_FIELD; // $M.oms270.PTR_SNDVAD_FLAG_FIELD

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
        $M.CVC_HEADSET.CONFIG.RCVOMSENA,
                                // $M.oms270.ENABLE_BIT_MASK_FIELD
        1,                      // $M.oms270.MIN_SEARCH_ON_FIELD
        1,                      // $M.oms270.HARM_ON_FIELD
        1,                      // $M.oms270.MMSE_LSA_ON_FIELD
        &bufdr_inp-2*$M.CVC.Num_Samples_Per_Frame, // $M.oms270.PTR_INP_X_FIELD
        &D_real,                // $M.oms270.PTR_X_REAL_FIELD
        &D_imag,                // $M.oms270.PTR_X_IMAG_FIELD
        &RcvAnalysisBank + $M.filter_bank.Parameters.OFFSET_BEXP,
                                // $M.oms270.PTR_BEXP_X_FIELD
.if uses_AEQ
        &D_real,                // $M.oms270.PTR_Y_REAL_FIELD
        &D_imag,                // $M.oms270.PTR_Y_IMAG_FIELD
.else
        &E_real,                // $M.oms270.PTR_Y_REAL_FIELD
        &E_imag,                // $M.oms270.PTR_Y_IMAG_FIELD
.endif
        $M.oms270.LDELTAP,    // $M.oms270.INITIAL_POWER_FIELD
        &rcvLpX_queue,          // $M.oms270.PTR_LPX_QUEUE_FIELD
        0,                      // $M.oms270.LPX_PREV_MIN_HHB_FIELD
        &rcvoms_G,                 // $M.oms270.G_FIELD;
        &rcvoms_LpXnz,             // $M.oms270.LPXNZ_FIELD,
        &rcvoms_state,             // $M.oms270.PTR_STATE_FIELD
        &oms_scratch,           // $M.oms270.PTR_SCRATCH_FIELD
        0.03921056084768,       // $M.oms270.ALFANZ_FIELD
        0xFF23A7,               // $M.oms270.LALFAS_FIELD
        0xFED889,               // $M.oms270.LALFAS1_FIELD
        0.45,                   // $M.oms270.HARMONICITY_THRESHOLD_FIELD
        0x733333,               // $M.oms270.VAD_THRESHOLD_FIELD
        0.9,                    // $M.oms270.AGRESSIVENESS_FIELD
.if uses_AEQ                    // $M.oms270.PTR_TONE_FLAG_FIELD
        &AEQ_DataObject + $M.AdapEq.AEQ_POWER_TEST_FIELD;
.else
        0;
.endif

.endif

// AEC
.if uses_AEC
   .VAR/DM1 aec_dm1[$M.AEC_Headset.STRUCT_SIZE] =
      &D_real,                         // OFFSET_D_REAL_PTR
      &D_imag,                         // OFFSET_D_IMAG_PTR
      &AecAnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_BEXP,
      &X_real,                         // OFFSET_X_REAL_PTR
      &X_imag,                         // OFFSET_X_IMAG_PTR
      &AecAnalysisBank + $M.filter_bank.Parameters.OFFSET_CH2_BEXP,
      &sndoms_G,
      &sndoms_LpXnz,
      &sndoms_G,
      &E_real,                         // OFFSET_E_REAL_PTR 9
      &E_imag,                         // OFFSET_E_IMAG_PTR 10
      1.0,                            // OFFSET_OMS_AGGRESSIVENESS 11
      0x200000,                       // OFFSET_CNG_Q_ADJUST 12
      0x6Ed9eb,                       // OFFSET_CNG_G_ADJUST(unused) 13
      0.5,                            // OFFSET_DTC_AGRESSIVENESS 14
      0.5,                            // OFFSET_RER_WGT_L2PXR 15
      0.5,                            // OFFSET_RER_WGT_L2PDR 16
      0,                              // OFFSET_ENABLE_AEC_REUSE
      $M.CVC_HEADSET.CONFIG.AECENA,   // OFFSET_ENABLE_AEC
      0;                              // OFFSET_CONFIG
.endif


.if uses_DCBLOCK
   // DC Blocker
   // Filter format: b2,b1,b0,a2,a1
   .VAR/DM1CIRC dcblock_coeffs[5] =
      0.948607495176447/2, -1.897214990352894/2, 0.948607495176447/2,
      0.899857926182383/2, -1.894572054523406/2;

   .VAR/DM1 dcblock_scale = 1;
   .VAR/DM1 dcblock_gain_mts = 0.5;
   .VAR/DM1 dcblock_gain_exp = 0x000001;
   .VAR/DM1CIRC adc_dc_block_delaybuf_dm1[4]; // SIZE = (NUM_STAGES_FIELD+1)*2
   .VAR/DM1 adc_dc_block_dm1[$audio_proc.peq.STRUC_SIZE] =
      0,                               // PTR_INPUT_DATA_BUFF_FIELD
      0,                               // INPUT_CIRCBUFF_SIZE_FIELD
      0,                               // PTR_OUTPUT_DATA_BUFF_FIELD
      0,                               // OUTPUT_CIRCBUFF_SIZE_FIELD
      &adc_dc_block_delaybuf_dm1,      // PTR_DELAY_LINE_FIELD
      &dcblock_coeffs,                 // PTR_COEFS_BUFF_FIELD
      1,                               // NUM_STAGES_FIELD
      0,                               // DELAY_BUF_SIZE
      0,                               // COEFF_BUF_SIZE
      $M.CVC.Num_Samples_Per_Frame,    // BLOCK_SIZE_FIELD
      &dcblock_scale,                  // PTR_SCALE_BUFF_FIELD
      &dcblock_gain_exp,               // INPUT_GAIN_EXPONENT_PTR
      &dcblock_gain_mts;               // INPUT_GAIN_MANTISSA_PTR

   .VAR/DM1CIRC sco_dc_block_delaybuf_dm1[4]; // SIZE = (NUM_STAGES_FIELD+1)*2
   .VAR/DM1 sco_dc_block_dm1[$audio_proc.peq.STRUC_SIZE] =
      0,                               // PTR_INPUT_DATA_BUFF_FIELD
      0,                               // INPUT_CIRCBUFF_SIZE_FIELD
      0,                               // PTR_OUTPUT_DATA_BUFF_FIELD
      0,                               // OUTPUT_CIRCBUFF_SIZE_FIELD
      &sco_dc_block_delaybuf_dm1,      // PTR_DELAY_LINE_FIELD
      &dcblock_coeffs,                 // PTR_COEFS_BUFF_FIELD
      1,                               // NUM_STAGES_FIELD
      0,                               // DELAY_BUF_SIZE
      0,                               // COEFF_BUF_SIZE
      $M.CVC.Num_Samples_Per_Frame,    // BLOCK_SIZE_FIELD
      &dcblock_scale,                  // PTR_SCALE_BUFF_FIELD
      &dcblock_gain_exp,               // INPUT_GAIN_EXPONENT_PTR
      &dcblock_gain_mts;               // INPUT_GAIN_MANTISSA_PTR
.endif

   .VAR/DM1 mute_cntrl_dm1[$M.MUTE_CONTROL.STRUC_SIZE] =
      0,                               // OFFSET_INPUT_PTR
      0,                               // OFFSET_INPUT_LEN
      $M.CVC.Num_Samples_Per_Frame,    // OFFSET_NUM_SAMPLES
      &$M.CVC_HEADSET.CurCallState,    // OFFSET_PTR_STATE
      $M.CVC.CALLST.MUTE;              // OFFSET_MUTE_VAL


.if uses_SND_PEQ
   // Parameteric EQ
   .VAR/DM2CIRC snd_peq_delaybuf_dm2[2 * (MAX_NUM_PEQ_STAGES + 1)];
   .VAR/DM1CIRC snd_peq_coeffs[5 * MAX_NUM_PEQ_STAGES];
   .VAR snd_peq_scale[5];
   .VAR/DM2	snd_peq_dm2[$audio_proc.peq.STRUC_SIZE] =
      0,                               // PTR_INPUT_DATA_BUFF_FIELD
      0,                               // INPUT_CIRCBUFF_SIZE_FIELD
      0,                               // PTR_OUTPUT_DATA_BUFF_FIELD
      0,                               // OUTPUT_CIRCBUFF_SIZE_FIELD
      &snd_peq_delaybuf_dm2,           // PTR_DELAY_LINE_FIELD
      &snd_peq_coeffs,                 // PTR_COEFS_BUFF_FIELD
      0,                               // NUM_STAGES_FIELD
      0,                               // DELAY_BUF_SIZE
      0,                               // COEFF_BUF_SIZE
      $M.CVC.Num_Samples_Per_Frame,    // BLOCK_SIZE_FIELD
      &snd_peq_scale,
      &ZeroValue,
      &OneValue;
.endif

   // SND AGC Pre-Gain stage
   .VAR/DM1 out_gain_dm1[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,                               // OFFSET_INPUT_PTR
      0,                               // OFFSET_INPUT_LEN
      0,                               // OFFSET_OUTPUT_PTR
      0,                               // OFFSET_OUTPUT_LEN
      $M.CVC.Num_Samples_Per_Frame,    // NUM_SAMPLES
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SNDGAIN_MANTISSA,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SNDGAIN_EXPONENT;

.if uses_SND_AGC || uses_RCV_VAD
   // Filter format: (b2,b1,b0,a2,a1)/2

   .VAR/DM1CIRC  vad_peq_coeffs[15] =
      3658586,    -7303920,     3662890,     3363562,    -7470041,
      3874204,    -7787540,     4194304,     3702500,    -7573428,
      4101184,    -7581562,     4194304,     4082490,    -7559795;
.endif   // uses_XXX_AGC

.if uses_SND_AGC
   .VAR/DM1      snd_vad_peq_scale[3] = 1,1,1;  // SIZE = NUM_STAGES_FIELD
   .VAR/DMCIRC   snd_vad_delaybuf[8];           // SIZE = (NUM_STAGES_FIELD+1)*2
   .VAR/DMCIRC   snd_vad_peq_output[$M.CVC.Num_Samples_Per_Frame];

   .VAR/DM snd_vad_peq[$audio_proc.peq.STRUC_SIZE] =
      0,                                        // PTR_INPUT_DATA_BUFF_FIELD
      0,                                        // INPUT_CIRCBUFF_SIZE_FIELD
      &snd_vad_peq_output,                      // PTR_OUTPUT_DATA_BUFF_FIELD
      $M.CVC.Num_Samples_Per_Frame,             // OUTPUT_CIRCBUFF_SIZE_FIELD
      &snd_vad_delaybuf,                        // PTR_DELAY_LINE_FIELD
      &vad_peq_coeffs,                      // PTR_COEFS_BUFF_FIELD
      3,                                        // NUM_STAGES_FIELD
      0,                                        // DELAY_BUF_SIZE
      0,                                        // COEFF_BUF_SIZE
      $M.CVC.Num_Samples_Per_Frame,             // BLOCK_SIZE_FIELD
      &snd_vad_peq_scale,                       // PTR_SCALE_BUFF_FIELD
      &ZeroValue,                               // INPUT_GAIN_EXPONENT_PTR
      &OneValue;                                // INPUT_GAIN_MANTISSA_PTR

   // SND VAD
   .VAR/DM1 snd_vad400_dm1[$M.vad400.DM1_OBJECT_SIZE_FIELD] =
      &snd_vad_peq_output, // INPUT_PTR_FIELD
      $M.CVC.Num_Samples_Per_Frame,                  // INPUT_LENGTH_FIELD
      0x1C5042,            // ENVELOPE_TC_FIELD
      0xFF6000,            // E_FILTER_FIELD
      0,                   // E_FILTER_MAX_FIELD
      0x5744,              // DECAY_TC_FIELD
      0x7FA8BC,            // ONEMINUS_DECAY_TC_FIELD
      0x5744,              // ATTACK_TC_FIELD
      0x7FA8BC,            // ONEMINUS_ATTACK_TC_FIELD
      0x400000,            // RATIO_FIELD
      0x1000,              // MIN_SIGNAL_FIELD
      0,                   // COUNTER_DELTA_FIELD
      56;                  // COUNT_THRESHOLD_FIELD

   .VAR/DM2 snd_vad400_dm2[$M.vad400.DM2_OBJECT_SIZE_FIELD] =
      0,                   // E_FILTER_MIN_FIELD
      0x63AFBE,            // ONEMINUS_ENVELOPE_TC_FIELD
      0,                   // COUNTER_FIELD
      25,                  // INIT_FRAME_THRESH_FIELD
      0xFD4000,            // MIN_MAX_ENVELOPE_FIELD
      0x5000,              // DELTA_THRESHOLD
      0,                   // FLAG_FIELD
      $M.CVC.Num_Samples_Per_Frame;                  // BLOCK_SIZE_FIELD

   // SND AGC
   .VAR/DM snd_agc400_dm[$M.agc400.STRUC_SIZE] =
      0,                   //OFFSET_SYS_CON_WORD_FIELD
      $M.CVC_HEADSET.CONFIG.SND_AGCBYP,
                           //OFFSET_BYPASS_BIT_MASK_FIELD
      0,                   //OFFSET_PTR_INPUT_FIELD
      $M.CVC.Num_Samples_Per_Frame,                  //OFFSET_INPUT_CBUF_LEN_FIELD
      0,                   //OFFSET_PTR_OUTPUT_FIELD
      $M.CVC.Num_Samples_Per_Frame,                  //OFFSET_OUTPUT_CBUF_LEN_FIELD
      $M.CVC.Num_Samples_Per_Frame,                  //OFFSET_FRAME_SIZE_FIELD
      &snd_vad400_dm2 + $M.vad400.FLAG_FIELD,
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
      0x140000,            //OFFSET_G_MAX_FIELD
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
      0,                   //OFFSET_PTR_TONE_FLAG_FIELD
      0x020000,            //OFFSET_INITIAL_GAIN_FIELD (Q17 format)
      0x28000;             //OFFSET_PWR_SCALE_FIELD
.endif


.if uses_RCV_PEQ
   .VAR/DM2CIRC rcv_peq_delaybuf_dm2[2 * (MAX_NUM_PEQ_STAGES + 1)];
   .VAR/DM1CIRC   rcv_peq_coeffs[5 * MAX_NUM_PEQ_STAGES];
   .VAR rcv_peq_scale[5];
   .VAR/DM2 rcv_peq_dm2[$audio_proc.peq.STRUC_SIZE] =
      0,                               // PTR_INPUT_DATA_BUFF_FIELD
      0,                               // INPUT_CIRCBUFF_SIZE_FIELD
      0,                               // PTR_OUTPUT_DATA_BUFF_FIELD
      0,                               // OUTPUT_CIRCBUFF_SIZE_FIELD
      &rcv_peq_delaybuf_dm2,           // PTR_DELAY_LINE_FIELD
      &rcv_peq_coeffs,                 // PTR_COEFS_BUFF_FIELD
      0,                               // NUM_STAGES_FIELD
      0,                               // DELAY_BUF_SIZE
      0,                               // COEFF_BUF_SIZE
      $M.CVC.Num_Samples_Per_Frame,    // BLOCK_SIZE_FIELD
      &rcv_peq_scale,
      &ZeroValue,                      // INPUT_GAIN_EXPONENT_PTR
      &OneValue;                       // INPUT_GAIN_MANTISSA_PTR
.endif

   // Pre RCV AGC gain stage
   .VAR/DM1 rcvout_gain_dm2[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,                               // OFFSET_INPUT_PTR
      0,                               // OFFSET_INPUT_LEN
      0,                               // OFFSET_OUTPUT_PTR
      0,                               // OFFSET_OUTPUT_LEN
      $M.CVC.Num_Samples_Per_Frame,                              // NUM_SAMPLES
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCVGAIN_MANTISSA,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCVGAIN_EXPONENT;

.if uses_RCV_VAD
   .VAR/DM1      rcv_vad_peq_scale[3] = 1,1,1;  // SIZE = NUM_STAGES_FIELD
   .VAR/DMCIRC   rcv_vad_delaybuf[8];           // SIZE = (NUM_STAGES_FIELD+1)*2
   .VAR/DMCIRC   rcv_vad_peq_output[$M.CVC.Num_Samples_Per_Frame];

   .VAR/DM rcv_vad_peq[$audio_proc.peq.STRUC_SIZE] =
      0,                                        // PTR_INPUT_DATA_BUFF_FIELD
      0,                                        // INPUT_CIRCBUFF_SIZE_FIELD
      &rcv_vad_peq_output,                      // PTR_OUTPUT_DATA_BUFF_FIELD
      $M.CVC.Num_Samples_Per_Frame,                                       // OUTPUT_CIRCBUFF_SIZE_FIELD
      &rcv_vad_delaybuf,                        // PTR_DELAY_LINE_FIELD
      &vad_peq_coeffs,                          // PTR_COEFS_BUFF_FIELD
      3,                                        // NUM_STAGES_FIELD
      0,                                        // DELAY_BUF_SIZE
      0,                                        // COEFF_BUF_SIZE
      $M.CVC.Num_Samples_Per_Frame,                                       // BLOCK_SIZE_FIELD
      &rcv_vad_peq_scale,                       // PTR_SCALE_BUFF_FIELD
      &ZeroValue,                               // INPUT_GAIN_EXPONENT_PTR
      &OneValue;                                // INPUT_GAIN_MANTISSA_PTR

   // RCV VAD
   .VAR/DM1 vad400_dm1[$M.vad400.DM1_OBJECT_SIZE_FIELD] =
      &rcv_vad_peq_output, // INPUT_PTR_FIELD
      $M.CVC.Num_Samples_Per_Frame,                  // INPUT_LENGTH_FIELD
      0x1C5042,            // ENVELOPE_TC_FIELD
      0xFF6000,            // E_FILTER_FIELD
      0,                   // E_FILTER_MAX_FIELD
      0x5744,              // DECAY_TC_FIELD
      0x7FA8BC,            // ONEMINUS_DECAY_TC_FIELD
      0x5744,              // ATTACK_TC_FIELD
      0x7FA8BC,            // ONEMINUS_ATTACK_TC_FIELD
      0x400000,            // RATIO_FIELD
      0x1000,              // MIN_SIGNAL_FIELD
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
      $M.CVC.Num_Samples_Per_Frame;                  // BLOCK_SIZE_FIELD

.endif

.if uses_RCV_AGC
   // RCV AGC
   .VAR/DM rcv_agc400_dm[$M.agc400.STRUC_SIZE] =
      0,                   //OFFSET_SYS_CON_WORD_FIELD
      $M.CVC_HEADSET.CONFIG.RCV_AGCBYP,
                           //OFFSET_BYPASS_BIT_MASK_FIELD
      0,                   //OFFSET_PTR_INPUT_FIELD
      $M.CVC.Num_Samples_Per_Frame,                  //OFFSET_INPUT_CBUF_LEN_FIELD
      0,                   //OFFSET_PTR_OUTPUT_FIELD
      $M.CVC.Num_Samples_Per_Frame,                  //OFFSET_OUTPUT_CBUF_LEN_FIELD
      $M.CVC.Num_Samples_Per_Frame,                  //OFFSET_FRAME_SIZE_FIELD
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
      0x140000,            //OFFSET_G_MAX_FIELD
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
.if uses_AEQ               //OFFSET_PTR_TONE_FLAG_FIELD
      &AEQ_DataObject + $M.AdapEq.AEQ_POWER_TEST_FIELD,
.else
      0,
.endif
      0x040000,            //OFFSET_INITIAL_GAIN_FIELD (Q17 format)
      0x28000;             //OFFSET_PWR_SCALE_FIELD
.endif

   // Hard Clipper
   .VAR/DM1 rcv_hc_dm1[$M.HC_Alg_1_0_0.Parameters.OBJECT_BLOCK_SIZE] =
      0,                               // OFFSET_PTR_IN_FRAME
      0,                               // OFFSET_CONST_CBUF_LEN_IN_FRAME
      0,                               // OFFSET_PTR_OUT_FRAME
      0,                               // OFFSET_CONST_CBUF_LEN_OUT_FRAME
      $M.CVC.Num_Samples_Per_Frame,                              // OFFSET_FRAME_SIZE
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_INV_DAC_GAIN_TABLE,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_STF_GAIN_MANTISSA,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_STF_GAIN_EXP,
      0,                               // OFFSET_CLIP_POINT
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_BOOST_CLIP_POINT,
      0x23D70A,                        // OFFSET_SIDETONE_LIMIT 28%FS = -11 dBFS
      &$M.CVC_HEADSET.current_side_tone_gain,
      1.0,                             // OFFSET_BOOST
      &$M.CVC_HEADSET.PeakAux,         // OFFSET_PTR_PEAK_AUXVAL
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_CLIP_POINT, // OFFSET_ST_CLIP_POINT
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_ADJUST_LIMIT, // OFFSET_ST_ADJUST_LIMIT
      &$M.CVC_HEADSET.PeakSideTone;                             // OFFSET_PTR_PEAK_ST

.if uses_NSVOLUME
   // NDVC - Noise Controled Volume
   .VAR/DM1 ndvc_noise_elv_tbl[8];     // Length equals OFFSET_NUMVOLSTEPS
   .VAR/DM1 ndvc_dm1[$M.NDVC_Alg1_0_0.Parameters.BLOCK_SIZE] =
      0,                               // OFFSET_CONTROL_WORD
      $M.CVC_HEADSET.CONFIG.NDVCBYP,   // OFFSET_BITMASK_BYPASS
      &sndoms_LpXnz,                      // FROM OMS_270 LPXNZ
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
      $M.CVC_HEADSET.CONFIG.AEQENA,
      &D_real,                                  // PTR_X_REAL_FIELD             2
      &D_imag,                                  // PTR_X_IMAG_FIELD             3
      &RcvAnalysisBank + $M.filter_bank.Parameters.OFFSET_BEXP,   // PTR_BEXP_X_FIELD             4
      &E_real,                                  // PTR_Z_REAL_FIELD             5
      &E_imag,                                  // PTR_Z_IMAG_FIELD             6
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
      &$M.CVC_HEADSET.SysDACadjust,             // PTR_VOL_STEP_UP_FIELD        27
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
.if uses_NSVOLUME	                     // NOISE_LEVEL_PTR_FIELD: from NDVC FILTSUMLPDNZ
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


   // Hard Clipper for passthru, standby, and loopback modes
   .VAR/DM1	loopback_hc_dm1[$M.HC_Alg_1_0_0.Parameters.OBJECT_BLOCK_SIZE] =
      0,                               // OFFSET_PTR_IN_FRAME
      0,                               // OFFSET_CONST_CBUF_LEN_IN_FRAME
      0,                               // OFFSET_PTR_OUT_FRAME
      0,                               // OFFSET_CONST_CBUF_LEN_OUT_FRAME
      $M.CVC.Num_Samples_Per_Frame,                              // OFFSET_FRAME_SIZE
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_INV_DAC_GAIN_TABLE,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_STF_GAIN_MANTISSA,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_STF_GAIN_EXP,
      0,                               // OFFSET_CLIP_POINT
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_BOOST_CLIP_POINT,
      0,                               // OFFSET_SIDETONE_LIMIT 28%FS = -11 dBFS
      &$M.CVC_HEADSET.current_side_tone_gain,
      1.0,                             // OFFSET_BOOST
      &$M.CVC_HEADSET.PeakAux,         // OFFSET_PTR_PEAK_AUXVAL
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_CLIP_POINT, // OFFSET_ST_CLIP_POINT
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_ADJUST_LIMIT, // OFFSET_ST_ADJUST_LIMIT
      &$M.CVC_HEADSET.PeakSideTone;                             // OFFSET_PTR_PEAK_ST

   // This gain is used in ASR mode when PEQ and AGC does not get called.
   .VAR/DM1 out_gain_asr[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,                               // OFFSET_INPUT_PTR
      0,                               // OFFSET_INPUT_LEN
      0,                               // OFFSET_OUTPUT_PTR
      0,                               // OFFSET_OUTPUT_LEN
      $M.CVC.Num_Samples_Per_Frame,                              // NUM_SAMPLES
      &OneValue,                       // OFFSET_PTR_MANTISSA
      &ZeroValue;                      // OFFSET_PTR_EXPONENT

   .VAR mic_in_pk_dtct[] =
      0,                               // PTR_INPUT_BUFFER_FIELD
      0,                               // INPUT_BUFF_SIZE_FIELD
     $M.CVC.Num_Samples_Per_Frame,                              // NUM_SAMPLES_FEILD
      &$M.CVC_HEADSET.PeakMic;         // PEAK_LEVEL_PTR

   .VAR sco_in_pk_dtct[] =
      0,                               // PTR_INPUT_BUFFER_FIELD
      0,                               // INPUT_BUFF_SIZE_FIELD
      $M.CVC.Num_Samples_Per_Frame,                              // NUM_SAMPLES_FEILD
      &$M.CVC_HEADSET.PeakScoIn;       // PEAK_LEVEL_PTR

   .VAR sco_out_pk_dtct[] =
      0,                               // PTR_INPUT_BUFFER_FIELD
      0,                               // INPUT_BUFF_SIZE_FIELD
      $M.CVC.Num_Samples_Per_Frame,                              // NUM_SAMPLES_FEILD
      &$M.CVC_HEADSET.PeakScoOut;      // PEAK_LEVEL_PTR

   .VAR spkr_out_pk_dtct[] =
      0,                               // PTR_INPUT_BUFFER_FIELD
      0,                               // INPUT_BUFF_SIZE_FIELD
      $M.CVC.Num_Samples_Per_Frame,                              // NUM_SAMPLES_FEILD
      &$M.CVC_HEADSET.PeakSpkr;        // PEAK_LEVEL_PTR
   .VAR     ModeControl[$M.SET_MODE_GAIN.STRUC_SIZE] =
      &$M.CVC_HEADSET.cur_mode;

   .VAR passthru_rcv_gain[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,
      0,
      0,
      0,
      $M.CVC.Num_Samples_Per_Frame,
      &ModeControl + $M.SET_MODE_GAIN.SCO_IN_MANT, // OFFSET_PTR_MANTISSA
      &ModeControl + $M.SET_MODE_GAIN.SCO_IN_EXP;  // OFFSET_PTR_EXPONENT

   .VAR/DM1 passthru_snd_gain[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,                                        // OFFSET_INPUT_PTR
      0,                                        // OFFSET_INPUT_LEN
      0,                                        // OFFSET_OUTPUT_PTR
      0,                                        // OFFSET_OUTPUT_LEN
      $M.CVC.Num_Samples_Per_Frame,             // NUM_SAMPLES
      &ModeControl + $M.SET_MODE_GAIN.ADC_MANT, // OFFSET_PTR_MANTISSA
      &ModeControl + $M.SET_MODE_GAIN.ADC_EXP;  // OFFSET_PTR_EXPONENT

   .VAR loopback_sco_gain[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,
      0,
      0,
      0,
      $M.CVC.Num_Samples_Per_Frame,
      &ModeControl + $M.SET_MODE_GAIN.SCO_IN_MANT, // OFFSET_PTR_MANTISSA
      &ModeControl + $M.SET_MODE_GAIN.SCO_IN_EXP;  // OFFSET_PTR_EXPONENT

   .VAR/DM1 loopback_pcm_gain[$M.audio_proc.stream_gain.STRUC_SIZE] =
      0,                                        // OFFSET_INPUT_PTR
      0,                                        // OFFSET_INPUT_LEN
      0,                                        // OFFSET_OUTPUT_PTR
      0,                                        // OFFSET_OUTPUT_LEN
      $M.CVC.Num_Samples_Per_Frame,             // NUM_SAMPLES
      &ModeControl + $M.SET_MODE_GAIN.ADC_MANT, // OFFSET_PTR_MANTISSA
      &ModeControl + $M.SET_MODE_GAIN.ADC_EXP;  // OFFSET_PTR_EXPONENT
   // ----------------------------------------------------------------------------
   // Stream Load Tables

   // sndin stream map
   .VAR stream_map_sndin[] =

.if uses_DCBLOCK
      &adc_dc_block_dm1 + $audio_proc.peq.INPUT_ADDR_FIELD,
      &adc_dc_block_dm1 + $audio_proc.peq.INPUT_SIZE_FIELD,
      &adc_dc_block_dm1 + $audio_proc.peq.OUTPUT_ADDR_FIELD,
      &adc_dc_block_dm1 + $audio_proc.peq.OUTPUT_SIZE_FIELD,
.endif
      &AecAnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_PTR_FRAME,
      &AecAnalysisBank + $M.filter_bank.Parameters.OFFSET_CH1_CONST_CBUF_LEN,
      &mic_in_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &mic_in_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD,
      &passthru_snd_gain + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &passthru_snd_gain + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN,
      &loopback_pcm_gain + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &loopback_pcm_gain + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN;

   // sndout stream map
   .VAR stream_map_sndout[] =
      &SndSynthesisBank + $M.filter_bank.Parameters.OFFSET_PTR_FRAME,
      &SndSynthesisBank + $M.filter_bank.Parameters.OFFSET_CONST_CBUF_LEN,
      &out_gain_asr + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &out_gain_asr + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN,
      &out_gain_asr + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &out_gain_asr + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN,
      &out_gain_dm1 + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &out_gain_dm1 + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN,
      &out_gain_dm1 + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &out_gain_dm1 + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN,
      &passthru_snd_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &passthru_snd_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN,

.if uses_SND_AGC
      &snd_vad_peq + $audio_proc.peq.INPUT_ADDR_FIELD,
      &snd_vad_peq + $audio_proc.peq.INPUT_SIZE_FIELD,
      // Output of PEQ is input to VAD
      // Input to PEQ and VAD output is then input to AGC
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

      &loopback_sco_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &loopback_sco_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN;

   // rcvin stream map
   .VAR stream_map_rcvin[] =


.if uses_DCBLOCK
      &sco_dc_block_dm1 + $audio_proc.peq.INPUT_ADDR_FIELD,
      &sco_dc_block_dm1 + $audio_proc.peq.INPUT_SIZE_FIELD,
      &sco_dc_block_dm1 + $audio_proc.peq.OUTPUT_ADDR_FIELD,
      &sco_dc_block_dm1 + $audio_proc.peq.OUTPUT_SIZE_FIELD,
.endif

      &sco_in_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &sco_in_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD,


.if uses_RCV_FREQPROC
      &RcvAnalysisBank + $M.filter_bank.Parameters.OFFSET_PTR_FRAME,
      &RcvAnalysisBank + $M.filter_bank.Parameters.OFFSET_CONST_CBUF_LEN,
      &RcvSynthesisBank + $M.filter_bank.Parameters.OFFSET_PTR_FRAME,
      &RcvSynthesisBank + $M.filter_bank.Parameters.OFFSET_CONST_CBUF_LEN,
.endif


.if uses_RCV_PEQ
      &rcv_peq_dm2 + $audio_proc.peq.INPUT_ADDR_FIELD,
      &rcv_peq_dm2 + $audio_proc.peq.INPUT_SIZE_FIELD,
      &rcv_peq_dm2 + $audio_proc.peq.OUTPUT_ADDR_FIELD,
      &rcv_peq_dm2 + $audio_proc.peq.OUTPUT_SIZE_FIELD,
.endif

      &rcvout_gain_dm2 + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &rcvout_gain_dm2 + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN,
      &rcvout_gain_dm2 + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &rcvout_gain_dm2 + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN,

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


      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_PTR_IN_FRAME,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CONST_CBUF_LEN_IN_FRAME,

      &passthru_rcv_gain + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &passthru_rcv_gain + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN,
      &passthru_rcv_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &passthru_rcv_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN,

      &loopback_sco_gain + $M.audio_proc.stream_gain.OFFSET_INPUT_PTR,
      &loopback_sco_gain + $M.audio_proc.stream_gain.OFFSET_INPUT_LEN;

   // rcvout stream map
   .VAR stream_map_rcvout[] =

      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_PTR_OUT_FRAME,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CONST_CBUF_LEN_OUT_FRAME,
      &AecAnalysisBank + $M.filter_bank.Parameters.OFFSET_CH2_PTR_FRAME,
      &AecAnalysisBank + $M.filter_bank.Parameters.OFFSET_CH2_CONST_CBUF_LEN,
      &spkr_out_pk_dtct + $M.audio_proc.peak_monitor.PTR_INPUT_BUFFER_FIELD,
      &spkr_out_pk_dtct + $M.audio_proc.peak_monitor.INPUT_BUFF_SIZE_FIELD,

      &loopback_pcm_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR,
      &loopback_pcm_gain + $M.audio_proc.stream_gain.OFFSET_OUTPUT_LEN,

      &loopback_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_PTR_IN_FRAME,
      &loopback_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CONST_CBUF_LEN_IN_FRAME,
      &loopback_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_PTR_OUT_FRAME,
      &loopback_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CONST_CBUF_LEN_OUT_FRAME;

   // -----------------------------------------------------------------------------

   // Parameter to Module Map
   .VAR/DM2	ParameterMap[] =

   // sco gain
   &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SCO_STREAM_MIX,
   &$M.App.AuxilaryAudio.Tone.ScoGain,

   // aux gain
   &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AUX_STREAM_MIX,
   &$M.App.AuxilaryAudio.Tone.AuxGain,

.if uses_AEC
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_HFK_OMS_AGGR,
      &aec_dm1 + $M.AEC_Headset.OFFSET_OMS_AGGRESSIVENESS,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_CNG_Q,
      &aec_dm1 + $M.AEC_Headset.OFFSET_CNG_Q_ADJUST,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_DTC_AGGR,
      &aec_dm1 + $M.AEC_Headset.OFFSET_DTC_AGRESSIVENESS,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ENABLE_AEC_REUSE,
      &aec_dm1 + $M.AEC_Headset.OFFSET_ENABLE_AEC_REUSE,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_HFK_CONFIG,
      &aec_dm1 + $M.AEC_Headset.OFFSET_CONFIG,
.endif

.if uses_SND_NS
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_HFK_CONFIG,      &oms270snd_obj + $M.oms270.CONTROL_WORD_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ASR_OMS_AGGR,    &oms270snd_obj + $M.oms270.AGRESSIVENESS_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_OMS_HARMONICITY, &oms270snd_obj + $M.oms270.HARM_ON_FIELD,
.endif

.if uses_RCV_NS
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_HFK_CONFIG,      &oms270rcv_obj + $M.oms270.CONTROL_WORD_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_OMS_HFK_AGGR,&oms270rcv_obj + $M.oms270.AGRESSIVENESS_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_OMS_HI_RES_MODE, &oms270rcv_obj + $M.oms270.HARM_ON_FIELD,
.endif

.if uses_RCV_VAD
      // RCV VAD parameters
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_VAD_ATTACK_TC,         &vad400_dm1 + $M.vad400.ATTACK_TC_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_VAD_DECAY_TC,          &vad400_dm1 + $M.vad400.DECAY_TC_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_VAD_ENVELOPE_TC,       &vad400_dm1 + $M.vad400.ENVELOPE_TC_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_VAD_INIT_FRAME_THRESH, &vad400_dm2 + $M.vad400.INIT_FRAME_THRESH_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_VAD_RATIO,             &vad400_dm1 + $M.vad400.RATIO_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_VAD_MIN_SIGNAL,        &vad400_dm1 + $M.vad400.MIN_SIGNAL_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_VAD_MIN_MAX_ENVELOPE,  &vad400_dm2 + $M.vad400.MIN_MAX_ENVELOPE_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_VAD_DELTA_THRESHOLD,   &vad400_dm2 + $M.vad400.DELTA_THRESHOLD_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_VAD_COUNT_THRESHOLD,   &vad400_dm1 + $M.vad400.COUNT_THRESHOLD_FIELD,
.endif

.if uses_RCV_AGC
      // RCV AGC parameters
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_HFK_CONFIG,         &rcv_agc400_dm + $M.agc400.OFFSET_SYS_CON_WORD_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_TARGET,     &rcv_agc400_dm + $M.agc400.OFFSET_AGC_TARGET_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_ATTACK_TC,  &rcv_agc400_dm + $M.agc400.OFFSET_ATTACK_TC_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_DECAY_TC,   &rcv_agc400_dm + $M.agc400.OFFSET_DECAY_TC_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_A_90_PK,    &rcv_agc400_dm + $M.agc400.OFFSET_ALPHA_A_90_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_D_90_PK,    &rcv_agc400_dm + $M.agc400.OFFSET_ALPHA_D_90_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_G_MAX,      &rcv_agc400_dm + $M.agc400.OFFSET_G_MAX_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_COMP,       &rcv_agc400_dm + $M.agc400.OFFSET_COMP_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_INP_THRESH, &rcv_agc400_dm + $M.agc400.OFFSET_INPUT_THRESHOLD_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_SP_ATTACK,  &rcv_agc400_dm + $M.agc400.OFFSET_ATTACK_SPEED_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_AD_THRESH1, &rcv_agc400_dm + $M.agc400.OFFSET_ADAPT_THRESHOLD1_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_AD_THRESH2, &rcv_agc400_dm + $M.agc400.OFFSET_ADAPT_THRESHOLD2_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_AGC_G_MIN,      &rcv_agc400_dm + $M.agc400.OFFSET_G_MIN_FIELD,
.endif

.if uses_NSVOLUME
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_NDVC_HYSTERESIS,
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_HYSTERESIS,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_NDVC_ATK_TC,
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_ATK_TC,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_NDVC_DEC_TC,
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_DEC_TC,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_NDVC_NUMVOLSTEPS,
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_NUMVOLSTEPS,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_NDVC_MAXNOISELVL,
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_MAXNOISELVL,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_NDVC_MINNOISELVL,
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_MINNOISELVL,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_NDVC_LB,
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_LPDNZ_LB,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_NDVC_HB,
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_LPDNZ_HB,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_HFK_CONFIG,
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_CONTROL_WORD,
.endif

.if uses_SND_PEQ
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_NUMSTAGES,
      &snd_peq_dm2 + $audio_proc.peq.NUM_STAGES_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE1_B2,
      &snd_peq_coeffs,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE1_B1,
      &snd_peq_coeffs + 1,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE1_B0,
      &snd_peq_coeffs + 2,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE1_A2,
      &snd_peq_coeffs + 3,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE1_A1,
      &snd_peq_coeffs + 4,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE2_B2,
      &snd_peq_coeffs + 5,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE2_B1,
      &snd_peq_coeffs + 6,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE2_B0,
      &snd_peq_coeffs + 7,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE2_A2,
      &snd_peq_coeffs + 8,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE2_A1,
      &snd_peq_coeffs + 9,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE3_B2,
      &snd_peq_coeffs + 10,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE3_B1,
      &snd_peq_coeffs + 11,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE3_B0,
      &snd_peq_coeffs + 12,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE3_A2,
      &snd_peq_coeffs + 13,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE3_A1,
      &snd_peq_coeffs + 14,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE4_B2,
      &snd_peq_coeffs + 15,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE4_B1,
      &snd_peq_coeffs + 16,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE4_B0,
      &snd_peq_coeffs + 17,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE4_A2,
      &snd_peq_coeffs + 18,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE4_A1,
      &snd_peq_coeffs + 19,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE5_B2,
      &snd_peq_coeffs + 20,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE5_B1,
      &snd_peq_coeffs + 21,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE5_B0,
      &snd_peq_coeffs + 22,
      &CurParams+$M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE5_A2,
      &snd_peq_coeffs + 23,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_STAGE5_A1,
      &snd_peq_coeffs + 24,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_SCALE1,
      &snd_peq_scale,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_SCALE2,
      &snd_peq_scale + 1,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_SCALE3,
      &snd_peq_scale + 2,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_SCALE4,
      &snd_peq_scale + 3,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_PEQ_SCALE5,
      &snd_peq_scale + 4,
.endif

.if uses_RCV_PEQ
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_NUMSTAGES,
      &rcv_peq_dm2 + $audio_proc.peq.NUM_STAGES_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE1_B2,
      &rcv_peq_coeffs,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE1_B1,
      &rcv_peq_coeffs + 1,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE1_B0,
      &rcv_peq_coeffs + 2,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE1_A2,
      &rcv_peq_coeffs + 3,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE1_A1,
      &rcv_peq_coeffs + 4,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE2_B2,
      &rcv_peq_coeffs + 5,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE2_B1,
      &rcv_peq_coeffs + 6,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE2_B0,
      &rcv_peq_coeffs + 7,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE2_A2,
      &rcv_peq_coeffs + 8,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE2_A1,
      &rcv_peq_coeffs + 9,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE3_B2,
      &rcv_peq_coeffs + 10,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE3_B1,
      &rcv_peq_coeffs + 11,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE3_B0,
      &rcv_peq_coeffs + 12,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE3_A2,
      &rcv_peq_coeffs + 13,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE3_A1,
      &rcv_peq_coeffs + 14,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE4_B2,
      &rcv_peq_coeffs + 15,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE4_B1,
      &rcv_peq_coeffs + 16,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE4_B0,
      &rcv_peq_coeffs + 17,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE4_A2,
      &rcv_peq_coeffs + 18,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE4_A1,
      &rcv_peq_coeffs + 19,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE5_B2,
      &rcv_peq_coeffs + 20,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE5_B1,
      &rcv_peq_coeffs + 21,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE5_B0,
      &rcv_peq_coeffs + 22,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE5_A2,
      &rcv_peq_coeffs + 23,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_STAGE5_A1,
      &rcv_peq_coeffs + 24,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_SCALE1,
      &rcv_peq_scale,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_SCALE2,
      &rcv_peq_scale + 1,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_SCALE3,
      &rcv_peq_scale + 2,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_SCALE4,
      &rcv_peq_scale + 3,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_RCV_PEQ_SCALE5,
      &rcv_peq_scale + 4,
.endif

.if uses_SND_AGC
      // SND VAD parameters
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_VAD_ATTACK_TC,         &snd_vad400_dm1 + $M.vad400.ATTACK_TC_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_VAD_DECAY_TC,          &snd_vad400_dm1 + $M.vad400.DECAY_TC_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_VAD_ENVELOPE_TC,       &snd_vad400_dm1 + $M.vad400.ENVELOPE_TC_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_VAD_INIT_FRAME_THRESH, &snd_vad400_dm2 + $M.vad400.INIT_FRAME_THRESH_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_VAD_RATIO,             &snd_vad400_dm1 + $M.vad400.RATIO_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_VAD_MIN_SIGNAL,        &snd_vad400_dm1 + $M.vad400.MIN_SIGNAL_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_VAD_MIN_MAX_ENVELOPE,  &snd_vad400_dm2 + $M.vad400.MIN_MAX_ENVELOPE_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_VAD_DELTA_THRESHOLD,   &snd_vad400_dm2 + $M.vad400.DELTA_THRESHOLD_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_VAD_COUNT_THRESHOLD,   &snd_vad400_dm1 + $M.vad400.COUNT_THRESHOLD_FIELD,

      // SND AGC parameters
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_HFK_CONFIG,         &snd_agc400_dm + $M.agc400.OFFSET_SYS_CON_WORD_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_TARGET,     &snd_agc400_dm + $M.agc400.OFFSET_AGC_TARGET_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_ATTACK_TC,  &snd_agc400_dm + $M.agc400.OFFSET_ATTACK_TC_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_DECAY_TC,   &snd_agc400_dm + $M.agc400.OFFSET_DECAY_TC_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_A_90_PK,    &snd_agc400_dm + $M.agc400.OFFSET_ALPHA_A_90_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_D_90_PK,    &snd_agc400_dm + $M.agc400.OFFSET_ALPHA_D_90_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_G_MAX,      &snd_agc400_dm + $M.agc400.OFFSET_G_MAX_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_COMP,       &snd_agc400_dm + $M.agc400.OFFSET_COMP_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_INP_THRESH, &snd_agc400_dm + $M.agc400.OFFSET_INPUT_THRESHOLD_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_SP_ATTACK,  &snd_agc400_dm + $M.agc400.OFFSET_ATTACK_SPEED_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_AD_THRESH1, &snd_agc400_dm + $M.agc400.OFFSET_ADAPT_THRESHOLD1_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_AD_THRESH2, &snd_agc400_dm + $M.agc400.OFFSET_ADAPT_THRESHOLD2_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SND_AGC_G_MIN,      &snd_agc400_dm + $M.agc400.OFFSET_G_MIN_FIELD,
.endif

.if uses_AEQ
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_HFK_CONFIG,             &AEQ_DataObject + $M.AdapEq.CONTROL_WORD_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_ATK_TC,             &AEQ_DataObject + $M.AdapEq.ALFA_A_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_ATK_1MTC,           &AEQ_DataObject + $M.AdapEq.ONE_MINUS_ALFA_A_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_DEC_TC,             &AEQ_DataObject + $M.AdapEq.ALFA_D_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_DEC_1MTC,           &AEQ_DataObject + $M.AdapEq.ONE_MINUS_ALFA_D_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_LO_GOAL_LOW,        &goal_low,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_LO_GOAL_MID,        &goal_low + 1,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_LO_GOAL_HIGH,       &goal_low + 2,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_HI_GOAL_LOW,        &goal_high,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_HI_GOAL_MID,        &goal_high + 1,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_HI_GOAL_HIGH,       &goal_high + 2,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_POWER_TH,           &AEQ_DataObject + $M.AdapEq.AEQ_POWER_TH_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_MIN_GAIN,           &AEQ_DataObject + $M.AdapEq.AEQ_MIN_GAIN_TH_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_MAX_GAIN,           &AEQ_DataObject + $M.AdapEq.AEQ_MAX_GAIN_TH_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_VOL_STEP_UP_TH1,    &AEQ_DataObject + $M.AdapEq.VOL_STEP_UP_TH1_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_VOL_STEP_UP_TH2,    &AEQ_DataObject + $M.AdapEq.VOL_STEP_UP_TH2_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_LOW_STEP,           &AEQ_DataObject + $M.AdapEq.AEQ_PASS_LOW_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_LOW_STEP_INV,       &AEQ_DataObject + $M.AdapEq.INV_AEQ_PASS_LOW_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_HIGH_STEP,          &AEQ_DataObject + $M.AdapEq.AEQ_PASS_HIGH_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_HIGH_STEP_INV,      &AEQ_DataObject + $M.AdapEq.INV_AEQ_PASS_HIGH_FIELD,

      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_LOW_BAND_INDEX,     &AEQ_DataObject + $M.AdapEq.LOW_INDEX_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_LOW_BANDWIDTH,      &AEQ_DataObject + $M.AdapEq.LOW_BW_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_LOG2_LOW_BANDWIDTH, &AEQ_DataObject + $M.AdapEq.LOG2_LOW_INDEX_DIF_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_MID_BANDWIDTH,      &AEQ_DataObject + $M.AdapEq.MID_BW_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_LOG2_MID_BANDWIDTH, &AEQ_DataObject + $M.AdapEq.LOG2_MID_INDEX_DIF_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_HIGH_BANDWIDTH,     &AEQ_DataObject + $M.AdapEq.HIGH_BW_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_LOG2_HIGH_BANDWIDTH,&AEQ_DataObject + $M.AdapEq.LOG2_HIGH_INDEX_DIF_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_MID1_BAND_INDEX,    &AEQ_DataObject + $M.AdapEq.MID1_INDEX_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_MID2_BAND_INDEX,    &AEQ_DataObject + $M.AdapEq.MID2_INDEX_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_AEQ_HIGH_BAND_INDEX,    &AEQ_DataObject + $M.AdapEq.HIGH_INDEX_FIELD,
.endif

.if uses_PLC
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_PLC_STAT_INTERVAL,      &$sco_in.decoder_obj + $sco_pkt_handler.STAT_LIMIT_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_HFK_CONFIG,             &$sco_in.decoder_obj + $sco_pkt_handler.CONFIG_FIELD,
.endif

      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_CLIP_POINT,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CLIP_POINT,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_BOOST_CLIP_POINT,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_BOOST_CLIP_POINT,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SIDETONE_LIMIT,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_SIDETONE_LIMIT,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_BOOST,
      &rcv_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_BOOST,

      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_CLIP_POINT,
      &loopback_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_CLIP_POINT,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_BOOST_CLIP_POINT,
      &loopback_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_BOOST_CLIP_POINT,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_SIDETONE_LIMIT,
      &loopback_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_SIDETONE_LIMIT,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_BOOST,
      &loopback_hc_dm1 + $M.HC_Alg_1_0_0.Parameters.OFFSET_BOOST,

      // sidetone filter control
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_HFK_CONFIG,
      &sidetone_filter_object_dm2 + $CVC.sidetone_filter.OFFSET_ST_CONFIG,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_STF_SWITCH,
      &sidetone_filter_object_dm2 + $CVC.sidetone_filter.SWITCH_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_STF_NOISE_LOW_THRES,
      &sidetone_filter_object_dm2 + $CVC.sidetone_filter.NOISE_LOW_THRES_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_STF_NOISE_HIGH_THRES,
      &sidetone_filter_object_dm2 + $CVC.sidetone_filter.NOISE_HIGH_THRES_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_NUMSTAGES,
      &sidetone_filter_object_dm2 + $CVC.sidetone_filter.PEQ_START_FIELD + $audio_proc.peq.NUM_STAGES_FIELD,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE1_B2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE1 - 1)*5 + 0,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE1_B1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE1 - 1)*5 + 1,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE1_B0,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE1 - 1)*5 + 2,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE1_A2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE1 - 1)*5 + 3,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE1_A1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE1 - 1)*5 + 4,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_SCALE1,         &sidetone_peq_scale +  (STF_PEQ_CTRL_STAGE1 - 1),
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE2_B2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE2 - 1)*5 + 0,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE2_B1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE2 - 1)*5 + 1,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE2_B0,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE2 - 1)*5 + 2,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE2_A2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE2 - 1)*5 + 3,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE2_A1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE2 - 1)*5 + 4,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_SCALE2,         &sidetone_peq_scale +  (STF_PEQ_CTRL_STAGE2 - 1),
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE3_B2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE3 - 1)*5 + 0,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE3_B1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE3 - 1)*5 + 1,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE3_B0,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE3 - 1)*5 + 2,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE3_A2,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE3 - 1)*5 + 3,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_STAGE3_A1,      &sidetone_peq_coeffs + (STF_PEQ_CTRL_STAGE3 - 1)*5 + 4,
      &CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_SCALE3,         &sidetone_peq_scale +  (STF_PEQ_CTRL_STAGE3 - 1),


      // End of Parameter Map
      0;

   // Statistics from Modules sent via SPI
   // ------------------------------------------------------------------------
   .VAR StatisticsPtrs[$M.CVC_HEADSET.STATUS.BLOCK_SIZE] =
      &$M.CVC_HEADSET.cur_mode,
      &$M.CVC_HEADSET.CurCallState,
      &$M.CVC_HEADSET.SysControl,
      &$M.CVC_HEADSET.CurDAC,
      &$M.CVC_HEADSET.Last_PsKey,
      &$M.CVC_HEADSET.SecStatus,
      &$M.CVC_HEADSET.PeakSpkr,
      &$M.CVC_HEADSET.PeakMic,
      &$M.CVC_HEADSET.PeakScoIn,
      &$M.CVC_HEADSET.PeakScoOut,
      &$M.CVC_HEADSET.PeakMips,

.if uses_NSVOLUME
      &ndvc_dm1 + $M.NDVC_Alg1_0_0.Parameters.OFFSET_FILTSUMLPDNZ,
      &$M.CVC_HEADSET.SysDACadjust,
.else
      &ZeroValue,
      &ZeroValue,
.endif
      &$M.CVC_HEADSET.PeakAux,
      &$M.CVC_MODULES_STAMP.CompConfig,
      &$M.CVC_HEADSET.current_side_tone_gain,
      &$M.CVC_HEADSET.Volume,
      &$M.CVC_HEADSET.ConnectStatus,                           // $M.CVC_HEADSET.STATUS.CONNSTAT
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
      &$M.CVC_HEADSET.PeakSideTone;
   // Processing Tables
   // ----------------------------------------------------------------------------
   .VAR ReInitializeTable[] =

      // Function                             					r7                 r8

      $M.filter_bank.two_channel.analysis.Initialize.func,  &fft_obj,         &AecAnalysisBank,
      $M.filter_bank.one_channel.synthesis.Initialize.func, &fft_obj,         &SndSynthesisBank,

.if uses_RCV_FREQPROC
      $M.filter_bank.one_channel.analysis.Initialize.func,  &fft_obj,         &RcvAnalysisBank,
      $M.filter_bank.one_channel.synthesis.Initialize.func, &fft_obj,         &RcvSynthesisBank,
.endif

.if uses_RCV_NS
      $M.oms270.initialize.func,           &oms270rcv_obj,         	0,
.endif

.if uses_SND_NS
      $M.oms270.initialize.func,           &oms270snd_obj,         	0,
.ifdef wnr_params
      $M.oms270.wnr.initialize.func,       &oms270snd_obj,         	&wnr_params,
.endif
.endif

.if uses_AEC
      $M.AEC_Headset.Initialize.func,      0,                   &aec_dm1,
.endif

.if uses_DCBLOCK
      $audio_proc.peq.initialize,         &adc_dc_block_dm1,        0,
      $audio_proc.peq.initialize,         &sco_dc_block_dm1,        0,
.endif

.if uses_SND_PEQ
      $audio_proc.peq.initialize,         &snd_peq_dm2,             0,
.endif

.if uses_RCV_PEQ
      $audio_proc.peq.initialize,         &rcv_peq_dm2,              0,
.endif


.if uses_RCV_VAD
      $audio_proc.peq.initialize,         &rcv_vad_peq,                 0,
      $M.vad400.initialize.func,          &vad400_dm1,             &vad400_dm2,
.endif
.if uses_RCV_AGC
      $M.agc400.initialize.func,                0,                 &rcv_agc400_dm,
.endif

.if uses_NSVOLUME
      $M.NDVC_alg1_0_0.Initialize.func,   &ndvc_dm1,                    0,
.endif

.if uses_SND_AGC
      $audio_proc.peq.initialize,         &snd_vad_peq,                 0,
      $M.vad400.initialize.func,          &snd_vad400_dm1,         &snd_vad400_dm2,
      $M.agc400.initialize.func,                0,                 &snd_agc400_dm,
.endif

.if uses_AEQ
      $M.AdapEq.initialize.func,                0,                 &AEQ_DataObject,
      $M.CVC.data.set_fb_scale,                 0,                 0,
.endif

.if uses_PLC
      $frame.sco_initialize,               &$sco_in.decoder_obj,         0,
.endif

      $CVC.sidetone_filter.Initialize,     &sidetone_filter_object_dm2,  &rcv_hc_dm1,

      0;                                    // END OF TABLE

   // -------------------------------------------------------------------------------
   // Table of functions for current mode
   .VAR ModeProcTable[$M.CVC.SYSMODE.MAX_MODES] =
      &PsThru_proc_funcs,                 // undefined state
      &hfk_proc_funcs,                    // hfk mode
      &asr_proc_funcs,                    // asr mode
      &PsThru_proc_funcs,                 // pass-thru mode
      &PsThru_proc_funcs,                 // undefined state
      &LpBack_proc_funcs,                 // loop-back mode
      &PsThru_proc_funcs,                 // standby-mode
      &lvm_proc_funcs;                    // low volume mode

   // -----------------------------------------------------------------------------
   .VAR hfk_proc_funcs[] =
      // Function                               r7                   r8
      $frame.distribute_streams_rm,               &stream_setup_table,  &$cvc_cbuffers,

.if uses_DCBLOCK
      $audio_proc.peq.process,                  &adc_dc_block_dm1,      0,
      $audio_proc.peq.process,                  &sco_dc_block_dm1,      0,
.endif

      $M.audio_proc.peak_monitor.Process.func,  &mic_in_pk_dtct,        0,
      $M.audio_proc.peak_monitor.Process.func,  &sco_in_pk_dtct,        0,

.if uses_RCV_VAD
      $audio_proc.peq.process,                  &rcv_vad_peq,           0,
      $M.vad400.process.func,                   &vad400_dm1,        &vad400_dm2,
.endif

// New Processing - TBD
.if uses_RCV_FREQPROC
.if uses_RCV_NS
      $M.CVC.data.copyInpBuffer2HarmBuffer,     0, &oms270rcv_obj + $M.oms270.PTR_INP_X_FIELD,
.endif
      $M.filter_bank.one_channel.analysis.Process.func, &fft_obj,   &RcvAnalysisBank,

.if uses_AEQ
      $M.AdapEq.process.tone_detect,            0,                   &AEQ_DataObject,
.endif

.if uses_RCV_NS
      $M.oms270.process.func,       			   &oms270rcv_obj,        	0,
      $M.oms270.apply_gain.func,                &oms270rcv_obj,        	0,
.endif

.if uses_AEQ
      $M.AdapEq.process.func,                   0,                   &AEQ_DataObject,
.endif
      $M.CVC.Zero_DC_Nyquist.func,              &E_real,              &E_imag,
      $M.filter_bank.one_channel.synthesis.Process.func, &fft_obj,   &RcvSynthesisBank,
.endif

.if uses_RCV_PEQ
      $audio_proc.peq.process,                  &rcv_peq_dm2,           0,
.endif

      $M.audio_proc.stream_gain.Process.func,   &rcvout_gain_dm2,       0,

.if uses_RCV_AGC
      $M.agc400.process.func,                       0,              &rcv_agc400_dm,
.endif

      $M.HC_Alg_1_0_0.Process.func,                 0,              &rcv_hc_dm1,

.if uses_SND_NS
      $M.CVC.data.copyInpBuffer2HarmBuffer,     0, &oms270snd_obj + $M.oms270.PTR_INP_X_FIELD,
.endif
      $M.filter_bank.two_channel.analysis.Process.func, &fft_obj,   &AecAnalysisBank,

.if uses_SND_NS
      $M.oms270.process.func,       			&oms270snd_obj,        	0,
      $M.oms270.apply_gain.func,                &oms270snd_obj,        	0,
.endif

.if uses_AEC
      $M.AEC_Headset.fnmls_process.func,            0,                   0,
      $M.AEC_Headset.comfort_noise_generator.func,  0,                   0,
.endif

.if uses_SND_NS
      $M.CVC.Zero_DC_Nyquist.func,              &E_real,             &E_imag,
.else
      $M.CVC.Zero_DC_Nyquist.func,              &D_real,             &D_imag,
.endif

      $M.filter_bank.one_channel.synthesis.Process.func, &fft_obj,   &SndSynthesisBank,

.if uses_NSVOLUME
      $M.NDVC_alg1_0_0.Process.func,            &ndvc_dm1,                  0,
.endif

.if uses_SND_PEQ
      $audio_proc.peq.process,                  &snd_peq_dm2,               0,
.endif

      $M.audio_proc.stream_gain.Process.func,   &out_gain_dm1,              0,

.if uses_SND_AGC
      $audio_proc.peq.process,            		&snd_vad_peq,               0,
      $M.vad400.process.func,                   &snd_vad400_dm1,     &snd_vad400_dm2,
      $M.agc400.process.func,                   0,                   &snd_agc400_dm,
.endif

      $M.MUTE_CONTROL.Process.func,             &mute_cntrl_dm1,            0,

      $M.audio_proc.peak_monitor.Process.func,  &sco_out_pk_dtct,           0,
      $M.audio_proc.peak_monitor.Process.func,  &spkr_out_pk_dtct,          0,

      $frame.update_streams_rm,                 &stream_setup_table, &$cvc_cbuffers,

      0;                                     // END OF TABLE

   // ----------------------------------------------------------------------------
   .VAR asr_proc_funcs[] =
      // Function                               r7                   r8
      $frame.distribute_streams_rm,                &stream_setup_table, &$cvc_cbuffers,


.if uses_DCBLOCK
      $audio_proc.peq.process,                  &adc_dc_block_dm1,       0,
      $audio_proc.peq.process,                  &sco_dc_block_dm1,       0,
.endif

      $M.audio_proc.peak_monitor.Process.func,  &mic_in_pk_dtct,         0,
      $M.audio_proc.peak_monitor.Process.func,  &sco_in_pk_dtct,         0,

.if uses_RCV_VAD
      $audio_proc.peq.process,                  &rcv_vad_peq,            0,
      $M.vad400.process.func,                   &vad400_dm1,          &vad400_dm2,
.endif

// New Processing - TBD
.if uses_RCV_FREQPROC
.if uses_RCV_NS
      $M.CVC.data.copyInpBuffer2HarmBuffer,     0, &oms270rcv_obj + $M.oms270.PTR_INP_X_FIELD,
.endif

      $M.filter_bank.one_channel.analysis.Process.func, &fft_obj,   &RcvAnalysisBank,

.if uses_AEQ
      $M.AdapEq.process.tone_detect,            0,                   &AEQ_DataObject,
.endif

.if uses_RCV_NS
      $M.oms270.process.func,       			   &oms270rcv_obj,        	0,
      $M.oms270.apply_gain.func,                &oms270rcv_obj,        	0,
.endif

.if uses_AEQ
      $M.AdapEq.process.func,                   0,                   &AEQ_DataObject,
.endif
      $M.CVC.Zero_DC_Nyquist.func,              &E_real,              &E_imag,
      $M.filter_bank.one_channel.synthesis.Process.func, &fft_obj,   &RcvSynthesisBank,
.endif

.if uses_RCV_PEQ
      $audio_proc.peq.process,                  &rcv_peq_dm2,            0,
.endif

      // This is pre RCVAGC gain stage
      $M.audio_proc.stream_gain.Process.func,   &rcvout_gain_dm2,        0,

.if uses_RCV_AGC
      $M.agc400.process.func,                       0,                &rcv_agc400_dm,
.endif

      $M.HC_Alg_1_0_0.Process.func,                 0,                &rcv_hc_dm1,


.if uses_SND_NS
      $M.CVC.data.copyInpBuffer2HarmBuffer,     0, &oms270snd_obj + $M.oms270.PTR_INP_X_FIELD,
.endif
      $M.filter_bank.two_channel.analysis.Process.func, &fft_obj,     &AecAnalysisBank,

.if uses_SND_NS
     $M.oms270.process.func,        			&oms270snd_obj,          	 0,
     $M.oms270.apply_gain.func,                 &oms270snd_obj,          	 0,
.endif

.if uses_SND_NS
      $M.CVC.Zero_DC_Nyquist.func,              &E_real,              &E_imag,
.else
      $M.CVC.Zero_DC_Nyquist.func,              &D_real,              &D_imag,
.endif

     $M.filter_bank.one_channel.synthesis.Process.func, &fft_obj,     &SndSynthesisBank,

.if uses_NSVOLUME
      $M.NDVC_alg1_0_0.Process.func,            &ndvc_dm1,               0,
.endif

      $M.audio_proc.stream_gain.Process.func,   &out_gain_asr,           0,

      $M.audio_proc.peak_monitor.Process.func,  &sco_out_pk_dtct,        0,
      $M.audio_proc.peak_monitor.Process.func,  &spkr_out_pk_dtct,       0,

      $frame.update_streams_rm,                 &stream_setup_table,  &$cvc_cbuffers,
      0;                                     // END OF TABLE

   // -----------------------------------------------------------------------------
   .VAR PsThru_proc_funcs[] =
      // Function                               r7                   r8
      $frame.distribute_streams_rm,             &stream_setup_table,  &$cvc_cbuffers,
      $M.set_mode_gains.func,                   &ModeControl,         0,
      $M.audio_proc.peak_monitor.Process.func,  &mic_in_pk_dtct,      0,
      $M.audio_proc.peak_monitor.Process.func,  &sco_in_pk_dtct,      0,
      $M.audio_proc.stream_gain.Process.func,   &passthru_snd_gain,   0,
      $M.audio_proc.stream_gain.Process.func,   &passthru_rcv_gain,   0,
      $M.HC_Alg_1_0_0.Process.func,             0,                    &rcv_hc_dm1,
      $M.CVC.sidetone_filter.DisableSideTone,   0,                    &sidetone_filter_object_dm2,
      $M.audio_proc.peak_monitor.Process.func,  &sco_out_pk_dtct,     0,
      $M.audio_proc.peak_monitor.Process.func,  &spkr_out_pk_dtct,    0,
      $frame.update_streams_rm,                 &stream_setup_table,  &$cvc_cbuffers,
      0;                                     // END OF TABLE

   // -------------------------------------------------------------------------------
   .VAR LpBack_proc_funcs[] =
      // Function                               r7                   r8
      $frame.distribute_streams_rm,             &stream_setup_table, &$cvc_cbuffers,
      $M.set_mode_gains.func,                   &ModeControl,        0,
      $M.audio_proc.peak_monitor.Process.func,  &mic_in_pk_dtct,     0,
      $M.audio_proc.peak_monitor.Process.func,  &sco_in_pk_dtct,     0,
      $M.audio_proc.stream_gain.Process.func,   &loopback_pcm_gain,  0,
      $M.audio_proc.stream_gain.Process.func,   &loopback_sco_gain,  0,
      $M.HC_Alg_1_0_0.Process.func,             0,                   &loopback_hc_dm1,
      $M.CVC.sidetone_filter.DisableSideTone,   0,                   &sidetone_filter_object_dm2,
      $M.audio_proc.peak_monitor.Process.func,  &sco_out_pk_dtct,    0,
      $M.audio_proc.peak_monitor.Process.func,  &spkr_out_pk_dtct,   0,
      $frame.update_streams_rm,                 &stream_setup_table, &$cvc_cbuffers,
      0;                                     // END OF TABLE

   // -----------------------------------------------------------------------------
   .VAR lvm_proc_funcs[] =
      // Function                               r7                   r8
      $frame.distribute_streams_rm,             &stream_setup_table, &$cvc_cbuffers,


.if uses_DCBLOCK
      $audio_proc.peq.process,                  &adc_dc_block_dm1,       0,
      $audio_proc.peq.process,                  &sco_dc_block_dm1,       0,
.endif

      $M.audio_proc.peak_monitor.Process.func,  &mic_in_pk_dtct,         0,
      $M.audio_proc.peak_monitor.Process.func,  &sco_in_pk_dtct,         0,

.if uses_RCV_VAD
      $audio_proc.peq.process,                  &rcv_vad_peq,            0,
      $M.vad400.process.func,                   &vad400_dm1,          &vad400_dm2,
.endif
// New Processing - TBD
.if uses_RCV_FREQPROC
.if uses_RCV_NS
      $M.CVC.data.copyInpBuffer2HarmBuffer,     0, &oms270rcv_obj + $M.oms270.PTR_INP_X_FIELD,
.endif

      $M.filter_bank.one_channel.analysis.Process.func, &fft_obj,   &RcvAnalysisBank,

.if uses_AEQ
      $M.AdapEq.process.tone_detect,            0,                   &AEQ_DataObject,
.endif

.if uses_RCV_NS
      $M.oms270.process.func,       			   &oms270rcv_obj,        	0,
      $M.oms270.apply_gain.func,                &oms270rcv_obj,        	0,
.endif

.if uses_AEQ
      $M.AdapEq.process.func,                   0,                   &AEQ_DataObject,
.endif
      $M.CVC.Zero_DC_Nyquist.func,              &E_real,              &E_imag,
      $M.filter_bank.one_channel.synthesis.Process.func, &fft_obj,   &RcvSynthesisBank,
.endif

.if uses_RCV_PEQ
      $audio_proc.peq.process,                  &rcv_peq_dm2,            0,
.endif

      $M.audio_proc.stream_gain.Process.func,   &rcvout_gain_dm2,        0,

.if uses_RCV_AGC
      $M.agc400.process.func,                       0,                &rcv_agc400_dm,
.endif

      $M.HC_Alg_1_0_0.Process.func,                 0,                &rcv_hc_dm1,


.if uses_SND_NS
      $M.CVC.data.copyInpBuffer2HarmBuffer,     0, &oms270snd_obj + $M.oms270.PTR_INP_X_FIELD,
.endif

      $M.filter_bank.two_channel.analysis.Process.func, &fft_obj,     &AecAnalysisBank,

.if uses_SND_NS
.if uses_AEC
     $M.CVC.data.copyHFKaggr2OMSaggr,               0,                   0,
.endif
     $M.oms270.process.func,        			&oms270snd_obj,          	 0,
     $M.oms270.apply_gain.func,                 &oms270snd_obj,          	 0,
.endif

.if uses_SND_NS
      $M.CVC.Zero_DC_Nyquist.func,              &E_real,              &E_imag,
.else
      $M.CVC.Zero_DC_Nyquist.func,              &D_real,              &D_imag,
.endif

      $M.filter_bank.one_channel.synthesis.Process.func, &fft_obj,    &SndSynthesisBank,

.if uses_NSVOLUME
      $M.NDVC_alg1_0_0.Process.func,            &ndvc_dm1,              0,
.endif

.if uses_SND_PEQ
      $audio_proc.peq.process,                  &snd_peq_dm2,           0,
.endif

      $M.audio_proc.stream_gain.Process.func,   &out_gain_dm1,          0,

.if uses_SND_AGC
      $audio_proc.peq.process,                  &snd_vad_peq,           0,
      $M.vad400.process.func,                   &snd_vad400_dm1,      &snd_vad400_dm2,
      $M.agc400.process.func,                   0,                    &snd_agc400_dm,
.endif

      $M.MUTE_CONTROL.Process.func,             &mute_cntrl_dm1,        0,

      $M.audio_proc.peak_monitor.Process.func,  &sco_out_pk_dtct,       0,
      $M.audio_proc.peak_monitor.Process.func,  &spkr_out_pk_dtct,      0,
      $frame.update_streams_rm,                    &stream_setup_table,  &$cvc_cbuffers,
      0;                                     // END OF TABLE

   // -----------------------------------------------------------------------------

   // STREAM SETUP TABLE
   // This table is used with $frame.distribute_streams and $frame.update_streams.
   //
   // $frame.distribute_streams populates processing module data objects with
   // current address into the cbuffers so modules know where to get and write
   // their data.
   //
   // $frame.update_streams updates the cbuffer read and write addresses into the
   // the cbuffers after modules have finished their processing so that the
   // pointers are in the correct positions for the next time the processing //modules   are called.

   .VAR	stream_setup_table[] =
      // Frame Properties
      0.125,     // Frame Jitter

      // LEFT INPUT
      &stream_map_rcvin, LENGTH(stream_map_rcvin)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      $M.CVC.Num_Samples_Per_Frame,0,0,0,0,

      // RIGHT INPUT
      &stream_map_sndin, LENGTH(stream_map_sndin)/2,
      $frame.distribute_input_stream_rm, $frame.update_input_streams_rm,
      $M.CVC.Num_Samples_Per_Frame,0,0,0,0,

      // LEFT OUTPUT
      &stream_map_sndout, LENGTH(stream_map_sndout)/2,
      $frame.distribute_output_stream_rm, $frame.update_output_streams_rm,
      $M.CVC.Num_Samples_Per_Frame,0,0,0,0,

      // RIGHT OUTPUT
      &stream_map_rcvout, LENGTH(stream_map_rcvout)/2,
      $frame.distribute_output_stream_rm, $frame.update_output_streams_rm,
      $M.CVC.Num_Samples_Per_Frame,0,0,0,0,

      0;                                     // END OF TABLE
// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio plc 2006             http://www.csr.com
//
// $Revision:$  $Date:$
//
// MODULE:
//          $M.CVC.data.copyHFKaggr2OMSaggr
//DESCRIPTION:
//      Copies the Noise reduction Aggressiveness value of HFK to Noise reduction
//      value to the OMS aggressivenss to maintain the same aggressiveness value in
//      low volume mode (actually a subset of HFK mode)
// INPUTS:
//      r7 - none
//      r8 - none
// OUTPUT:
//      None
// MODIFIED REGISTERS: r0
//
// *****************************************************************************
.ifdef BLD_PRIVATE
   .PRIVATE;
.endif

.if uses_SND_NS
.if uses_AEC
   .CODESEGMENT PM;

copyHFKaggr2OMSaggr:
   r0 = M[&aec_dm1 + $M.AEC_Headset.OFFSET_OMS_AGGRESSIVENESS];
   M[&oms270snd_obj + $M.oms270.AGRESSIVENESS_FIELD] = r0;
   rts;

.endif
.endif


// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio plc 2006             http://www.csr.com
//
// $Revision:$  $Date:$
//
// MODULE:
//          $M.CVC.data.copyInpBuffer2HarmBuffer
//
//DESCRIPTION:
//       Updates the Harmonicity buffer with the latest frame from the input buffer.
//
// INPUTS:
//      r8 - Pointer to the harmonicity buffer.
// OUTPUT:
//      None
// MODIFIED REGISTERS:
//
// *****************************************************************************

.if uses_RCV_NS || uses_SND_NS
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

// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio plc 2006             http://www.csr.com
//
// $Revision:$  $Date:$
//
// MODULE:
//      $M.CVC.data.set_fb_scale
//
//DESCRIPTION:
//      sets analysis and synthesis filterbank scaling
//
// INPUTS:
//      None
// OUTPUT:
//      None
// MODIFIED REGISTERS:
//      r0, r1
// *****************************************************************************

.if uses_AEQ
set_fb_scale:
   $push_rLink_macro;
   // Extra scaling for AEQ
   r0 = -1;
   r1 = 1;
   call $M.filter_bank.configuration.fft_extra_scaling.set.func;
   jump $pop_rLink_and_rts;
.endif
.ENDMODULE;

// *****************************************************************************
//
// MODULE:
//    $M.set_mode_gains.func
//
// DESCRIPTION:
//    Sets input gains (ADC and SCO) based on the current mode.
//    (Note: this function should only be called from within standy,
//    loopback, and pass-through modes).
//
//    MODE              ADC GAIN        SCO GAIN
//    pass-through      user specified  unity
//    standby           zero            zero
//    loopback          unity           unity
//
//
// INPUTS:
//    r7 - Pointer to the data structure
//
// *****************************************************************************

.MODULE $M.set_mode_gains;

    .CODESEGMENT PM;
    .DATASEGMENT DM;
    .VAR FunctionTable[] = &zero_gain,     // undefined state
                           &zero_gain,     // hfk mode
                           &zero_gain,     // asr mode
                           &passthru_gain, // pass-through mode
                           &zero_gain,     // undefined state
                           &loopback_gain, // loop-back mode
                           &zero_gain,     // standby mode
                           &zero_gain;     // low volume mode

   .VAR  zero_gain[] = 0,1,0,1;
   .VAR  passthru_gain[] = 0.5,1,0.5,1;
   .VAR  loopback_gain[] = 0.5,1,0.5,1;

func:

   // load ADC mantissa and exponent from user parameters
   r0 = M[$M.CVC.data.CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_PT_SNDGAIN_MANTISSA];
   r1 = M[$M.CVC.data.CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_PT_SNDGAIN_EXPONENT];
   M[passthru_gain] = r0;
   M[passthru_gain + 1] = r1;

   r0 = M[$M.CVC.data.CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_PT_RCVGAIN_MANTISSA];
   r1 = M[$M.CVC.data.CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_PT_RCVGAIN_EXPONENT];
   M[passthru_gain + 2] = r0;
   M[passthru_gain + 3] = r1;

   // Get Mode
   r0 = M[r7 + $M.SET_MODE_GAIN.MODE_PTR];
   r0 = M[r0];
   r1 = M[FunctionTable + r0];
   I0 = r1;
   r10 = 4;
   // Set Gains
   I4 = r7 + $M.SET_MODE_GAIN.ADC_MANT;
   do lp_copy;
      r0 = M[I0,1];
      M[I4,1] = r0;
lp_copy:
   rts;
.ENDMODULE;
