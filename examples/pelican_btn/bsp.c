#include "qpn_port.h"
#include "bsp.h"
#include <avr/io.h>                                             /* AVR I/O */
#ifndef NDEBUG
#include <stdlib.h>
#include <stdio.h>
#endif
#include "lcd.h"
#include "pelican_btn.h"

#define LED_OFF(num_)           (PORTD &= ~_BV(num_))
#define LED_ON(num_)            (PORTD |= _BV(num_))
#define LED_TOGGLE(num_)        (PORTD ^= _BV(num_))
#define LED_OFF_ALL()           PORTD = 0x00
#define LED_ON_ALL()            (PORTD = ~(_BV(PD2) | _BV(PD3)))

#ifndef NDEBUG
Q_DEFINE_THIS_FILE
#endif

static volatile uint8_t onoff_sig = OFF_SIG;
static volatile uint8_t onoff_poll = 0;

static volatile uint8_t peds_poll = 0;

/*..........................................................................*/
ISR(INT0_vect)
{
    QActive_postISR((QActive *)&AO_Pelican, onoff_sig, 0);
    if(!onoff_poll)
        {
            QActive_postISR((QActive *)&AO_Pelican, onoff_sig, 0);
            onoff_poll = 1;
            GICR &= ~_BV(INT0);
            if(onoff_sig == ON_SIG)
                onoff_sig = OFF_SIG;
            else
                onoff_sig = ON_SIG;
        }

}
/*..........................................................................*/
ISR(INT1_vect)
{
    if(!peds_poll)
        {
            QActive_postISR((QActive *)&AO_Pelican, PEDS_WAITING_SIG, 0);
            peds_poll = 1;
            GICR &= ~_BV(INT1);
        }
}
/*..........................................................................*/
ISR(TIMER1_COMPA_vect) {
    /* No need to clear the interrupt source since the Timer1 compare
     * interrupt is automatically cleared in hardware when the ISR runs.
     */
    QK_ISR_ENTRY();                     /* inform QK kernel about ISR entry */
    if(onoff_poll)
        {
            if(PIND & _BV(PC2)) {
                    onoff_poll = 0;
                    GICR |= _BV(INT0);
            }
        }
    if(peds_poll)
        {
            if(PIND & _BV(PC3)) {
                    peds_poll = 0;
                    GICR |= _BV(INT1);
            }
        }
    QF_tick();

    QK_ISR_EXIT();                       /* inform QK kernel about ISR exit */
}
/*..........................................................................*/
void BSP_init(void) {
    DDRD = 0xFF & ~(_BV(PD2) | _BV(PD3));
    LED_OFF_ALL();                                     /* turn off all LEDs */
    lcd_init();
}
/*..........................................................................*/
void QF_onStartup(void) {
    /* set Timer2 in CTC mode, 1/1024 prescaler, start the timer ticking */
    TIMSK = (1 << OCIE1A);
    TCCR1B = (1 << WGM12);
    TCCR1B |= (1 << CS12) | (1 << CS10);
    OCR1A = ((F_CPU / BSP_TICKS_PER_SEC / 1024) - 1);          /* keep last */

    GICR = _BV(INT0) | _BV(INT1);					/* Enable INT0 and INT1 */
    MCUCR = 0x00;               				   /* Trigger INT0 and INT1 */
    /* on low level          */
}
/*..........................................................................*/
void QK_onIdle(void) {        /* entered with interrupts LOCKED, see NOTE01 */

    QF_INT_DISABLE();
    LED_ON(5);
    LED_OFF(5);
    QF_INT_ENABLE();

#ifdef NDEBUG

    MCUCR = (0 << SM0) | (1 << SE);/*idle sleep mode, adjust to your project */

    /* never separate the following two assembly instructions, see NOTE03 */
    __asm__ __volatile__ ("sei" "\n\t" :: );
    __asm__ __volatile__ ("sleep" "\n\t" :: );

    MCUCR = 0;                                          /* clear the SE bit */
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
    lcd_clear();
#ifndef NDEBUG
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
void BSP_signalCars(enum BSP_CarsSignal sig) {
    switch (sig) {
        case CARS_RED:
            LED_ON(0);
            LED_OFF(1);
            LED_OFF(4);
            lcd_set_line(1);
            lcd_putstr("CARS: RED       ");
            break;

        case CARS_YELLOW:
            LED_OFF(0);
            LED_OFF(4);
            LED_ON(1);
            lcd_set_line(1);
            lcd_putstr("CARS: YELLOW    ");
            break;

        case CARS_GREEN:
            LED_OFF(0);
            LED_OFF(1);
            LED_ON(4);
            lcd_set_line(1);
            lcd_putstr("CARS: GREEN     ");
            break;

        case CARS_BLANK:
            LED_OFF(0);
            LED_OFF(1);
            LED_OFF(4);
            lcd_set_line(1);
            lcd_putstr("                ");
            break;
    }
}
/*..........................................................................*/
void BSP_signalPeds(enum BSP_PedsSignal sig) {
    switch (sig) {
        case PEDS_DONT_WALK:
            LED_OFF(7);
            lcd_set_line(0);
            lcd_putstr("***DON'T WALK***");
            break;
        case PEDS_BLANK:
            LED_ON(7);
            lcd_set_line(0);
            lcd_putstr("                ");
            break;
        case PEDS_WALK:
            LED_OFF(7);
            lcd_set_line(0);
            lcd_putstr("*** WALK NOW ***");
            break;
    }
}
