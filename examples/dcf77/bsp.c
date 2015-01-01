#include "qpn_port.h"
#include "bsp.h"

#include <avr/io.h>                                              /* AVR I/O */
#include <string.h>

#include "lcd.h"
#include "lcd_font.h"
#include "phase_detector.h"
#include "decoder.h"
#include "led_pulser.h"

#define DCF_DEBUG

#define IDLE_LED            (PD5)
#define SIGNAL_LED          (PD0)
#define PWM_LED_G           (PD3)
#define PWM_LED_R           (PD4)
#define DCF77_INPUT         (PD7)

#define LED_MASK_PD         (_BV(IDLE_LED) | _BV(SIGNAL_LED) | _BV(PWM_LED_G) | _BV(PWM_LED_R))
#define LED_OFF(num_)       (PORTD &= ~(1 << (num_)))
#define LED_ON(num_)        (PORTD |= (1 << (num_)))
#define LED_OFF_ALL()       (PORTD &= ~(LED_MASK_PD))
#define LED_ON_ALL()        (PORTD |= LED_MASK_PD)

#define BIN_COUNT           (100)
#define SAMPLES_PER_BIN     (BSP_TICKS_PER_SEC / BIN_COUNT)
#define N_PREC              (300)
#define MAX_INDEX           (255)
#define BINS_PER_10MS       (BIN_COUNT / 100)
#define BINS_PER_50MS       (5 * BINS_PER_10MS)
#define BINS_PER_100MS      (10 * BINS_PER_10MS)
#define BINS_PER_200MS      (20 * BINS_PER_10MS)
#define THRESHOLD           (30)
#define HOURS_PER_DAY       (24)
#define SECONDS_PER_MINUTE  (60)
#define MINUTES_PER_HOUR    (60)
#define DAYS_PER_MONTH      (31)
#define WEEKDAYS_PER_WEEK   (7)
#define MONTHS_PER_YEAR     (12)
#define YEARS_PER_CENTURY   (100)
#define SYNC_MARK           (0)
#define UNDEFINED           (1)
#define SHORT_TICK          (2)
#define LONG_TICK           (3)
#define LOCK_TRESHOLD       (12)

#define BOUNDED_INCREMENT(_val, n) if (_val >= 255 - n) { _val = 255; } else { _val += n; }
#define BOUNDED_DECREMENT(_val, n) if (_val <= n) { _val = 0; } else { _val -= n; }
#define SCORE(_bin, _input, _candidate, _sig) do { \
        const uint8_t the_score = _sig - bit_count(_input.val ^ _candidate.val); \
        bounded_add(_bin, the_score); \
} while(0) \

#define INIT_BIN(_name, _size) \
        memset (_name.data, 0, _size); \
        _name.max = 0; \
        _name.max_index = 0; \
        _name.noise_max = 0; \
        _name.tick = 0;

#define ADVANCE_TICK_DECL(_name, _size) \
        void advance_## _name(_name ## _bins_t *bins) { \
    if (bins->tick < _size - 1) { \
        ++bins->tick; \
    } else { \
        bins->tick = 0; \
    } \
} \

#define GET_TIME_VALUE_DECL(_name, _size) \
        bcd_t get_## _name(const _name ## _bins_t *bins) { \
    \
    const uint8_t threshold = 2; \
    \
    if (bins->max - bins->noise_max >= threshold) { \
        return int_to_bcd((bins->max_index + bins->tick + 1) % _size); \
    } else { \
        bcd_t undefined; \
        undefined.val = 0xff; \
        return undefined; \
    } \
} \

#define COMPUTE_MAX_INDEX_DECL(_name, _size) \
        inline void compute_max_ ## _name ## _index(_name ## _bins_t *bins) { \
    \
    uint8_t index; \
    bins->noise_max = 0; \
    bins->max = 0; \
    bins->max_index = 255; \
    for (index = 0; index < _size; ++index) { \
        const uint8_t bin_data = bins->data[index]; \
        \
        if (bin_data >= bins->max) { \
            bins->noise_max = sbins.max; \
            bins->max = bin_data; \
            bins->max_index = index; \
        } else if (bin_data > bins->noise_max) { \
            bins->noise_max = bin_data; \
        } \
    } \
} \

#define HAMMING_BINNING_DECL(_name, _num, _sig, _par) \
        void hamming_ ## _name ## _binning(_name ## _bins_t *bins, const bcd_t input) { \
    uint8_t bin_index; \
    \
    if (bins->max > 255 - _sig) { \
        for (bin_index = 0; bin_index < _num; ++bin_index) { \
            BOUNDED_DECREMENT(bins->data[bin_index], _sig); \
        } \
        bins->max -= _sig; \
        BOUNDED_DECREMENT(bins->noise_max, _sig); \
    } \
    \
    const uint8_t offset = _num - 1 - bins->tick; \
    uint8_t pass; \
    bin_index = offset; \
    \
    bcd_t candidate; \
    candidate.val = 0x00; \
    for (pass=0; pass < _num; ++pass) { \
        \
        if (_par) { \
            candidate.bit.b7 = parity(candidate.val); \
            SCORE(&bins->data[bin_index], input, candidate, _sig); \
            candidate.bit.b7 = 0; \
        } else { \
            SCORE(&bins->data[bin_index], input, candidate, _sig); \
        } \
        \
        bin_index = bin_index < _num - 1 ? bin_index + 1 : 0; \
        increment(&candidate); \
    } \
} \

typedef union {
    struct {
        uint8_t lo:4;
        uint8_t hi:4;
    } digit;

    struct {
        uint8_t b0:1;
        uint8_t b1:1;
        uint8_t b2:1;
        uint8_t b3:1;
        uint8_t b4:1;
        uint8_t b5:1;
        uint8_t b6:1;
        uint8_t b7:1;
    } bit;

    uint8_t val;
} bcd_t;


typedef struct {
    uint8_t data[YEARS_PER_CENTURY];
    uint8_t tick;

    uint32_t noise_max;
    uint32_t max;
    uint8_t max_index;
} year_bins_t;

typedef struct {
    uint8_t data[MONTHS_PER_YEAR];
    uint8_t tick;

    uint32_t noise_max;
    uint32_t max;
    uint8_t max_index;
} month_bins_t;

typedef struct {
    uint8_t data[WEEKDAYS_PER_WEEK];
    uint8_t tick;

    uint32_t noise_max;
    uint32_t max;
    uint8_t max_index;
} weekday_bins_t;

typedef struct {
    uint8_t data[DAYS_PER_MONTH];
    uint8_t tick;

    uint32_t noise_max;
    uint32_t max;
    uint8_t max_index;
} day_bins_t;

typedef struct {
    uint8_t data[MINUTES_PER_HOUR];
    uint8_t tick;

    uint32_t noise_max;
    uint32_t max;
    uint8_t max_index;
} minute_bins_t;

typedef struct {
    uint8_t data[HOURS_PER_DAY];
    uint8_t tick;

    uint32_t noise_max;
    uint32_t max;
    uint8_t max_index;
} hour_bins_t;

typedef struct {
    uint16_t data[BIN_COUNT];
    uint8_t tick;

    uint32_t noise_max;
    uint32_t max;
    uint8_t max_index;
} phase_bins_t;

typedef struct {
    uint16_t data[SECONDS_PER_MINUTE];
    uint8_t tick;

    uint32_t noise_max;
    uint32_t max;
    uint8_t max_index;
} sync_bins_t;

typedef struct {
    bcd_t hour;
    bcd_t minute;
    bcd_t day;
    bcd_t weekday;
    bcd_t month;
    bcd_t year;

    bcd_t prev_minute;
    uint8_t second;      // 0..60

} time_data_t;

static phase_bins_t pbins;
static sync_bins_t sbins;
static hour_bins_t hbins;
static minute_bins_t mbins;
static day_bins_t dbins;
static weekday_bins_t wbins;
static month_bins_t mobins;
static year_bins_t ybins;

static time_data_t now = {.prev_minute.val = 0xFF};

#define LED_PULSES      (50)

static bcd_t int_to_bcd(const uint8_t value) {
    const uint8_t hi = value / 10;

    bcd_t result;
    result.digit.hi = hi;
    result.digit.lo = value-10*hi;

    return result;
}

static void increment(bcd_t *value) {
    if (value->digit.lo < 9) {
        ++value->digit.lo;
    } else {
        value->digit.lo = 0;

        if (value->digit.hi < 9) {
            ++value->digit.hi;
        } else {
            value->digit.hi = 0;
        }
    }
}

inline void bounded_add(uint8_t *value, const uint8_t amount) __attribute__((always_inline));
void bounded_add(uint8_t *value, const uint8_t amount) {
    if (*value >= 255-amount) { *value = 255; } else { *value += amount; }
}

inline void bounded_sub(uint8_t *value, const uint8_t amount) __attribute__((always_inline));
void bounded_sub(uint8_t *value, const uint8_t amount) {
    if (*value <= amount) { *value = 0; } else { *value -= amount; }
}

inline uint8_t bit_count(const uint8_t value) __attribute__((always_inline));
uint8_t bit_count(const uint8_t value) {
    const uint8_t tmp1 = (value & 0b01010101) + ((value>>1) & 0b01010101);
    const uint8_t tmp2 = (tmp1  & 0b00110011) + ((tmp1>>2) & 0b00110011);
    return (tmp2 & 0x0f) + (tmp2>>4);
}

inline uint8_t parity(const uint8_t value) __attribute__((always_inline));
uint8_t parity(const uint8_t value) {
    uint8_t tmp = value;

    tmp = (tmp & 0xf) ^ (tmp >> 4);
    tmp = (tmp & 0x3) ^ (tmp >> 2);
    tmp = (tmp & 0x1) ^ (tmp >> 1);

    return tmp;
}

static void increment_bcd(bcd_t *value) {
    if (value->digit.lo < 9) {
        ++value->digit.lo;
    } else {
        value->digit.lo = 0;

        if (value->digit.hi < 9) {
            ++value->digit.hi;
        } else {
            value->digit.hi = 0;
        }
    }
}

static void advance_second(time_data_t *now) {
    // in case some value is out of range it will not be advanced
    // this is on purpose
    if (now->second < 59) {
        ++now->second;
    } else {
        now->second = 0;

        if (now->minute.val < 0x59) {
            increment_bcd(&now->minute);
        } else if (now->minute.val == 0x59) {
            now->minute.val = 0x00;

            if (now->hour.val < 0x23) {
                increment_bcd(&now->hour);
            } else if (now->hour.val == 0x23) {
                now->hour.val = 0x00;
            }
        }
    }
}

static COMPUTE_MAX_INDEX_DECL(sync, SECONDS_PER_MINUTE);

static GET_TIME_VALUE_DECL(hour, HOURS_PER_DAY);
static COMPUTE_MAX_INDEX_DECL(hour, HOURS_PER_DAY);
static HAMMING_BINNING_DECL(hour, 24, HOURS_PER_DAY, 1);
static ADVANCE_TICK_DECL(hour, HOURS_PER_DAY);

static GET_TIME_VALUE_DECL(minute, MINUTES_PER_HOUR);
static COMPUTE_MAX_INDEX_DECL(minute, MINUTES_PER_HOUR);
static HAMMING_BINNING_DECL(minute, MINUTES_PER_HOUR, 8, 1);
static ADVANCE_TICK_DECL(minute, MINUTES_PER_HOUR);

static GET_TIME_VALUE_DECL(day, DAYS_PER_MONTH);
static COMPUTE_MAX_INDEX_DECL(day, DAYS_PER_MONTH);
static HAMMING_BINNING_DECL(day, DAYS_PER_MONTH, 6, 0);
static ADVANCE_TICK_DECL(day, DAYS_PER_MONTH);

static GET_TIME_VALUE_DECL(weekday, WEEKDAYS_PER_WEEK);
static COMPUTE_MAX_INDEX_DECL(weekday, WEEKDAYS_PER_WEEK);
static HAMMING_BINNING_DECL(weekday, WEEKDAYS_PER_WEEK, 3, 0);
static ADVANCE_TICK_DECL(weekday, WEEKDAYS_PER_WEEK);

static GET_TIME_VALUE_DECL(month, MONTHS_PER_YEAR);
static COMPUTE_MAX_INDEX_DECL(month, MONTHS_PER_YEAR);
static HAMMING_BINNING_DECL(month, MONTHS_PER_YEAR, 5, 0);
static ADVANCE_TICK_DECL(month, MONTHS_PER_YEAR);

static GET_TIME_VALUE_DECL(year, YEARS_PER_CENTURY);
static COMPUTE_MAX_INDEX_DECL(year, YEARS_PER_CENTURY);
static HAMMING_BINNING_DECL(year, YEARS_PER_CENTURY, 8, 0);
static ADVANCE_TICK_DECL(year, YEARS_PER_CENTURY);

/*..........................................................................*/
ISR(TIMER1_COMPA_vect) {
    /* No need to clear the interrupt source since the Timer1 compare
     * interrupt is automatically cleared in hardware when the ISR runs.
     */

    static uint8_t tick = 0;
    static uint8_t curr_sample = 0;
    static uint8_t average = 0;

    QK_ISR_ENTRY();                     /* inform QK kernel about ISR entry */

    if(PIND & _BV(DCF77_INPUT))
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
    DDRD = LED_MASK_PD;                    /* All PORTD pins are outputs for LEDs */
    LED_OFF_ALL();                                     /* turn off all LEDs */

    LED_ON(PWM_LED_G);
    LED_ON(PWM_LED_R);

    lcd_init();
#ifndef DCF_DEBUG
    lcd_font_init();
#endif
    INIT_BIN(pbins, HOURS_PER_DAY);
    INIT_BIN(sbins, HOURS_PER_DAY);
    INIT_BIN(hbins, HOURS_PER_DAY);
    INIT_BIN(mbins, MINUTES_PER_HOUR);
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
    LED_ON(IDLE_LED);
    LED_OFF(IDLE_LED);
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
static char const *bin2dec2(uint8_t val) {
    static char str[] = "DD";
    str[1] = '0' + (val % 10);
    val /= 10;
    str[0] = '0' + (val % 10);
    return str;
}

/*..........................................................................*/
static char const *bin2dec3(uint16_t val) {
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

static char const *get_cursor()
{
    static char curs[] = ".  ";
    char tmp;

    tmp = curs[2];
    curs[2] = curs[1];
    curs[1] = curs[0];
    curs[0] = tmp;

    return curs;
}

#ifdef DCF_DEBUG
static void display_time(uint8_t tick_value)
{
    lcd_set_line(0);
    lcd_putchar('0' + now.hour.digit.hi);
    lcd_putchar('0' + now.hour.digit.lo);
    lcd_putchar(':');
    lcd_putchar('0' + now.prev_minute.digit.hi);
    lcd_putchar('0' + now.prev_minute.digit.lo);
    lcd_putchar(':');
    lcd_putstr((now.second == 0xFF) ? "??" : bin2dec2(now.second));
    lcd_putchar('0' + now.day.digit.hi);
    lcd_putchar('0' + now.day.digit.lo);
//    lcd_putchar('0' + now.weekday.digit.hi);
//    lcd_putchar('0' + now.weekday.digit.lo);
    lcd_putchar('0' + now.month.digit.hi);
    lcd_putchar('0' + now.month.digit.lo);
    lcd_putchar('0' + now.year.digit.hi);
    lcd_putchar('0' + now.year.digit.lo);

    lcd_set_line(1);
    lcd_putstr("p");
    lcd_putstr(bin2dec3(pbins.max - pbins.noise_max));
    lcd_putstr("s");
    lcd_putstr(bin2dec3(sbins.max - sbins.noise_max));
    lcd_putstr("m");
    lcd_putstr(bin2dec3(mbins.max - mbins.noise_max));
    lcd_putstr("h");
    lcd_putstr(bin2dec3(hbins.max - hbins.noise_max));

    QActive_post((QActive *)&AO_LEDPulser, LED_PULSE_SIG, tick_value);
}
#else
static void display_time(uint8_t tick_value)
{
#define DISP_OFFSET (0)

    lcd_font_num(now.hour.digit.hi, DISP_OFFSET);
    lcd_font_num(now.hour.digit.lo, DISP_OFFSET+3);

    if(now.second & 0x01)
    {
        lcd_set_position(6);
        lcd_putchar(0xA5);
        lcd_set_position(LCD_COLS + DISP_OFFSET + 6);
        lcd_putchar(0xA5);
    } else {
        lcd_set_position(6);
        lcd_putchar(32);
        lcd_set_position(LCD_COLS + DISP_OFFSET + 6);
        lcd_putchar(32);
    }

    lcd_font_num(now.prev_minute.digit.hi, DISP_OFFSET + 7);
    lcd_font_num(now.prev_minute.digit.lo, DISP_OFFSET + 10);

    lcd_set_position(LCD_COLS - 2);
    lcd_putstr((now.second == 0xFF) ? "??" : bin2dec2(now.second));

    QActive_post((QActive *)&AO_LEDPulser, LED_PULSE_SIG, tick_value);
}
#endif

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

        compute_max_sync_index(&sbins);
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

            QActive_post((QActive *)&AO_Decoder, DCF_DATA_SIG, decoded_data);

        }
    }
}

/*..........................................................................*/
void BSP_binning(uint16_t par) {

    pbins.tick = (par >> 8) & 0xFF;

    if (par & 0x01) {
        if (pbins.data[pbins.tick] < N_PREC) {
            ++pbins.data[pbins.tick];
        }
    } else {
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
        LED_ON(SIGNAL_LED);
    } else {
        LED_OFF(SIGNAL_LED);
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

    QActive_post((QActive *)&AO_Decoder, PHASE_UPDATE_SIG, 0);

    if (pbins.max - pbins.noise_max < THRESHOLD || wrap(BIN_COUNT + pbins.tick - pbins.max_index) == 53)
        return false;
    else
        return true;

}

/*..........................................................................*/
void BSP_dispLocking(void) {
    lcd_set_line(0);
    lcd_putstr("Locking");
    lcd_putstr(get_cursor());
    lcd_set_line(1);
    lcd_putstr("phase: ");
    lcd_putstr(bin2dec3(pbins.max_index));
#ifdef DCF_DEBUG
    lcd_putstr(" ");
    lcd_putstr(bin2dec3(pbins.max - pbins.noise_max));
#endif
}

/*..........................................................................*/
uint8_t BSP_dispSyncing(uint8_t tick_data) {

    sync_mark_binning(tick_data);
    lcd_set_line(0);
    lcd_putstr("Syncing");
    lcd_putstr(get_cursor());
    lcd_set_line(1);
    lcd_putstr("QoS: ");
    lcd_putstr(bin2dec3(sbins.max - sbins.noise_max));
    lcd_putchar('/');
    lcd_putstr(bin2dec2(LOCK_TRESHOLD));

    return get_second(&sbins);

}

/*..........................................................................*/
uint8_t BSP_dispDecoding(uint8_t tick_data) {

    now.second = get_second(&sbins);
    now.hour = get_hour(&hbins);
    now.minute = get_minute(&mbins);
    now.day = get_day(&dbins);
    now.weekday = get_weekday(&wbins);
    now.month = get_month(&mobins);
    now.year = get_year(&ybins);

    advance_second(&now);
    sync_mark_binning(tick_data);

    if (sbins.max - sbins.noise_max <= LOCK_TRESHOLD)
        return 0xFF;

    if (now.second == 0) {

        advance_minute(&mbins);
        if (now.minute.val == 0x01) {

            // "while" takes automatically care of timezone change
            while (get_hour(&hbins).val <= 0x23 && get_hour(&hbins).val != now.hour.val) { advance_hour(&hbins); }

            if (now.hour.val == 0x00) {
                if (get_weekday(&wbins).val <= 0x07) { advance_weekday(&wbins); }

                // "while" takes automatically care if different month lengths
                while (get_day(&dbins).val <= 0x31 && get_day(&dbins).val != now.day.val) { advance_day(&dbins); }

                if (now.day.val == 0x01) {
                    if (get_month(&mobins).val <= 0x12) { advance_month(&mobins); }
                    if (now.month.val == 0x01) {
                        if (now.year.val <= 0x99) { advance_year(&ybins); }
                    }
                }
            }
        }
    }

    const uint8_t tick_value = (tick_data == LONG_TICK || tick_data == UNDEFINED)? 1: 0;

    static bcd_t hour_data;

    switch (now.second) {
    case 29: hour_data.val +=      tick_value; break;
    case 30: hour_data.val +=  0x2*tick_value; break;
    case 31: hour_data.val +=  0x4*tick_value; break;
    case 32: hour_data.val +=  0x8*tick_value; break;
    case 33: hour_data.val += 0x10*tick_value; break;
    case 34: hour_data.val += 0x20*tick_value; break;
    case 35: hour_data.val += 0x80*tick_value;        // Parity !!!
    hamming_hour_binning(&hbins, hour_data); break;

    case 36: compute_max_hour_index(&hbins);
    // fall through on purpose
    default: hour_data.val = 0;
    }

    static bcd_t minute_data;

    switch (now.second) {
    case 21: minute_data.val +=      tick_value; break;
    case 22: minute_data.val +=  0x2*tick_value; break;
    case 23: minute_data.val +=  0x4*tick_value; break;
    case 24: minute_data.val +=  0x8*tick_value; break;
    case 25: minute_data.val += 0x10*tick_value; break;
    case 26: minute_data.val += 0x20*tick_value; break;
    case 27: minute_data.val += 0x40*tick_value; break;
    case 28: minute_data.val += 0x80*tick_value;        // Parity !!!
    hamming_minute_binning(&mbins, minute_data); break;
    case 29: compute_max_minute_index(&mbins);
    // fall through on purpose
    default: minute_data.val = 0;

    }

    static bcd_t day_data;

    switch (now.second) {
    case 36: day_data.val +=      tick_value; break;
    case 37: day_data.val +=  0x2*tick_value; break;
    case 38: day_data.val +=  0x4*tick_value; break;
    case 39: day_data.val +=  0x8*tick_value; break;
    case 40: day_data.val += 0x10*tick_value; break;
    case 41: day_data.val += 0x20*tick_value;
    hamming_day_binning(&dbins, day_data); break;
    case 42: compute_max_day_index(&dbins);
    // fall through on purpose
    default: day_data.val = 0;
    }

    static bcd_t weekday_data;

    switch (now.second) {
    case 42: weekday_data.val +=      tick_value; break;
    case 43: weekday_data.val +=  0x2*tick_value; break;
    case 44: weekday_data.val +=  0x4*tick_value;
    hamming_weekday_binning(&wbins, weekday_data); break;
    case 45: compute_max_weekday_index(&wbins);
    // fall through on purpose
    default: weekday_data.val = 0;
    }

    static bcd_t month_data;

    switch (now.second) {
    case 45: month_data.val +=      tick_value; break;
    case 46: month_data.val +=  0x2*tick_value; break;
    case 47: month_data.val +=  0x4*tick_value; break;
    case 48: month_data.val +=  0x8*tick_value; break;
    case 49: month_data.val += 0x10*tick_value;
    hamming_month_binning(&mobins, month_data); break;

    case 50: compute_max_month_index(&mobins);
    // fall through on purpose
    default: month_data.val = 0;
    }

    static bcd_t year_data;

    switch (now.second) {
    case 50: year_data.val +=      tick_value; break;
    case 51: year_data.val +=  0x2*tick_value; break;
    case 52: year_data.val +=  0x4*tick_value; break;
    case 53: year_data.val +=  0x8*tick_value; break;
    case 54: year_data.val += 0x10*tick_value; break;
    case 55: year_data.val += 0x20*tick_value; break;
    case 56: year_data.val += 0x20*tick_value; break;
    case 57: year_data.val += 0x80*tick_value;
    hamming_year_binning(&ybins, year_data); break;

    case 58: compute_max_year_index(&ybins);
    // fall through on purpose
    default: year_data.val = 0;
    }

    display_time(tick_value);

    if (now.second == 59) {
        now.prev_minute = now.minute;
    }

    return 0;

}

/*..........................................................................*/
void BSP_dispClear(void) {
    lcd_clear();
}

/*..........................................................................*/
void BSP_LEDPulse(uint8_t data) {

    if(data == 1)
    {
        LED_OFF(PWM_LED_G);
        LED_ON(PWM_LED_R);
    }
    else if(data == 0)
    {
        LED_OFF(PWM_LED_R);
        LED_ON(PWM_LED_G);
    } else {
        LED_ON(PWM_LED_R);
        LED_ON(PWM_LED_G);
    }
}
