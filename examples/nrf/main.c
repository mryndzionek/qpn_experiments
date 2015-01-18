
#include "config.h"
#include "mirf.h"
#include "nRF24L01.h"
#include "spi.h"
#include "lcd.h"

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define BUFFER_SIZE (10)
#define CHANNELS    (128)
#define SCALE       (128 / CHANNELS)
#define DELAY       (32)

const uint8_t PROGMEM minibars[][8] = {
        {
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b01110,
        },
        {
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b01110,
                0b01110,
        },
        {
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b01110,
                0b01110,
                0b01110,
        },
        {
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
        },
        {
                0b00000,
                0b00000,
                0b00000,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
        },
        {
                0b00000,
                0b00000,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
        },
        {
                0b00000,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
        },
        {
                0b01110,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
                0b01110,
        },
};


uint8_t channel[CHANNELS];
uint8_t buffer[BUFFER_SIZE];

void plot_minibars(int location, int strngth)
{
    if (strngth>7)
    {
        lcd_set_position(LCD_COLS+location);
        lcd_putchar(8);
        lcd_set_position(location);
        lcd_putchar(strngth-7);
    }
    else
    {
        lcd_set_position(LCD_COLS+location);
        lcd_putchar(strngth+1);
        lcd_set_position(location);
        lcd_putchar(32);
    }
}

void scanChannels(void)
{
    uint8_t v;
    uint8_t rfchannel = 0;

    for(rfchannel = 0; rfchannel < CHANNELS; rfchannel++) {
        mirf_config_register(RF_CH, rfchannel*SCALE);

        // Or focus on a specific channel
        //mirf_config_register(RF_CH, 2);

        mirf_CE_lo;

        mirf_CSN_lo; // Pull down chip select
        spi_fast_shift(FLUSH_RX);  // Write cmd to flush rx fifo
        mirf_CSN_hi; // Pull up chip select

        mirf_CE_hi; // Start listening

        int delaytime = 0;
        while (delaytime < DELAY) {
            delaytime++;
            mirf_read_register(CD, &v, 1);
            if (v == 1) {
                //PORTD ^= (1 << (PD5));
                channel[rfchannel]++;
            }
        }

    }
}

void outputChannels(void)
{
    uint8_t rfchannel = 0;

    for(rfchannel=0 ; rfchannel<CHANNELS ; rfchannel++)
    {
        uint8_t pos = (channel[rfchannel]<<4)>>5;

        if( pos==0 && channel[rfchannel]>0 ) pos++;
        if( pos>16 ) pos = 16;
        plot_minibars(rfchannel>>3, pos);
        channel[rfchannel] = 0;
    }
}

/*..........................................................................*/
int main(void)
{
    uint8_t v;

    DDRD |= (_BV(PD5) | _BV(PD0));

    lcd_init();

    for(v=1;v<=8;v++)
        lcd_customchar_P(v, minibars[v-1]);

    mirf_init();
    // Wait for mirf to come up
    _delay_ms(50);
    // Activate interrupts
    sei();
    // Configure mirf
    mirf_config();
    mirf_config_register(EN_AA, 0x0);
    //mirf_config_register(RF_SETUP, 0x0f);
    //mirf_config_register(SETUP_AW, 0x00);
    mirf_read_register(RF_CH, &v, 1);
    if(v == mirf_CH)
    {
        PORTD ^= (1 << (PD5));
        _delay_ms(100);
        PORTD ^= (1 << (PD5));
    }

    while(1) {

        RX_POWERUP;
        mirf_CSN_lo; // Pull down chip select
        spi_fast_shift(FLUSH_RX);  // Write cmd to flush rx fifo
        mirf_CSN_hi; // Pull up chip select
        mirf_CE_hi; // Start listening

        scanChannels();
        outputChannels();
        PORTD ^= (1 << (PD0));
    }
}
