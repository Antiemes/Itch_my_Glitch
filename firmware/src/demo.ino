#include <avr/pgmspace.h>

#include "../../gfx/nem.h"

#define PLASMA_LEN 600

// -----------------------------
// |          FAST SIN/COS     |
// -----------------------------
int8_t sinTable[17];
uint16_t fct=0;

int8_t fs(uint8_t a) //Full wave: 0..63, values: -127..127
{
  uint8_t n;
  a&=63;
  n=(a>=32);
  a&=31;
  a=(a<=16)?a:32-a;
  return n?-sinTable[a]:sinTable[a];
}

int8_t fc(uint8_t a)
{
  uint8_t n;
  a+=16;
  a&=63;
  n=(a>=32);
  a&=31;
  a=(a<=16)?a:32-a;
  return n?-sinTable[a]:sinTable[a];
}


// -----------------------------
// |          RAND             |
// -----------------------------
#define FASTLED_RAND16_2053  ((uint16_t)(2053))
#define FASTLED_RAND16_13849 ((uint16_t)(13849))
uint16_t rand16seed;

uint8_t random8()
{
  rand16seed = (rand16seed * FASTLED_RAND16_2053) + FASTLED_RAND16_13849;
  // return the sum of the high and low bytes, for better
  //  mixing and non-sequential correlation
  return (uint8_t)(((uint8_t)(rand16seed & 0xFF)) +
                   ((uint8_t)(rand16seed >> 8)));
}

uint16_t random16()
{
  rand16seed = (rand16seed * FASTLED_RAND16_2053) + FASTLED_RAND16_13849;
  return rand16seed;
}


// some globals
static byte oled_addr;
static void oledWriteCommand(unsigned char c);

#define DIRECT_PORT
#define I2CPORT PORTB
// A bit set to 1 in the DDR is an output, 0 is an INPUT
#define I2CDDR DDRB

// Pin or port numbers for SDA and SCL
#define BB_SDA 0
#define BB_SCL 2

#define I2C_CLK_LOW() I2CPORT &= ~(1 << BB_SCL) //compiles to cbi instruction taking 2 clock cycles, extending the clock pulse

//
// Transmit a byte and ack bit
//
static inline void i2cByteOut(byte b)
{
byte i;
byte bOld = I2CPORT & ~((1 << BB_SDA) | (1 << BB_SCL));
     for (i=0; i<8; i++)
     {
         bOld &= ~(1 << BB_SDA);
         if (b & 0x80)
            bOld |= (1 << BB_SDA);
         I2CPORT = bOld;
         I2CPORT |= (1 << BB_SCL);
         I2C_CLK_LOW();
         b <<= 1;
     } // for i
// ack bit
  I2CPORT = bOld & ~(1 << BB_SDA); // set data low
  I2CPORT |= (1 << BB_SCL); // toggle clock
  I2C_CLK_LOW();
} /* i2cByteOut() */

void i2cBegin(byte addr)
{
   I2CPORT |= ((1 << BB_SDA) + (1 << BB_SCL));
   I2CDDR |= ((1 << BB_SDA) + (1 << BB_SCL));
   I2CPORT &= ~(1 << BB_SDA); // data line low first
   I2CPORT &= ~(1 << BB_SCL); // then clock line low is a START signal
   i2cByteOut(addr << 1); // send the slave address
} /* i2cBegin() */

void i2cWrite(byte *pData, byte bLen)
{
byte i, b;
byte bOld = I2CPORT & ~((1 << BB_SDA) | (1 << BB_SCL));

   while (bLen--)
   {
      b = *pData++;
      if (b == 0 || b == 0xff) // special case can save time
      {
         bOld &= ~(1 << BB_SDA);
         if (b & 0x80)
            bOld |= (1 << BB_SDA);
         I2CPORT = bOld;
         for (i=0; i<8; i++)
         {
            I2CPORT |= (1 << BB_SCL); // just toggle SCL, SDA stays the same
            I2C_CLK_LOW();
         } // for i    
     }
     else // normal byte needs every bit tested
     {
        for (i=0; i<8; i++)
        {
          
         bOld &= ~(1 << BB_SDA);
         if (b & 0x80)
            bOld |= (1 << BB_SDA);

         I2CPORT = bOld;
         I2CPORT |= (1 << BB_SCL);
         I2C_CLK_LOW();
         b <<= 1;
        } // for i
     }
// ACK bit seems to need to be set to 0, but SDA line doesn't need to be tri-state
      I2CPORT &= ~(1 << BB_SDA);
      I2CPORT |= (1 << BB_SCL); // toggle clock
      I2CPORT &= ~(1 << BB_SCL);
   } // for each byte
} /* i2cWrite() */

//
// Send I2C STOP condition
//
void i2cEnd()
{
  I2CPORT &= ~(1 << BB_SDA);
  I2CPORT |= (1 << BB_SCL);
  I2CPORT |= (1 << BB_SDA);
  I2CDDR &= ~((1 << BB_SDA) | (1 << BB_SCL)); // let the lines float (tri-state)
} /* i2cEnd() */

// Wrapper function to write I2C data on Arduino
static void I2CWrite(int iAddr, unsigned char *pData, int iLen)
{
  i2cBegin(oled_addr);
  i2cWrite(pData, iLen);
  i2cEnd();
} /* I2CWrite() */

//
// Initializes the OLED controller into "page mode"
//
void oledInit(byte bAddr, int bFlip, int bInvert)
{
unsigned char uc[4];
unsigned char oled_initbuf[]={0x00,0xae,0xa8,0x3f,0xd3,0x00,0x40,0xa1,0xc8,
      0xda,0x12,0x81,0xff,0xa4,0xa6,0xd5,0x80,0x8d,0x14,
      0xaf,0x20,0x02};

  oled_addr = bAddr;
  I2CDDR &= ~(1 << BB_SDA);
  I2CDDR &= ~(1 << BB_SCL); // let them float high
  I2CPORT |= (1 << BB_SDA); // set both lines to get pulled up
  I2CPORT |= (1 << BB_SCL);
  
  I2CWrite(oled_addr, oled_initbuf, sizeof(oled_initbuf));
  if (bInvert)
  {
    uc[0] = 0; // command
    uc[1] = 0xa7; // invert command
    I2CWrite(oled_addr, uc, 2);
  }
  if (bFlip) // rotate display 180
  {
    uc[0] = 0; // command
    uc[1] = 0xa0;
    I2CWrite(oled_addr, uc, 2);
    uc[1] = 0xc0;
    I2CWrite(oled_addr, uc, 2);
  }
} /* oledInit() */
//
// Sends a command to turn off the OLED display
//
void oledShutdown()
{
    oledWriteCommand(0xaE); // turn off OLED
}

// Send a single byte command to the OLED controller
static void oledWriteCommand(unsigned char c)
{
unsigned char buf[2];

  buf[0] = 0x00; // command introducer
  buf[1] = c;
  I2CWrite(oled_addr, buf, 2);
} /* oledWriteCommand() */

static void oledWriteCommand2(unsigned char c, unsigned char d)
{
unsigned char buf[3];

  buf[0] = 0x00;
  buf[1] = c;
  buf[2] = d;
  I2CWrite(oled_addr, buf, 3);
} /* oledWriteCommand2() */

//
// Sets the brightness (0=off, 255=brightest)
//
void oledSetContrast(unsigned char ucContrast)
{
  oledWriteCommand2(0x81, ucContrast);
} /* oledSetContrast() */

//
// Send commands to position the "cursor" (aka memory write address)
// to the given row and column
//
static void oledSetPosition(int x, int y)
{
  oledWriteCommand(0xb0 | y); // go to page Y
  oledWriteCommand(0x00 | (x & 0xf)); // // lower col addr
  oledWriteCommand(0x10 | ((x >> 4) & 0xf)); // upper col addr
}

//
// Write a block of pixel data to the OLED
// Length can be anything from 1 to 1024 (whole display)
//
static void oledWriteDataBlock(unsigned char *ucBuf, int iLen)
{
unsigned char ucTemp[21];

  ucTemp[0] = 0x40; // data command
  memcpy(&ucTemp[1], ucBuf, iLen);
  I2CWrite(oled_addr, ucTemp, iLen+1);
  // Keep a copy in local buffer
}

//
// Fill the frame buffer with a byte pattern
// e.g. all off (0x00) or all on (0xff)
//
void oledFill(unsigned char ucData)
{
int x, y;
unsigned char temp[8];

  memset(temp, ucData, 8);
  for (y=0; y<8; y++)
  {
    oledSetPosition(2,y); // set to (0,Y)
    for (x=0; x<16; x++)
    {
      oledWriteDataBlock(temp, 8); 
    } // for x
  } // for y

} /* oledFill() */

// -----------------------------
// |          PLASMA           |
// -----------------------------
void plasma()
{
  static uint16_t psp=0;
  static uint8_t pbias=255;
  static uint8_t pbias2=0;
  static uint8_t a=0;
  int16_t x, y;
  int16_t v1, v2;
  uint16_t p4;
  uint8_t bits;
  uint16_t rnd;
  int16_t x2, x3, y2, y3;

  uint8_t temp[8];

/*
  memset(temp, ucData, 8);
  for (y=0; y<8; y++)
  {
    oledSetPosition(2,y); // set to (0,Y)
    for (x=0; x<16; x++)
    {
      oledWriteDataBlock(temp, 8); 

*/

  rand16seed=fct;
  for (y=0; y<64; y+=8)
  {
    oledSetPosition(2, y >> 3);
    for (x=0; x<128; x+=4)
    {
      x2=((int16_t)fc(a)*x-(int16_t)fs(a)*(y+4))/128;
      x3=((int16_t)fc(a)*x-(int16_t)fs(a)*y)/128;
      y2=((int16_t)fs(a)*x+(int16_t)fc(a)*(y+4))/128;
      y3=((int16_t)fs(a)*x+(int16_t)fc(a)*(y))/128;
      v1=((int16_t)fs(x2*0.7+psp*0.5+fs(y2-9)/11)+fs(y2+psp+fs(x2+41)/8))*3/2;
      v2=((int16_t)fs(x3*0.7+psp*0.5+fs(y3-9)/11)+fs(y3+psp+fs(x3+41)/8))*3/2;
    
      if (v1<0)
      {
        v1=-v1;
      }
      if (v2<0)
      {
        v2=-v2;
      }
      v1+=pbias;
      v2+=pbias;
      if (v1>255)
      {
        v1=255;
      }
      v1-=pbias2;
      if (v1<0)
      {
        v1=0;
      }
      if (v2>255)
      {
        v2=255;
      }
      v2-=pbias2;
      if (v2<0)
      {
        v2=0;
      }
      for (p4=0; p4<4; p4++)
      {
        rnd=random16();
        bits=(v1<=(rnd&0xff));
        bits<<=1;
        bits|=(v1<=(rnd>>8));
        bits<<=1;
        rnd=random16();
        bits|=(v1<=(rnd&0xff));
        bits<<=1;
        bits|=(v1<=(rnd>>8));

        rnd=random16();
        bits<<=1;
        bits|=(v2<=(rnd&0xff));
        bits<<=1;
        bits|=(v2<=(rnd>>8));
        bits<<=1;
        bits|=(v2<=(rnd&0xff));
        bits<<=1;
        bits|=(v2<=(rnd>>8));

        temp[p4]=bits;
      }
      oledWriteDataBlock(temp, 4);
    }
  }
  psp++;
  a++;
  if (psp<85)
  {
    pbias-=3;
  }
  else if (psp>(PLASMA_LEN-255/3) && pbias2<255)
  {
    pbias2+=3;
  }
}

void setup()
{
  // put your setup code here, to run once:
  delay(200); // wait for the OLED to fully power up
  oledInit(0x3c, 0, 0);
  uint8_t i;
  for (i=0; i<=16; i++)
  {
    sinTable[i]=sin(i/16.0*M_PI/2)*127;
  }
}

void nem_eff()
{
  int16_t of = 0;
  rand16seed=fct;
  uint8_t x, y;
  uint8_t buf[16];
  uint16_t ro=random16();
  if (ro < 4000)
  {
    of = ro % 71 - 35;
  }
  for (y=0; y<8; y++)
  {
    uint16_t rr=random16();
    oledSetPosition(2 + (rr < 20000) + (rr < 40000) + (rr < 50000), y);
    for (x=0; x<128; x++)
    {
      uint8_t p;
      uint8_t b = 0;
      uint8_t bp;
      bp = x & 7;
      uint8_t ys;
      buf[0] = 0;
      for (ys=0; ys<8; ys++)
      {
        p = pgm_read_byte(nem + of + y * 128 + ys * 16 + x / 8);
        buf[0] |= (p & (1<<bp)) ? (1 << ys) : 0;
      }
      uint16_t rnd=random16();
      buf[0] &= ((rnd&0xff) & (rnd >> 8));
      if (rnd > 10000) oledWriteDataBlock(buf, 1);
    }
  }
}


void wtf()
{
  uint8_t x1 = 30, y1 = 6, x2 = 102, y2 = 40;

  uint8_t x, ym;
  uint8_t buf[16];

  for (ym=0; ym<8; ym++)
  {
    uint16_t rr=random16();
    oledSetPosition(2, ym);
    for (x=0; x<128; x++)
    {
      uint8_t p;
      uint8_t b = 0;
      uint8_t bp;
      bp = x & 7;
      uint8_t ys;
      buf[0] = 0;
      for (ys=0; ys<8; ys++)
      {
        uint8_t y = ym * 8 + ys;

        int16_t dsq = (x - x1) * (x - x1) + (y - y1) * (y - y1)
                    + (x - x2) * (x - x2) + (y - y2) * (y - y2);
        dsq -= 300;
        if (dsq < 0)
        {
          dsq = -dsq;
        }
        dsq %= (fct + 3);
        if (dsq < 10)
        {
          dsq = 1;
        }
        else
        {
          dsq = 0;
        }

        buf[0] |= (dsq) ? (1 << ys) : 0;
      }
      oledWriteDataBlock(buf, 1);
    }
  }


}

uint16_t nem_ctr = 0;
void loop()
{
//  uint16_t rnd=random16();
//  if (rnd<2000)
//  {
//    nem_ctr = rnd / 1024 + 10;
//  }
//  if (nem_ctr)
//  {
//    nem_eff();
//    nem_ctr--;
//  }
//  else
//  {
//    plasma();
//  }
  wtf();
  fct++;

}

