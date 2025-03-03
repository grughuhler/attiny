/*  This program demonstrates using the periodic interval
 *  timer (PIT) on an attiny to generate a periodic interrupt.
 *  
 *  This interrupt is used to time samples for the switch
 *  debounce algorithm described by Jack Ganssle in
 *  https://www.ganssle.com/debouncing.htm (part 2).
 *  
 *  License: BSD 2-Clause as per SPDX.
 */
#include <TM1637Display.h>

/* For 7-segment LED display */
#define CLK_PIN PIN_PA3     // package pin 7
#define DIO_PIN PIN_PA2     // package pin 5

#define SW_PIN  PIN_PA1     // package pin 4
#define TOG_PIN PIN_PA7     // package pin 3
#define IRQ_PIN PIN_PA6     // package pin 2

/* define TOGGLE_OPTION to be one of these: toggle when
 *  PIT_INT - PIT interrupt happens
 *  SW_PRESSED - switch press is detected
 *  IRQ_INT    - external pin interrupt happens
 */
//#define PIT_INT
#define SW_PRESSED
//#define IRQ_INT

TM1637Display display(CLK_PIN, DIO_PIN);

volatile uint8_t button_ctr = 0;

/* ISR macro defines a direct interrupt handler.  See
 * https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/extras/Ref_Interrupts.md
 */
 
ISR(RTC_PIT_vect)
{
  static unsigned short button_state = 0;

  RTC.PITINTFLAGS = RTC_PI_bm; /* clear PIT interrupt flag */
  
#ifdef PIT_INT
  PORTA.OUTTGL = 0x80; /* Pin 7 */
#endif

  /* Our button reads LOW when pressed */
  button_state = (button_state << 1) | digitalRead(SW_PIN) | 0xe000;
  if (button_state == 0xf000) {
    button_ctr += 1;
#ifdef SW_PRESSED
    PORTA.OUTTGL = 0x80; /* Pin 7 */
#endif
  }
}

ISR(PORTA_PORT_vect)
{
  /* This is ASSUMING no other interrupts on Port A */
  PORTA.INTFLAGS = 0x40; /* clear port A pin 6 interrupt flag */
  
#ifdef IRQ_INT
  PORTA.OUTTGL = 0x80; /* Pin 7 */
#endif
}

void setup()
{
  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(IRQ_PIN, INPUT_PULLUP); /* pullup in case not driven */
  pinMode(TOG_PIN, OUTPUT);
  
  /* Set PIT in RTC to create an interrupt every ~2 ms */
  /* use internal 32768 Hz osc */ 
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
  /* enable PIT interrupt */
  RTC.PITINTCTRL = RTC_PI_bm;
  /* Select period of 64 cycles and enable the PIT */
  RTC.PITCTRLA = RTC_PERIOD_CYC64_gc | RTC_PITEN_bm;

  /* Enable PORT A pin 6 as a pin (external IRQ) */
  /* enable interrupt on rising edge */
  PORTA.PIN6CTRL = PORT_ISC_RISING_gc; 
  
  display.setBrightness(0x0);
  display.showNumberDec(0, false);
}

void loop()
{
  static uint8_t old_button_ctr = 0;

  if (button_ctr != old_button_ctr) {
    /* button was pressed */
    old_button_ctr = button_ctr;
    display.showNumberDec(old_button_ctr, false);
  }

}
