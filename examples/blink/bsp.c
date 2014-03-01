#include "qpn_port.h"
#include "bsp.h"
#include <avr/io.h>                                              /* AVR I/O */

#define LED_ON(num_)       (PORTD &= ~(1 << (num_)))
#define LED_OFF(num_)      (PORTD |= (1 << (num_)))
#define LED_ON_ALL()       (PORTD = 0x00)
#define LED_OFF_ALL()      (PORTD = 0xFF)

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
	LED_ON_ALL();                                     /* turn off all LEDs */
}
/*..........................................................................*/
void QF_onStartup(void) {
	/* set Timer2 in CTC mode, 1/1024 prescaler, start the timer ticking */
	TIMSK = (1 << OCIE1A);
	TCCR1B = (1 << WGM12);
	TCCR1B |= (1 << CS12) | (1 << CS10);
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
void BSP_ledOn(void) {
	LED_ON(0);
}
/*..........................................................................*/
void BSP_ledOff(void) {
	LED_OFF(0);
}
