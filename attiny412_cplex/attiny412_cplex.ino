/*  This program implements charlieplexing of 12 LEDs
 *  using 4 signal pins.
 *
 *  License: BSD 2-Clause as per SPDX.
 */

/* what pins are used, defined by bit position in PORT
 * A registers.  Also color of wire I used on board.
 */

#define PIN0_YEL_BIT 0x40 /* PIN_PA6, yellow wire */
#define PIN1_WHI_BIT 0x80 /* PIN_PA7, white wire */
#define PIN2_BLU_BIT 0x02 /* PIN_PA1, blue wire */
#define PIN3_GRE_BIT 0x04 /* PIN_PA2, green wire */

/* Value to display on LEDs.  There are 12 LEDs so the
 * least significant 12 bits give the LED on or off
 * setting.  Main loop writes, PIT ISR reads.
 */

volatile uint16_t val_to_display = 0x0;

/* Cathodes are referenced in a loop */

const uint8_t   cathode_to_bit[] = {
  PIN0_YEL_BIT, // cathode 0
  PIN1_WHI_BIT, // cathode 1
  PIN2_BLU_BIT, // cathode 2
  PIN3_GRE_BIT  // cathode 3
};

#define NUM_CATHODES (sizeof(cathode_to_bit)/sizeof(cathode_to_bit[0]))

/* 12 LEDs are arranged in a line on the board.  The rightmost is
 * LED 0; the leftmost LED 11.  This table defines which anode
 * and cathode is connected to each LED. It is arranged as 4 rows
 * of cathodes.  The columns are anodes.  LEDs, anodes, and
 * cathodes are all defined by bit positions. Take the first
 * entry in the table as an example.  LED 3 is connected to
 * anode PIN1_WHI_BIT and cathode 0 (PIN0_YEL_BIT).
 */

const struct {
  uint16_t led_bit;
  uint8_t anode_bit;
} bit_table[NUM_CATHODES][NUM_CATHODES - 1] = {
  {{1<<3, PIN1_WHI_BIT}, {1<<6, PIN2_BLU_BIT}, {1<<9, PIN3_GRE_BIT}},   /* cathode 0 */
  {{1<<0, PIN0_YEL_BIT}, {1<<7, PIN2_BLU_BIT}, {1<<10, PIN3_GRE_BIT}},  /* cathode 1 */
  {{1<<1, PIN0_YEL_BIT}, {1<<4, PIN1_WHI_BIT}, {1<<11, PIN3_GRE_BIT}},  /* cathode 2 */
  {{1<<2, PIN0_YEL_BIT}, {1<<5, PIN1_WHI_BIT}, {1<<8, PIN2_BLU_BIT}}    /* cathode 3 */
};

/* All of the LED display is done within the PIT interrupt handler.
 * interrupt handler fires periodically.  Each invocation activates
 * the next cathode and lights all LEDs connected to that cathode.
 * They remain lit until the next invocation.  Since there are 4
 * cathodes, the lit duty cycle is 25%,
 */

ISR(RTC_PIT_vect)
{
  static uint8_t cathode = 0; /* Currently active cathode */
  uint8_t pin, anode_val = 0;
  uint16_t cur_val, prev_val;

  RTC.PITINTFLAGS = RTC_PI_bm; /* clear PIT interrupt flag */

  /* set all pins to inputs */
  PORTA.DIRCLR = PIN0_YEL_BIT | PIN1_WHI_BIT | PIN2_BLU_BIT | PIN3_GRE_BIT;

  /* main loop does not set val_to_display atomically so look to see that
   * its value is the same in two consecutive reads.  This assumes that
   * the main loop does not change it faster than that.
   */

  prev_val = val_to_display;
  while ((cur_val = prev_val) != val_to_display) prev_val = cur_val;
  
  for   (pin = 0; pin < NUM_CATHODES - 1; pin++) {
    if (cur_val & bit_table[cathode][pin].led_bit)
      anode_val |= bit_table[cathode][pin].anode_bit;
  }

  /* cathode pin to be output LOW */  
  PORTA.OUTCLR = cathode_to_bit[cathode]; 
  PORTA.DIRSET = cathode_to_bit[cathode];

  /* Needed anode pins to be output HIGH */
  PORTA.OUTSET = anode_val;
  PORTA.DIRSET = anode_val;

  cathode = cathode < NUM_CATHODES - 1 ? cathode + 1 : 0;
}

void setup(void)
{
  /* Set PIT in RTC to create an interrupt every ~2 ms */
  /* use internal 32768 Hz osc */ 
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
  /* enable PIT interrupt */
  RTC.PITINTCTRL = RTC_PI_bm;
  /* Select period of 64 cycles and enable the PIT */
  RTC.PITCTRLA = RTC_PERIOD_CYC64_gc | RTC_PITEN_bm;
}

void loop()
{
  static uint32_t next_millis = 0;
  uint32_t cur_millis = millis();

  if (cur_millis > next_millis) {
    /* time to update display */
    next_millis = cur_millis + 150;
    val_to_display = random(0, 4095);
  }
}
