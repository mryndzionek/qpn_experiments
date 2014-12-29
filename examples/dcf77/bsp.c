#include "qpn_port.h"
#include "bsp.h"

#include <avr/io.h>                                              /* AVR I/O */

#include "lcd.h"
#include "phase_detector.h"

#define LED_OFF(num_)       (PORTD &= ~(1 << (num_)))
#define LED_ON(num_)        (PORTD |= (1 << (num_)))
#define LED_OFF_ALL()       (PORTD &= ~(0x7F))
#define LED_ON_ALL()        (PORTD |= 0x7F)

#define BIN_COUNT           (100)
#define SAMPLES_PER_BIN     (BSP_TICKS_PER_SEC / BIN_COUNT)
#define N_PREC              (300)
#define MAX_INDEX           (255)
#define BINS_PER_10MS       (BIN_COUNT / 100)
#define BINS_PER_100MS      (10 * BINS_PER_10MS)
#define BINS_PER_200MS      (20 * BINS_PER_10MS)
#define THRESHOLD           (30)

static uint16_t bins[BIN_COUNT];
static uint8_t cur_tick;

/*..........................................................................*/
ISR(TIMER1_COMPA_vect) {
    /* No need to clear the interrupt source since the Timer1 compare
     * interrupt is automatically cleared in hardware when the ISR runs.
     */

    static uint8_t tick = 0;
    static uint8_t curr_sample = 0;
    static uint8_t average = 0;

    QK_ISR_ENTRY();                     /* inform QK kernel about ISR entry */

    if(PIND & (1 << 7))
        average++;

    if (++curr_sample >= SAMPLES_PER_BIN) {

        if(tick < BIN_COUNT - 1) 
            ++tick;
        else 
            tick = 0;

        const uint16_t input = (average > SAMPLES_PER_BIN/2) ? 1 : 0;

        curr_sample = 0;
        average =  0;

        QACTIVE_POST_ISR((QActive *)&AO_PhaseDetector, SAMPLE_READY_SIG, input | (tick << 8));

    }

    QF_tickXISR(0U);

    QK_ISR_EXIT();                       /* inform QK kernel about ISR exit */
}
/*..........................................................................*/
void BSP_init(void) {
    DDRD  = 0x7F;                    /* All PORTD pins are outputs for LEDs */
    LED_OFF_ALL();                                     /* turn off all LEDs */
    lcd_init();
    lcd_set_line(0);
    lcd_putstr("phase:???");
}
/*..........................................................................*/
void QF_onStartup(void) {
    /* set Timer2 in CTC mode, 1/64 prescaler, start the timer ticking */
    TIMSK = _BV(OCIE1A);
    TCCR1B = _BV(WGM12);
    TCCR1B |= _BV(CS11) | _BV(CS10);
    OCR1A = ((F_CPU / BSP_TICKS_PER_SEC / 64) - 1);          /* keep last */
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
    (void)file;                                   /* avoid compiler warning */
    (void)line;                                   /* avoid compiler warning */
    QF_INT_DISABLE();
    LED_ON_ALL();                                            /* all LEDs on */
    for (;;) {       /* NOTE: replace the loop with reset for final version */
    }
}

static inline uint16_t wrap(const uint16_t value) {
    // faster modulo function which avoids division
    uint16_t result = value;
    while (result >= BIN_COUNT) {
        result-= BIN_COUNT;
    }
    return result;
}

/*..........................................................................*/
char const *bin2dec3(uint32_t val) {
    static char str[] = "DDD";
    str[2] = '0' + (val % 10);
    val /= 10;
    str[1] = '0' + (val % 10);
    val /= 10;
    str[0] = '0' + (val % 10);
    if (str[0] == '0') {
        str[0] = ' ';
        if (str[1] == '0') {
            str[1] = ' ';
        }
    }
    return str;
}

/*..........................................................................*/
void BSP_binning(uint16_t par) {

    cur_tick = (par >> 8) & 0xFF;

    if (par & 0x01) {
        LED_ON(0);
        if (bins[cur_tick] < N_PREC) {
            ++bins[cur_tick];
        }
    } else {
        LED_OFF(0);
        if (bins[cur_tick] > 0) {
            --bins[cur_tick];
        }
    }
}
/*..........................................................................*/
void BSP_convolution(void) {
    uint32_t integral = 0;
    uint32_t noise_max = 0;
    uint32_t max = 0;
    uint8_t max_index = 0;
    uint16_t bin;


    for (bin = 0; bin < BINS_PER_100MS; ++bin)
        integral += ((uint32_t)bins[bin])<<1;

    for (bin = BINS_PER_100MS; bin < BINS_PER_200MS; ++bin)
        integral += (uint32_t)bins[bin];

    for (bin = 0; bin < BIN_COUNT; ++bin) {
        if (integral > max) {
            max = integral;
            max_index = bin;
        }

        integral -= (uint32_t)bins[bin]<<1;
        integral += (uint32_t)(bins[wrap(bin + BINS_PER_100MS)] + bins[wrap(bin + BINS_PER_200MS)]);
    }

    const uint16_t noise_index = wrap(max_index + BINS_PER_200MS);

    for (bin = 0; bin < BINS_PER_100MS; ++bin)  {
        noise_max += ((uint32_t)bins[wrap(noise_index + bin)])<<1;
    }

    for (bin = BINS_PER_100MS; bin < BINS_PER_200MS; ++bin)  {
        noise_max += (uint32_t)bins[wrap(noise_index + bin)];
    }

    if (max - noise_max < THRESHOLD || wrap(BIN_COUNT + cur_tick - max_index) == 53)
    {
        LED_OFF(4);
        lcd_set_line(0);
        lcd_putstr("phase:???");
    } else
    {
        LED_ON(4);
        lcd_set_line(0);
        lcd_putstr("phase:");
        lcd_putstr(bin2dec3(max_index));
    }



}
