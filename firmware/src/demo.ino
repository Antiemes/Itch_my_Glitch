#include <avr/pgmspace.h>

#include "../../gfx/gfx_this.h"
#include "../../gfx/gfx_no.h"
#include "../../gfx/gfx_d.h"
#include "../../gfx/gfx_get.h"
#include "../../gfx/gfx_start.h"

#include "../../gfx/gfx_ohno.h"
#include "../../gfx/gfx_plasma.h"
#include "../../gfx/gfx_blob.h"
#include "../../gfx/gfx_plasmaback.h"
#include "../../gfx/gfx_never.h"


#define PLASMA_LEN 600

// -----------------------------
// |          FAST SIN/COS     |
// -----------------------------
int8_t sinTable[33];
static const uint8_t isq[] PROGMEM = {

255, 248, 241, 234, 228, 223, 218, 213, 209, 204, 200, 197, 193, 190, 186, 183, 181, 178, 175, 173, 170, 168, 166, 163, 161, 159, 158, 156, 154, 152, 150, 149, 147, 146, 144, 143, 142, 140, 139, 138, 136, 135, 134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120, 119, 119, 118, 117, 116, 115, 115, 114, 113, 113, 112, 111, 111, 110, 109, 109, 108, 107, 107, 106, 106, 105, 105, 104, 103, 103, 102, 102, 101, 101, 100, 100, 99, 99, 98, 98, 98, 97, 97, 96, 96, 95, 95, 95, 94, 94, 93, 93, 93, 92, 92, 91, 91, 91, 90, 90, 90, 89, 89, 89, 88, 88, 88, 87, 87, 87, 86, 86, 86, 85, 85, 85, 85, 84, 84, 84, 83, 83, 83, 83, 82, 82, 82, 81, 81, 81, 81, 80, 80, 80, 80, 79, 79, 79, 79, 79, 78, 78, 78, 78, 77, 77, 77, 77, 76, 76, 76, 76, 76, 75, 75, 75, 75, 75, 74, 74, 74, 74, 74, 73, 73, 73, 73, 73, 72, 72, 72, 72, 72, 72, 71, 71, 71, 71, 71, 71, 70, 70, 70, 70, 70, 69, 69, 69, 69, 69, 69, 69, 68, 68, 68, 68, 68, 68, 67, 67, 67, 67, 67, 67, 67, 66, 66, 66, 66, 66, 66, 66, 65, 65, 65, 65, 65, 65, 65, 65, 64, 64, 64, 64, 64, 64, 64, 64, 63, 63, 63, 63, 63, 63, 63, 63, 62, 62, 62, 62, 62, 62, 62, 62, 61, 61, 61, 61, 61, 61, 61, 61, 61, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
};

uint16_t fct=0;

int8_t fs(uint8_t a) //Full wave: 0..127, values: -127..127
{
  uint8_t n;
  a&=127;
  n=(a>=64);
  a&=63;
  a=(a<=32)?a:64-a;
  return n?-sinTable[a]:sinTable[a];
}

int8_t fc(uint8_t a)
{
  uint8_t n;
  a+=32;
  a&=127;
  n=(a>=64);
  a&=63;
  a=(a<=32)?a:64-a;
  return n?-sinTable[a]:sinTable[a];
}


// -----------------------------
// |          RAND             |
// -----------------------------
#define FASTLED_RAND16_2053  ((uint16_t)(2053))
#define FASTLED_RAND16_13849 ((uint16_t)(13849))
uint16_t rand16seed;

inline uint8_t random8()
{
  rand16seed = (rand16seed * FASTLED_RAND16_2053) + FASTLED_RAND16_13849;
  // return the sum of the high and low bytes, for better
  //  mixing and non-sequential correlation
  return (uint8_t)(((uint8_t)(rand16seed & 0xFF)) +
                   ((uint8_t)(rand16seed >> 8)));
}

inline uint8_t random8b()
{
  rand16seed = (rand16seed * FASTLED_RAND16_2053) + FASTLED_RAND16_13849;
  // return the sum of the high and low bytes, for better
  //  mixing and non-sequential correlation
  return (uint8_t)(((uint8_t)(rand16seed & 0xFF)) &
                   ((uint8_t)(rand16seed >> 8)));
}

inline uint16_t random16()
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
void plasma(uint8_t ver, uint16_t pos, uint16_t len)
{
  static uint16_t psp=0;
  uint8_t pbias = 0;
  static uint8_t a=0;
  int16_t x, y;
  int16_t v1, v2;
  uint16_t p4;
  uint8_t bits;
  uint16_t rnd;
  int16_t x2, x3, y2, y3;

  uint8_t temp[8];

  if (pos < 51)
  {
    pbias = (51 - pos) * 5;
  }
  else if (pos > (len - 51))
  {
    pbias = (51 - len + pos) * 5;
  }

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
      if (ver == 0)
      {
        x2=((int16_t)fc(a)*x-(int16_t)fs(a)*(y+4))/128;
        x3=((int16_t)fc(a)*x-(int16_t)fs(a)*y)/128;
        y2=((int16_t)fs(a)*x+(int16_t)fc(a)*(y+4))/128;
        y3=((int16_t)fs(a)*x+(int16_t)fc(a)*(y))/128;
        v1=((int16_t)fs(x2*1.7+psp*0.3+fs(y2*2+15+psp)/15)+fs(y2-psp+fs(x2+41)/123))*3/2;
        v2=((int16_t)fs(x3*1.7+psp*0.3+fs(y3*2+15+psp)/15)+fs(y3-psp+fs(x3+41)/123))*3/2;
      }
      else if (ver == 1)
      {
        x2=((int16_t)fc(a)*x-(int16_t)fs(a)*(y+4))/128;
        x3=((int16_t)fc(a)*x-(int16_t)fs(a)*y)/128;
        y2=((int16_t)fs(a)*x+(int16_t)fc(a)*(y+4))/128;
        y3=((int16_t)fs(a)*x+(int16_t)fc(a)*(y))/128;
        v1=((int16_t)fs(fs(y2)+x2*1.7+psp*0.3+fs(y2*2+15+psp)/15)+fs(y2-psp+fs(x2+41)/123))*3/2;
        v2=((int16_t)fs(fs(y3)+x3*1.7+psp*0.3+fs(y3*2+15+psp)/15)+fs(y3-psp+fs(x3+41)/123))*3/2;
      }
      else if (ver == 2)
      {
        x2=((int16_t)fc(a)*x-(int16_t)fs(a)*(y+4))/128;
        x3=((int16_t)fc(a)*x-(int16_t)fs(a)*y)/128;
        y2=((int16_t)fs(a)*x+(int16_t)fc(a)*(y+4))/128;
        y3=((int16_t)fs(a)*x+(int16_t)fc(a)*(y))/128;
        v1=((int16_t)fs(fs(y2)+x2*1.7+psp*0.3+fs(y2*2+15+psp)/15)+fs(y2-psp+fs(x2+41)/123))*3/2;
        v2=((int16_t)fs(fs(y3)+x3*1.7+psp*0.3+fs(y3*2+15+psp)/15)+fs(y3-psp+fs(x3+41)/123))*3/2;
      }
    
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
      //v1-=pbias2;
      if (v1<0)
      {
        v1=0;
      }
      if (v2>255)
      {
        v2=255;
      }
      //v2-=pbias2;
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
  //if (psp<85)
  //{
  //  pbias-=3;
  //}
  //else if (psp>(PLASMA_LEN-255/3) && pbias2<255)
  //{
  //  pbias2+=3;
  //}
}

void setup()
{
  // put your setup code here, to run once:
  delay(200); // wait for the OLED to fully power up
  oledInit(0x3c, 0, 0);
  uint8_t i;
  for (i=0; i<=32; i++)
  {
    sinTable[i]=sin(i/32.0*M_PI/2)*127;
  }
}

void text_eff(void* gfx, uint8_t intensity, uint8_t randomness)
{
  int16_t of = 0;
  rand16seed=fct;
  uint16_t rn = (uint16_t)randomness * 8;
  uint8_t x, y;
  uint8_t buf[16];
  uint16_t ro=random16();
  if (ro < 5*rn)
  {
    of = ro % randomness - (randomness >> 1);
  }
  for (y=0; y<8; y++)
  {
    uint16_t rr=random16();
    oledSetPosition(2 + (rr < 20*rn) + (rr < 40*rn) + (rr < 50*rn), y);
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
        p = pgm_read_byte(gfx + of + y * 128 + ys * 16 + x / 8);
        if (intensity > (random8() >> 1))
        {
          buf[0] |= (p & (1<<bp)) ? (1 << ys) : 0;
        }
      }
      uint16_t rnd=random16();
      if (intensity > (random8() >> 1) + 127)
      {
        buf[0] &= ((rnd&0xff) ^ (rnd >> 8));
      }
      if (rnd > rn*12) oledWriteDataBlock(buf, 1);
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

void wtf2()
{
  uint8_t x, ym;
  uint8_t a, b;
  static uint16_t psp = 0;

  for (ym=0; ym<4; ym++)
  {
    uint16_t rr=random16();
    for (x=0; x<128; x++)
    {
      uint8_t p;
      uint8_t b = 0;
      uint8_t bp;
      bp = x & 7;
      uint8_t ys;
      a = 0;
      b = 0;
      for (ys=0; ys<8; ys++)
      {
        uint8_t y = ym * 8 + ys;
        
        uint16_t dsq = (x - 63) * (x - 63) + (y - 31) * (y - 31);
        //uint16_t dsq = (x - 30) * (x - 30) + (y - 14) * (y - 14)
        //            + (x - 97) * (x - 97) + (y - 48) * (y - 48);
        uint8_t dd = dsq % (psp + 20);
        if (dd < 10)
        {
          a |= (1 << ys);
          b |= (128 >> ys);
        }
      }
      oledSetPosition(2 + x, ym);
      oledWriteDataBlock(&a, 1);
      oledSetPosition(2 + x, 7 - ym);
      oledWriteDataBlock(&b, 1);
    }
  }
  psp++;
}

inline uint8_t invsqr(uint16_t x)
{
  //return 1024/sqrt(x);
  if (x < 16)
  {
    return 255;
  }
  else if (x >= 400)
  {
    return invsqr(x >> 2) >> 1;
    //return invsqr(x >> 4) >> 2;
    //return 58 - (x >> 6);
  }
  else
  {
    //return 405 - 0.8858*x;
    return pgm_read_byte(isq + ((x - 16) >> 1));
  }
}

void blob()
{
//#define TH1 48
//#define TH2 53
//#define TH3 57
//#define TH4 62
#define TH1 61
#define TH2 (72-5)
#define TH3 (72+5)
#define TH4 88
  uint8_t x1 = 30, y1 = 6, x2 = fct & 127, y2 = 40;

  uint8_t x, ym;
  uint8_t buf[16];

  for (ym=0; ym<8; ym++)
  {
    uint16_t rr=random16();
    oledSetPosition(2, ym);
    for (x=0; x<128; x+=4)
    {
      uint8_t p;
      uint8_t b = 0;
      uint8_t bp;
      bp = x & 7;
      uint8_t ys;
      buf[0] = 0;
      uint8_t y = ym * 8;

      uint16_t dsq1;
      uint16_t dsq2;
      
      dsq1 = (x - x1) * (x - x1) + (y - y1) * (y - y1);
      dsq2 = (x - x2) * (x - x2) + (y - y2) * (y - y2);
      int16_t dsqa = invsqr(dsq1) + invsqr(dsq2);
      //int16_t dsqa = 1024/sqrt(dsq1) + 1024/sqrt(dsq2);

      y += 4;

      dsq1 = (x - x1) * (x - x1) + (y - y1) * (y - y1);
      dsq2 = (x - x2) * (x - x2) + (y - y2) * (y - y2);
      int16_t dsqb = invsqr(dsq1) + invsqr(dsq2);
      //int16_t dsqb = 1024/sqrt(dsq1) + 1024/sqrt(dsq2);

        //buf[0] |= (dsq) ? (1 << ys) : 0;

      uint8_t dsqp = 0, dsqq = 0;
      uint8_t dsqv = 0, dsqw = 0;
      if (dsqa > TH1 && dsqa < TH4)
      {
        dsqp = 15;
        if (dsqa > TH2 && dsqa < TH3)
        {
          dsqv = 15;
        }
      }
      if (dsqb > TH1 && dsqb < TH4)
      {
        dsqq = 15 * 16;
        if (dsqb > TH2 && dsqb < TH3)
        {
          dsqw = 15 * 16;
        }
      }

      uint8_t z = dsqp | dsqq;
      uint8_t h = dsqv | dsqw;

      uint16_t r = random16();

      buf[0] = z & (uint8_t)r;
      buf[0] &= (h | random8b());
      buf[1] = z & (r >> 8);
      buf[1] &= (h | random8b());

      r = random16();
      buf[2] = z & (uint8_t)r;
      buf[2] &= (h | random8b());
      buf[3] = z & (r >> 8);
      buf[3] &= (h | random8b());

      oledWriteDataBlock(buf, 4);
    }
  }
}

int ramp(int x, int s)
{
  if (x < s)
  {
    return 0;
  }
  else
  {
    return x - s;
  }
}

uint16_t ramp(uint16_t pos, uint16_t b, uint16_t e)
{
  return 255*(pos - b)/(e - b);
}

uint16_t nramp(uint16_t pos, uint16_t b, uint16_t e)
{
  return 255-255*(pos - b)/(e - b);
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
//  plasma();
//  if (fct < 512)
//  {
//    text_eff(tick, fct / 2);
//  }
//  else if (fct < 1024)
//  {
//    text_eff(never, 255-fct / 2);
//  }
//  else
//  {
//    uint8_t r = random8();
//    if (r < 85)
//    {
//      text_eff(cpu1, r);
//    }
//    else if (r < 170)
//    {
//      text_eff(cpu2, r);
//    }
//    else
//    {
//      text_eff(cpu3, r);
//    }
//
//  }
//
//  blob();
//  plasma();
//wtf2();
//    text_eff(tick, fct);
//    text_eff(cpu1, fct);
//    text_eff(cpu2, fct);
//    text_eff(cpu3, fct);
//    text_eff(never, fct);
//    blob();
//    wtf2();
//  plasma(0);
//  if (fct < 128)
//  {
//    text_eff(gfx_a, ramp(fct, 0, 128), nramp(fct, 0, 128));
//  }
//  else if (fct < 256)
//  {
//    text_eff(gfx_a, nramp(fct, 128, 256), 0);
//  }
//
//  else if (fct < 384)
//  {
//    text_eff(gfx_b, ramp(fct, 256, 384), nramp(fct, 256, 384));
//  }
//  else if (fct < 512)
//  {
//    text_eff(gfx_b, nramp(fct, 384, 512), 0);
//  }
//
//  else if (fct < 1024)
//  {
//    wtf2();
//  }
//
//  else if (fct < 1152)
//  {
//    text_eff(gfx_d, ramp(fct, 1024, 1152), nramp(fct, 0, 128) / 4);
//  }
//  else if (fct < 1280)
//  {
//    text_eff(gfx_d, nramp(fct, 1152, 1280), 0);
//  }
//
//  else if (fct < 1792)
//  {
//    plasma(0, 512, fct - 1280);
//  }
//
//
  if (fct < 16)
  {
    wtf2();
  }

  else if (fct < 80)
  {
    text_eff(gfx_this, ramp(fct, 16, 80), nramp(fct, 16, 80));
  }
  else if (fct < 144)
  {
    text_eff(gfx_this, nramp(fct, 80, 144), 0);
  }
  
  else if (fct < 160)
  {
    wtf2();
  }

  else if (fct < 224)
  {
    text_eff(gfx_no, ramp(fct, 160, 224), nramp(fct, 160, 224));
  }
  else if (fct < 288)
  {
    text_eff(gfx_no, nramp(fct, 224, 288), 0);
  }

  else if (fct < 304)
  {
    wtf2();
  }

  else if (fct < 336)
  {
    text_eff(gfx_d, ramp(fct, 304, 336), nramp(fct, 304, 336));
  }
  else if (fct < 368)
  {
    text_eff(gfx_d, nramp(fct, 336, 368), 0);
  }

  else if (fct < 384)
  {
    wtf2();
  }
  
  else if (fct < 416)
  {
    text_eff(gfx_get, ramp(fct, 384, 416), nramp(fct, 384, 416));
  }
  else if (fct < 448)
  {
    text_eff(gfx_get, nramp(fct, 416, 448), 0);
  }

  else if (fct < 464)
  {
    wtf2();
  }

  else if (fct < 496)
  {
    text_eff(gfx_start, ramp(fct, 464, 496), nramp(fct, 464, 496));
  }
  else if (fct < 528)
  {
    text_eff(gfx_start, nramp(fct, 496, 528), 0);
  }

  else if (fct < 1040)
  {
    plasma(1, fct-528, 512);
  }

  else if (fct < 1072)
  {
    text_eff(gfx_ohno, ramp(fct, 1040, 1072), nramp(fct, 1040, 1072));
  }
  else if (fct < 1104)
  {
    text_eff(gfx_ohno, nramp(fct, 1072, 1104), 0);
  }
  else if (fct < 1136)
  {
    text_eff(gfx_plasma, ramp(fct, 1072, 1136), nramp(fct, 1072, 1136));
  }
  else if (fct < 1168)
  {
    text_eff(gfx_plasma, nramp(fct, 1136, 1168), 0);
  }
  else if (fct < 1200)
  {
    text_eff(gfx_blob, ramp(fct, 1136, 1200), nramp(fct, 1136, 1200));
  }
  else if (fct < 1232)
  {
    text_eff(gfx_blob, nramp(fct, 1200, 1230), 0);
  }
  
 //plasmaback, never 

  else
  {
    blob();
  }
  

  fct++;

}

