
/* This program implements a night light.  An LED lights when a
 * phototransistor detects darkness.  A switch can override this
 * and cause the LED to be on all the time.  The switch is of
 * type single-pole, double-throw (SPDT) and is debounced by CCL
 * LUT0 acting as an SR latch.  This is pointless in this
 * application and is done only to show that it can be done.
 *
 * The point of this is to demonstrate how the attiny Event
 * System allows hardware blocks to interact without software
 * interaction after initialization.  This is even true when
 * the core is in the "standby" power state, sleeping.
 * 
 * The main loop causes the core/system to alternate between the
 * standby and awake states every four seconds. A second LED
 * lights while the system is awake. The night light operates
 * the same regardless of the standby/awake state because the
 * hardware blocks involved are condigured to remain awake.
 *
 * Hardware blocks that remain awake include: the Periodic Interval
 * Timer (PIT) which awakens the core, the Configurable Custom Logic
 * block (CCL LUTs 0 and 1), an Analog Comparator (AC0), and the Event
 * System (EvSys).
 *
 * This version is tested on an attiny 3224, a series 2 part and on
 * attiny 412, a series-1 part.  See #define for SERIES2 below.
 *
 * License: BSD 2-Clause as per SPDX.
 */
#include <avr/sleep.h>
#include <Logic.h>
#include <Event.h>
#include <Comparator.h>

/* Comment this out for attiny 412
 * or leave it in for attiny 3224 */
//#define SERIES2

#define SWITCH_S PIN_PA3 // EvSys map to LUT0 in0
#define SWITCH_R PIN_PA1 // direct to LUT0 in1

#define AWAKE_LED PIN_PA6 // Used by software
/* NGHT LED is PA2, Port A EVOUT */

volatile unsigned char pit_count = 0;

/* Does same thing as Logic::start() but also allows
 * RUNSTDBY to be set */
void logic_start(bool enable_logic, bool enable_runstby)
{
  if (enable_runstby)
    CCL.CTRLA = 0x40 | (enable_logic ? CCL_ENABLE_bm : 0);
  else
    CCL.CTRLA = (enable_logic ? CCL_ENABLE_bm : 0);
}

/* The purpose of this is to periodically awaken the core */
ISR(RTC_PIT_vect)
{
  RTC.PITINTFLAGS = RTC_PI_bm; /* clear PIT interrupt flag */
  pit_count += 1;
}

void setup()
{
  /* Set up to be able to sleep, go into standby mode */
  set_sleep_mode(SLEEP_MODE_STANDBY);
  sleep_enable();

  pinMode(AWAKE_LED, OUTPUT); // LED on when awake

  /* Event0 connects switch Set pin to LUT0.in0 because LUT0.in0 is
   * PA0 which is needed for UPDI. */
  pinMode(SWITCH_S, INPUT_PULLUP);
  Event0.set_generator(gen0::pin_pa3);
  Event0.set_user(user::ccl0_event_a);

  /* Event2 connects LUT0.out to LUT1.in0 because link from LUT0
   * goes to LUT3, and I am using LUT1 (also series 1 erratum) */
#ifdef SERIES2
  Event2.set_generator(gen::ccl0_out);
#else
  Event2.set_generator(gen::ccl_lut0);
#endif
  Event2.set_user(user::ccl1_event_a);

  /* Event3 connects LUT1.out to evout pin because LUT1.out is PA7
   * which is needed for ac0.inp so it cannot be enabled */
  Event3.set_generator(gen::ccl1_out);
  Event3.set_user(user::evouta_pin_pa2); 

#ifndef SERIES2
  /* Event4 connects LUT0.out to LUT0.in2 because it appears
   * that feedback does not work as needed, at least on the 412. */ 
  Event4.set_generator(gen::ccl_lut0);
  Event4.set_user(user::ccl0_event_b);
#endif

  /* Event5 connects AC0_out to LUT1.in1 */
  Event5.set_generator(gen::ac0_out);
  Event5.set_user(user::ccl1_event_b);

  /* Phototransistor feeds comparator to detect darkness */
  AC_t& AC_struct = Comparator0.getPeripheral();
  Comparator0.input_p = comparator::in_p::in0;          // pos input PA7.  See datasheet
#ifdef SERIES2
  Comparator0.input_n = comparator::in_n::dacref;       // neg pin to the DACREF voltage 
  Comparator0.reference = comparator::ref::vref_vdd;    // vref to VDD
  Comparator0.dacref = 29;                              // neg input: (dacref/256)*VREF
#else
  /* DAC unreliable on 412 for some reason */
  Comparator0.input_n = comparator::in_n::vref;         // neg pin to vref voltage
  Comparator0.reference = comparator::ref::vref_0v55;   // vref to 0.55V
#endif
  Comparator0.hysteresis = comparator::hyst::large;     // Use 50mV hysteresis
  Comparator0.output = comparator::out::disable_invert; // Pin in use for something else
  Comparator0.init();
  AC_struct.CTRLA |= AC_RUNSTDBY_bm;  // Keep running in stand-by power mode.
  Comparator0.start();

  /* LUT0 debounces the switch, switch output goes to LUT1 */
  Logic0.enable = true;
  Logic0.input0 = logic::in::event_a;       // SWITCH_S (set) 
  Logic0.input1 = logic::in::input_pullup;  // SWITCH_R (reset)
#ifdef SERIES2
  Logic0.input2 = logic::in::feedback;      // Feedback OK on 3224
#else
  Logic0.input2 = logic::in::event_b;       // wprkaround
#endif
  Logic0.truth = 0xd4;  // SR-latch by feedback, inputs active low
  Logic0.clocksource = logic::clocksource::clk_per;
#ifdef SERIES2
  Logic0.filter = logic::filter::disable;
#else
  /* 412 may not work in standby unless it uses the clock. I
   * think this is a hardware bug, but I do not see an erratum */
  Logic0.filter = logic::filter::synchronizer;
#endif
  Logic0.edgedetect = logic::edgedetect::disable;
  Logic0.sequencer = logic::sequencer::disable;
  Logic0.output = logic::out::disable;
  Logic0.init();

  /* LUT1 lights the LED when AC0 detects darkness OR the
   * switch is set.  Output to EVOUT pin to which LED attached */
  Logic1.enable = true;
  Logic1.input0 = logic::in::event_a;
  Logic1.input1 = logic::in::event_b;
  Logic1.input2 = logic::in::masked;
#ifdef SERIES2
  Logic1.truth = 0x1; // NOR (EVOUT inverted, why?)
#else
  Logic1.truth = 0xe; // OR (EVOUT not inverted)
#endif
  Logic1.clocksource = logic::clocksource::clk_per;
#ifdef SERIES2
  Logic1.filter = logic::filter::disable;
#else
  /* 412 may not work in standby unless it uses the clock. I
   * think this is a hardware bug, but I do not see an erratum */
  Logic1.filter = logic::filter::synchronizer;
#endif
  Logic1.edgedetect = logic::edgedetect::disable;
  Logic1.sequencer = logic::sequencer::disable;
  Logic1.output = logic::out::disable;
  Logic1.init();

  Event0.start();
  Event2.start();
  Event3.start();
#ifndef SERIES2
  Event4.start();
#endif
  Event5.start();

  //Logic::start() cannot set RUNSTDBY
#ifdef SERIES2
  /* Series 2 does not need RUNSTDBY to be set because its
   * configuration enables nothing that needs a clock */
  logic_start(true, false);
#else
  logic_start(true, true);
#endif

  /* enable PIT interrupt.  It will awaken CPU */
  RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; /* 1 KHz clock */
  RTC.PITINTCTRL = RTC_PI_bm;
  RTC.PITCTRLA = RTC_PERIOD_CYC4096_gc | RTC_PITEN_bm;
}

void loop()
{
  static unsigned char last_pit_count = 0;
  unsigned char cur_pit_count;
  static bool sleep_period = false;

  if ((cur_pit_count = pit_count) != last_pit_count) {
    last_pit_count = cur_pit_count;
    if (sleep_period) {
      digitalWrite(AWAKE_LED, LOW);
      sleep_cpu();
    } else {
      digitalWrite(AWAKE_LED, HIGH);
    }
    sleep_period = !sleep_period;
  }
}
