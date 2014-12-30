/*
 * lcd_font.c
 *
 *  Created on: 30 gru 2014
 *      Author: mr_gogul
 */

#include <stdint.h>
#include <avr/pgmspace.h>

#include "lcd.h"
#include "lcd_font.h"

const uint8_t PROGMEM bar1[] = {
        0b11100,
        0b11110,
        0b11110,
        0b11110,
        0b11110,
        0b11110,
        0b11110,
        0b11100
};

const uint8_t PROGMEM bar2[] = {
        0b00111,
        0b01111,
        0b01111,
        0b01111,
        0b01111,
        0b01111,
        0b01111,
        0b00111
};

const uint8_t PROGMEM bar3[] = {
        0b11111,
        0b11111,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b11111,
        0b11111
};

const uint8_t PROGMEM bar4[] = {
        0b11110,
        0b11100,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b11000,
        0b11100
};

const uint8_t PROGMEM bar5[] = {
        0b01111,
        0b00111,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00011,
        0b00111
};

const uint8_t PROGMEM bar6[] = {
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b11111,
        0b11111
};

const uint8_t PROGMEM bar7[] = {
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00111,
        0b01111
};

const uint8_t PROGMEM bar8[] = {
        0b11111,
        0b11111,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000
};

static void custom0(int col)
{ // uses segments to build the number 0
    lcd_set_position(col);
    lcd_putchar(2);
    lcd_putchar(8);
    lcd_putchar(1);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(2);
    lcd_putchar(6);
    lcd_putchar(1);
}

static void custom1(int col)
{
    lcd_set_position(col);
    lcd_putchar(32);
    lcd_putchar(32);
    lcd_putchar(1);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(32);
    lcd_putchar(32);
    lcd_putchar(1);
}

static void custom2(int col)
{
    lcd_set_position(col);
    lcd_putchar(5);
    lcd_putchar(3);
    lcd_putchar(1);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(2);
    lcd_putchar(6);
    lcd_putchar(6);
}

static void custom3(int col)
{
    lcd_set_position(col);
    lcd_putchar(5);
    lcd_putchar(3);
    lcd_putchar(1);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(7);
    lcd_putchar(6);
    lcd_putchar(1);
}

static void custom4(int col)
{
    lcd_set_position(col);
    lcd_putchar(2);
    lcd_putchar(6);
    lcd_putchar(1);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(32);
    lcd_putchar(32);
    lcd_putchar(1);
}

static void custom5(int col)
{
    lcd_set_position(col);
    lcd_putchar(2);
    lcd_putchar(3);
    lcd_putchar(4);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(7);
    lcd_putchar(6);
    lcd_putchar(1);
}

static void custom6(int col)
{
    lcd_set_position(col);
    lcd_putchar(2);
    lcd_putchar(3);
    lcd_putchar(4);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(2);
    lcd_putchar(6);
    lcd_putchar(1);
}

static void custom7(int col)
{
    lcd_set_position(col);
    lcd_putchar(2);
    lcd_putchar(8);
    lcd_putchar(1);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(32);
    lcd_putchar(32);
    lcd_putchar(1);
}

static void custom8(int col)
{
    lcd_set_position(col);
    lcd_putchar(2);
    lcd_putchar(3);
    lcd_putchar(1);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(2);
    lcd_putchar(6);
    lcd_putchar(1);
}

static void custom9(int col)
{
    lcd_set_position(col);
    lcd_putchar(2);
    lcd_putchar(3);
    lcd_putchar(1);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(7);
    lcd_putchar(6);
    lcd_putchar(1);
}

static void custom_undefined(int col)
{
    lcd_set_position(col);
    lcd_putchar(2);
    lcd_putchar(6);
    lcd_putchar(1);
    lcd_set_position(LCD_COLS+col);
    lcd_putchar(2);
    lcd_putchar(32);
    lcd_putchar(1);
}

void lcd_font_init()
{
    lcd_customchar_P(1, bar1);
    lcd_customchar_P(2, bar2);
    lcd_customchar_P(3, bar3);
    lcd_customchar_P(4, bar4);
    lcd_customchar_P(5, bar5);
    lcd_customchar_P(6, bar6);
    lcd_customchar_P(7, bar7);
    lcd_customchar_P(8, bar8);
}

void lcd_font_num(int value, int col) {
    switch(value)
    {
    case 0:
        custom0(col);
        break;

    case 1:
        custom1(col);
        break;

    case 2:
        custom2(col);
        break;

    case 3:
        custom3(col);
        break;

    case 4:
        custom4(col);
        break;

    case 5:
        custom5(col);
        break;

    case 6:
        custom6(col);
        break;

    case 7:
        custom7(col);
        break;

    case 8:
        custom8(col);
        break;

    case 9:
        custom9(col);
        break;

    default:
        custom_undefined(col);
        break;
    }

}
