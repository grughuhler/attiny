
/*  This program demonstrates using an AC comparator on
 *  ATtiny (tested on ATtiny 1614).  It registers an
 *  interrupt handler that fires on the rising edge of
 *  the AC's OUT signal and displays a count of interrupts
 *  per second on a 7-segment LED display.
 *  
 *  License: BSD 2-Clause as per SPDX.
 */

/* Comparator library is included in megatinycore */
#include <Comparator.h>

/* Library LedControl for MAX7219 7-segment LED display
 * (uses SPI).  Install via Arduino library manager.
 */
#include "LedControl.h"

/* LED display module SPI pins */
#define SPI_SCK PIN_PA3
#define SPI_MOSI PIN_PA1
#define SPI_SS PIN_PA4

LedControl lc=LedControl(SPI_MOSI, SPI_SCK, SPI_SS, 1);


void show_udec(uint32_t v)
{
  uint8_t i;
  
  lc.clearDisplay(0);

  for (i = 0; i < 8; i++) {
    lc.setDigit(0, i, v % 10, false);
    v = v / 10;
  }
}

volatile uint8_t int_count = 0;

void ac_interrupt(void)
{
  int_count += 1;
}

void setup()
{  
  lc.shutdown(0, false); /* Must do this first */
  lc.setIntensity(0, 0); /* 0 to 15 */
  lc.clearDisplay(0);

  Comparator1.input_p = comparator::in_p::in0;       // pos input PA7.  See datasheet
  Comparator1.input_n = comparator::in_n::dacref;    // neg pin to the DACREF voltage
  Comparator1.reference = comparator::ref::vref_4v3; // Set the DACREF voltage
  Comparator1.dacref = 127;                          // (dacref/256)*VREF

  Comparator1.hysteresis = comparator::hyst::large;  // Use 50mV hysteresis
  Comparator1.output = comparator::out::enable;      // Enable output PB3

  Comparator1.init();
  Comparator1.attachInterrupt(ac_interrupt, RISING);
  Comparator1.start();
  
}

#define INTERVAL 1000

void loop()
{
  uint32_t edge_count;
  uint8_t old_int_count;
  uint8_t cur_int_count;
  uint32_t next_millis;
  uint16_t minv = 1024, maxv = 0, meanv, v;
  
  /* Measure edges for one interval */
  next_millis = millis() + INTERVAL;
  edge_count = 0;
  old_int_count = int_count;
  
  while (millis() < next_millis) {
    /* update edge_count from 8-bit int_count taking care of wrap around */
    cur_int_count = int_count;
    if (cur_int_count >= old_int_count)
      edge_count += (cur_int_count - old_int_count);
    else
      edge_count += (cur_int_count + 256 - old_int_count); /* wrap around */
    old_int_count = cur_int_count;

    /* Get data for threshold for next interval */
    v = analogRead(PIN_PB0);
    minv = v < minv ? v : minv;
    maxv = v > maxv ? v : maxv;
  }
  
  maxv = map(maxv, 0, 1023, 0, 4999); /* to millivolts */
  minv = map(minv, 0, 1023, 0, 4999); /* to millivolts */
  meanv = (maxv + minv)/2;
  /* Can't set threshold above 4299 mV */
  meanv = meanv <= 4299 ? meanv : 4299;
  /* Threshold range 0..255 */
  v = map(meanv, 0, 4299, 0, 255);
  DAC1.DATA = v;
  
  show_udec(edge_count);
}
