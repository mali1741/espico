#include "font_a.c"

#define DISPLAY_X_OFFSET 12
#define SCREEN_WIDTH 128
#define SCREEN_WIDTH_BYTES 64
#define SCREEN_HEIGHT 128
#define SCREEN_SIZE (SCREEN_HEIGHT * SCREEN_WIDTH_BYTES)
// #define ONE_DIM_SCREEN_ARRAY

#ifndef ONE_DIM_SCREEN_ARRAY
#define SCREEN_ARRAY_DEF SCREEN_HEIGHT][SCREEN_WIDTH_BYTES
#define SCREEN_ADDR(x, y) y][x
#else
#define SCREEN_ARRAY_DEF SCREEN_SIZE
#define SCREEN_ADDR(x, y) ((int(y) << 6) + int(x))
#endif

// [high-n][y][low-n][x] [n << 4][y][n&0x0f][x]
// #define SPRITE_ARRAY_DEF 4][8][16][4
#define SPRITE_HEIGHT 8
#define SPRITE_WIDTH  8
#define SPRITE_WIDTH_BYTES 4
#define SPRITE_COUNT 128
#define SPRITE_MAP_SIZE 4096
#define SPRITE_FLAGS_SIZE 256
#define SPRITE_ARRAY_DEF SPRITE_MAP_SIZE
#define SPRITE_MEMMAP PRG_SIZE
#define SPRITE_ADDR(n)  (((int)(n & 0xf0) << 5) + ((n & 0x0f) << 2))
// #define SPRITE_ARRAY_DEF 128][64
// #define SPRITE_ADDR(n)  ((n & 0xf0) >> 2)][((n & 0xf) << 3)
#define SPRITE_PIX(x, y) ((int(y) << 6) + (x))
#define TILEMAP_SIZE 4096
#define TILEMAP_ADDR(x, y) ((int((y)&31) << 7) + ((x)&127))

#define PIX_LEFT_MASK(p)  ((p) & 0xf0)
#define GET_PIX_LEFT(p)  ((p) >> 4)
#define PIX_RIGHT_MASK(p) ((p) & 0x0f)
#define GET_PIX_RIGHT(p) PIX_RIGHT_MASK(p)
#define SET_PIX_LEFT(p,c)  p = (PIX_RIGHT_MASK(p) + ((c) << 4))
#define SET_PIX_RIGHT(p,c) p = (PIX_LEFT_MASK(p) + ((c) & 0x0f))

#define PARTICLE_COUNT 32

struct EspicoState {
  int8_t   drawing;
  int8_t   color;
  int8_t   bgcolor;
  int8_t   imageSize;
  int16_t  fillpattern;
  int16_t  palt;
  int8_t   regx;
  int8_t   regy;
  uint8_t  coordshift;
};  

#define ACTOR_IN_EVENT    0x8000
#define ACTOR_X_EVENT     0x4000
#define ACTOR_Y_EVENT     0x2000
#define ACTOR_MAP_COLL    0x0080

struct Actor {
  uint8_t sprite;
  uint8_t sw;
  uint8_t sh;
  int8_t frame;
  int8_t lives;
  uint8_t flags; //0 0 0 0 0 0 scrolled solid
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
  int16_t speedx;
  int16_t speedy;
  int16_t refval;
  int16_t gravity;
  int16_t angle;
  uint16_t collision;
  uint16_t oncollision;
  uint16_t onexitscreen;
  uint16_t onanimate;
};

struct Particle {
  int16_t time;
  int16_t x;
  int16_t y;
  int8_t gravity;
  int8_t speedx;
  int8_t speedy;
  int8_t color;
};

struct Emitter { 
  int16_t time;
  int16_t timer;
  int16_t timeparticle;
  uint8_t count;
  int8_t gravity;
  int16_t x;
  int16_t y;
  int8_t speedx;
  int8_t speedy;
  int8_t speedx1;
  int8_t speedy1;
  int8_t color;
};

static const int8_t cosT[] PROGMEM = {
  0x40, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3e, 0x3e, 0x3e, 0x3e, 0x3d, 0x3d, 0x3d, 0x3c, 0x3c, 
  0x3c, 0x3b, 0x3b, 0x3a, 0x3a, 0x3a, 0x39, 0x39, 0x38, 0x37, 0x37, 0x36, 0x36, 0x35, 0x35, 0x34, 0x33, 0x33, 0x32, 0x31, 
  0x31, 0x30, 0x2f, 0x2e, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x29, 0x28, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20, 
  0x20, 0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0xf, 0xe, 0xd, 0xc, 0xb, 
  0xa, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8, 0xf7, 0xf5, 0xf4, 0xf3, 0xf2, 
  0xf1, 0xf0, 0xef, 0xee, 0xed, 0xec, 0xeb, 0xea, 0xe9, 0xe8, 0xe6, 0xe5, 0xe4, 0xe3, 0xe2, 0xe1, 0xe0, 0xe0, 0xdf, 0xde, 
  0xdd, 0xdc, 0xdb, 0xda, 0xd9, 0xd8, 0xd7, 0xd6, 0xd6, 0xd5, 0xd4, 0xd3, 0xd2, 0xd1, 0xd1, 0xd0, 0xcf, 0xce, 0xce, 0xcd, 
  0xcc, 0xcc, 0xcb, 0xca, 0xca, 0xc9, 0xc9, 0xc8, 0xc8, 0xc7, 0xc6, 0xc6, 0xc5, 0xc5, 0xc5, 0xc4, 0xc4, 0xc3, 0xc3, 0xc3, 
  0xc2, 0xc2, 0xc2, 0xc1, 0xc1, 0xc1, 0xc1, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 
  0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc1, 0xc1, 0xc1, 0xc1, 0xc2, 0xc2, 0xc2, 0xc3, 0xc3, 0xc3, 0xc4, 0xc4, 
  0xc5, 0xc5, 0xc5, 0xc6, 0xc6, 0xc7, 0xc8, 0xc8, 0xc9, 0xc9, 0xca, 0xca, 0xcb, 0xcc, 0xcc, 0xcd, 0xce, 0xce, 0xcf, 0xd0, 
  0xd1, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xdf, 0xe0, 0xe1, 
  0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf7, 
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x1f, 0x20, 0x21, 0x22, 0x23, 
  0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2e, 0x2f, 0x30, 0x31, 0x31, 0x32, 0x33, 0x33, 
  0x34, 0x35, 0x35, 0x36, 0x36, 0x37, 0x37, 0x38, 0x39, 0x39, 0x3a, 0x3a, 0x3a, 0x3b, 0x3b, 0x3c, 0x3c, 0x3c, 0x3d, 0x3d, 
  0x3d, 0x3e, 0x3e, 0x3e, 0x3e, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f
};
static const int8_t sinT[] PROGMEM = {
  0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
  0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x29, 0x2a, 
  0x2b, 0x2c, 0x2d, 0x2e, 0x2e, 0x2f, 0x30, 0x31, 0x31, 0x32, 0x33, 0x33, 0x34, 0x35, 0x35, 0x36, 0x36, 0x37, 0x37, 0x38, 
  0x39, 0x39, 0x3a, 0x3a, 0x3a, 0x3b, 0x3b, 0x3c, 0x3c, 0x3c, 0x3d, 0x3d, 0x3d, 0x3e, 0x3e, 0x3e, 0x3e, 0x3f, 0x3f, 0x3f, 
  0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3e, 0x3e, 
  0x3e, 0x3e, 0x3d, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c, 0x3b, 0x3b, 0x3a, 0x3a, 0x3a, 0x39, 0x39, 0x38, 0x37, 0x37, 0x36, 0x36, 
  0x35, 0x35, 0x34, 0x33, 0x33, 0x32, 0x31, 0x31, 0x30, 0x2f, 0x2e, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x29, 0x28, 0x27, 
  0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20, 0x20, 0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x17, 0x16, 0x15, 0x14, 0x13, 
  0x12, 0x11, 0x10, 0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 
  0xf9, 0xf8, 0xf7, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0, 0xef, 0xee, 0xed, 0xec, 0xeb, 0xea, 0xe9, 0xe8, 0xe6, 0xe5, 0xe4, 
  0xe3, 0xe2, 0xe1, 0xe0, 0xe0, 0xdf, 0xde, 0xdd, 0xdc, 0xdb, 0xda, 0xd9, 0xd8, 0xd7, 0xd6, 0xd6, 0xd5, 0xd4, 0xd3, 0xd2, 
  0xd1, 0xd1, 0xd0, 0xcf, 0xce, 0xce, 0xcd, 0xcc, 0xcc, 0xcb, 0xca, 0xca, 0xc9, 0xc9, 0xc8, 0xc8, 0xc7, 0xc6, 0xc6, 0xc5, 
  0xc5, 0xc5, 0xc4, 0xc4, 0xc3, 0xc3, 0xc3, 0xc2, 0xc2, 0xc2, 0xc1, 0xc1, 0xc1, 0xc1, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 
  0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc1, 0xc1, 0xc1, 0xc1, 0xc2, 
  0xc2, 0xc2, 0xc3, 0xc3, 0xc3, 0xc4, 0xc4, 0xc5, 0xc5, 0xc5, 0xc6, 0xc6, 0xc7, 0xc8, 0xc8, 0xc9, 0xc9, 0xca, 0xca, 0xcb, 
  0xcc, 0xcc, 0xcd, 0xce, 0xce, 0xcf, 0xd0, 0xd1, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 
  0xdc, 0xdd, 0xde, 0xdf, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe
};

static const uint16_t epalette[] PROGMEM = {
     0x0000, 0x194A, 0x792A, 0x042A, 0xAA86, 0x5AA9, 0xC618, 0xFF9D,
     0xF809, 0xFD00, 0xFF64, 0x0726, 0x2D7F, 0x83B3, 0xFBB5, 0xFE75
};

uint16_t palette[16] __attribute__ ((aligned));
uint8_t drwpalette[16] __attribute__ ((aligned));
uint16_t pix_buffer[256] __attribute__ ((aligned));
uint8_t screen[SCREEN_ARRAY_DEF] __attribute__ ((aligned));
uint8_t tile_map[TILEMAP_SIZE] __attribute ((aligned));
struct EspicoState espico __attribute__ ((aligned));
// uint16_t *palette __attribute__ ((aligned)) = (uint16_t *)&mem[SPRITE_MEMMAP+SPRITE_MAP_SIZE+SPRITE_FLAGS_SIZE+128];
//uint8_t *drwpalette __attribute__ ((aligned)) = &mem[SPRITE_MEMMAP+SPRITE_MAP_SIZE+SPRITE_FLAGS_SIZE+128+32];
uint8_t *line_is_draw __attribute__ ((aligned)) = &mem[SPRITE_MEMMAP+SPRITE_MAP_SIZE+SPRITE_FLAGS_SIZE];
uint8_t *sprite_map __attribute__ ((aligned)) = &mem[SPRITE_MEMMAP];
uint8_t *sprite_flags __attribute__ ((aligned)) = &mem[SPRITE_MEMMAP+SPRITE_MAP_SIZE];
char charArray[340] __attribute__ ((aligned));
struct Actor actor_table[32] __attribute__ ((aligned));
struct Particle particles[PARTICLE_COUNT] __attribute__ ((aligned));
struct Emitter emitter __attribute__ ((aligned));
// struct TileMap tiles __attribute__ ((aligned));

#pragma GCC optimize ("-O2")
#pragma GCC push_options
inline uint8_t getSpriteFlag(uint8_t n) {
  return sprite_flags[n];
}

inline void setSpriteFlag(uint8_t n, uint8_t v) {
  sprite_flags[n] = v;
}

inline int16_t coord(int16_t c) {
  return (c / (1 << espico.coordshift));
}

inline int16_t getCos(int16_t g){
  g = g % 360;
  if(g < 0)
    g += 360;
  return (int16_t)(int8_t)pgm_read_byte_near(cosT + g);
}

inline int16_t getSin(int16_t g){
  g = g % 360;
  if(g < 0)
    g += 360;
  return (int16_t)(int8_t)pgm_read_byte_near(sinT + g);
}


int16_t atan2_rb(int16_t y, int16_t x) {
   // Fast XY vector to integer degree algorithm - Jan 2011 www.RomanBlack.com
   // Converts any XY values including 0 to a degree value that should be
   // within +/- 1 degree of the accurate value without needing
   // large slow trig functions like ArcTan() or ArcCos().
   // NOTE! at least one of the X or Y values must be non-zero!
   // This is the full version, for all 4 quadrants and will generate
   // the angle in integer degrees from 0-360.
   // Any values of X and Y are usable including negative values provided
   // they are between -1456 and 1456 so the 16bit multiply does not overflow.

   unsigned char negflag;
   unsigned char tempdegree;
   unsigned char comp;
   unsigned int degree;     // this will hold the result
//   signed int x;            // these hold the XY vector at the start
//   signed int y;            // (and they will be destroyed)
   unsigned int ux;
   unsigned int uy;

   // Save the sign flags then remove signs and get XY as unsigned ints
   negflag = 0;
   if(x < 0)
   {
      negflag += 0x01;    // x flag bit
      x = (0 - x);        // is now +
   }
   ux = x;                // copy to unsigned var before multiply
   if(y < 0)
   {
      negflag += 0x02;    // y flag bit
      y = (0 - y);        // is now +
   }
   uy = y;                // copy to unsigned var before multiply

   // 1. Calc the scaled "degrees"
   if(ux > uy)
   {
      degree = (uy * 45) / ux;   // degree result will be 0-45 range
      negflag += 0x10;    // octant flag bit
   }
   else
   {
      degree = (ux * 45) / uy;   // degree result will be 0-45 range
   }

   // 2. Compensate for the 4 degree error curve
   comp = 0;
   tempdegree = degree;    // use an unsigned char for speed!
   if(tempdegree > 22)      // if top half of range
   {
      if(tempdegree <= 44) comp++;
      if(tempdegree <= 41) comp++;
      if(tempdegree <= 37) comp++;
      if(tempdegree <= 32) comp++;  // max is 4 degrees compensated
   }
   else    // else is lower half of range
   {
      if(tempdegree >= 2) comp++;
      if(tempdegree >= 6) comp++;
      if(tempdegree >= 10) comp++;
      if(tempdegree >= 15) comp++;  // max is 4 degrees compensated
   }
   degree += comp;   // degree is now accurate to +/- 1 degree!

   // Invert degree if it was X>Y octant, makes 0-45 into 90-45
   if(negflag & 0x10) degree = (90 - degree);

   // 3. Degree is now 0-90 range for this quadrant,
   // need to invert it for whichever quadrant it was in
   if(negflag & 0x02)   // if -Y
   {
      if(negflag & 0x01)   // if -Y -X
            degree = (180 + degree);
      else        // else is -Y +X
            degree = (180 - degree);
   }
   else    // else is +Y
   {
      if(negflag & 0x01)   // if +Y -X
            degree = (360 - degree);
   }

   return degree;
}

/*
#define MULTIPLY_FP_RESOLUTION_BITS  6

int16_t atan2_fp(int16_t y_fp, int16_t x_fp){
  int32_t coeff_1 = 45;
  int32_t coeff_1b = -56; // 56.24;
  int32_t coeff_1c = 11;  // 11.25
  int16_t coeff_2 = 135;
  int16_t angle = 0;
  int32_t r;
  int32_t r3;
  int16_t y_abs_fp = y_fp;
  if (y_abs_fp < 0)
    y_abs_fp = -y_abs_fp;
  if (y_fp == 0){
    if (x_fp >= 0){
      angle = 0;
    }
    else{
      angle = 180;
    }
  }
  else if (x_fp >= 0){
    r = (((int32_t)(x_fp - y_abs_fp)) << MULTIPLY_FP_RESOLUTION_BITS) / ((int32_t)(x_fp + y_abs_fp));
    r3 = r * r;
    r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
    r3 *= r;
    r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
    r3 *= coeff_1c;
    angle = (int16_t) (coeff_1 + ((coeff_1b * r + r3) >> MULTIPLY_FP_RESOLUTION_BITS));
  }
  else{
    r = (((int32_t)(x_fp + y_abs_fp)) << MULTIPLY_FP_RESOLUTION_BITS) / ((int32_t)(y_abs_fp - x_fp));
    r3 = r * r;
    r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
    r3 *= r;
    r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
    r3 *= coeff_1c;
    angle = coeff_2 + ((int16_t)  (((coeff_1b * r + r3) >> MULTIPLY_FP_RESOLUTION_BITS)));
  }
  if (y_fp < 0)
    return (-angle);     // negate if in quad III or IV
  else
    return (angle);
}
*/

void resetPalette() {
  for(int i = 0; i < 16; i++){
    palette[i] = (uint16_t)pgm_read_word_near(epalette + i);
    drwpalette[i] = i;
  }
  espico.palt = 1; // black is transparent
}

void display_init(){
  initEspicoState();
  resetPalette();
  for(int i = 0; i < 32; i++){
    actor_table[i].sprite = 0;
    actor_table[i].sw = 8;
    actor_table[i].sh = 8;
    actor_table[i].frame = 0;
    actor_table[i].x = -255;
    actor_table[i].y = -255;
    actor_table[i].width = 0;
    actor_table[i].height = 0;
    actor_table[i].speedx = 0;
    actor_table[i].speedy = 0;
    actor_table[i].lives = 0;
    actor_table[i].collision = -1;
    actor_table[i].flags = 0; //scrolled = 1 solid = 0
    actor_table[i].gravity = 0;
    actor_table[i].angle = 0;
    actor_table[i].refval = 0;
    actor_table[i].oncollision = 0;
    actor_table[i].onexitscreen = 0;
    actor_table[i].onanimate = 0;
  }
  emitter.time = 0;
  emitter.timer = 0;
  for(int i = 0; i < PARTICLE_COUNT; i++)
    particles[i].time = 0;
  for(int i = 0; i < 340; i++)
    charArray[i] = 0;
  clearScr(0);
}

void initEspicoState() {
  espico.drawing = 0;
  espico.color = 7;
  espico.bgcolor = 0;
  espico.fillpattern = 0;
  espico.imageSize = 1;
  espico.regx = 0;
  espico.regy = 0;
}

void setEspicoState(int16_t s, int16_t v) {
  switch (s) {
   case 0:
     espico.drawing = v;
     break;
   case 1:
     espico.coordshift = v & 0xf;
     break;
   }
}  

void redrawScreen() {
  int i;
  if (espico.drawing) return;
  cadr_count++;
  for(int y = 0; y < 128; y++){
    i = 0;
    if(line_is_draw[y] == 1){
      if(y < 8)
        tft.setAddrWindow(DISPLAY_X_OFFSET + 0, y, DISPLAY_X_OFFSET + 127, y  + 1);
      else if(y > 120)
        tft.setAddrWindow(DISPLAY_X_OFFSET + 0, 232 - 120 + y, DISPLAY_X_OFFSET + 127, 232 - 120 + y  + 1);
      else
        tft.setAddrWindow(DISPLAY_X_OFFSET + 0, y * 2 - 8, DISPLAY_X_OFFSET + 127, y * 2 + 2 - 8);
      // Each byte contains two pixels
      for(int x = 0; x < 32; x++){
          pix_buffer[i++] = pix_buffer[i++] = palette[GET_PIX_LEFT(screen[SCREEN_ADDR(x,y)])];
          pix_buffer[i++] = pix_buffer[i++] = palette[GET_PIX_RIGHT(screen[SCREEN_ADDR(x,y)])];
      }
      tft.pushColors(pix_buffer, 128);
      if(y >= 8 && y <= 120)
        tft.pushColors(pix_buffer, 128);
      line_is_draw[y] = 0;
    } 
    else if(line_is_draw[y] == 2){
      if(y < 8)
        tft.setAddrWindow(DISPLAY_X_OFFSET + 128, y, DISPLAY_X_OFFSET + 255, y  + 1);
      else if(y > 120)
        tft.setAddrWindow(DISPLAY_X_OFFSET + 128, 232 - 120 + y, DISPLAY_X_OFFSET + 255, 232 - 120 + y  + 1);
      else
        tft.setAddrWindow(DISPLAY_X_OFFSET + 128, y * 2 - 8, DISPLAY_X_OFFSET + 255, y * 2 + 2 - 8);
      // Each byte contains two pixels
      for(int x = 0; x < 32; x++){
          pix_buffer[i++] = pix_buffer[i++] = palette[GET_PIX_LEFT(screen[SCREEN_ADDR(x+32,y)])];
          pix_buffer[i++] = pix_buffer[i++] = palette[GET_PIX_RIGHT(screen[SCREEN_ADDR(x+32,y)])];
      }
      tft.pushColors(pix_buffer, 128);
      if(y >= 8 && y <= 120)
        tft.pushColors(pix_buffer, 128);
      line_is_draw[y] = 0;
    } 
    else if(line_is_draw[y] == 3){
      if(y < 8)
        tft.setAddrWindow(DISPLAY_X_OFFSET + 0, y, DISPLAY_X_OFFSET + 255, y  + 1);
      else if(y > 120)
        tft.setAddrWindow(DISPLAY_X_OFFSET + 0, 232 - 120 + y, DISPLAY_X_OFFSET + 255, 232 - 120 + y  + 1);
      else
        tft.setAddrWindow(DISPLAY_X_OFFSET + 0, y * 2 - 8, DISPLAY_X_OFFSET + 255, y * 2 + 2 - 8);
      // Each byte contains two pixels
      for(int x = 0; x < 64; x++){
          pix_buffer[i++] = pix_buffer[i++] = palette[GET_PIX_LEFT(screen[SCREEN_ADDR(x,y)])];
          pix_buffer[i++] = pix_buffer[i++] = palette[GET_PIX_RIGHT(screen[SCREEN_ADDR(x,y)])];
      }
      tft.pushColors(pix_buffer, 256);
      if(y >= 8 && y <= 120)
        tft.pushColors(pix_buffer, 256);
      line_is_draw[y] = 0;
    }
  }
  setRedraw();
}

int8_t randomD(int8_t a, int8_t b) {
  int8_t minv = a < b ? a : b;
  int8_t maxv = a > b ? a : b;
  return random(minv, maxv + 1);
}

void setParticle(int8_t gravity, uint8_t count, uint16_t time){
  emitter.gravity = gravity;
  emitter.count = count;
  emitter.timeparticle = time;
}

void setEmitter(uint16_t time, int16_t dir, int16_t dir1, int16_t speed){
  emitter.time = time;
  emitter.speedx = (int8_t)((speed * getCos(dir)) >> 6);
  emitter.speedy = (int8_t)((speed * getSin(dir)) >> 6);
  emitter.speedx1 = (int8_t)((speed * getCos(dir1)) >> 6);
  emitter.speedy1 = (int8_t)((speed * getSin(dir1)) >> 6);
}

void drawParticle(int16_t x, int16_t y, uint8_t color){
  emitter.x = x;
  emitter.y = y;
  emitter.color = color;
  emitter.timer = emitter.time;
}

void redrawParticles(){
  int16_t n;
  const int i = emitter.count;
  uint8_t x, y;
  if(emitter.timer > 0){
      emitter.timer -= 50;
      for(n = 0; n < i; n++){
        if(particles[n].time <= 0){
          particles[n].time = emitter.timeparticle;
          particles[n].x = emitter.x;
          particles[n].y = emitter.y;
          particles[n].color = emitter.color;
          particles[n].speedx = randomD(emitter.speedx, emitter.speedx1);
          particles[n].speedy = randomD(emitter.speedy, emitter.speedy1);
          particles[n].gravity = emitter.gravity;
        }
      }
    }
    for(n = 0; n < PARTICLE_COUNT; n++)
      if(particles[n].time > 0){
        setPix(particles[n].x, particles[n].y, drwpalette[particles[n].color]); 
        particles[n].time -= 50;
        if(random(0,2)){
          particles[n].x += particles[n].speedx;
          particles[n].speedy += particles[n].gravity;
          particles[n].y += particles[n].speedy;
        }
        else{
          particles[n].x += particles[n].speedx / 2;
          particles[n].y += particles[n].speedy / 2;
        }
        if(particles[n].x < 0 || particles[n].x > 128 || particles[n].y < 0 || particles[n].y > 128)
            particles[n].time = 0;
      }
}

int8_t getActorInXY(int16_t x, int16_t y){
  for(int n = 0; n < 32; n++){
    if(actor_table[n].lives > 0)
      if(actor_table[n].x < x && actor_table[n].x + actor_table[n].width > x &&
        actor_table[n].y < y  && actor_table[n].y + actor_table[n].height > y)
          return n;
  }
  return -1;
}

void moveActor(int16_t i) {
  int n = i+1;
  if (i == -1) {
    i = 0;
    n = 32;
  }
 for(; i < n; i++) 
  if(actor_table[i].lives > 0){
    int16_t event = 0;
    actor_table[i].speedy += actor_table[i].gravity;
    actor_table[i].x += actor_table[i].speedx;
    actor_table[i].y += actor_table[i].speedy;
    if (coord(actor_table[i].x + actor_table[i].width) < 0 || coord(actor_table[i].x) > 127) {
      event |= ACTOR_X_EVENT;
    }
    if (coord(actor_table[i].y + actor_table[i].height) < 0 || coord(actor_table[i].y) > 127) {
      event |= ACTOR_Y_EVENT;
    }
    if (event != 0 && actor_table[i].onexitscreen > 0)
      setinterrupt(actor_table[i].onexitscreen, i | event);
    if (actor_table[i].onanimate > 0)
      setinterrupt(actor_table[i].onanimate, i);
  }
}

inline uint8_t getTile(int16_t x, int16_t y){
  return tile_map[TILEMAP_ADDR(x,y)];
}

inline void setTile(int16_t x, int16_t y, uint8_t v){
  tile_map[TILEMAP_ADDR(x,y)] = v;
}

// EFGYYYYYMXXXXXXX
// EFGIIIIIA00NNNNN

void testActorCollision(){
  byte n, i;
  for(n = 0; n < 32; n++)
    actor_table[n].collision = -1;
  for(n = 0; n < 32; n++){
    if(actor_table[n].lives > 0){
      for(i = n+1; i < 32; i++){
        if(actor_table[i].lives > 0){
          if(actor_table[n].x < actor_table[i].x + actor_table[i].width && 
          actor_table[n].x + actor_table[n].width > actor_table[i].x &&
          actor_table[n].y < actor_table[i].y + actor_table[i].height && 
          actor_table[n].y + actor_table[n].height > actor_table[i].y){
		int16_t nevent = 0;
		int16_t ievent = 0;
		if (actor_table[n].speedx != 0) nevent |= ACTOR_X_EVENT;
		if (actor_table[n].speedy != 0) nevent |= ACTOR_Y_EVENT;
		if (actor_table[i].speedx != 0) ievent |= ACTOR_X_EVENT;
		if (actor_table[i].speedy != 0) ievent |= ACTOR_Y_EVENT;
		if (nevent == 0) nevent = ACTOR_IN_EVENT;
		if (ievent == 0) ievent = ACTOR_IN_EVENT;
            if(actor_table[n].oncollision > 0)
              setinterrupt(actor_table[n].oncollision, ((i << 8) + n) | nevent);
            if(actor_table[i].oncollision > 0)
              setinterrupt(actor_table[i].oncollision, ((n << 8) + i) | ievent);
          }
        }
      }
    }
  }
}

void testActorMap(int16_t tx, int16_t ty, int16_t tw, int16_t th, uint8_t flags) {
  int16_t x0, y0;
  uint16_t event = 0;
  for(int n = 0; n < 32; n++){
    actor_table[n].collision = -1;
    event = 0;
    if(actor_table[n].lives > 0){
      if((actor_table[n].flags & flags) != 0){
          // check these to be more than 0
          // coordshift here!
          x0 = ((coord(actor_table[n].x + actor_table[n].width / 2) - tx) / (int16_t)SPRITE_WIDTH);
          y0 = ((coord(actor_table[n].y + actor_table[n].height / 2) - ty + SPRITE_HEIGHT) / (int16_t)SPRITE_HEIGHT) - 1;
          if(x0 >= 0 && x0 < tw && y0 >= 0 && y0 < th){
            if(sprite_flags[getTile(x0,y0)] & flags){
              actor_table[n].collision = (y0 << 8) + x0;
              if(actor_table[n].oncollision > 0)
                setinterrupt(actor_table[n].oncollision, n | ACTOR_IN_EVENT | ACTOR_MAP_COLL);
            } else {
              // all these require coord adjustments...
              if(actor_table[n].speedy > 0 && (sprite_flags[getTile(x0, y0 + 1)] & flags)){
                if((ty + (y0 + 1) * SPRITE_HEIGHT) - coord(actor_table[n].y + actor_table[n].height) < coord(actor_table[n].speedy * 2)){
                  event |= ACTOR_Y_EVENT;
                }
              } else if(actor_table[n].speedy < 0 && (sprite_flags[getTile(x0, y0 - 1)] & flags)){
                if(coord(actor_table[n].y) - (ty + y0 * SPRITE_HEIGHT) < coord(actor_table[n].speedy * 2)){
                  event |= ACTOR_Y_EVENT;
                }
              }
              if(actor_table[n].speedx > 0  && (sprite_flags[getTile(x0 + 1, y0)] & flags)){
                if((tx + (x0 + 1) * SPRITE_WIDTH - coord(actor_table[n].width)) - coord(actor_table[n].x) < coord(actor_table[n].speedx * 2)){
                  event |= ACTOR_X_EVENT;
                }
              } else if(actor_table[n].speedx < 0 && (sprite_flags[getTile(x0 - 1, y0)] & flags)){
                if(coord(actor_table[n].x) - (tx + x0 * SPRITE_WIDTH) < coord(actor_table[n].speedx * 2)){
                  event |= ACTOR_X_EVENT;
                }
              }
              if (event) {
                actor_table[n].collision = (y0 << 8) + x0;
                actor_table[n].collision |= ACTOR_MAP_COLL;
                setinterrupt(actor_table[n].oncollision, n | event | ACTOR_MAP_COLL);
              }
            } 
          }
        }   
    }
  }
}

inline void clearScr(uint8_t color){
  /*
  for(uint8_t y = 0; y < 128; y ++){
    for(uint8_t x = 0; x < 128; x++)
      setPix(x, y, color);
  }
  */
  
  uint8_t twocolor = ((drwpalette[color] << 4) | (drwpalette[color] & 0x0f));
  memset(screen, twocolor, SCREEN_SIZE);
  memset(line_is_draw, 3, 128);
}

void setImageSize(uint8_t size){
  espico.imageSize = size;
}

void setActorPosition(int8_t n, uint16_t x, uint16_t y){
  actor_table[n].x = x;
  actor_table[n].y = y;
}

void actorSetDirectionAndSpeed(int8_t n, uint16_t speed, int16_t dir){
  // coordshift here?
  actor_table[n].speedx = ((speed * getCos(dir)) >> 6);
  actor_table[n].speedy = ((speed * getSin(dir)) >> 6);
}



int16_t angleBetweenActors(int8_t n1, int8_t n2){
  // using fixed points will probably cause overflow and thus requires coord adjustments
  int16_t A = atan2_rb(coord(actor_table[n1].y - actor_table[n2].y), coord(actor_table[n1].x - actor_table[n2].x));
 // A = (A < 0) ? A + 360 : A;
  return A;
}


int16_t getActorValue(int8_t n, uint8_t t){
  switch(t){
    case 0:
      return actor_table[n].x;
    case 1:
      return actor_table[n].y;
    case 2:
      return actor_table[n].speedx;
    case 3:
      return actor_table[n].speedy;
    case 4:
      return actor_table[n].width;
    case 5:
      return actor_table[n].height;
    case 6:
      return actor_table[n].angle;
    case 7:
      return actor_table[n].lives;
    case 8:
      return actor_table[n].collision;
    case 9:
      return actor_table[n].flags;
    case 10:
      return actor_table[n].gravity;
    case 15:
      return actor_table[n].sprite;
    case 16:
      return actor_table[n].frame;
    case 17:
      return actor_table[n].sw;
    case 18:
      return actor_table[n].sh;
  }
  return 0;
}

void setActorValue(int8_t n, uint8_t t, int16_t v){
 switch(t){
    case 0:
      actor_table[n].x = v;
      return;
    case 1:
      actor_table[n].y = v;
      return;
    case 2:
      actor_table[n].speedx = v;
      return;
    case 3:
      actor_table[n].speedy = v;
      return;
    case 4:
      actor_table[n].width = v;
      return;
    case 5:
      actor_table[n].height = v;
      return;
    case 6:
      v = v % 360;
      if(v < 0)
        v += 360;
      actor_table[n].angle = v;
      return;
    case 7:
      actor_table[n].lives = v;
      return;
    case 8:
      // Collision cannot be set
      return;
    case 9:
      actor_table[n].flags = v;
      return;
    case 10:
      actor_table[n].gravity = v;
      return;
    case 11:
      actor_table[n].oncollision = (uint16_t)v;
      return;
    case 12:
      actor_table[n].onexitscreen = (uint16_t)v;
      return;
    case 13:
      actor_table[n].onanimate = (uint16_t)v;
      return;
    case 15:
      actor_table[n].sprite = (v < SPRITE_COUNT) ? v : 0; 
      return;
    case 16:
      actor_table[n].frame = v; 
      return;
    case 17:
      actor_table[n].sw = v; 
      return;
    case 18:
      actor_table[n].sh = v; 
      return;
 }
}

void drawSprite(int8_t n, int16_t x0, int16_t y0, int16_t w, int16_t h){
  drawImg(SPRITE_MEMMAP+SPRITE_ADDR(n), x0, y0, w, h);
  /*
  uint8_t *adr = (uint8_t *)&(sprite_map[SPRITE_ADDR(n)]);
  uint8_t w2 = w >> 1;
  uint8_t w1 = w & 1;
  uint8_t pixel;
  int16_t x;

  for(byte y1 = 0; y1 < h; y1 ++) {
    x = x0;
    for(byte x1 = 0; x1 < w2; x1++){
      pixel = adr[SPRITE_PIX(x1,y1)];
      if(PIX_LEFT_MASK(pixel) > 0)
        setPix(x, y0+y1, GET_PIX_LEFT(pixel));
      if(PIX_RIGHT_MASK(pixel) > 0)
        setPix(x+1, y0+y1, GET_PIX_RIGHT(pixel));
      x += 2;
    }
    if (w1) {
      pixel = adr[SPRITE_PIX(w2,y1)];
      if(PIX_LEFT_MASK(pixel) > 0)
        setPix(x, y0+y1, GET_PIX_LEFT(pixel));
    }
  }
  */
}

void drawActor(int8_t i) {  
  int n = i+1;
  if (i == -1) {
    i = 0;
    n = 32;
  }
  for(; i < n; i++)
    if (actor_table[i].lives > 0)
      // coord adjustments here!
      drawSprite(actor_table[i].sprite+actor_table[i].frame, coord(actor_table[i].x)-(actor_table[i].sw >> 1), coord(actor_table[i].y)-(actor_table[i].sh >> 1), actor_table[i].sw, actor_table[i].sh);
}

#define IS_TRANSPARENT(col) (espico.palt & (1 << col))

void drawImg(int16_t a, int16_t x, int16_t y, int16_t w, int16_t h){
  int add_next_row = 0;
  if (a >= SPRITE_MEMMAP) {
    add_next_row = 64 - (w >> 1);
  }
  if(espico.imageSize > 1){
    drawImgS(a, x, y, w, h, add_next_row);
  } else {
  uint8_t p, color;
  for(int16_t yi = 0; yi < h; yi++) {
    for(int16_t xi = 0; xi < w; xi++){
      p = readMem(a);
      color = GET_PIX_LEFT(p);
      if(!IS_TRANSPARENT(color)){
        setPix(xi + x, yi + y, drwpalette[color]);
      }
      xi++;
      color = GET_PIX_RIGHT(p);
      if(!IS_TRANSPARENT(color)){
        setPix(xi + x, yi + y, drwpalette[color]);
      }
      a++;
    }
    a += add_next_row;
  }
  }
}

void drawImageBit(int16_t adr, int16_t x1, int16_t y1, int16_t w, int16_t h){
  if(espico.imageSize > 1){
    drawImageBitS(adr, x1, y1, w, h);
    return;
  }
  int16_t i = 0;
  uint8_t ibit;
  const int8_t fgcolor = (IS_TRANSPARENT(espico.color)) ? -1 : drwpalette[espico.color];
  const int8_t bgcolor = drwpalette[espico.bgcolor];
  // const int8_t bgcolor = (IS_TRANSPARENT(espico.bgcolor)) ? -1 : drwpalette[espico.bgcolor];
  for(int16_t y = 0; y < h; y++)
    for(int16_t x = 0; x < w; x++){
      if(i % 8 == 0){
        ibit = readMem(adr);
        adr++;
      }
      if (ibit & 0x80) {
        if (fgcolor >= 0)
          setPix(x1 + x, y1 + y, fgcolor);
      } else if (bgcolor >= 0) {
        setPix(x1 + x, y1 + y, bgcolor);
      }
      ibit = ibit << 1;
      i++;
    }
}

void drawImgS(int16_t a, int16_t x, int16_t y, int16_t w, int16_t h, int add_next_row){
  uint8_t p, color;
  const uint8_t s = espico.imageSize;
  const int16_t ws = w * s;
  const int16_t w1 = (w & 1);
  const int16_t hs = h * s;
  for(int16_t yi = 0; yi < hs; yi += s) {
    for(int16_t xi = 0; xi < ws; xi += s){
      p = readMem(a);
      color = GET_PIX_LEFT(p);
      if(!IS_TRANSPARENT(color)){
        for(int jy = 0; jy < s; jy++)
          for(int jx = 0; jx < s; jx++)
            setPix(x + xi + jx, y + yi + jy, drwpalette[color]);
      }
      xi += s;
      color = GET_PIX_RIGHT(p);
      if(!IS_TRANSPARENT(color)){
        for(int jy = 0; jy < s; jy++)
          for(int jx = 0; jx < s; jx++)
            setPix(x + xi + jx, y + yi + jy, drwpalette[color]);
      }
      a++;
    }
    a += add_next_row;
  }
}

void drawImageBitS(int16_t adr, int16_t x1, int16_t y1, int16_t w, int16_t h){
  int16_t size = w * h / 8;
  int16_t i = 0;
  uint8_t ibit, jx, jy;
  const uint8_t s = espico.imageSize;
  const int16_t ws = w * s;
  const int16_t hs = h * s; 
  const int8_t fgcolor = (IS_TRANSPARENT(espico.color)) ? -1 : drwpalette[espico.color];
  const int8_t bgcolor = drwpalette[espico.bgcolor];
  // const int8_t bgcolor = (IS_TRANSPARENT(espico.bgcolor)) ? -1 : drwpalette[espico.bgcolor];
  for(int16_t y = 0; y < hs; y += s)
    for(int16_t x = 0; x < ws; x += s){
      if(i % 8 == 0){
        ibit = readMem(adr);
        adr++;
      }
      if (ibit & 0x80) {
        if (fgcolor >= 0) 
          for(int jy = 0; jy < s; jy++)
            for(int jx = 0; jx < s; jx++)
              setPix(x1 + x + jx, y1 + y + jy, fgcolor);
      } 
      else if (bgcolor >= 0) {
        for(int jy = 0; jy < s; jy++)
          for(int jx = 0; jx < s; jx++)
            setPix(x1 + x + jx, y1 + y + jy, bgcolor);
      } 
      ibit = ibit << 1;
      i++;
    }
}

void drawTileMap(int16_t x0, int16_t y0, int16_t celx, int16_t cely, int16_t celw, int16_t celh, uint8_t layer){
    int16_t x, y, nx, ny;
    uint8_t spr;
    for(x = 0; x < celw; x++){
      nx = x0 + x * 8;
      for(y = 0; y < celh; y++){
        spr = getTile(celx+x, cely+y);
        ny = y0 + y * 8;
        if (spr > 0 && ((sprite_flags[spr] & layer) == layer)) drawSprite(spr, nx, ny, 8, 8);
      }
    }
}
/*
void testTileMap() {
  drawTileMap(0,8,0,0,16,16);
}
*/

inline void drawFVLine(int16_t x, int16_t y1, int16_t y2, uint8_t color){
  for(int16_t  i = y1; i <= y2; i++)
    setPix(x, i, color);
}

inline void drawFHLine(int16_t x1, int16_t x2, int16_t y, uint8_t color){
  for(int16_t  i = x1; i <= x2; i++)
    setPix(i, y, color);
}

void drwLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
  const uint8_t fgcolor = drwpalette[espico.color];
    if(x1 == x2){
      if(y1 > y2)
        drawFVLine(x1, y2, y1, fgcolor);
      else
        drawFVLine(x1, y1, y2, fgcolor);
      return;
    }
    else if(y1 == y2){
      if(x1 > x2)
        drawFHLine(x2, x1, y1, fgcolor);
      else
        drawFHLine(x1, x2, y1, fgcolor);
      return;
    }
    int16_t deltaX = abs(x2 - x1);
    int16_t deltaY = abs(y2 - y1);
    int16_t signX = x1 < x2 ? 1 : -1;
    int16_t signY = y1 < y2 ? 1 : -1;
    int16_t error = deltaX - deltaY;
    int16_t error2;
    setPix(x2, y2, fgcolor);
    while(x1 != x2 || y1 != y2){
      setPix(x1, y1, fgcolor);
      error2 = error * 2;
      if(error2 > -deltaY){
        error -= deltaY;
        x1 += signX;
      }
      if(error2 < deltaX){
        error += deltaX;
        y1 += signY;
      }
    }
  }

inline void setPix(uint16_t x, uint16_t y, uint8_t c){
  uint8_t xi = x / 2;
  uint8_t b;
  if(x < 128 && y < 128){
    b = screen[SCREEN_ADDR(xi, y)];
    if(x & 1)
      SET_PIX_RIGHT(screen[SCREEN_ADDR(xi, y)], c);
    else
      SET_PIX_LEFT(screen[SCREEN_ADDR(xi, y)], c);
    if(b != screen[SCREEN_ADDR(xi, y)])
      line_is_draw[y] |= 1 + x / 64;
  }
}
/*
inline void setPix2(int16_t xi, int16_t y, uint8_t c){
  if(xi < 64 && y < 128){
    if (screen[SCREEN_ADDR(xi, y)] != c) {
      screen[SCREEN_ADDR(xi, y)] = c;
      line_is_draw[y] |= 1 + xi /32;
    }
  }
}
*/
byte getPix(byte x, byte y){
  byte b = 0;
  int16_t xi = x / 2;
  if(x >= 0 && x < 128 && y >= 0 && y < 128){
    if(x % 2 == 0)
      b = GET_PIX_LEFT(screen[SCREEN_ADDR(xi, y)]);
    else
      b = GET_PIX_RIGHT(screen[SCREEN_ADDR(xi, y)]);
  }
  return b;
}

void changePalette(uint8_t n, uint16_t c){
  c &= 0xf;
  if(n < 16){
    drwpalette[n] = c&0xf;
  } else if (n < 32) {
    n = n - 16;
    palette[n] = (uint16_t)pgm_read_word_near(epalette + c);
    for(uint8_t y = 0; y < 128; y++){
      for(uint8_t x = 0; x < 64; x++){
        if((GET_PIX_LEFT(screen[SCREEN_ADDR(x, y)]) == n || GET_PIX_RIGHT(screen[SCREEN_ADDR(x, y)]) == n))
          line_is_draw[y] |= 1 + x / 32;
      }
    }
  }
}

void setPalT(uint16_t palt) {
  espico.palt = palt;
}

void scrollScreen(uint8_t step, uint8_t direction){
    uint8_t bufPixel;
    if(direction == 2){
      for(uint8_t y = 0; y < 128; y++){
        bufPixel = screen[SCREEN_ADDR(0, y)];
        for(uint8_t x = 1; x < 64; x++){
          if(screen[SCREEN_ADDR(x - 1, y)] != screen[SCREEN_ADDR(x,y)])
            line_is_draw[y] |= 1 + x / 32;
          screen[SCREEN_ADDR(x - 1,  y)] = screen[SCREEN_ADDR(x,y)];
        }
        if(screen[SCREEN_ADDR(63, y)] != bufPixel)
            line_is_draw[y] |= 1;
        screen[SCREEN_ADDR(63, y)] = bufPixel;
      }
      /* for(uint8_t n = 0; n < 32; n++)
        if(actor_table[n].flags & 2)
          actor_table[n].x -= 2; */
    }
    else if(direction == 1){
      for(uint8_t x = 0; x < 64; x++){
        bufPixel = screen[SCREEN_ADDR(x, 0)];
        for(uint8_t y = 1; y < 128; y++){
          if(screen[SCREEN_ADDR(x, y-1)] != screen[SCREEN_ADDR(x,y)])
            line_is_draw[y] |= 1 + x / 32;
          screen[SCREEN_ADDR(x, y - 1)] = screen[SCREEN_ADDR(x,y)];
        }
        if(screen[SCREEN_ADDR(x, 127)] != bufPixel)
            line_is_draw[127] |= 2;
        screen[SCREEN_ADDR(x, 127)] = bufPixel;
      }
      /* for(uint8_t n = 0; n < 32; n++)
        if(actor_table[n].flags & 2)
          actor_table[n].y--; */
    }
    else if(direction == 0){
      for(uint8_t y = 0; y < 128; y++){
        bufPixel = screen[SCREEN_ADDR(63, y)];
        for(uint8_t x = 63; x > 0; x--){
          if(screen[SCREEN_ADDR(x,y)] != screen[SCREEN_ADDR(x - 1, y)])
            line_is_draw[y] |= 1 + x / 32;
          screen[SCREEN_ADDR(x,y)] = screen[SCREEN_ADDR(x - 1, y)];
        }
        if(screen[SCREEN_ADDR(0, y)] != bufPixel)
            line_is_draw[y] |= 1;
        screen[SCREEN_ADDR(0, y)] = bufPixel;
      }
      /* for(uint8_t n = 0; n < 32; n++)
        if(actor_table[n].flags & 2)
          actor_table[n].x += 2; */
    }
    else {
      for(uint8_t x = 0; x < 64; x++){
        bufPixel = screen[SCREEN_ADDR(x, 127)];
        for(uint8_t y = 127; y > 0; y--){
          if(screen[SCREEN_ADDR(x,y)] != screen[SCREEN_ADDR(x, y - 1)])
            line_is_draw[y] |= 1 + x / 32;
          screen[SCREEN_ADDR(x,y)] = screen[SCREEN_ADDR(x, y - 1)];
        }
        if(screen[SCREEN_ADDR(x, 0)] != bufPixel)
            line_is_draw[0] |= 1 + x / 32;
        screen[SCREEN_ADDR(x, 0)] = bufPixel;
      }
      /* for(uint8_t n = 0; n < 32; n++)
        if(actor_table[n].flags & 2)
          actor_table[n].y++; */
    }
}

void charLineUp(byte n){
  clearScr(espico.bgcolor);
  for(uint16_t i = 0; i < 336 - n * 21; i++){
    charArray[i] = charArray[i + n * 21];
    putchar(charArray[i], (i % 21) * 6, (i / 21) * 8);
  }
}

inline void setCharX(int8_t x){
  espico.regx = x;
}

inline void setCharY(int8_t y){
  espico.regy = y;
}

void printc(char c, byte fc, byte bc){
  if(c == '\n'){
    fillRect(espico.regx * 6, espico.regy * 8, 127, espico.regy * 8 + 7);
    for(byte i = espico.regx; i <= 21; i++){
      charArray[espico.regx + espico.regy * 21] = ' ';
    }
    espico.regy++;
    espico.regx = 0;
    if(espico.regy > 15){
      espico.regy = 15;
      charLineUp(1);
    }
  }
  else if(c == '\t'){
    for(byte i = 0; i <= espico.regx % 5; i++){
      fillRect(espico.regx * 6, espico.regy * 8, espico.regx * 6 + 5, espico.regy * 8 + 7);
      charArray[espico.regx + espico.regy * 21] = ' ';
      espico.regx++;
      if(espico.regx > 21){
        espico.regy++;
        espico.regx = 0;
        if(espico.regy > 15){
          espico.regy = 15;
          charLineUp(1);
        }
      }
    }
  }
  else{
    fillRect(espico.regx * 6, espico.regy * 8, espico.regx * 6 + 5, espico.regy * 8 + 7);
    putchar(c, espico.regx * 6, espico.regy * 8);
    charArray[espico.regx + espico.regy * 21] = c;
    espico.regx++;
    if(espico.regx > 20){
      espico.regy++;
      espico.regx = 0;
      if(espico.regy > 15){
        espico.regy = 15;
        charLineUp(1);
      }
    }
  }
}

inline void setColor(uint8_t c){
  espico.color = c & 0xf;
}

void drawRect(int8_t x0, int8_t y0, int8_t x1, uint8_t y1){
  const uint8_t fgcolor = drwpalette[espico.color];
  drawFHLine(x0, x1, y0, fgcolor);
  drawFHLine(x0, x1, y1, fgcolor);
  drawFVLine(x0, y0, y1, fgcolor);
  drawFVLine(x1, y1, y1, fgcolor);
}

void fillRect(int8_t x0, int8_t y0, int8_t x1, uint8_t y1){
  const uint8_t bgcolor = drwpalette[espico.bgcolor];
  for(int16_t jy = y0; jy <= y1; jy++)
    drawFHLine(x0, x1, jy, bgcolor);
}

void drawCirc(int16_t x0, int16_t y0, int16_t r) {
  const uint8_t fgcolor = drwpalette[espico.color];
  int16_t  x  = 0;
  int16_t  dx = 1;
  int16_t  dy = r+r;
  int16_t  p  = -(r>>1);

  // These are ordered to minimise coordinate changes in x or y
  // drawPixel can then send fewer bounding box commands
  setPix(x0 + r, y0, fgcolor);
  setPix(x0 - r, y0, fgcolor);
  setPix(x0, y0 - r, fgcolor);
  setPix(x0, y0 + r, fgcolor);

  while(x<r){

    if(p>=0) {
      dy-=2;
      p-=dy;
      r--;
    }

    dx+=2;
    p+=dx;

    x++;

    // These are ordered to minimise coordinate changes in x or y
    // drawPixel can then send fewer bounding box commands
    setPix(x0 + x, y0 + r, fgcolor);
    setPix(x0 - x, y0 + r, fgcolor);
    setPix(x0 - x, y0 - r, fgcolor);
    setPix(x0 + x, y0 - r, fgcolor);

    setPix(x0 + r, y0 + x, fgcolor);
    setPix(x0 - r, y0 + x, fgcolor);
    setPix(x0 - r, y0 - x, fgcolor);
    setPix(x0 + r, y0 - x, fgcolor);
  }
}

void fillCirc(int16_t x0, int16_t y0, int16_t r) {
  const uint8_t bgcolor = drwpalette[espico.bgcolor];
  int16_t  x  = 0;
  int16_t  dx = 1;
  int16_t  dy = r+r;
  int16_t  p  = -(r>>1);

  drawFHLine(x0 - r, y0, x0 + r, bgcolor);

  while(x<r){

    if(p>=0) {
      dy-=2;
      p-=dy;
      r--;
    }

    dx+=2;
    p+=dx;

    x++;

    drawFHLine(x0 - r, y0 + x, x0 + r, bgcolor);
    drawFHLine(x0 - r, y0 - x, x0 + r, bgcolor);
    drawFHLine(x0 - x, y0 + r, x0 + x, bgcolor);
    drawFHLine(x0 - x, y0 - r, x0 + x, bgcolor);

  }
}

void putString(char s[], int8_t y){
  int8_t i = 0;
  while(s[i] != 0 && i < 21){
    putchar(s[i], i * 6, y);
    i++;
  }
}

void putchar(char c, uint8_t x, uint8_t y) {
  const uint8_t fgcolor = drwpalette[espico.color];
    for(int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
      uint8_t line = pgm_read_byte(&font_a[c * 5 + i]);
      for(int8_t j=0; j<8; j++, line >>= 1) {
        if(line & 1)
         setPix(x+i, y+j, fgcolor);
      }
  }
}

#pragma GCC pop_options

