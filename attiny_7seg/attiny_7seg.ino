/* This program demonstrates an AtTiny412 using Arduino via megatinycore.
 * It shows:
 *  - Analog input (analog to digital conversion) via reading volts from a
 *    potentiometer,
 *  - Analog output (pulse width modulation) via changing the brightness of
 *    an LED.
 *  - Time from the millis() function.
 *  - Integration with a standard library, TM1637 (7 segment LED display)
 *  - Printing via a serial to USB comverter.
 *
 *  License: BSD 2-Clause as per SPDX.
 */

#include <Arduino.h>
#include <TM1637Display.h>

#define DO_PRINTS

#define CLK_PIN 1 // package pin 3 on AtTiny 412
#define DIO_PIN 2 // package pin 4
#define LED_PIN 3 // package pin 5
#define POT_PIN 4 // package pin 7
// Serial TX is always on package pin 2.

TM1637Display display(CLK_PIN, DIO_PIN);

void setup()
{
#ifdef DO_PRINTS
  Serial.begin(115200); // Serial baud = 115200
  delay(500);
  Serial.println("\n\nattiny demonstration starting...");
#endif

  pinMode(LED_PIN, OUTPUT);
  display.setBrightness(0x0); 
}

void loop()
{
  int pot_val;
  static int led_val = 200;
#ifdef DO_PRINTS
  static int print_count = 0;
#endif
  static unsigned long next_millis = 0;
  unsigned long cur_millis;

  /* Read value from POT 0-1023 and show it on the 7 segement LED display
   * each time in loop */
  pot_val = analogRead(POT_PIN);
  pot_val = ((pot_val + 5)/10)*10;
  display.showNumberDec(pot_val, false);

  /* Use POT value interpreted as milliseconds to control how often the LED
   * changes brightness and the POT value is printed. */
  cur_millis = millis();
  if (cur_millis >= next_millis) {
    next_millis = cur_millis + pot_val;
    led_val = 255 -led_val;
    analogWrite(LED_PIN, led_val);
#ifdef DO_PRINTS    
    Serial.print(print_count++);
    Serial.print(": POT val is ");
    Serial.println(pot_val);
#endif
  }
}
