/* This program implements charlieplexing of 20 LEDs using 5 signal pins
 * while also acting as an i2c slave device.
 *
 * An i2c master tells it what value to display on the LEDs.  The master
 * sees registers that this i2c slave implements.
 *
 *   reg 0: least significant byte of value to display on LEDs
 *   reg 1: middle byte of value to display
 *   reg 2: most signifcant byte of value to display
 *   reg 3: value does not matter, but....
 *
 * A write to reg 3 triggers the value in reg2, reg1, reg 0 to be
 * displayed on the 20 LEDs.
 *
 * i2c transactions other than byte reads/writes are illegal.
 *
 * License: BSD 2-Clause as per SPDX.
 */

 #include <Wire.h> // For i2c

 #define SLAVE_I2C_ADDR 0x54

/* On many 14-pin series 1 and 2 attiny microcontrollers
 *   PB0 is i2c SCL
 *   PB1 is i2c SDA
 * but check your datasheet.  Also, don't forget that i2c requires
 * pull-up resistors on SCL and SDA.  Your master device may
 * already have them.  Check this.
 */

/* Define pins used for charlieplexing, defined by bit position
 * in PORT A registers.
 */

#define LED_PIN0 0x08 // PA3
#define LED_PIN1 0x04 // PA2
#define LED_PIN2 0x02 // PA1
#define LED_PIN3 0x40 // PA6
#define LED_PIN4 0x80 // PA7

#define ALL_PINS (LED_PIN0 | LED_PIN1 | LED_PIN2 | LED_PIN3 | LED_PIN4)

/* Value to display on LEDs.  There are 20 LEDs so the
 * least significant 20 bits give the LED on or off
 * setting. 
 */

volatile uint32_t val_to_display = 0x0;

/* Cathodes are referenced in a loop */

const uint8_t   cathode_to_bit[] = {
  LED_PIN0, // cathode 0
  LED_PIN1, // cathode 1
  LED_PIN2, // cathode 2
  LED_PIN3, // cathode 3
  LED_PIN4  // cathode 4
};

#define NUM_CATHODES (sizeof(cathode_to_bit)/sizeof(cathode_to_bit[0]))

/* 20 LEDs come from two 10-LED bars.  LED0 is the leftmost of the
 * top bar.  LED 10 is the leftmost of the bottom bar.
 */

const struct {
  uint32_t led_bit;
  uint8_t anode_bit;
} bit_table[NUM_CATHODES][NUM_CATHODES - 1] = {
  {{1L<<0, LED_PIN1}, {1L<<1, LED_PIN2}, {1L<<2, LED_PIN3}, {1L<<3, LED_PIN4}},     /* cathode 0 */
  {{1L<<4, LED_PIN4}, {1L<<5, LED_PIN3}, {1L<<6, LED_PIN2}, {1L<<7, LED_PIN0}},     /* cathode 1 */
  {{1L<<8, LED_PIN0}, {1L<<9, LED_PIN3}, {1L<<10, LED_PIN1}, {1L<<11, LED_PIN4}},   /* cathode 2 */
  {{1L<<12, LED_PIN4}, {1L<<13, LED_PIN2}, {1L<<14, LED_PIN1}, {1L<<15, LED_PIN0}}, /* cathode 3 */
  {{1L<<16, LED_PIN0}, {1L<<17, LED_PIN1}, {1L<<18, LED_PIN2}, {1L<<19, LED_PIN3}}  /* cathode 4 */
};

/* All of the LED display is done within the PIT interrupt handler.
 * interrupt handler fires periodically.  Each invocation activates
 * the next cathode and lights all LEDs connected to that cathode.
 * They remain lit until the next invocation.  Since there are 5
 * cathodes, the lit duty cycle is 20%,
 */

ISR(RTC_PIT_vect)
{
  static uint8_t cathode = 0; /* Currently active cathode */
  uint8_t pin, anode_val = 0;
  uint32_t cur_val, prev_val;

  RTC.PITINTFLAGS = RTC_PI_bm; /* clear PIT interrupt flag */

  /* set all pins to inputs */
  PORTA.DIRCLR = ALL_PINS;

  /* writer does not set val_to_display atomically so look to see that
   * its value is the same in two consecutive reads.  This assumes that
   * the writer does not change it faster than that.
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

/* The i2c registers */
union {
 uint8_t bytes[4];
 uint32_t val;
} registers;

uint8_t cur_reg = 0;

/* handle_write is invoked when the i2c master does a write */

void handle_write(int16_t num_bytes)
{
  int16_t i;
  uint8_t v;

  /* What is received:
   *   A byte write from the master will be 2 bytes:
   *     1. Register address
   *     2. Value
   *  A byte read from the master will be one byte:
   *     1.  Register address.
   *  The read will then trigger a call to handle_read
   *  which should do a write of the value in register
   *  cur_reg.
   */

  switch (num_bytes) {
    case 1: /* A read */
      cur_reg = Wire.read();
      break;
    case 2: /* A write */
      cur_reg = Wire.read();
      v = Wire.read();
      if (cur_reg < 4) registers.bytes[cur_reg] = v;
      if (cur_reg == 3) val_to_display = registers.val;
      break; 
    default: /* unsupported */
      /* consume bytes of unexpected length master access */
      for (i = 0; i < num_bytes; i++) (void) Wire.read();
      break;
  }
}

/* handle_read is invoked when the i2c master does a read */

void handle_read(void)
{
  uint8_t v;

  v = (cur_reg < 4) ? registers.bytes[cur_reg] : 0xff;
  Wire.write(v);
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

  Wire.onReceive(handle_write);  /* Handle i2c write from master */
  Wire.onRequest(handle_read);   /* Handle i2c read from master */
  Wire.begin(SLAVE_I2C_ADDR);
}

void loop()
{
  /* nothing to do */
}
