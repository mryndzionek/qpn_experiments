#include "qpn_port.h"
#include "bsp.h"
#include <avr/io.h>                                              /* AVR I/O */
#include "lcd.h"

#define LED_OFF(num_)       (PORTD &= ~_BV(num_))
#define LED_ON(num_)        (PORTD |= _BV(num_))
#define LED_TOGGLE(num_)    (PORTD ^= _BV(num_))
#define LED_OFF_ALL()       (PORTD = 0x00)
#define LED_ON_ALL()        (PORTD = 0xFF)
#define LCD_BL_ON()			LED_ON(5)
#define LCD_BL_OFF()		LED_OFF(5)

/*..........................................................................*/
ISR(TIMER1_COMPA_vect) {
	/* No need to clear the interrupt source since the Timer1 compare
	 * interrupt is automatically cleared in hardware when the ISR runs.
	 */
	QF_tick();
}
/*..........................................................................*/
void BSP_init(void) {
	DDRD  = 0xFF;                    /* All PORTD pins are outputs for LEDs */
	LED_OFF_ALL();                                     /* turn off all LEDs */
	lcd_init();
	LCD_BL_ON();                                   /* turn LCD backlight on */
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

	LED_ON(6);
	LED_OFF(6);

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
	(void)file;                                   /* avoid compiler warning */
	(void)line;                                   /* avoid compiler warning */
	QF_INT_DISABLE();
	LED_ON_ALL();                                            /* all LEDs on */
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
		break;
	}
}
/*..........................................................................*/
void BSP_signalPeds(enum BSP_PedsSignal sig) {
	switch (sig) {
	case PEDS_DONT_WALK:
		LCD_BL_ON();
		lcd_set_line(0);
		lcd_putstr("***DON'T WALK***");
		break;
	case PEDS_BLANK:
		break;
	case PEDS_WALK:
		lcd_set_line(0);
		lcd_putstr("*** WALK NOW ***");
		break;
	}
}

void BSP_flashPeds(void) {
	LED_TOGGLE(5);
}
