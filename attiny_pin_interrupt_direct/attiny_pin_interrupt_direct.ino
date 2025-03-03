/*  This program demonstrates pin interrupts on an attiny 3224 
 *  microcontroller and counts interrupts per second.  It uses
 *  direct interrupts rather than standard Arduino attachInterrupt.
 *  License: BSD 2-Clause as per SPDX.
 */

#define TwoWire_h
#include <Tiny4kOLED.h>

#define IRQ1_PIN PIN_PA6    // package pin 4
#define IRQ2_PIN PIN_PA7    // package pin 5
#define TOGGLE1_PIN PIN_PB3 // package pin 6
#define TOGGLE2_PIN PIN_PB2 // package pin 7

volatile uint32_t count1 = 0, count2 = 0;

ISR(PORTA_PORT_vect)
{
  uint8_t flags = PORTA.INTFLAGS;

  if (flags & 0x40) {
    PORTB.OUTTGL = 0x8; // PB3
    count1 += 1;
    PORTA.INTFLAGS = 0x40; // Clear PA6
  }
  
  if (flags & 0x80) {
    PORTB.OUTTGL = 0x4; // PB2
    count2 += 1;
    PORTA.INTFLAGS = 0x80; // Clear PA7
  }

}

void setup()
{
  oled.begin(128, 32, sizeof(tiny4koled_init_128x32br),
             tiny4koled_init_128x32br);
  oled.setFont(FONT8X16);
  oled.on();
  
  pinMode(IRQ1_PIN, INPUT_PULLUP);
  pinMode(IRQ2_PIN, INPUT_PULLUP);
  pinMode(TOGGLE1_PIN, OUTPUT);
  pinMode(TOGGLE2_PIN, OUTPUT);

  /* disable interrupts */
  PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.INTFLAGS = 0xc0;
}

#define INTERVAL 1000

void loop()
{
  uint32_t next_millis;
  static uint32_t iteration = 0;

  count1 = 0;
  count2 = 0;
  /* enable interrupts */
  PORTA.PIN6CTRL = PORT_ISC_RISING_gc;
  PORTA.PIN7CTRL = PORT_ISC_RISING_gc;
  
  next_millis = millis() + INTERVAL;
  while (millis() < next_millis) {};

  /* disable interrupts */
  PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.INTFLAGS = 0xc0;
  
  oled.clear();
  oled.print(count1/(INTERVAL/1000));
  oled.print(" : ");
  oled.println(count2/(INTERVAL/1000));
  iteration += 1;
  oled.println(iteration);
}
