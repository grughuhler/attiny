/*  This program demonstrates pin interrupts on an attiny 3224 
 *  microcontroller and counts interrupts per second.
 *  License: BSD 2-Clause as per SPDX.
 */

#define TwoWire_h
#include <Tiny4kOLED.h>

#define IRQ1_PIN PIN_PA6    // package pin 4
#define IRQ2_PIN PIN_PA7    // package pin 5
#define TOGGLE1_PIN PIN_PB3 // package pin 6
#define TOGGLE2_PIN PIN_PB2 // package pin 7

volatile uint32_t count1 = 0, count2 = 0;

void isr1_function(void)
{
  count1 += 1;

  // Directly writing port toggle register is ~ 1 usec faster @ 20 MHz.
  PORTB.OUTTGL = 0x8;  // Toggle pin PB3
}

void isr2_function(void)
{
  static bool state = false;

  count2 += 1;

  state = !state;
  digitalWrite(TOGGLE2_PIN, state);
}

void setup()
{
  oled.begin(128, 32, sizeof(tiny4koled_init_128x32br), tiny4koled_init_128x32br);
  oled.setFont(FONT8X16);
  oled.on();
  
  pinMode(IRQ1_PIN, INPUT);
  pinMode(IRQ2_PIN, INPUT);
  pinMode(TOGGLE1_PIN, OUTPUT);
  pinMode(TOGGLE2_PIN, OUTPUT);

  attachInterrupt(IRQ1_PIN, isr1_function, RISING);
  attachInterrupt(IRQ2_PIN, isr2_function, RISING);
  noInterrupts();
}

#define INTERVAL 1000

void loop()
{
  uint32_t next_millis;
  static uint32_t iteration = 0;

  count1 = 0;
  count2 = 0;
  interrupts();
  next_millis = millis() + INTERVAL;
  while (millis() < next_millis) {};
  noInterrupts();
  oled.clear();
  oled.print(count1/(INTERVAL/1000));
  oled.print(" : ");
  oled.println(count2/(INTERVAL/1000));
  iteration += 1;
  oled.println(iteration);
}
