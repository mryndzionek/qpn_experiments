#include "qpn_port.h"
#include "bsp.h"
#include <avr/io.h>                                              /* AVR I/O */
#ifndef NDEBUG
#include <stdlib.h>
#include <stdio.h>
#include "lcd.h"
#endif

#define LED_OFF(num_)       (PORTD &= ~(_BV(num_)))
#define LED_ON(num_)        (PORTD |= _BV(num_))
#define LED_TOGGLE(num_)    (PORTD ^= _BV(num_))
#define LED_OFF_ALL()       (PORTD = 0x00)
#define LED_ON_ALL()        (PORTD = 0xFF)

/*..........................................................................*/
ISR(TIMER1_COMPA_vect) {
    /* No need to clear the interrupt source since the Timer1 compare
     * interrupt is automatically cleared in hardware when the ISR runs.
     */
    QF_tickXISR(0U);
}
/*..........................................................................*/
void BSP_init(void) {
    DDRD  = 0xFF;                    /* All PORTD pins are outputs for LEDs */
    LED_OFF_ALL();                                     /* turn off all LEDs */

#ifndef NDEBUG
    lcd_init();
#endif
}
/*..........................................................................*/
void QF_onStartup(void) {
    /* set Timer2 in CTC mode, 1/1024 prescaler, start the timer ticking */
    TIMSK = _BV(OCIE1A);
    TCCR1B = _BV(WGM12);
    TCCR1B |= _BV(CS12) | _BV(CS10);
    OCR1A = ((F_CPU / BSP_TICKS_PER_SEC / 1024) - 1);          /* keep last */
}
/*..........................................................................*/
void QF_onIdle(void) {        /* entered with interrupts LOCKED, see NOTE01 */

    LED_ON(5);
    LED_OFF(5);

#ifdef NDEBUG

    MCUCR = (0 << SM0) | (1 << SE);/*idle sleep mode, adjust to your project */

    /* never separate the following two assembly instructions, see NOTE03 */
    __asm__ __volatile__ ("sei" "\n\t" :: );
    __asm__ __volatile__ ("sleep" "\n\t" :: );

    MCUCR = 0;                                           /* clear the SE bit */
#else
    QF_INT_ENABLE();
#endif

}
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
#ifndef NDEBUG
    char buff[16];
    char fbuf[16];
    size_t n;
#endif

    (void)file;                                   /* avoid compiler warning */
    (void)line;                                   /* avoid compiler warning */
    QF_INT_DISABLE();
    LED_ON_ALL();                                            /* all LEDs on */
#ifndef NDEBUG
    lcd_clear();
    n = strlen_P(file);
    if(n > 15)
        n = 15;
    memcpy_P(fbuf, file, n);
    fbuf[n] = '\0';

    lcd_set_line(0);
    snprintf (buff, sizeof(buff), "file: %s", fbuf);
    lcd_putstr(buff);
    lcd_set_line(1);
    snprintf (buff, sizeof(buff), "line: %d", line);
    lcd_putstr(buff);
#endif
    for (;;) {       /* NOTE: replace the loop with reset for final version */
    }
}
/*..........................................................................*/
void BSP_ledOn(void) {
    LED_ON(0);
}
/*..........................................................................*/
void BSP_ledOff(void) {
    LED_OFF(0);
}
