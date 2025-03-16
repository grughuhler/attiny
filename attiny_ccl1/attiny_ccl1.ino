/*  This program demonstrates the configurable custom logic
 *  block on an attiny 3226.  This is a 20-pin series 2
 *  device chosen because it has relatively many CCL signals
 *  pinned out.
 *  
 *  Demonstrates:
 *  
 *    - LUT2: 3-input AND OR pulse detection.
 *    - LUT2/LUT3: Sequencer acts as SR latch
 *    - LUT2 acts as SR latch using feedback instead of synchronizer.
 *    - LUT2 filter/sync and edge detect demo
 *
 *  License: BSD 2-Clause as per SPDX.
 */

#include <Logic.h> /* part of megatinycore */

void and3(void)
{
  Logic2.enable = true;
  Logic2.input0 = logic::in::input_pullup; // in0 on PB0
  Logic2.input1 = logic::in::input_pullup; // in1 on PB1
  Logic2.input2 = logic::in::input_pullup; // in2 on PB2
  Logic2.truth = 0x80;                     // 3-input AND
  Logic2.filter = logic::filter::disable;
  Logic2.edgedetect = logic::edgedetect::disable;
  Logic2.sequencer = logic::sequencer::disable;
  Logic2.output = logic::out::enable;      // Output on PB3

  Logic2.init(); // Apply above settings and also call start later.
}


void sr_latch_sequencer(void)
{
  Logic2.enable = true;
  Logic2.input0 = logic::in::input_pullup; // PB0 is S input
  Logic2.input1 = logic::in::masked;
  Logic2.input2 = logic::in::masked;
  Logic2.truth = 0x2;                      // out = in
  Logic2.filter = logic::filter::disable;
  Logic2.edgedetect = logic::edgedetect::disable;
  Logic2.sequencer = logic::sequencer::rs_latch;
  Logic2.output = logic::out::enable;      // Output on PB3

  Logic3.enable = true;
  Logic3.input0 = logic::in::input_pullup; // PC0 is R input
  Logic3.input1 = logic::in::masked;
  Logic3.input2 = logic::in::masked;
  Logic3.truth = 0x2;                      // out = in
  Logic3.filter = logic::filter::disable;
  Logic3.edgedetect = logic::edgedetect::disable;
  Logic3.output = logic::out::disable;    // out not enabled; lut2 out used

  Logic2.init(); // Apply above settings and also call start later.
  Logic3.init();  
}

void sr_latch_feedback(void)
{
  Logic2.enable = true;
  Logic2.input0 = logic::in::input_pullup; // PB0 is S
  Logic2.input1 = logic::in::input_pullup; // PB1 is R
  Logic2.input2 = logic::in::feedback;
  Logic2.truth = 0xb2;
  Logic2.filter = logic::filter::disable;
  Logic2.edgedetect = logic::edgedetect::disable;
  Logic2.sequencer = logic::sequencer::disable;
  Logic2.output = logic::out::enable;      // Output on PB3

  Logic2.init(); // Apply above settings and also call start later.
}

/* This function is described in a second video */

void filter_edge_demo(void)
{
  Logic2.enable = true;
  Logic2.clocksource = logic::clocksource::in2; // clock in from PB2
  Logic2.input0 = logic::in::input_pullup;      // in0 on PB0
  Logic2.input1 = logic::in::masked;
  Logic2.input2 = logic::in::input;             // in2 on PB2 (for clk)
  Logic2.truth = 0x2;                           // out = in0
  Logic2.filter = logic::filter::disable;
  Logic2.edgedetect = logic::edgedetect::disable;
  Logic2.sequencer = logic::sequencer::disable;
  Logic2.output = logic::out::enable;           // Output on PB3

  Logic2.init(); // Apply above settings and also call start later.
}

void setup()
{
  pinMode(PIN_PA7, OUTPUT);

  /* uncomment one of these */
  and3();
  //sr_latch_sequencer();
  //sr_latch_feedback();
  //filter_edge_demo();
  
  Logic::start();
}

void loop()
{
  uint8_t v;

  /* I am not sure that digitalRead of an output is portable in
   *  Arduino, so read port directly to emphasize that.  It shows
   *  how software can sample the CCL OUT state.
   */
  v = PORTB.IN & 0x8; // non-zero when PB3 is high
  digitalWrite(PIN_PA7, v);
}
