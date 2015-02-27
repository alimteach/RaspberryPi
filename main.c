#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/twi.h>
#include <stdio.h>
#include <stdlib.h>

#define LINE_INT		(1 << PB6)
#define FRAME_INT		(1 << PB7)

#define LED_SDO			(1 << PC0)
#define LED_CLKR		(1 << PC1)
#define LED_LE			(1 << PC2)
#define LED_SDI			(1 << PC3)
#define LED_OE_N		(1 << PC7)

#define set(port,x) port |= (x)
#define clr(port,x) port &= ~(x)

extern void draw_loop(volatile uint8_t *data);
extern void clear_gain(void);
extern void delay(uint8_t ticks);

enum REG_ADDR {
	REG_ID = 192,
	REG_CFG_LOW,
	REG_CFG_HIGH,
};

typedef enum {
	DAT_LATCH = 22,
	CONF_WRITE = 20,
	CONF_READ  = 18,
	GAIN_WRITE = 16,
	GAIN_READ = 14,
	DET_OPEN = 13,
	DET_SHORT = 12,
	DET_OPEN_SHORT = 11,
	THERM_READ = 10,
} le_key;

volatile uint8_t intensity[192] = {
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,

	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,

	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,

	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,

	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,

	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,

	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,

	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
};

int main(void)
{
	PORTA = 0;
	PORTB = 0;
	PORTC = 0;
	PORTD = 0xFF;
	DDRB = FRAME_INT | LINE_INT;
	DDRC = LED_SDI | LED_CLKR | LED_LE | LED_OE_N;
	DDRD = 0xFF;

	TCCR0A = (1<<CS12);

	TWBR = 0xff;
	TWAR = 0x46 << 1;
	TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT) | (1 << TWIE);
	//clear_gain();
	sei();
	draw_loop(intensity);
	for(;;);
}