#include "qpn_port.h"
#include "bsp.h"

#include <avr/io.h>                                              /* AVR I/O */
#include <string.h>

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
#define BINS_PER_50MS       (5 * BINS_PER_10MS)
#define BINS_PER_100MS      (10 * BINS_PER_10MS)
#define BINS_PER_200MS      (20 * BINS_PER_10MS)
#define THRESHOLD           (30)
#define SECONDS_PER_MINUTE  (60)
#define SYNC_MARK           (0)
#define UNDEFINED           (1)
#define SHORT_TICK          (2)
#define LONG_TICK           (3)
#define LOCK_TRESHOLD       (12)

#define BOUNDED_INCREMENT(_val, n) if (_val >= 255 - n) { _val = 255; } else { _val += n; }
#define BOUNDED_DECREMENT(_val, n) if (_val <= n) { _val = 0; } else { _val -= n; }

typedef struct {
    uint16_t data[BIN_COUNT];
    uint8_t tick;

    uint32_t noise_max;
    uint32_t max;
    uint8_t max_index;
} phase_bins;

typedef struct {
    uint16_t data[SECONDS_PER_MINUTE];
    uint8_t tick;

    uint32_t noise_max;
    uint32_t max;
    uint8_t max_index;
} sync_bins;


static phase_bins pbins;
static sync_bins sbins;

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

    memset (pbins.data, 0, BIN_COUNT);
    memset (sbins.data, 0, SECONDS_PER_MINUTE);

    pbins.max = 0;
    pbins.max_index = 0;
    pbins.noise_max = 0;
    pbins.tick = 0;

    sbins.max = 0;
    sbins.max_index = 0;
    sbins.noise_max = 0;
    sbins.tick = 0;
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
static char const *bin2dec3(uint32_t val) {
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

static uint8_t get_second() {
    if (sbins.max - sbins.noise_max >= LOCK_TRESHOLD) {
        // at least one sync mark and a 0 and a 1 seen
        // the threshold is tricky:
        //   higher --> takes longer to acquire an initial lock, but higher probability of an accurate lock
        //
        //   lower  --> higher probability that the lock will oscillate at the beginning
        //              and thus spoil the downstream stages

        // we have to subtract 2 seconds
        //   1 because the seconds already advanced by 1 tick
        //   1 because the sync mark is not second 0 but second 59

        uint8_t second = 2*SECONDS_PER_MINUTE + sbins.tick - 2 - sbins.max_index;
        while (second >= SECONDS_PER_MINUTE) { second-= SECONDS_PER_MINUTE; }

        return second;
    } else {
        return 0xff;
    }
}

static void compute_max_index() {

    uint8_t index;
    sbins.noise_max = 0;
    sbins.max = 0;
    sbins.max_index = 255;
    for (index = 0; index < SECONDS_PER_MINUTE; ++index) {
        const uint8_t bin_data = sbins.data[index];

        if (bin_data >= sbins.max) {
            sbins.noise_max = sbins.max;
            sbins.max = bin_data;
            sbins.max_index = index;
        } else if (bin_data > sbins.noise_max) {
            sbins.noise_max = bin_data;
        }
    }
}

static void sync_mark_binning(const uint8_t tick_data) {
    // We use a binning approach to find out the proper phase.
    // The goal is to localize the sync_mark. Due to noise
    // there may be wrong marks of course. The idea is to not
    // only look at the statistics of the marks but to exploit
    // additional data properties:

    // Bit position  0 after a proper sync is a 0.
    // Bit position 20 after a proper sync is a 1.

    // The binning will work as follows:

    //   1) A sync mark will score +6 points for the current bin
    //      it will also score -2 points for the previous bin
    //                         -2 points for the following bin
    //                     and -2 points 20 bins later
    //  In total this will ensure that a completely lost signal
    //  will not alter the buffer state (on average)

    //   2) A 0 will score +1 point for the previous bin
    //      it also scores -2 point 20 bins back
    //                 and -2 points for the current bin

    //   3) A 1 will score +1 point 20 bins back
    //      it will also score -2 point for the previous bin
    //                     and -2 points for the current bin

    //   4) An undefined value will score -2 point for the current bin
    //                                    -2 point for the previous bin
    //                                    -2 point 20 bins back

    //   5) Scores have an upper limit of 255 and a lower limit of 0.

    // Summary: sync mark earns 6 points, a 0 in position 0 and a 1 in position 20 earn 1 bonus point
    //          anything that allows to infer that any of the "connected" positions is not a sync will remove 2 points

    // It follows that the score of a sync mark (during good reception)
    // may move up/down the whole scale in slightly below 64 minutes.
    // If the receiver should glitch for whatever reason this implies
    // that the clock will take about 33 minutes to recover the proper
    // phase (during phases of good reception). During bad reception things
    // are more tricky.

    const uint8_t previous_tick = sbins.tick > 0 ? sbins.tick - 1: SECONDS_PER_MINUTE - 1;
    const uint8_t previous_21_tick = sbins.tick > 20 ? sbins.tick - 21 : sbins.tick + SECONDS_PER_MINUTE - 21;

    switch (tick_data) {
    case SYNC_MARK:
        BOUNDED_INCREMENT(sbins.data[sbins.tick], 6);
        BOUNDED_DECREMENT(sbins.data[previous_tick], 2);
        BOUNDED_DECREMENT(sbins.data[previous_21_tick], 2);

        { const uint8_t next_tick = sbins.tick < SECONDS_PER_MINUTE - 1 ? sbins.tick + 1 : 0;
        BOUNDED_DECREMENT(sbins.data[next_tick], 2); }
        break;

    case SHORT_TICK:
        BOUNDED_INCREMENT(sbins.data[previous_tick], 1);
        BOUNDED_DECREMENT(sbins.data[sbins.tick], 2);
        BOUNDED_DECREMENT(sbins.data[previous_21_tick], 2);
        break;

    case LONG_TICK:
        BOUNDED_INCREMENT(sbins.data[previous_21_tick],1);
        BOUNDED_DECREMENT(sbins.data[sbins.tick], 2);
        BOUNDED_DECREMENT(sbins.data[previous_tick], 2);
        break;

    case UNDEFINED:
    default:
        BOUNDED_DECREMENT(sbins.data[sbins.tick], 2);
        BOUNDED_DECREMENT(sbins.data[previous_tick], 2);
        BOUNDED_DECREMENT(sbins.data[previous_21_tick], 2);
    }
    sbins.tick = sbins.tick < SECONDS_PER_MINUTE - 1 ? sbins.tick + 1: 0;

    // determine sync lock
    if (sbins.max - sbins.noise_max <= LOCK_TRESHOLD ||
            get_second() == 3) {
        // after a lock is acquired this happens only once per minute and it is
        // reasonable cheap to process,
        //
        // that is: after we have a "lock" this will be processed whenever
        // the sync mark was detected

        compute_max_index();
        lcd_set_line(1);
        lcd_putstr("sync:");
        lcd_putstr(bin2dec3(sbins.max_index));
    }
}


static void decode_220ms(const uint8_t input, const uint8_t bins_to_go) {
    // will be called for each bin during the "interesting" 220ms

    static uint8_t count = 0;
    static uint8_t decoded_data = 0;

    count += input;
    if (bins_to_go >= BINS_PER_100MS + BINS_PER_10MS) {
        if (bins_to_go == BINS_PER_100MS + BINS_PER_10MS) {
            decoded_data = count > BINS_PER_50MS? 2: 0;
            count = 0;
        }
    } else {
        if (bins_to_go == 0) {
            decoded_data += count > BINS_PER_50MS ? 1 : 0;
            count = 0;
            // pass control further
            // decoded_data: 3 --> 1
            //               2 --> 0,
            //               1 --> undefined,
            //               0 --> sync_mark

            lcd_clear();
            lcd_set_line(0);
            lcd_putstr("data:");
            lcd_putstr(bin2dec3(decoded_data));
            lcd_putstr(" tck:");
            lcd_putstr(bin2dec3(sbins.tick));

            sync_mark_binning(decoded_data);
        }
    }
}

/*..........................................................................*/
void BSP_binning(uint16_t par) {

    pbins.tick = (par >> 8) & 0xFF;

    if (par & 0x01) {
        LED_ON(0);
        if (pbins.data[pbins.tick] < N_PREC) {
            ++pbins.data[pbins.tick];
        }
    } else {
        LED_OFF(0);
        if (pbins.data[pbins.tick] > 0) {
            --pbins.data[pbins.tick];
        }
    }
}
/*..........................................................................*/
void BSP_decoding(uint16_t par) {

    static uint8_t bins_to_process = 0;

    pbins.tick = (par >> 8) & 0xFF;

    if (par & 0x01) {
        LED_ON(0);
    } else {
        LED_OFF(0);
    }

    if (bins_to_process == 0) {
        if (wrap((BIN_COUNT + pbins.tick - pbins.max_index)) <= BINS_PER_100MS ||   // current_bin at most 100ms after phase_bin
                wrap((BIN_COUNT + pbins.max_index - pbins.tick)) <= BINS_PER_10MS ) {   // current bin at most 10ms before phase_bin
            // if phase bin varies to much during one period we will always be screwed in may ways...

            // start processing of bins
            bins_to_process = BINS_PER_200MS + 2*BINS_PER_10MS;
        }
    }

    if (bins_to_process > 0) {
        --bins_to_process;

        // this will be called for each bin in the "interesting" 220ms
        // this is also a good place for a "monitoring hook"
        decode_220ms(par & 0x01, bins_to_process);
    }
}

/*..........................................................................*/
bool BSP_convolution(void) {
    uint32_t integral = 0;
    pbins.noise_max = 0;
    pbins.max = 0;
    pbins.max_index = 0;

    uint16_t bin;


    for (bin = 0; bin < BINS_PER_100MS; ++bin)
        integral += ((uint32_t)pbins.data[bin])<<1;

    for (bin = BINS_PER_100MS; bin < BINS_PER_200MS; ++bin)
        integral += (uint32_t)pbins.data[bin];

    for (bin = 0; bin < BIN_COUNT; ++bin) {
        if (integral > pbins.max) {
            pbins.max = integral;
            pbins.max_index = bin;
        }

        integral -= (uint32_t)pbins.data[bin]<<1;
        integral += (uint32_t)(pbins.data[wrap(bin + BINS_PER_100MS)] + pbins.data[wrap(bin + BINS_PER_200MS)]);
    }

    const uint16_t noise_index = wrap(pbins.max_index + BINS_PER_200MS);

    for (bin = 0; bin < BINS_PER_100MS; ++bin)  {
        pbins.noise_max += ((uint32_t)pbins.data[wrap(noise_index + bin)])<<1;
    }

    for (bin = BINS_PER_100MS; bin < BINS_PER_200MS; ++bin)  {
        pbins.noise_max += (uint32_t)pbins.data[wrap(noise_index + bin)];
    }

    if (pbins.max - pbins.noise_max < THRESHOLD || wrap(BIN_COUNT + pbins.tick - pbins.max_index) == 53)
        return false;
    else
        return true;


}

/*..........................................................................*/
void BSP_MsgNotLocked(void) {

    lcd_set_line(0);
    lcd_putstr("phase:???");
}

/*..........................................................................*/
void BSP_MsgLocked(void) {

    lcd_set_line(0);
    lcd_putstr("phase:");
    lcd_putstr(bin2dec3(pbins.max_index));

    lcd_set_line(1);
    lcd_putstr("QoS:");
    lcd_putstr(bin2dec3(pbins.max));
    lcd_set_line(0);
}
