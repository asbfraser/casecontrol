#include <avr/io.h>
#include <util/delay.h>     /* for _delay_ms() */

#define SWITCH0	0
#define LED0	1
#define LED1	2

void set_led(int led, int value);

int __attribute__((noreturn)) main(void)
{
	DDRB &= ~(1 << SWITCH0);			/* Configure SWITCH0 as input */
	DDRB |= (1 << LED1) | (1 << LED0);		/* Configure LED0 and LED1 as output */

	while(1)
	{
		int pressed = (((PINB >> SWITCH0) & 1) == 0);	/* SWITCH0 is pulled low when pressed */

		if(pressed)
		{
			set_led(LED0, 1);
			set_led(LED1, 1);
		}
		else
		{
			set_led(LED0, 0);
			set_led(LED1, 0);
		}
		_delay_ms(50);
	}
}

void set_led(int led, int value)
{
	if(value > 0)
	{
		PORTB |= (1 << led);
	}
	else
	{
		PORTB &= ~(1 << led);
	}
}
