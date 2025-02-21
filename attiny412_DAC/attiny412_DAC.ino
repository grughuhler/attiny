/*  This program demonstrates the digital to analog conversion
 *  hardware (DAC) and digitalREAD on the attiny412.
 *  
 *  If you don't have a TM1627 7-segment LED display, you can
 *  delete that part of the program.
 *  
 *  License: BSD 2-Clause as per SPDX.
 */

#include <TM1637Display.h>

#define POT_PIN PIN_PA3     // package pin 7 on AtTiny 412
#define SW_PIN  PIN_PA2     // package pin 5
#define CLK_PIN PIN_PA7     // package pin 3
#define DIO_PIN PIN_PA1     // package pin 4
#define DAC_OUT_PIN PIN_PA6 // It must be package pin 2

TM1637Display display(CLK_PIN, DIO_PIN);

/* DAC Reference options:
 *  INTERNAL0V55, INTERNAL1V1, INTERNAL1V5, INTERNAL2V5,
 *  INTERNAL4V3.  analogWrite of 255 on the DAC_OUT pin
 *  will produce the voltage of the option.
 */

void setup()
{
  DACReference(INTERNAL4V3);
  display.setBrightness(0x0);
  pinMode(SW_PIN, INPUT_PULLUP);
}

void loop()
{
  static unsigned long int next_millis = 0;
  unsigned long cur_millis;
  static unsigned int v = 0;

  if (digitalRead(SW_PIN)) {
    v = analogRead(POT_PIN);
    v = map(v, 0, 1023, 0, 255);

    cur_millis = millis();
    if (cur_millis >= next_millis) {
      next_millis = cur_millis + 50;
      display.showNumberDec(v, false);
    }
  } else {
    v = (v < 255) ? v + 1 : 0;
  }
  analogWrite(PIN_PA6, v);
}
