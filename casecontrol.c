#include <avr/io.h>
#include <util/delay.h>     /* for _delay_ms() */
#include <avr/interrupt.h>  /* for sei() */
#include <avr/power.h>  /* for clock_div_1 */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#define SWITCH0	0
#define LED0	1
#define LED1	2

#define CUSTOM_RQ_GET_STATUS	0
#define CUSTOM_RQ_SET_STATUS	1

void set_led(int led, uchar value);
uchar output = 1;

int __attribute__((noreturn)) main(void)
{
	cli();

        // run at full speed, because Trinket defaults to 8MHz for low voltage compatibility reasons
        clock_prescale_set(clock_div_1);

	/* Setup USB pins */
	DDRB &= ~(_BV(USB_CFG_DMINUS_BIT));		/* Required by v-usb */
	PORTB &= ~(_BV(USB_CFG_DMINUS_BIT));		/* Required by v-usb */
	DDRB &= ~(_BV(USB_CFG_DPLUS_BIT));		/* Required by v-usb */
	PORTB &= ~(_BV(USB_CFG_DPLUS_BIT));		/* Required by v-usb */

	/* Setup switch and LED pins */
	DDRB &= ~(_BV(SWITCH0));			/* Configure SWITCH0 as input */
	DDRB |= _BV(LED1) | _BV(LED0);		/* Configure LED0 and LED1 as output */

	usbDeviceDisconnect();
	_delay_ms(250);
	usbDeviceConnect();

	usbInit();
	sei();

	while(1)
	{
		usbPoll();

		if(usbInterruptIsReady())
		{
			if(((PINB >> SWITCH0) & 1) == 0) /* Switch is pressed */
			{
				usbSetInterrupt((void *) &output, 1);
			}
		}
	}
}

void set_led(int led, uchar value)
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

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t *rq = (void *)data;
	static uchar dataBuffer[3];  /* buffer must stay valid when usbFunctionSetup returns */

	if(rq->bRequest == CUSTOM_RQ_SET_STATUS)
	{
		set_led(LED0, rq->wValue.bytes[0]); /* Set LED0 */
		set_led(LED1, rq->wValue.bytes[1]); /* Set LED1 */
	}
	else if(rq->bRequest != CUSTOM_RQ_GET_STATUS)
	{
		return 0;   /* default for not implemented requests: return no data back to host */
	}

	/* Always return status */

	dataBuffer[0] = (((PINB >> SWITCH0) & 1) == 0) ? 1 : 0;
	dataBuffer[1] = (PORTB >> LED0) & 1;
	dataBuffer[2] = (PORTB >> LED1) & 1;
	usbMsgPtr = (usbMsgPtr_t) dataBuffer;         /* tell the driver which data to return */
	return 3;
}

#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__)
/* ------------------------------------------------------------------------- */
/* ------------------------ Oscillator Calibration ------------------------- */
/* ------------------------------------------------------------------------- */
// section copied from EasyLogger
/* Calibrate the RC oscillator to 8.25 MHz. The core clock of 16.5 MHz is
 * derived from the 66 MHz peripheral clock by dividing. Our timing reference
 * is the Start Of Frame signal (a single SE0 bit) available immediately after
 * a USB RESET. We first do a binary search for the OSCCAL value and then
 * optimize this value with a neighboorhod search.
 * This algorithm may also be used to calibrate the RC oscillator directly to
 * 12 MHz (no PLL involved, can therefore be used on almost ALL AVRs), but this
 * is wide outside the spec for the OSCCAL value and the required precision for
 * the 12 MHz clock! Use the RC oscillator calibrated to 12 MHz for
 * experimental purposes only!
 */
void calibrateOscillator(void)
{
    uchar       step = 128;
    uchar       trialValue = 0, optimumValue;
    int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);

    /* do a binary search: */
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    /* proportional to current real frequency */
        if(x < targetValue)             /* frequency still too low */
            trialValue += step;
        step >>= 1;
    }while(step > 0);
    /* We have a precision of +/- 1 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; /* this is certainly far away from optimum */
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
            x = -x;
        if(x < optimumDev){
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;
}
/*
Note: This calibration algorithm may try OSCCAL values of up to 192 even if
the optimum value is far below 192. It may therefore exceed the allowed clock
frequency of the CPU in low voltage designs!
You may replace this search algorithm with any other algorithm you like if
you have additional constraints such as a maximum CPU clock.
For version 5.x RC oscillators (those with a split range of 2x128 steps, e.g.
ATTiny25, ATTiny45, ATTiny85), it may be useful to search for the optimum in
both regions.
*/
#endif
