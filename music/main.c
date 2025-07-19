#include <avr/io.h>
#include <util/delay.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
//#include <util/atomic.h>

#include <avr/wdt.h>
#include <avr/power.h>

#define TONE_LENGTH1 2310
#define TONE_LENGTH2 1650
#define GAP1 900
#define GAP2 100

#include "notes___.h"

uint32_t dds1=0, dds2=0;
uint32_t dds1_add=0, dds2_add=0;
uint32_t i=0;
uint8_t repeat = 0;
uint32_t j=0;

uint8_t env1[32]={10, 30, 32, 40, 40, 40, 40, 40, 40, 30, 25, 22, 20, 19, 18, 17, 16, 0};
uint8_t env2[37]={6,18,16,15,14,13,12,11,10,10, 10,10,10,10,10,10,10,10,10,10, 10,10,10,10,10,10,10,10,10,10, 8,7,6,5,3,1,0};
uint8_t pattl = 64;
uint8_t current_pattern = 0;
#define NUM_PATTERNS 36
uint8_t patt[NUM_PATTERNS] = {0, 0, 1, 1, 2, 1, 3, 1, 3, 4, 5, 5, 6, 3, 1, 1, 0, 0, 1, 1, 2, 1, 3, 1, 1, 0, 0, 1, 1 , 2, 1, 3, 1, 1, 0, 0};

uint8_t par = 0;

// -----------------------------
// |          RAND             |
// -----------------------------
#define FASTLED_RAND16_2053  ((uint16_t)(2053))
#define FASTLED_RAND16_13849 ((uint16_t)(13849))
uint16_t rand16seed;

uint8_t random8()
{
  rand16seed = (rand16seed * FASTLED_RAND16_2053) + FASTLED_RAND16_13849;
  return (uint8_t)(((uint8_t)(rand16seed & 0xFF)) +
                   ((uint8_t)(rand16seed >> 8)));
}

uint16_t random16()
{
  rand16seed = (rand16seed * FASTLED_RAND16_2053) + FASTLED_RAND16_13849;
  return rand16seed;
}

void timerInit(void)
{
  cli();
  OCR1A=1000; // Counting up to 1000, generates 32000 interrupts / sec
  //OCR0B=250; // Counting up to 250, generates 64000 interrupts / sec
	TCCR1A=0;
  //TCCR0B=_BV(CS01) | _BV(CS00);
  //TCCR0B=_BV(CS00); // No prescaler
  TCCR1B = _BV(CS10) | _BV(WGM12); // no prescaler, CTC mode
  sei();
}

void timerStart(void)
{
  cli();
  TCNT1=0;
  //TIMSK0 |= _BV(TOIE1);
  TIMSK1 |= _BV(OCIE1A);
  sei();
}

void timerStop(void)
{
  cli();
  TIMSK0 &= ~_BV(TOIE1);
  sei();
}

void dds()
{
  static uint16_t noise;
  uint8_t x;
  uint8_t y;
  static uint8_t z;
  if (j<TONE_LENGTH1-GAP1)
  {
    dds1+=dds1_add;
  }
  if (j<TONE_LENGTH1-GAP2)
  {
  	dds2+=dds2_add;
  }
  //x=((dds1>>(31)) ^ (dds1>>29)) & 1;
  //x=(dds1>>31)&1;
  x=((dds1>>(31-4))>(j>>6));
  y=(dds2>>29)&1 + ((dds2>>30)&1)*3;
  //digitalWrite(SOUND_PIN1, x);
  //digitalWrite(SOUND_PIN2, y);
  //OCR0A=(x+y)*64+(i%2?random8()&63:0);
  if (noise & 0x08)
  {
    //if (j<2000 && !((j>>1) & (1<<(noise&7))))
    if (j<((noise & 0xf0) << 3) && !((j<<1) & (1<<(noise&7))))
    {
      z=(uint8_t)((uint16_t)((TONE_LENGTH1-j)*random8())>>9);
    }
  }
  else
  {
    z=0;
  }
  //OCR0A=128+x*60+y*40-z;
  //if ((x>20)^y)
  //if (x ^ y)
	//{
	//	PORTB |= (1<<PB7);
	//}
	//else
	//{
	//	PORTB &= ~(1<<PB7);
	//}
  //if (z > 31)
	//{
	//	PORTB |= (1<<PB1);
	//}
	//else
	//{
	//	PORTB &= ~(1<<PB1);
	//}

  OCR0A=x*env1[j>>6]+y*env2[j>>6]+z;
  j++;
  if (par == 0 && j==TONE_LENGTH1 || par == 1 && j==TONE_LENGTH2)
  {
    par = 1 - par;
    j=0;
    i++;
    if (i % pattl == 0) {
        current_pattern++;
        current_pattern = current_pattern % NUM_PATTERNS;
        i = patt[current_pattern] * pattl;
    }
    int16_t main_note = pgm_read_word(&notes[i][0]);
//    if (pgm_read_word(&notes[i][0])==-2)
//    {
//      //Timer1.detachInterrupt();
//      if (repeat == 0) {
//          i-=64;
//          repeat = 1;
//      } else {
//          i++;
//          repeat = 0;
//      }
//    }
    if (main_note == -1)
    {
      //Timer1.detachInterrupt();
      i=0;
      main_note = pgm_read_word(&notes[i][0]);
    }
    uint32_t foo=pgm_read_word(&notes[i][0])*134213;
    if (main_note > 0)
    {
        dds1_add=foo;
    } else if (main_note == -2) {
        dds1_add=0;
    }
    //if (dds1_add==0)
    //{
    //  dds1=-1;
    //}
    dds2_add=pgm_read_word(&notes[i][1])*134213;
    noise=pgm_read_word(&notes[i][2]);
  }
	
}

ISR(TIMER1_COMPA_vect) //64 kHz
{
	dds();
}

void pwmInit()
{
  DDRB = _BV(PB7); //PORTB7 is output (OC0A / OC1C)
  TCCR0A = _BV(COM0A1) | _BV(WGM00) | _BV(WGM01);
  TCCR0B = _BV(CS00) ;// | _BV(WGM02);
}

int main(void)
{
	cli();
  USBCON = 0;  // Disable USB interrupts
  MCUSR &= ~(1 << WDRF);
	wdt_disable();
	clock_prescale_set(clock_div_1);
	
  dds1_add=pgm_read_word(&notes[i][0])*134213;
  dds2_add=pgm_read_word(&notes[i][1])*134213;

	DDRB |= (1<<PB1); //PORTB1 is output
	DDRB |= (1<<PB2); //PORTB2 is output

	pwmInit();
	timerInit();
	timerStart();

	while(1)
	{
		//_delay_us(32);
		//dds();
		//PORTB |= (1<<PB2);
		//_delay_us(2000);
		//PORTB &= ~(1<<PB2);
		//_delay_us(2000);
		//_delay_us(20);
	}
}

