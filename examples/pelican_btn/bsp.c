#include "qpn_port.h"
#include "bsp.h"
#include <avr/io.h>                                              /* AVR I/O */
#include <stdlib.h>
#include <stdio.h>
#include "lcd.h"
#include "pelican_btn.h"

#define LED_OFF(num_)       (PORTD &= ~(1 << (num_)))
#define LED_ON(num_)        (PORTD |= (1 << (num_)))
#define LED_TOGGLE(num_)    (PORTD ^= (1 << (num_)))
#define LED_OFF_ALL()       (PORTD &= (_BV(PD2) | _BV(PD3)))
#define LED_ON_ALL()        (PORTD |= ~(_BV(PD2) | _BV(PD3)))
#define LCD_BL_ON()			LED_ON(5)
#define LCD_BL_OFF()		LED_OFF(5)

#ifndef NDEBUG
Q_DEFINE_THIS_FILE
#endif

static volatile uint8_t press = 0;
static volatile uint8_t detect = 0;

/*..........................................................................*/
ISR(INT0_vect)
{
	if(!press)
	{
		QActive_postISR((QActive *)&AO_Ped, BTN1_SIG, 0);
		press++;
	}
}
/*..........................................................................*/
ISR(INT1_vect)
{

}
/*..........................................................................*/
ISR(TIMER1_COMPA_vect) {
	/* No need to clear the interrupt source since the Timer1 compare
	 * interrupt is automatically cleared in hardware when the ISR runs.
	 */
	QK_ISR_ENTRY();                     /* inform QK kernel about ISR entry */
	if(!detect)
	{
		if(press)
			detect = 1;
	} else {
		detect = 0;
		press = 0;
	}
	QF_tick();

	QK_ISR_EXIT();                       /* inform QK kernel about ISR exit */
}
/*..........................................................................*/
void BSP_init(void) {
	DDRD = 0xFF & ~(_BV(PD2) | _BV(PD3));
	PORTD = _BV(PD2) | _BV(PD3);						         /* pullups */
	LED_OFF_ALL();                                     /* turn off all LEDs */
	lcd_init();
	LCD_BL_ON();                                   /* turn LCD backlight on */
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
	/* on low level */
}
/*..........................................................................*/
void QK_onIdle(void) {        /* entered with interrupts LOCKED, see NOTE01 */

	QF_INT_DISABLE();
	LED_ON(6);
	LED_OFF(6);
	QF_INT_ENABLE();

#ifdef NDEBUG

	MCUCR = (0 << SM0) | (1 << SE);/*idle sleep mode, adjust to your project */

	/* never separate the following two assembly instructions, see NOTE03 */
	__asm__ __volatile__ ("sei" "\n\t" :: );
	__asm__ __volatile__ ("sleep" "\n\t" :: );

	MCUCR = 0;                                           /* clear the SE bit */
#endif

}
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
	char buff[16];
	(void)file;                                   /* avoid compiler warning */
	(void)line;                                   /* avoid compiler warning */
	QF_INT_DISABLE();
	LED_ON_ALL();                                            /* all LEDs on */
	lcd_clear();
	lcd_set_line(0);
	snprintf (buff, sizeof(buff), "file: %d", press);
	lcd_putstr(buff);
	lcd_set_line(1);
	snprintf (buff, sizeof(buff), "line: %d", line);
	lcd_putstr(buff);
	for (;;) {       /* NOTE: replace the loop with reset for final version */
	}
}
/*..........................................................................*/
void BSP_signalCars(enum BSP_CarsSignal sig) {
	switch (sig) {
	case CARS_RED:
		LED_ON(0);
		LED_OFF(1);
		LED_OFF(2);
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
		LCD_BL_ON();
		lcd_set_line(0);
		lcd_putstr("***DON'T WALK***");
		break;
	case PEDS_BLANK:
		lcd_set_line(0);
		lcd_putstr("                ");
		LCD_BL_OFF();
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
