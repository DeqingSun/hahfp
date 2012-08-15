// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio Limited 2003-2009        http://www.csr.com
// Part of BlueLab 4.1.2-Release
//
// $Revision$  $Date$
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//    SBC Decoder/Encoder example
//
// *****************************************************************************

// 1.5ms is chosen as the interrupt rate for the audio input/output because:
// adc/dac mmu buffer is 256byte = 128samples
//                               - upto 8 sample fifo in voice interface
//                               = 120samples = 2.5ms @ 48KHz
// assume obsolute worst case jitter on interrupts = 1ms
// Hence choose 1.5ms between audio input/output interrupts
.define TMR_PERIOD_AUDIO_COPY         1500

.define TMR_PERIOD_TONE_COPY          8000

.define AUDIO_CBUFFER_SIZE            384

// includes
.include "core_library.h"
.include "cbops_library.h"


.MODULE $M.main;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $main:

   .CONST  $AUDIO_LEFT_OUT_PORT    ($cbuffer.WRITE_PORT_MASK + 0);
   .CONST  $AUDIO_RIGHT_OUT_PORT   ($cbuffer.WRITE_PORT_MASK + 1);
   .CONST  $AUDIO_LEFT_IN_PORT     ($cbuffer.READ_PORT_MASK + 0);
   .CONST  $AUDIO_RIGHT_IN_PORT    ($cbuffer.READ_PORT_MASK + 1);
   .CONST  $TONE_IN_PORT           ($cbuffer.READ_PORT_MASK + $cbuffer.FORCE_PCM_AUDIO + 3);


   // ** allocate memory for cbuffers **
   .VAR/DMCIRC $audio_in_left[AUDIO_CBUFFER_SIZE];
   .VAR/DMCIRC $audio_in_right[AUDIO_CBUFFER_SIZE];
   .VAR/DMCIRC $audio_out_left[AUDIO_CBUFFER_SIZE];
   .VAR/DMCIRC $audio_out_right[AUDIO_CBUFFER_SIZE];
   .VAR/DMCIRC $tone_in[AUDIO_CBUFFER_SIZE];


   // ** allocate memory for cbuffer structures **
   .VAR $audio_out_left_cbuffer_struc[$cbuffer.STRUC_SIZE] =
       LENGTH($audio_out_left),        // size
       &$audio_out_left,               // read pointer
       &$audio_out_left;               // write pointer
   .VAR $audio_out_right_cbuffer_struc[$cbuffer.STRUC_SIZE] =
       LENGTH($audio_out_right),       // size
       &$audio_out_right,              // read pointer
       &$audio_out_right;              // write pointer
   .VAR $audio_in_left_cbuffer_struc[$cbuffer.STRUC_SIZE] =
       LENGTH($audio_in_left),         // size
       &$audio_in_left,                // read pointer
       &$audio_in_left;                // write pointer
   .VAR $audio_in_right_cbuffer_struc[$cbuffer.STRUC_SIZE] =
       LENGTH($audio_in_right),        // size
       &$audio_in_right,               // read pointer
       &$audio_in_right;               // write pointer
   .VAR $tone_in_cbuffer_struc[$cbuffer.STRUC_SIZE] =
       LENGTH($tone_in),               // size
       &$tone_in,                      // read pointer
       &$tone_in;                      // write pointer


   // ** allocate memory for timer structures **
   .VAR $audio_in_timer_struc[$timer.STRUC_SIZE];
   .VAR $audio_out_timer_struc[$timer.STRUC_SIZE];
   .VAR $tone_in_timer_struc[$timer.STRUC_SIZE];

   // ** allocate memory for tone cbops copy routine **
   .VAR $tone_in_copy_struc[] =
      &$tone_copy_op,
      1,     // number of inputs
      $TONE_IN_PORT,
      1,     // number of outputs
      &$tone_in_cbuffer_struc;

   .BLOCK $tone_copy_op;
      .VAR $tone_copy_op.next = $cbops.NO_MORE_OPERATORS;
      .VAR $tone_copy_op.func = &$cbops.copy_op;
      .VAR $tone_copy_op.param[$cbops.copy_op.STRUC_SIZE] =
         0,                            // Input index
         1;                            // Output index
   .ENDBLOCK;


   // ** allocate memory for stereo audio in cbops copy routine **
   .VAR $stereo_in_copy_struc[] =
      &$audio_in_shift_op_left,
      2,                               // Number of inputs
      $AUDIO_LEFT_IN_PORT,             // Input
      $AUDIO_RIGHT_IN_PORT,            // Input
      2,                               // Number of outputs
      &$audio_out_left_cbuffer_struc,   // Output
      &$audio_out_right_cbuffer_struc;  // Output

   .BLOCK $audio_in_shift_op_left;
      .VAR audio_in_shift_op_left.next = &$audio_in_dc_remove_op_left;
      .VAR audio_in_shift_op_left.func = &$cbops.shift;
      .VAR audio_in_shift_op_left.param[$cbops.shift.STRUC_SIZE] =
         0,                            // Input index (left input port)
         2,                            // Output index (left cbuffer)
         8;                            // Shift amount
   .ENDBLOCK;

   .BLOCK $audio_in_dc_remove_op_left;
      .VAR audio_in_dc_remove_op_left.next = &$audio_in_noise_gate_op_left;
      .VAR audio_in_dc_remove_op_left.func = &$cbops.dc_remove;
      .VAR audio_in_dc_remove_op_left.param[$cbops.dc_remove.STRUC_SIZE] =
         2,                            // Input index (left cbuffer)
         2;                            // Output index (left cbuffer)
   .ENDBLOCK;

   .BLOCK $audio_in_noise_gate_op_left;
      .VAR audio_in_noise_gate_op_left.next = &$audio_in_shift_op_right;
      .VAR audio_in_noise_gate_op_left.func = &$cbops.noise_gate;
      .VAR audio_in_noise_gate_op_left.param[$cbops.noise_gate.STRUC_SIZE] =
         2,                            // Input index (left cbuffer)
         2;                            // Output index (left cbuffer)
   .ENDBLOCK;

   .BLOCK $audio_in_shift_op_right;
      .VAR audio_in_shift_op_right.next = &$audio_in_dc_remove_op_right;
      .VAR audio_in_shift_op_right.func = &$cbops.shift;
      .VAR audio_in_shift_op_right.param[$cbops.shift.STRUC_SIZE] =
         1,                            // Input index (right input port)
         3,                            // Output index (right cbuffer)
         8;                            // Shift amount
   .ENDBLOCK;

   .BLOCK $audio_in_dc_remove_op_right;
      .VAR audio_in_dc_remove_op_right.next = &$audio_in_noise_gate_op_right;
      .VAR audio_in_dc_remove_op_right.func = &$cbops.dc_remove;
      .VAR audio_in_dc_remove_op_right.param[$cbops.dc_remove.STRUC_SIZE] =
         3,                            // Input index (right cbuffer)
         3;                            // Output index (right cbuffer)
   .ENDBLOCK;

   .BLOCK $audio_in_noise_gate_op_right;
      .VAR audio_in_noise_gate_op_right.next = $cbops.NO_MORE_OPERATORS;
      .VAR audio_in_noise_gate_op_right.func = &$cbops.noise_gate;
      .VAR audio_in_noise_gate_op_right.param[$cbops.noise_gate.STRUC_SIZE] =
         3,                            // Input index (right cbuffer)
         3;                            // Output index (right cbuffer)
   .ENDBLOCK;


   // ** allocate memory for stereo audio out cbops copy routine **
   .VAR $stereo_out_copy_struc[] =
      &$audio_out_upsample_mix_op,     // First operator block
      2,                               // Number of inputs
      &$audio_out_left_cbuffer_struc,  // Input
      &$audio_out_right_cbuffer_struc, // Input
      2,                               // Number of outputs
      $AUDIO_LEFT_OUT_PORT,            // Output
      $AUDIO_RIGHT_OUT_PORT;           // Output

   .VAR/DMCIRC $audio_out_upsample_mix.local_buffer[$cbops.upsample_mix.RESAMPLE_BUFFER_LENGTH_HIGH_QUALITY];
   .VAR/DM1CIRC audio_out_warp_and_shift_op.left_buf[$cbops.warp_and_shift.high_quality_filter_data_size];
   .VAR/DM1CIRC audio_out_warp_and_shift_op.right_buf[$cbops.warp_and_shift.high_quality_filter_data_size];

   .BLOCK $audio_out_upsample_mix_op;
      .VAR audio_out_upsample_mix_op.next = &$audio_out_dc_remove_op_left;
      .VAR audio_out_upsample_mix_op.func = &$cbops.upsample_mix;
      .VAR audio_out_upsample_mix_op.stereo[$cbops.upsample_mix.STRUC_SIZE] =
         0,                            // Input index (left)
         1,                            // Input index (right)
         &$tone_in_cbuffer_struc,      // Tone source
         1.0,                          // Tone vol
         0.1,                          // Audio vol
                                       // Resample coefs addr
         &$cbops.upsample_mix.resample_filter_coefs_x6_high_quality,
                                       // Resample coefs size
         LENGTH($cbops.upsample_mix.resample_filter_coefs_x6_high_quality),
                                       // Resample buffer addr
         &$audio_out_upsample_mix.local_buffer,
                                       // Resample buffer size
         LENGTH($audio_out_upsample_mix.local_buffer),
         (16000 / 8000),               // Upsample ratio
         ((16000.0/16000.0)-1);        // Interp ratio
   .ENDBLOCK;

   .BLOCK $audio_out_dc_remove_op_left;
      .VAR audio_out_dc_remove_op_left.next = &$audio_out_shift_op_left;
      .VAR audio_out_dc_remove_op_left.func = &$cbops.dc_remove;
      .VAR audio_out_dc_remove_op_left.param[$cbops.dc_remove.STRUC_SIZE] =
         0,                            // Input index (left cbuffer)
         0;                            // Output index (left cbuffer)
   .ENDBLOCK;

   .BLOCK $audio_out_shift_op_left;
      .VAR audio_out_shift_op_left.next = &$audio_out_dc_remove_op_right;
      .VAR audio_out_shift_op_left.func = &$cbops.shift;
      .VAR audio_out_shift_op_left.param[$cbops.shift.STRUC_SIZE] =
         0,                            // Input index (left cbuffer)
         2,                            // Output index (left output port)
         -8;                           // Shift amount
   .ENDBLOCK;

   .BLOCK $audio_out_dc_remove_op_right;
      .VAR audio_out_dc_remove_op_right.next = &$audio_out_shift_op_right;
      .VAR audio_out_dc_remove_op_right.func = &$cbops.dc_remove;
      .VAR audio_out_dc_remove_op_right.param[$cbops.dc_remove.STRUC_SIZE] =
         1,                            // Input index (right cbuffer)
         1;                            // Output index (right cbuffer)
   .ENDBLOCK;
   
   .BLOCK $audio_out_shift_op_right;
      .VAR audio_out_shift_op_right.next = $cbops.NO_MORE_OPERATORS;
      .VAR audio_out_shift_op_right.func = &$cbops.shift;
      .VAR audio_out_shift_op_right.param[$cbops.shift.STRUC_SIZE] =
         1,                            // Input index (right cbuffer)
         3,                            // Output index (right output port)
         -8;                           // Shift amount
   .ENDBLOCK;

   // initialise the stack library
   call $stack.initialise;
   // initialise the interrupt library
   call $interrupt.initialise;
   // initialise the message library
   call $message.initialise;
   // initialise the cbuffer library
   call $cbuffer.initialise;
   .ifdef DEBUG_ON
      // initialise the profiler library
      call $profiler.initialise;
   .endif
   // tell vm we're ready and wait for the go message
   call $message.send_ready_wait_for_go;


   // left and right audio channels from the mmu have been synced to each other
   // by the vm app but are free running in that the dsp doesn't tell them to
   // start.  We need to make sure that our copying between the cbuffers and
   // the mmu buffers starts off in sync with respect to left and right
   // channels.  To do this we make sure that when we start the copying timers
   // that there is no chance of a buffer wrap around occuring within the timer
   // period.  The easiest way to do this is to start the timers just after a
   // buffer wrap around occurs.
   // wait for ADC buffers to have just wrapped around
   wait_for_adc_buffer_wraparound:
      r0 = $AUDIO_LEFT_IN_PORT;
      call $cbuffer.calc_amount_data;
      // if the amount of data in the buffer is less than 16 samples then a
      // buffer wrap around must have just ocurred.
      Null = r0 - 16;
   if POS jump wait_for_adc_buffer_wraparound;


   // start timer that copies input samples
   r1 = &$audio_in_timer_struc;
   r2 = TMR_PERIOD_AUDIO_COPY;
   r3 = &$audio_in_copy_handler;
   call $timer.schedule_event_in;


   // start timer that copies tone samples
   r1 = &$tone_in_timer_struc;
   r2 = TMR_PERIOD_TONE_COPY;
   r3 = &$tone_in_copy_handler;
   call $timer.schedule_event_in;


   // wait for DAC buffers to have just wrapped around
   wait_for_dac_buffer_wraparound:
      r0 = $AUDIO_LEFT_OUT_PORT;
      call $cbuffer.calc_amount_space;
      // if the amount of space in the buffer is less than 16 bytes then a
      // buffer wrap around must have just ocurred.
      Null = r0 - 16;
   if POS jump wait_for_dac_buffer_wraparound;


   // start timer that copies output samples
   r1 = &$audio_out_timer_struc;
   r2 = TMR_PERIOD_AUDIO_COPY;
   r3 = &$audio_out_copy_handler;
   call $timer.schedule_event_in;

   // continually encode and decode codec frames
   frame_loop:
      
	  call $timer.1ms_delay;

   jump frame_loop;

.ENDMODULE;





// *****************************************************************************
// MODULE:
//    $audio_in_copy_handler
//
// DESCRIPTION:
//    Function to copy the stereo samples from the input port on a timer
//    interrupt.
//
// *****************************************************************************
.MODULE $M.audio_in_copy_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $audio_in_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

   // copy stereo data to the mmu port and do optional processing
   r8 = &$stereo_in_copy_struc;
   call $cbops.copy;

   // post another timer event
   r1 = &$audio_in_timer_struc;
   r2 = TMR_PERIOD_AUDIO_COPY;
   r3 = &$audio_in_copy_handler;
   call $timer.schedule_event_in_period;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;





// *****************************************************************************
// MODULE:
//    $audio_out_copy_handler
//
// DESCRIPTION:
//    Function to copy the stereo samples to the output port on a timer
//    interrupt.
//
// *****************************************************************************
.MODULE $M.audio_out_copy_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $audio_out_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

   // copy stereo data to the mmu port and do optional processing
   r8 = &$stereo_out_copy_struc;
   call $cbops.copy;

   // post another timer event
   r1 = &$audio_out_timer_struc;
   r2 = TMR_PERIOD_AUDIO_COPY;
   r3 = &$audio_out_copy_handler;
   call $timer.schedule_event_in_period;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;






// *****************************************************************************
// MODULE:
//    $tone_in_copy_handler
//
// DESCRIPTION:
//    Function to copy the tone samples from the input port on a timer
//    interrupt.
//
// *****************************************************************************
.MODULE $M.tone_in_copy_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tone_in_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

   // copy tone samples from the port to a cbuffer for mixing
   r8 = &$tone_in_copy_struc;
   call $cbops.copy;

   // post another timer event
   r1 = &$tone_in_timer_struc;
   r2 = TMR_PERIOD_TONE_COPY;
   r3 = &$tone_in_copy_handler;
   call $timer.schedule_event_in_period;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
