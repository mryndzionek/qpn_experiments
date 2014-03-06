#include "qpn_port.h"
#include "bsp.h"
#include <avr/io.h>                                              /* AVR I/O */
#ifndef NDEBUG
#include <stdlib.h>
#include <stdio.h>
#endif
#include "lcd.h"
#include "capstone.h"

#define PROGRESSPIXELS_PER_CHAR 6

const uint8_t PROGMEM bar0[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
const uint8_t PROGMEM bar1[] = {
        0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00
};
const uint8_t PROGMEM bar2[] = {
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00
};
const uint8_t PROGMEM bar3[] = {
        0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x00
};
const uint8_t PROGMEM bar4[] = {
        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x00
};
const uint8_t PROGMEM bar5[] = {
        0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00
};

#define LED_OFF(num_)       (PORTD &= ~_BV(num_))
#define LED_ON(num_)        (PORTD |= _BV(num_))
#define LED_TOGGLE(num_)    (PORTD ^= _BV(num_))
#define LED_OFF_ALL()       (PORTD = 0x00)
#define LED_ON_ALL()        (PORTD = 0xFF)

#ifndef NDEBUG
Q_DEFINE_THIS_FILE
#endif

static volatile uint8_t btn1_poll = 0;
static volatile uint8_t btn2_poll = 0;

/*..........................................................................*/
ISR(INT0_vect)
{
    if(!btn1_poll)
        {
            QActive_postISR((QActive *)&AO_Capstone, BTN1_DOWN_SIG, 0);
            btn1_poll = 1;
            GICR &= ~_BV(INT0);
        }

}
/*..........................................................................*/
ISR(INT1_vect)
{
    if(!btn2_poll)
        {
            QActive_postISR((QActive *)&AO_Capstone, BTN2_DOWN_SIG, 0);
            btn2_poll = 1;
            GICR &= ~_BV(INT1);
        }
}
/*..........................................................................*/
ISR(TIMER1_COMPA_vect) {
    /* No need to clear the interrupt source since the Timer1 compare
     * interrupt is automatically cleared in hardware when the ISR runs.
     */
    if(btn1_poll)
        {
            if(PIND & _BV(PC2)) {
                    QActive_postISR((QActive *)&AO_Capstone, BTN1_UP_SIG, 0);
                    btn1_poll = 0;
                    GICR |= _BV(INT0);
            }
        }
    if(btn2_poll)
        {
            if(PIND & _BV(PC3)) {
                    QActive_postISR((QActive *)&AO_Capstone, BTN2_UP_SIG, 0);
                    btn2_poll = 0;
                    GICR |= _BV(INT1);
            }
        }
    QF_tick();
}
/*..........................................................................*/
void BSP_init(void) {
    DDRD = 0xFF & ~(_BV(PD2) | _BV(PD3));
    LED_OFF_ALL();                                     /* turn off all LEDs */
    lcd_init();

    lcd_customchar_P(0, bar0);
    lcd_customchar_P(1, bar1);
    lcd_customchar_P(2, bar2);
    lcd_customchar_P(3, bar3);
    lcd_customchar_P(4, bar4);
    lcd_customchar_P(5, bar5);
}
/*..........................................................................*/
void QF_onStartup(void) {
    /* set Timer2 in CTC mode, 1/1024 prescaler, start the timer ticking */
    TIMSK = (1 << OCIE1A);
    TCCR1B = (1 << WGM12);
    TCCR1B |= (1 << CS12) | (1 << CS10);
    OCR1A = ((F_CPU / BSP_TICKS_PER_SEC / 1024) - 1);          /* keep last */

    GICR = _BV(INT0) | _BV(INT1);                   /* Enable INT0 and INT1 */
    MCUCR = 0x00;                                  /* Trigger INT0 and INT1 */
    /* on low level          */
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
void BSP_signalLeds(enum BSP_LedSignal sig) {
    switch(sig)
    {
        case UP:
            LED_OFF(0);
            LED_ON(4);
            break;

        case DOWN:
            LED_OFF(4);
            LED_ON(0);
            break;

        default:
            break;
    }

}
/*..........................................................................*/
void BSP_progressBar(uint8_t progress, uint8_t maxprogress, uint8_t length) {
        uint8_t i;
        uint16_t pixelprogress;
        uint8_t c;
        char buff[16];

        pixelprogress = ((progress*(length*PROGRESSPIXELS_PER_CHAR))/maxprogress);

        // print exactly "length" characters
        for(i=0; i<length; i++)
        {
                // check if this is a full block, or partial or empty
                // (u16) cast is needed to avoid sign comparison warning
                if( ((i*(uint16_t)PROGRESSPIXELS_PER_CHAR)+5) > pixelprogress )
                {
                        // this is a partial or empty block
                        if( ((i*(uint16_t)PROGRESSPIXELS_PER_CHAR)) > pixelprogress )
                        {
                                // this is an empty block
                                // use space character?
                                c = 0;
                        }
                        else
                        {
                                // this is a partial block
                                c = pixelprogress % PROGRESSPIXELS_PER_CHAR;
                        }
                }
                else
                {
                        // this is a full block
                        c = 5;
                }

                // write character to display
                buff[i] = c;
        }

        lcd_set_line(1);
        lcd_putstr(buff);

}
