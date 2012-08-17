// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio plc 2007             http://www.csr.com
// %%version
//
// $Revision$ $Date$
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//    This application uses the SCO framwork (sco_process library) to copy 
//    samples across from the SCO in buffer to the DAC buffer, and from the ADC
//    buffer to the SCO out buffer.  CVC (cvc_headset_b100 library) is called by 
//    the SCO framework to process the data before it is copied to the output
//    buffers.
//
// NOTES
//    CVC is comprised of the following processing modules:
//     - AEC (Acoustic Echo Canceller) applied to SCO output
//     - ALT (Automatic Level Tuner) applied to DAC output
//     - NS (Noise Suppression) applied to SCO output
//     - NDVC (Noise Dependent Volume Control) applied to DAC output
//     - SCO in parametric equalizer
//     - SCO out parametric equalizer
//     - SCO in and ADC in DC-Block filters
//
//    These modules can be included/excluded from the build using define the
//    define statements in cvc_headset_config.h
//
// *****************************************************************************

//.define CBOPS_STREAM_DEBUG // Define this if using cbops debug library.
                             // It must be defined before the following .include
                             // statements.

.include "core_library.h"
.include "cbops_library.h"
.include "frame_sync_library.h"
.include "frame_codec.h"
.include "plc100_library.h"

.include "cvc_headset_2mic_library.h"
.include "spi_comm_library.h"
.include "kalimba_messages.h"
.include "streams_rm.h"
.include "flash.h"
.include "cvc_headset_2mic_config.h" 
.include "cbuffer_macro.h"

// declare twiddle factors
.define  FFT_TWIDDLE_MAX_POINTS     128
.include "fft_twiddle.h"

   .CONST   $MESSAGE_SCO_CONFIG  0x2000;
   
   .CONST $SCO_ENCODING_CVSD        0;
   .CONST $SCO_ENCODING_AURISTREAM  1;

   // System defines
   // --------------
   .CONST $SAMPLE_RATE                 8000;
   .CONST $BLOCK_SIZE                  64;
    
   // 1ms is chosen as the interrupt rate for the audio input/output to minimise 
   // latency:
   .CONST $TIMER_PERIOD_AUDIO_COPY_MS  1;   
   .CONST $TIMER_PERIOD_AUDIO_COPY_SAMPLES      (($SAMPLE_RATE / 1000) * ($TIMER_PERIOD_AUDIO_COPY_MS));
   .CONST $TIMER_PERIOD_AUDIO_COPY_MICROS       ($TIMER_PERIOD_AUDIO_COPY_MS * 1000);
        
   .CONST $BUFFER_SCALING              3;
   .CONST $TONE_BUFFER_SIZE            ($BLOCK_SIZE + 1);
   .CONST $CVC_BUFFER_SIZE             $BLOCK_SIZE;
   // Latency is based on the processing time of the frame process
   .CONST $STREAM_LATENCY_VARIANCE              $TIMER_PERIOD_AUDIO_COPY_SAMPLES*5;   
   .CONST $MAX_DECODED_PACKET_SIZE_W    (55 * 4); //ev5,auri2,lat=30ms
  
   .CONST $SBC_COMPRESSION_RATIO 4; // SBC is 4/1
   .CONST $AURI4_COMPRESSION_RATIO 4; // auri4 is 4/1
   .CONST $AURI2_COMPRESSION_RATIO 8; // auri2 is 8/1
       
   .CONST  $SCO_IN_PORT          ($cbuffer.READ_PORT_MASK  +1);   
   .CONST  $SCO_OUT_PORT         ($cbuffer.WRITE_PORT_MASK +1);  
   .CONST  $ADC_PORT_L           ($cbuffer.READ_PORT_MASK  | $cbuffer.FORCE_PCM_AUDIO + 0);   
   .CONST  $ADC_PORT_R           ($cbuffer.READ_PORT_MASK  | $cbuffer.FORCE_PCM_AUDIO + 2);   
   .CONST  $DAC_PORT_L           ($cbuffer.WRITE_PORT_MASK | $cbuffer.FORCE_PCM_AUDIO +0);
   .CONST  $DAC_PORT_R           ($cbuffer.WRITE_PORT_MASK | $cbuffer.FORCE_PCM_AUDIO + 2);
   .CONST  $TONE_PORT            (($cbuffer.READ_PORT_MASK  | $cbuffer.FORCE_PCM_AUDIO) + 3); 
   
   
   // NOTE: When using macros ensure that no characters 
   // or white space follow the '\' line continuation token
   // This will cause the assembler error: 
   //    FATAL:    Illegal character '\'                                   

// INPUT stream
DeclareStereoAdcInputStreamWithSideTone($adc_in,                  /* Name */ \
                       $ADC_PORT_L,                               /* PORT*/ \
                       $ADC_PORT_R,                               /* PORT*/ \
                       $BLOCK_SIZE * 2,                           /* Buffer Size */ \
                       8,                                         /* Shift */ \
                       4*$TIMER_PERIOD_AUDIO_COPY_SAMPLES);       /*Side Tone Buffer Size */
                       
// OUTPUT streams
DeclareScoOutputStream($sco_out,                                  /* Name */ \
                        $SCO_OUT_PORT | $cbuffer.FORCE_PCM_AUDIO, /* PORT*/ \
                        $BLOCK_SIZE * $BUFFER_SCALING,            /* CBuffer Size */ \
                        -8,                                       /* Shift */ \
                        $TIMER_PERIOD_AUDIO_COPY_SAMPLES);

DeclareDualChannelDacOutputStreamWithSideTone($dac_out,            /* Name */ \
                       $DAC_PORT_L,                                /* Left Port */ \
                       $DAC_PORT_R,                                /* Right Port */ \
                       $BLOCK_SIZE * 2,                            /* Buffer Size */ \
                       -8,                                         /* Shift */ \
                       &$adc_in.sidetone_cbuffer_struc);           /* Side Tone Buffer */

.MODULE $sco_in;
    .DATASEGMENT DM;

    DeclareCBuffer(input.cbuffer_struc,mem_input,3*93);            // Big enough for 2 SCO packets with headers 
    DeclareCBuffer(output.cbuffer_struc,mem_output,3*90);          // Big enough for 2 SCO packets   
    
    .VAR/DM pcm_decoder[$sco_decoder.STRUC_SIZE] = 
            $sco_decoder.pcm.validate,        // VALIDATE_FUNC
            $sco_decoder.pcm.process,         // DECODE_FUNC
            $sco_decoder.pcm.initialize,      // RESET_FUNC
            0,                                // DATA_PTR
            0.3;                              // THRESHOLD (between 0 and 1)

    // declare all plc memory for testing plc module
   .VAR/DMCIRC buffer_speech[$plc100.SP_BUF_LEN_NB];
   .VAR/DM ola_win[$plc100.OLA_LEN_NB] = 
      0x066666,
      0x133333,
      0x200000,
      0x2CCCCC,
      0x399999,
      0x466666,
      0x533333,
      0x600000,
      0x6CCCCC,
      0x799999;
         
   .VAR/DM buffer_ola[$plc100.OLA_LEN_NB];

    .VAR/DM decoder_obj[$sco_pkt_handler.STRUC_SIZE ] = 
                   $SCO_IN_PORT | $cbuffer.FORCE_LITTLE_ENDIAN,    // SCO PORT (header)
                   $SCO_IN_PORT | $cbuffer.FORCE_PCM_AUDIO,        // SCO port (payload)
                   &input.cbuffer_struc,               // INPUT_PTR_FIELD
                   &output.cbuffer_struc,              // OUTPUT_PTR_FIELD
                   0x2000,                             // ENABLE_FIELD
                   0x0000,                             // CONFIG_FIELD
                   0,                                  // STAT_LIMIT_FIELD
                   0,                                  // PACKET_IN_LEN_FIELD
                   0,                                  // PACKET_OUT_LEN_FIELD
                   &pcm_decoder,                       // DECODER_PTR
                   $plc100.process,                    // PLC_PROCESS_PTR
                   $plc100.initialize,                 // PLC_RESET_PTR
                   0,                                  // BFI_FIELD
                   0,                                  // PACKET_LOSS_FIELD
                   0,                                  // INV_STAT_LIMIT_FIELD
                   0,                                  // PACKET_COUNT_FIELD
                   0,                                  // BAD_PACKET_COUNT_FIELD
                   // PLC Internal Data Initialization
                   &buffer_speech,                     // SPEECH_BUF_PTR_FIELD
                   $plc100.SP_BUF_LEN_NB,              // SPEECH_BUF_LEN_FIELD
                   &ola_win,                           // OLA_WIN_PTR_FIELD
                   &buffer_ola,                        // OLA_BUF_PTR_FIELD
                   $plc100.OLA_LEN_NB,                 // SPEECH_BUF_LEN_FIELD
                   $plc100.RANGE_NB,                   // RANGE_FIELD
                   $plc100.MIN_DELAY_NB,               // MIN_DELAY_FIELD
                   $plc100.MAX_DELAY_NB,               // MAX_DELAY_FIELD
                   $plc100.HARM_THRESHOLD,             // HARM_THRESH_FIELD
                   $plc100.PER_TC_INV_NB,              // PER_TC_INV_FIELD
                   0;                                  // PER_FIELD
.ENDMODULE;

   // amount of history needed for
   // up-sampler. Depends on #coeffs in 
   // filter. Current upsampler needs 33. Use 40 
   // to be safe
   .CONST $TONE_BUF_HIST         40;     
                                         
   // tone buffer size: 
   //    8k len=frm_sz+1   
   .CONST $TONE_BUF_SZ           $BLOCK_SIZE+1;
   

.MODULE $tone_in;
    .DATASEGMENT DM;
    
    DeclareCBuffer(cbuffer_struc,mem, ($TONE_BUF_SZ) );

   .VAR copy_struc[] =
      &copy_op,
      // number of inputs
      1,     
      $TONE_PORT,
      // number of outputs
      1,     
      &cbuffer_struc;

   .BLOCK copy_op;
      .VAR copy_op.next = $cbops.NO_MORE_OPERATORS;
      .VAR copy_op.func = &$cbops.shift;
      .VAR copy_op.param[$cbops.shift.STRUC_SIZE] =
         // Input index (TONE port)
         0,    
         // Output index (TONE cbuffer)
         1,    
         // SHIFT_AMOUNT_FIELD
         8;    
   .ENDBLOCK;
   
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.Main 
//
// DESCRIPTION:
//    This is the main routine.  It initializes the libraries and then loops 
//    "forever" in the processing loop.
//
// *****************************************************************************
.MODULE $M.main;
   .CODESEGMENT PM;
   .DATASEGMENT DM;
 

    // ** allocate memory for timer structures **
   .VAR $audio_copy_timer_struc[$timer.STRUC_SIZE];

   // ** Memory Allocation for CVC config structure
   .VAR  CVC_config_struct[$M.CVC.CONFIG.STRUC_SIZE] = 
      &$App.AuxilaryAudio.Tone,   // AUX_AUDIO_FUNC
      &$App.Codec.Apply,          // CODEC_CNTL_FUNC
      &$App.Config.Apply;         // RESET_FUNC

      // CBuffers Used by CVC
      .VAR $cvc_cbuffers[] = 
         // Input CBuffers
         // Set by router SCO IN
      &$sco_in.output.cbuffer_struc,
         &$adc_in.left.cbuffer_struc,
         &$adc_in.right.cbuffer_struc,
         // Output CBuffers
         // Set by router SCO OUT
         &$sco_out.cbuffer_struc,
         &$dac_out.cbuffer_struc;
         


$main:
   call $flash.init_pm;  //  (Needed if using PMFLASH segment)
   // initialise the stack library
   call $stack.initialise;
   // initialise the interrupt library
   call $interrupt.initialise;
   // initialise the message library
   call $message.initialise;
   // initialise the cbuffer library
   call $cbuffer.initialise;
   // initialise the pskey library
   call $pskey.initialise;
.ifdef PROFILER_ON
   // initialise the profiler library
   call $profiler.initialise;
.endif

   
   // set up message handler for SCO routing message
   r1 = &$M.audio_config.set_port_type_message_struc;
   r2 = $MESSAGE_SCO_CONFIG;
   r3 = $audio_config_handler;
   call $message.register_handler;
   
   // Initialize SCO packet size detection
   call $sco_port.monitor.Initialize;
   
   // intialize SPI communications library
   call $spi_comm.initialize;
   
   
   // initialize CVC library 
   r8 = &CVC_config_struct; 
   call $CVC.PowerUpReset;

   r1 = $SCO_ENCODING_CVSD;
   call $audio_config_handler;
   // send message saying we're up and running!
   r2 = $MESSAGE_KALIMBA_READY;
   call $message.send_short;
  
   // Activate CVC system
   call $CVC.Start;

   // start timer that copies audio samples
   r1 = &$audio_copy_timer_struc;
   r2 = $TIMER_PERIOD_AUDIO_COPY_MICROS;
   r3 = &$audio_copy_handler;
   call $timer.schedule_event_in;
   
   // Start a loop to filter the data
main_loop:
      // Check Communication
      call $spi_comm.polled_service_routine;
     
      // Run selected codecs
      r7 = &$sco_in.decoder_obj;
      call $frame.sco_decode;
            
      // Update Side Tone Filter
      // r0 = side tone filter function address
      call $CVC.get_sidetone_filter;
      M[$adc_in.sidetone_filter_ptr] = r0;

      // Update Side Tone Gain
      r0 = 0x7fffff;
      M[$dac_out.sidetone_gain] = r0;
     
      // to synchronize frame process to audio interrupt
      call $frame_sync.1ms_delay;
                  

      r0 = &$adc_in.left.cbuffer_struc;       
      call $cbuffer.calc_amount_data;
      NULL = r0 - $CVC_BUFFER_SIZE;      
      // call processing function if block-size worth of data/space available         
      if POS call $CVC.frame_process_all_ports_connected;
            
      r1 = M[$M.audio_config.encoding_config];
      r2 = M[&$sco_in.decoder_obj + $sco_pkt_handler.PACKET_IN_LEN_FIELD];
      r3 = -1;
      r0 = M[$M.audio_config.encoding_mode];
      if Z r2 = r2 LSHIFT r3;
      call $CVC.set_connection_status;
      
   jump main_loop;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.audio_config
//
// DESCRIPTION:
//    Set up audio routing for SCO data type
//
// INPUTS:
//    r1 - Encoding mode (0=cvsd,1=auristream)
//    r2 - Auristream mode (2='2-bit',4='4-bit', else invalid, but treated as 4)
//
// *****************************************************************************
.MODULE $M.audio_config;
   .CODESEGMENT PM;
   .DATASEGMENT DM;
   
   // For routing config message
   .VAR    set_port_type_message_struc[$message.STRUC_SIZE];  
   
   // Routing Control
   .VAR    encoding_mode = $SCO_ENCODING_CVSD;
   .VAR    encoding_config = 0;
   
$audio_config_handler:

   $push_rLink_macro;
   
   // Save encoding mode
   M[encoding_config] = r2;
   M[encoding_mode] = r1;

   // Initialize Codecs 
   r7 = &$sco_in.decoder_obj;
   call $frame.sco_initialize;

   jump $pop_rLink_and_rts;
   
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.audio_copy_handler 
//
// DESCRIPTION:
//    This routine copies data between firmware MMU buffers and dsp processing
//    buffers.  It is called every msec.
//
// *****************************************************************************
.MODULE $M.audio_copy_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

      
   // Operator list for ENCODED routing
   .VAR stream_table[] =
      // Function                      r7         r8
      $cbops.copy,                     0,    &$adc_in.copy_struc,
      $cbops.copy,                     0,    &$dac_out.copy_struc,      
      $frame.sco_port_handler,         0,    &$sco_in.decoder_obj,
      $cbops.copy,                     0,    &$sco_out.copy_struc,
      
            
      $cbops.copy,                     0,    &$tone_in.copy_struc,
      0;

$audio_copy_handler:
   $push_rLink_macro;
   // Run Operators for current routing
   r4 = &stream_table;
   call $frame.run_function_table;
   // Signal main loop to wake up
   M[$cbops.sync_flag] = Null;
   // Cludge for Encoders.  Set Connectivity state of SCO OUT
   r1 = M[$cbuffer.port_offset_addr + ($SCO_OUT_PORT & ($cbuffer.PORT_NUMBER_MASK + $cbuffer.WRITE_PORT_OFFSET))];
   M[&$sco_out.cbuffer_struc + $cbuffer.CONNECT_STATUS_FIELD]=r1;
   // post another timer event
   r1 = &$audio_copy_timer_struc;
   r2 = $TIMER_PERIOD_AUDIO_COPY_MICROS;
   r3 = &$audio_copy_handler;
   call $timer.schedule_event_in_period;
   jump $pop_rLink_and_rts;

.ENDMODULE; 



// *****************************************************************************
// MODULE:
//    $M.App.Codec.Apply 
//
// DESCRIPTION:
//    A pointer to this function is passed in via the CVC_data_obj data object.
//    CVC uses the pointer to call this funcion to set the ADC and DAC gains.
//    Most users will probably keep the function as is, sending the 
//    $M.CVC.VMMSG.CODEC message to the VM application.  However, this function
//    can be modified to set the gains directly using the BC5MM.
//
// INPUTS:
//    r3 - DAC Gain
//    r4 - ADC Gain
//
//    The DAC Gain is in a form suitable for the VM function CodecSetOutputGainNow.
//    The ADC Gain is in a form suitable for the VM function CodecSetInputGainNow.
//    However, before calling CodecSetInputGainNow the top bit of ADC must be 
//    checked.  If it is high CodecEnableMicInputGainA(1) must be called to turn
//    on the microphone preamp.  If it is low, CodecEnableMicInputGainA(0) must
//    be called to disable the preamp.  The high bit must then be cleared before
//    calling CodecSetInputGainNow.
//
//    This table describes the DAC and ADC values:
//
//    DAC Value   Gain     Analog Register   Digital
//    0xf         0 dB         
//
// OUTPUTS:
//    sends a CODEC message to the VM application 
//
// REGISTER RESTRICTIONS:
//          
// CPU USAGE:
//    cycles = 
//    CODE memory:    5 words
//    DATA memory:    0  words
//
// *****************************************************************************
.MODULE $M.App.Codec.Apply;
   .CODESEGMENT   PM;
   .DATASEGMENT   DM;

$App.Codec.Apply:
   $push_rLink_macro;   
   r2 = $M.CVC.VMMSG.CODEC;   
   call $message.send_short;
   jump $pop_rLink_and_rts;
.ENDMODULE; 

// *****************************************************************************
// MODULE:
//    M.App.AuxiliaryAudio.Tone 
//
// DESCRIPTION:
//    A pointer to this function is passed in via the CVC_data_obj data object.
//    CVC uses the pointer to call this funcion to retrieve auxiliary audio
//    data that it mixes with the SCO-Input stream.  In this example data is
//    taken from the TonePort, but the function can be modified to get data 
//    from any source.  When this example is used tones generated by the VM
//    application will be mixed with the SCO as long as the VM connects a tone
//    stream to port 3 of the kalimba.
//
//    For example:
//    stream_source = StreamAudioSource(gFixedTones[5])
//    StreamDisconnect(0, StreamKalimbaSink(3));
//    StreamConnectAndDispose(stream_source, StreamKalimbaSink(3));
//
//    Note that "gFixedtones" came from the stereo headset example application.
//
//    Here is a digram of where the mixing occurs within CVC_Headset:
//
//

//   DAC <-- + <--Gain of (1-SG) <-- + <----- ScoGain <---- 
//           ^                       ^                                 
//           |                       |                      
//           |                       |                       
//        SideTone                 AuxGain                 
//          Gain                     ^                      
//          = SG                     |                       
//                                   |                      
//                            Q5.19 Gain From             
//                           VM volume message               
//                                   ^                      
//                                   |                      
//                                   |                  
//                             Auxiliary Audio
//                                
//
// INPUTS:
//    r10 - number of samples to retrieve for CVC
//
// OUTPUTS:
//    I4 - ptr auxilary buffer of data
//    L4 - circular buffer length (set to zero for non-circular buffers)
//    r2 - pointer to a cBuffer data structure
//    r4 - AuxGain.  Ranges from 0 to 1.0.
//    r6 - ScoGain.  Ranges from 0 to 1.0.  This value is only applied
//         when the DSP sees auxiliary audio.

//
//    r4 - AuxGain  Q5.19
//    r6 - ScoGain  Q5.19
//
//
// REGISTER RESTRICTIONS:
//    r8,r9 is reserved
//          
// CPU USAGE:
//    cycles = 
//    CODE memory:    29 words
//    DATA memory:    0  words
// *****************************************************************************

.MODULE $M.App.AuxilaryAudio.Tone;
.ifdef BLD_FLASH_LVL1
   .CODESEGMENT PM_FLASH;
.else
   .CODESEGMENT PM;
.endif
   .DATASEGMENT   DM;
   
   // 0dB gain (Q5.19)
   .VAR  ScoGain0dB = 0x80000;  
   .VAR  AuxTimer = 0;
   .VAR AuxGain = 0;
   .VAR ScoGain = 0;
   
$App.AuxilaryAudio.Tone:
   $push_rLink_macro;   
   r6 = M[ScoGain];   
   r4 = M[AuxGain];   
    
   // Adjust SCO gain to Auxillary DAC level
   call $CVC.GetAuxDacAdj;
   r6 = r6 * r0 (frac);     
   // CBuffer
   r5 = &$tone_in.cbuffer_struc;       
   // Get stream data   
   r0 = r5;
   call $cbuffer.get_read_address_and_size;
   I4 = r0;
   L4 = r1;
   
   // Check amount of data.
   r0 = r5;
   call $cbuffer.calc_amount_data;
   r2 = r5;
   r10 = r10 - r0;      
   // there's at least a frame of aux data
   if LE jump lp_in_tone;           
      M0 = r0;
      // there is no data to process
      if Z jump lp_no_tone_proc;      
      
         // There's less than frame of aux data.  If the port is enabled, it must have
         // just been connected and we're just starting to aquire data.  If the port is
         // disabled, we must have just disconnected the port and we need to mix the 
         // remaining data.   
         r0 = $TONE_PORT;
         call $cbuffer.is_it_enabled;
         // port is enabled, but we'll mix later when
         // we have a frame of data.
         if NZ jump lp_start_of_tone;      
                                       
lp_end_of_tone:   
         // Last chunk of data so zero padd and advance write pointer
         // Save Pointer
         r3  = I4;                        
         // Advance past good data, r1=0  
         r1 = r1 XOR r1, r0 = M[I4,M0];    
         do lp_zero;
            // Zero Pad
            M[I4,1] = r1;                   
lp_zero:             
         r0 = r2;
         r1 = I4;
         // Update Write address past frame
         call $cbuffer.set_write_address; 
         // Restore Pointer
         I4 = r3;                         
         // r2 (non-zero) 
         jump $pop_rLink_and_rts;

lp_no_tone_proc:
      // Not enough data
      r0 = M[AuxTimer];
      r0 = r0 + 1;
      if NEG jump lp_start_of_tone;
      r0 = Null;
      r6 = M[ScoGain0dB];   
lp_start_of_tone:   
      M[AuxTimer] = r0;
      r2 = Null;
      // r2 (zero) 
      jump $pop_rLink_and_rts;
lp_in_tone:
   r0 = -12;
   M[AuxTimer] = r0;
   jump $pop_rLink_and_rts;
.ENDMODULE;
// *****************************************************************************
// MODULE:
//    $M.App.Config.Apply
//
// DESCRIPTION:
//    Retrieves Custom DSP Application Parameters.
//    cVc calls this function each time it resets.  Resets occur when the 
//    Parameter Manager is used to tune parameter values or when the VM
//    application sends a CVC_SET_MODE message to cVc.
//
// INPUTS:
//   - r0 = ptr to a sequential block in memory containing 10 24-bit locations
//   that can be used by the customer to store non-cVc parameters, which are
//   accessed as r0 + 0 through r0 + 9.  These values can be tuned in real time
//   using the Parameter Manager.  The Parameter Manager can also be used to
//   download the parameters to PS and / or to create a psr file that contains
//   the tuned values.
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// *****************************************************************************

.MODULE $M.App.Config.Apply;
   .CODESEGMENT   PM;
   .DATASEGMENT   DM;
   
$App.Config.Apply:   
   rts;
.ENDMODULE;
