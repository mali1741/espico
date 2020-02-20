#include "font_a.c"

#ifdef ESPBOY
#define DISPLAY_X_OFFSET 0
#else
#define DISPLAY_X_OFFSET 12
#endif

#define SCREEN_WIDTH 128
#define SCREEN_WIDTH_BYTES 64
#define SCREEN_HEIGHT 128
#define SCREEN_SIZE (SCREEN_HEIGHT * SCREEN_WIDTH_BYTES)
#define LINE_REDRAW_SIZE SCREEN_HEIGHT

#define SCREEN_ARRAY_DEF SCREEN_SIZE
#define SCREEN_ADDR(x, y) ((int(y) << 6) + int(x))

#define SPRITE_HEIGHT 8
#define SPRITE_WIDTH  8
#define SPRITE_WIDTH_BYTES 4
#define SPRITE_COUNT 128
#define SPRITE_MAP_SIZE 4096
#define SPRITE_FLAGS_SIZE 256
#define SPRITE_ARRAY_DEF SPRITE_MAP_SIZE
#define SPRITE_ADDR(n)  (((int)(n & 0xf0) << 5) + ((n & 0x0f) << 2))
#define SPRITE_PIX(x, y) ((int(y) << 6) + int(x))
#define TILEMAP_SIZE 4096
#define TILEMAP_ADDR(x, y) ((int((y)&31) << 7) + ((x)&127))

#define PIX_LEFT_MASK(p)  ((p) & 0xf0)
#define GET_PIX_LEFT(p)  ((p) >> 4)
#define PIX_RIGHT_MASK(p) ((p) & 0x0f)
#define GET_PIX_RIGHT(p) PIX_RIGHT_MASK(p)
#define SET_PIX_LEFT(p,c)  p = (PIX_RIGHT_MASK(p) + ((c) << 4))
#define SET_PIX_RIGHT(p,c) p = (PIX_LEFT_MASK(p) + ((c) & 0x0f))

#define PARTICLE_COUNT  32
#define PARTICLE_SHRINK 0x10
#define PARTICLE_GRAV   0x20
#define PARTICLE_FRIC   0x40
#define PARTICLE_STAR   0x80

#define FLIP_Y_ON (espico.imageSize & 0x80)
#define FLIP_X_ON ((espico.imageSize << 1) & 0x80)

struct EspicoState {
  int8_t   drawing;
  int8_t   color;
  int8_t   bgcolor;
  int8_t   imageSize;
  int16_t  fillpattern;
  int16_t  palt;
  int16_t  clipx0;
  int16_t  clipx1;
  int16_t  clipy0;
  int16_t  clipy1;
  int16_t  camx;
  int16_t  camy;
  int8_t   nlregx;
  int8_t   regx;
  int8_t   regy;
  uint8_t  coordshift;
};  

#define ACTOR_IN_EVENT    0x1000
#define ACTOR_X_EVENT     0x0300
#define ACTOR_Y_EVENT     0x0C00
#define ACTOR_T_EVENT     0x0800
#define ACTOR_B_EVENT     0x0400
#define ACTOR_L_EVENT     0x0200
#define ACTOR_R_EVENT     0x0100
#define ACTOR_ANIM_EVENT  0x8000
#define ACTOR_MAP_COLL    0x8000
#define ACTOR_OPTS_EXIT_ONLY 0x0200

struct Actor {
  uint8_t sprite;
  int8_t  frame;
  uint8_t sw;
  uint8_t sh;

  int8_t  lives;
  uint8_t flags; // 0 0 0 0 0 0 scrolled solid
  int16_t refval;

  int16_t x;
  int16_t y;

  int16_t hw;
  int16_t hh;

  int16_t speedx;
  int16_t speedy;

  int16_t gravity;
  uint16_t opts_angle;   //  XY00 00EA AAAA AAAA    - bits use: A - angle (0-359) , E - cb_anim on exit only, V - flip X, H - flip Y 

  uint16_t oncollision;
  uint16_t onanimate;
};

struct Particle {
  uint8_t time;
  uint8_t radpt;
  uint8_t radq;
  uint8_t color;

  int8_t x;
  int8_t y;
  int8_t speedx;
  int8_t speedy;
};

struct Emitter { 
  uint8_t timeparticle;
  uint8_t timediff;
  uint8_t nextp;
  int8_t gravity;

  int8_t speedx;
  int8_t speedy;
  int8_t speedx1;
  int8_t speedy1;
};
/*
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
*/
static const uint8_t isinTable8[] PROGMEM = {
    0,   4,   9,  13,  18,  22,  27,  31,  35,  40, 
   44,  49,  53,  57,  62,  66,  70,  75,  79,  83,
   87,  91,  96, 100, 104, 108, 112, 116, 120, 124, 

  128, 131, 135, 139, 143, 146, 150, 153, 157, 160,
  164, 167, 171, 174, 177, 180, 183, 186, 190, 192,
  195, 198, 201, 204, 206, 209, 211, 214, 216, 219,
  221, 223, 225, 227, 229, 231, 233, 235, 236, 238, 
  240, 241, 243, 244, 245, 246, 247, 248, 249, 250,
  251, 252, 253, 253, 254, 254, 254, 255, 255, 255
};

static const uint16_t epalette[] PROGMEM = {
     0x0000, 0x194A, 0x792A, 0x042A, 0xAA86, 0x5AA9, 0xC618, 0xFF9D,
     0xF809, 0xFD00, 0xFF64, 0x0726, 0x2D7F, 0x83B3, 0xFBB5, 0xFE75
};

uint16_t palette[16] __attribute__ ((aligned));
uint8_t  drwpalette[16] __attribute__ ((aligned));
uint16_t pix_buffer[256] __attribute__ ((aligned));

#define SCREEN_MEMMAP (PRG_SIZE+SPRITE_MAP_SIZE+TILEMAP_SIZE)
#define TILE_MEMMAP   (PRG_SIZE+SPRITE_MAP_SIZE)
#define SPRITE_MEMMAP (PRG_SIZE)

uint8_t *redrawscreen __attribute__ ((aligned));
uint8_t *screen __attribute__ ((aligned));
uint8_t *tile_map __attribute__ ((aligned));
uint8_t *sprite_map __attribute__ ((aligned));

// uint8_t sprite_flags[SPRITE_FLAGS_SIZE+LINE_REDRAW_SIZE+LINE_REDRAW_SIZE] __attribute__ ((aligned));
// uint8_t *line_redraw __attribute__ ((aligned)) = &sprite_flags[SPRITE_FLAGS_SIZE];
// uint8_t *line_draw __attribute__ ((aligned)) = &sprite_flags[SPRITE_FLAGS_SIZE];

uint8_t *sprite_flags __attribute__ ((aligned));
uint8_t *line_redraw __attribute__ ((aligned));
uint8_t *line_draw __attribute__ ((aligned));

struct Actor actor_table[32] __attribute__ ((aligned));
struct Particle particles[PARTICLE_COUNT] __attribute__ ((aligned));
struct Emitter emitter __attribute__ ((aligned));
volatile struct EspicoState espico __attribute__ ((aligned));
uint16_t frame_count = 0; 
// uint16_t seqr __attribute__ ((aligned)) = 1; 

#pragma GCC optimize ("-O2")
#pragma GCC push_options
inline uint8_t getSpriteFlag(uint16_t n) {
  return ((n < (SPRITE_FLAGS_SIZE+LINE_REDRAW_SIZE+LINE_REDRAW_SIZE)) ? sprite_flags[n] : 0);
}

inline void setSpriteFlag(uint16_t n, uint8_t v) {
  if (n < (SPRITE_FLAGS_SIZE+LINE_REDRAW_SIZE+LINE_REDRAW_SIZE)) sprite_flags[n] = v;
}

inline int16_t coord(int16_t c) {
  return (c / (1 << espico.coordshift));
}

int16_t isin(int16_t x) {
  boolean pos = true;  // positive - keeps an eye on the sign.
  if (x < 0)
  {
    x = -x;
    pos = !pos;
  }
  if (x >= 360) x %= 360;
  if (x > 180)
  {
    x -= 180;
    pos = !pos;
  }
  if (x > 90) x = 180 - x;
  if (x == 90) return ((pos) ? 128 : -128);
  if (pos) return (int16_t)(pgm_read_byte_near(isinTable8 + x)/2);
  return -(int16_t)(pgm_read_byte_near(isinTable8 + x)/2);
}

inline int16_t getCos(int16_t g){
  return isin(g+90);
}

inline int16_t getSin(int16_t g){
  return isin(g);
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
void memoryAlloc(){
  // mem = (uint8_t*)malloc(RAM_SIZE);
  mem = (uint8_t*)malloc(RAM_SIZE+SPRITE_FLAGS_SIZE+LINE_REDRAW_SIZE+LINE_REDRAW_SIZE);
  if(mem == NULL) {
    Serial.println(F("Out of memory"));
    return;
  }
  redrawscreen = &mem[SCREEN_MEMMAP];
  screen = &mem[SCREEN_MEMMAP];
  tile_map = &mem[TILE_MEMMAP];
  sprite_map = &mem[SPRITE_MEMMAP];
  
  sprite_flags = &mem[RAM_SIZE];
  line_redraw = &sprite_flags[SPRITE_FLAGS_SIZE];
  line_draw = line_redraw;
}

inline void setDrawAddr(uint16_t addr) {
  screen = &mem[(addr > SCREEN_MEMMAP) ? SCREEN_MEMMAP : addr];
  if (redrawscreen == screen) {
    line_draw = line_redraw;
  } else {
    line_draw = &sprite_flags[SPRITE_FLAGS_SIZE+LINE_REDRAW_SIZE];
    // memset(line_draw, 0, LINE_REDRAW_SIZE);    
  }
}

inline void resetDrawAddr() {
  screen = redrawscreen;
  line_draw = line_redraw;
}

void resetPalette() {
  for(int i = 0; i < 16; i++){
    palette[i] = (uint16_t)pgm_read_word_near(epalette + i);
    drwpalette[i] = i;
  }
  espico.palt = 1; // black is transparent
}

void resetActor(int16_t n) {
  int m = 0;

  if (n == -1) {
    n = 0;
    m = 32;
  } else {
    n = n & 31;
    m = n + 1;
  }

  for(; n < m; n++){
    actor_table[n].sprite = 0;
    actor_table[n].sw = 8;
    actor_table[n].sh = 8;
    actor_table[n].frame = 0;
    actor_table[n].x = 0x8000;
    actor_table[n].y = 0x8000;
    actor_table[n].hw = 0;
    actor_table[n].hh = 0;
    actor_table[n].speedx = 0;
    actor_table[n].speedy = 0;
    actor_table[n].lives = 0;
    actor_table[n].flags = 0;
    actor_table[n].gravity = 0;
    actor_table[n].opts_angle = 0;
    actor_table[n].refval = 0;
    actor_table[n].oncollision = 0;
    actor_table[n].onanimate = 0;
  }
}

void display_init(){
  resetDrawAddr();
  initEspicoState();
  resetPalette();
  resetActor(-1);
  // emitter.time = 0;
  for(int i = 0; i < PARTICLE_COUNT; i++)
    particles[i].time = 0;
  // reset screen redraw
  memset(line_redraw, 255, LINE_REDRAW_SIZE);
  // memset(screen, 0, SCREEN_WIDTH_BYTES*SCREEN_HEIGHT);
}

void initEspicoState() {
  espico.coordshift = 0;
  espico.drawing = 0;
  espico.color = 7;
  espico.bgcolor = 0;
  espico.fillpattern = 0;
  espico.imageSize = 1; 
  espico.nlregx = 0;
  espico.regx = 0;
  espico.regy = 0;
  espico.clipx0 = 0;
  espico.clipx1 = 128;
  espico.clipy0 = 0;
  espico.clipy1 = 128;
  espico.camx = 0;
  espico.camy = 0;
}

void setEspicoState(int16_t s, int16_t v) {
  switch (s) {
   case 0:
     espico.drawing = v;
     break;
   case 1:
     espico.coordshift = v & 0xf;
     break;
   case 2:
     espico.camx = v;
     break;
   case 3:
     espico.camy = v;
     break;
   }
}  

void setClip(int16_t x, int16_t y, int16_t w, int16_t h) {
   espico.clipx0 = (x < 0) ? 0 : (x & 127);
   espico.clipx1 = ((x + w) > 128) ? 128 : (x+w);
   espico.clipy0 = (y < 0) ? 0 : (y & 127);
   espico.clipy1 = ((y + h) > 128) ? 128 : (y+h);
}

inline uint8_t setlower8bits(uint16_t n){
  n |= (n >>  1);
  n |= (n >>  2);
  n |= (n >>  4);
  return (n & 0xff);
}

inline uint8_t highestbit(uint8_t l) {
  l = setlower8bits(l);
  return (l - (l >> 1));
}

inline uint8_t lowestbit(uint8_t l) {
  return (l & ((~l)+1));
}

inline int pos8bit(uint8_t b) {
  int p = 0;
  if (b & 0xAA) p |= 1;
  if (b & 0xCC) p |= 2;
  if (b & 0xF0) p |= 4;
  return p;
}

#ifdef _ODROID_GO_H_
inline void pushPixels(int num) {
  tft.startWrite();
  tft.writePixels(pix_buffer, num);
  tft.endWrite();
}
#else
inline void pushPixels(int num) {
  tft.pushColors(pix_buffer, num);
}
#endif

void redrawScreen() {
  int ypos = 0;
  uint8_t line_is_drawn = 0;
  if (espico.drawing) return;
  cadr_count++;
  frame_count++;
  for(int y = 0; y < 128; y++){
    // make every 8th line single
    // int yinc = ((y & 7) == 7) ? 1 : 2;
    int yinc = ((y & 1) && (y < 16 || y > 111)) ? 1 : 2;

    line_is_drawn = line_redraw[y];
    if (line_is_drawn == 0) {
      ypos += yinc;
      continue;
    }
    // reset line (in uncached multi-cpu this would probably save some bits from clearing)
    line_redraw[y] ^= line_is_drawn;
    // line_is_draw[y] is a bitfield representing 8*2 pixels for each bit
    // find highest and lowest set bits
    // then calculate start and stop bytes (2 pixels)
    int xstart = pos8bit(lowestbit(line_is_drawn)) * 8;
    int xend = (pos8bit(highestbit(line_is_drawn)) + 1) * 8;

#ifdef ESPBOY
    tft.setWindow(DISPLAY_X_OFFSET + (xstart * 2), y, DISPLAY_X_OFFSET - 1 + (xend * 2), y);
#else
    tft.setWindow(DISPLAY_X_OFFSET + (xstart * 4), ypos, DISPLAY_X_OFFSET - 1 + (xend * 4), ypos + yinc);
#endif

    // Each byte contains two pixels
    int i = 0;
/*    pix_buffer[i++] = palette[0x07];
    pix_buffer[i++] = palette[0x07];
    pix_buffer[i++] = palette[0x07];
    pix_buffer[i++] = palette[0x07];
*/
    for(int x = xstart; x < xend; x++){
#ifdef ESPBOY
        pix_buffer[i++] = palette[GET_PIX_LEFT(redrawscreen[SCREEN_ADDR(x,y)])];
        pix_buffer[i++] = palette[GET_PIX_RIGHT(redrawscreen[SCREEN_ADDR(x,y)])];
#else
        pix_buffer[i++] = pix_buffer[i++] = palette[GET_PIX_LEFT(redrawscreen[SCREEN_ADDR(x,y)])];
        pix_buffer[i++] = pix_buffer[i++] = palette[GET_PIX_RIGHT(redrawscreen[SCREEN_ADDR(x,y)])];
#endif
    }
/*    pix_buffer[i++] = palette[0x08];
    pix_buffer[i++] = palette[0x08];
    pix_buffer[i++] = palette[0x08];
    pix_buffer[i++] = palette[0x08];
*/
    pushPixels(i);

#ifndef ESPBOY
    // double line? then push again
    if (yinc == 2)
      pushPixels(i);

    ypos += yinc;
#endif
  }
  
  // memset(line_redraw, 0, 128);
  setRedraw();
}

int8_t randomD(int8_t a, int8_t b) {
  int8_t minv = a < b ? a : b;
  int8_t maxv = a > b ? a : b;
  return random(minv, maxv + 1);
}

inline uint8_t hibits(uint16_t n) {
  n |= (n >>  1);
  n |= (n >>  2);
  n |= (n >>  4);
  return n - (n >> 1) - 1;
}

void setParticleTime(uint16_t time, uint16_t timediff){
  emitter.timeparticle = (time <= 10000)?(time / 32):255;
  emitter.timediff = (time-timediff <= 10000)?((time-timediff)/ 32):255;
}

void setEmitter(int16_t gravity, int16_t dir, int16_t dir1, int16_t speed){
  emitter.gravity = coord(gravity);
  emitter.speedx = coord((speed * getCos(dir)) / 128);
  emitter.speedy = coord((speed * getSin(dir)) / 128);
  emitter.speedx1 = coord((speed * getCos(dir1)) / 128);
  emitter.speedy1 = coord((speed * getSin(dir1)) / 128);
}

uint16_t makeParticleColor(uint16_t col1, uint16_t col2, uint16_t prefsteps, uint16_t ptype) {
  return (((ptype & 0xf0) << 8) | ((prefsteps & 0xf)<<8) | ((col1 & 0xf) | (col2 << 4)));
}

inline uint8_t nearesthibit(uint16_t n){
  uint8_t u = setlower8bits(n);
  uint8_t l = u-(u >> 1);
  u += 1;
  uint8_t m = (u|l) >> 1;
  return (n & m == m)?u:l;
}
/*
inline uint8_t setlower8bits(uint16_t n){
  n |= (n >>  1);
  n |= (n >>  2);
  n |= (n >>  4);
  return (n & 0xff);
}
*/

void drawParticles(int16_t x, int16_t y, uint16_t pcolor, int16_t radpx, int16_t count){
  x = coord(x);
  y = coord(y);
  if (count <= 0 || x < 0 || x > 127 || y < 0 || y > 127) return;
  
  if (radpx < 0) radpx = 0;
  else if (radpx > 15) radpx = 15;
  // to enable colorfading without growing

  uint8_t ccolor = (pcolor & 0xff);
  uint8_t radpt = (pcolor >> 8) & 0xf0;
  uint8_t colsteps = (pcolor >> 8) & 0x0f;
  uint8_t radq = 255;
  if (radpx == 0) radpt |= PARTICLE_SHRINK;
  if (colsteps > 1) radq = nearesthibit(((uint16_t)emitter.timeparticle+1) / colsteps)-1;
  else radq = nearesthibit(((uint16_t)emitter.timeparticle+1) / (radpx+1))-1;
  radpt |= (radpt & PARTICLE_SHRINK) ? radpx : 0;
  // create count particles
  if (count > PARTICLE_COUNT) count = PARTICLE_COUNT;
  int n = emitter.nextp;
  do {
    if(particles[n].time == 0){
      particles[n].time = randomD(emitter.timeparticle, emitter.timediff);
      particles[n].x = x;
      particles[n].y = y;
      particles[n].radpt = radpt;
      particles[n].radq = radq;
      particles[n].color = ccolor;
      particles[n].speedx = randomD(emitter.speedx, emitter.speedx1);
      particles[n].speedy = randomD(emitter.speedy, emitter.speedy1);
      count--;
    }
    n++;
    if (n >= PARTICLE_COUNT) n = 0;
  } while ((count > 0) && (n != emitter.nextp));
  emitter.nextp = n;
}

void animateParticles(){
  for(int n = 0; n < PARTICLE_COUNT; n++)
    if(particles[n].time > 0){
      // calculate x and y
      int16_t x, y;
      uint8_t radpx = (particles[n].radpt & 0xf);
      uint8_t ptype = (particles[n].radpt & 0xf0);
      x = particles[n].x;
      y = particles[n].y;
      if (particles[n].color == 0){
        if (ptype & PARTICLE_STAR) {
          drwLine(x-radpx,y,x+radpx,y);
          drwLine(x,y-radpx,x,y+radpx);
        } else {
          if (espico.bgcolor == 0) drawCirc(x,y,radpx);
          else fillCirc(x,y,radpx);
        }
      } else {
        if (ptype & PARTICLE_STAR) {
          uint8_t tmpc = espico.color;
          espico.color = (particles[n].color & 0xf);
          drwLine(x-radpx,y,x+radpx,y);
          drwLine(x,y-radpx,x,y+radpx);
          espico.color = tmpc;
        } else {
          uint8_t tmpc = espico.bgcolor;
          espico.bgcolor = (particles[n].color & 0xf);
          fillCirc(x,y,radpx);
          espico.bgcolor = tmpc;
        }
      }
      // calc new radius
      if ((particles[n].time & particles[n].radq) == 0) {
        if ((particles[n].radpt & (PARTICLE_SHRINK | 0xf)) > PARTICLE_SHRINK) particles[n].radpt--;
        else if ((particles[n].radpt & (PARTICLE_SHRINK | 0xf)) < PARTICLE_SHRINK) particles[n].radpt++;

        // cycle color here 
        if ((particles[n].color & 0xf) > (particles[n].color >> 4)) particles[n].color -= 1;
        else if ((particles[n].color & 0xf) < (particles[n].color >> 4)) particles[n].color +=1;
        // particles[n].color = (particles[n].color << 4) | (particles[n].color >> 4);
      }

      if (particles[n].time & 1) {
        x += particles[n].speedx;
        y += particles[n].speedy;
        if (ptype & PARTICLE_GRAV) particles[n].speedy += emitter.gravity;
      } else {
        x += particles[n].speedx / 4;
        y += particles[n].speedy / 4;
      }
      if (ptype & PARTICLE_FRIC) {
        particles[n].speedx = particles[n].speedx / 2;
        particles[n].speedy = particles[n].speedy / 2;
      }

      // delete if outside screen
      if (x < 0 || x > 128 || y < 0 || y > 128) particles[n].time = 0;
      else {
        particles[n].time--;
        particles[n].x = x;
        particles[n].y = y;
      }
        
    }
}

int8_t getActorInXY(int16_t x, int16_t y){
  for(int n = 0; n < 32; n++){
    if(actor_table[n].lives > 0)
      if(abs(actor_table[n].x - x) < actor_table[n].hw &&
         abs(actor_table[n].y - y) < actor_table[n].hh)
          return n;
  }
  return -1;
}

#define ACTOR_ONLY_EXIT(i) (actor_table[i].opts_angle & ACTOR_OPT_EXIT_ONLY)

void moveActor(int16_t i) {
  int n = i+1;
  if (i == -1) {
    i = 0;
    n = 32;
  }
 for(; i < n; i++) 
  if(actor_table[i].lives > 0){
    int16_t event = 0;
    actor_table[i].x += actor_table[i].speedx;
    actor_table[i].y += actor_table[i].speedy;
    actor_table[i].speedy += actor_table[i].gravity;

    if (coord(actor_table[i].x + actor_table[i].hw) < 0) event = ACTOR_L_EVENT;
    else if (coord(actor_table[i].x) > 127) event = ACTOR_R_EVENT;
    
    if (coord(actor_table[i].y + actor_table[i].hh) < 0) event |= ACTOR_T_EVENT;
    else if (coord(actor_table[i].y) > 127) event |= ACTOR_B_EVENT;

    if (event == 0) event = ACTOR_IN_EVENT;
    event |= ACTOR_ANIM_EVENT;

    if (actor_table[i].onanimate > 0)
      setinterrupt(actor_table[i].onanimate, event | i, (frame_count & 31));
  }
}

inline uint8_t getTile(int16_t x, int16_t y){
  return tile_map[TILEMAP_ADDR(x,y)];
}

inline void setTile(int16_t x, int16_t y, uint8_t v){
  tile_map[TILEMAP_ADDR(x,y)] = v;
}

// 000ITBLR 000NNNNN   M00YYYYY EXXXXXXX
// A00ITBLR 000NNNNN   00000000 000IIIII

void testActorCollision(){
  byte n, i;
  for(n = 0; n < 32; n++){
    if(actor_table[n].lives > 0){
      for(i = n+1; i < 32; i++){
        if(actor_table[i].lives > 0){
          int16_t x0 = abs(actor_table[n].x - actor_table[i].x);
          int16_t y0 = abs(actor_table[n].y - actor_table[i].y);
          if(x0 < (actor_table[n].hw + actor_table[i].hw) &&
             y0 < (actor_table[n].hh + actor_table[i].hh)) {
            int16_t nevent = 0;
            int16_t ievent = 0;
            if (actor_table[n].x < actor_table[i].x) {
              if (actor_table[n].x > actor_table[i].x - actor_table[i].hw) nevent |= ACTOR_IN_EVENT;
              else nevent |= ACTOR_R_EVENT;
              if (actor_table[i].x < actor_table[n].x + actor_table[n].hw) ievent |= ACTOR_IN_EVENT;
              else ievent |= ACTOR_L_EVENT;
            } else if (actor_table[n].x > actor_table[i].x) {
              if (actor_table[i].x > actor_table[n].x - actor_table[n].hw) nevent |= ACTOR_IN_EVENT;
              else nevent |= ACTOR_L_EVENT;
              if (actor_table[n].x < actor_table[i].x + actor_table[i].hw) ievent |= ACTOR_IN_EVENT;
              else ievent |= ACTOR_R_EVENT;
            } else {
              nevent |= ACTOR_IN_EVENT;
              ievent |= ACTOR_IN_EVENT;
            }
            if (actor_table[n].y < actor_table[i].y) {
              if (actor_table[n].y > actor_table[i].y - actor_table[i].hh) nevent |= ACTOR_IN_EVENT;
              else nevent |= ACTOR_B_EVENT;
              if (actor_table[i].y < actor_table[n].y + actor_table[n].hh) ievent |= ACTOR_IN_EVENT;
              else ievent |= ACTOR_T_EVENT;
            } else if (actor_table[n].y > actor_table[i].y) {
              if (actor_table[i].y > actor_table[n].y - actor_table[n].hh) nevent |= ACTOR_IN_EVENT;
              else nevent |= ACTOR_T_EVENT;
              if (actor_table[n].y < actor_table[i].y + actor_table[i].hh) ievent |= ACTOR_IN_EVENT;
              else ievent |= ACTOR_B_EVENT;
            } else {
              nevent |= ACTOR_IN_EVENT;
              ievent |= ACTOR_IN_EVENT;
            }
            if (nevent & (ACTOR_X_EVENT | ACTOR_Y_EVENT)) {
              // not actually inside I
              nevent &= (ACTOR_X_EVENT | ACTOR_Y_EVENT);
            }
            if (ievent & (ACTOR_X_EVENT | ACTOR_Y_EVENT)) {
              // not actually inside N
              ievent &= (ACTOR_X_EVENT | ACTOR_Y_EVENT);
            }
            
            if(actor_table[n].oncollision > 0)
              setinterrupt(actor_table[n].oncollision, nevent | n, i);
            if(actor_table[i].oncollision > 0)
              setinterrupt(actor_table[i].oncollision, ievent | i, n);
          }
        }
      }
    }
  }
}

inline int testTile(int16_t celx, int16_t cely, int16_t x, int16_t y, int16_t tw, int16_t th, uint8_t flags) {
  return ((x >= 0 && x < tw && y >= 0 && y < th) && (sprite_flags[getTile(celx+x,cely+y)] & flags));
}


inline void testMapX(int16_t celx, int16_t cely, int16_t xs, int16_t xe, int16_t xdir, int16_t y, int16_t tw, int16_t th, uint8_t flags, int16_t n, int16_t event){
  for(int16_t x = xs; (xdir == 1) ? x<=xe : x>=xe; x+=xdir){
    if(testTile(celx,cely,x,y,tw,th,flags)){
      setinterrupt(actor_table[n].oncollision, event | n, (((cely+y) << 8) + (celx+x)) | ACTOR_MAP_COLL);
     }
  }
}

inline void testMapY(int16_t celx, int16_t cely, int16_t ys, int16_t ye, int16_t ydir, int16_t x, int16_t tw, int16_t th, uint8_t flags, int16_t n, int16_t event){
  for(int16_t y = ys; (ydir == 1) ? y<=ye : y>=ye; y+=ydir){
    if(testTile(celx,cely,x,y,tw,th,flags)){
      setinterrupt(actor_table[n].oncollision, event | n, (((cely+y) << 8) + (celx+x)) | ACTOR_MAP_COLL);
    }
  }
}

void testActorMap(int16_t celx, int16_t cely, int16_t sx, int16_t sy, int16_t tw, int16_t th, uint8_t flags) {
  int16_t x0, y0, x1, y1;
  uint16_t event = 0;
  for(int n = 0; n < 32; n++){
    event = 0;
    if(actor_table[n].lives > 0){
      if(actor_table[n].oncollision > 0){
        int16_t ydir, xdir, xm, ym;
        int16_t e_x0, e_x1, e_y0, e_y1;
        ydir = 1;
        xdir = 1;
        e_x0 = ACTOR_L_EVENT;
        e_y0 = ACTOR_T_EVENT;
        e_x1 = ACTOR_R_EVENT;
        e_y1 = ACTOR_B_EVENT;
        x0 = (((coord(actor_table[n].x + actor_table[n].speedx - actor_table[n].hw) - sx) / SPRITE_WIDTH));
        xm = (((coord(actor_table[n].x + actor_table[n].speedx) - sx) / SPRITE_WIDTH));
        x1 = (((coord(actor_table[n].x + actor_table[n].speedx + actor_table[n].hw) - sx) / SPRITE_WIDTH));
        y0 = (((coord(actor_table[n].y + actor_table[n].speedy - actor_table[n].hh) - sy) / SPRITE_HEIGHT));
        ym = (((coord(actor_table[n].y + actor_table[n].speedy) - sy) / SPRITE_HEIGHT));
        y1 = (((coord(actor_table[n].y + actor_table[n].speedy + actor_table[n].hh) - sy) / SPRITE_HEIGHT));
        if ((actor_table[n].speedx > 0 && x0 != xm) || (x1 == xm)){
          int16_t tmpx = x0;
          x0 = x1; x1 = tmpx;
          xdir = -1;
          e_x0 = ACTOR_R_EVENT;
          e_x1 = ACTOR_L_EVENT;
        }
        if ((actor_table[n].speedy > 0 && y0 != ym) || (y1 == ym)){
          int16_t tmpy = y0;
          y0 = y1; y1 = tmpy;
          ydir = -1;
          e_y0 = ACTOR_B_EVENT;
          e_y1 = ACTOR_T_EVENT;
        }
        if (x0 == xm) e_x0 = ACTOR_IN_EVENT;
        if (x1 == xm) e_x1 = ACTOR_IN_EVENT;
        if (y0 == ym) e_y0 = ACTOR_IN_EVENT;
        if (y1 == ym) e_y1 = ACTOR_IN_EVENT;

        for(int16_t y = y0+ydir; (ydir == 1) ? y<y1 : y>y1; y+=ydir){
          testMapX(celx,cely,x0+xdir,x1-xdir,xdir,y,tw,th,flags,n,ACTOR_IN_EVENT);
        }
        testMapX(celx,cely,x0+xdir,x1-xdir,xdir,y0,tw,th,flags,n,e_y0);
        if (y0 != y1) testMapX(celx,cely,x0+xdir,x1-xdir,xdir,y1,tw,th,flags,n,e_y1);
        testMapY(celx,cely,y0+ydir,y1-ydir,ydir,x0,tw,th,flags,n,e_x0);
        if (x0 != x1) testMapY(celx,cely,y0+ydir,y1-ydir,ydir,x1,tw,th,flags,n,e_x1);
        if (testTile(celx,cely,x0,y0,tw,th,flags))
          setinterrupt(actor_table[n].oncollision, e_x0 | e_y0 | n, (((cely+y0) << 8) + (celx+x0)) | ACTOR_MAP_COLL);
        if ((y0 != y1) && (testTile(celx,cely,x0,y1,tw,th,flags)))
          setinterrupt(actor_table[n].oncollision, e_x0 | e_y1 | n, (((cely+y1) << 8) + (celx+x0)) | ACTOR_MAP_COLL);
        if (x0 != x1){
          if (testTile(celx,cely,x1,y0,tw,th,flags))
            setinterrupt(actor_table[n].oncollision, e_x1 | e_y0 | n, (((cely+y0) << 8) + (celx+x1)) | ACTOR_MAP_COLL);
          if ((y0 != y1) && (testTile(celx,cely,x1,y1,tw,th,flags)))
            setinterrupt(actor_table[n].oncollision, e_x1 | e_y1 | n, (((cely+y1) << 8) + (celx+x1)) | ACTOR_MAP_COLL);
        }
      }
    }
  }
}

inline void clearScr(uint8_t color){
/* #if 0 // def _ODROID_GO_H_
  color = ((drwpalette[color] << 4) | (drwpalette[color] & 0x0f));
  memset(screen, color, SCREEN_WIDTH_BYTES*SCREEN_HEIGHT);
  memset(line_draw, 255, LINE_REDRAW_SIZE);
#else */
  uint8_t twocolor[SCREEN_WIDTH_BYTES];
  memset(twocolor, ((drwpalette[color] << 4) | (drwpalette[color] & 0x0f)), SCREEN_WIDTH_BYTES);
  for (int i = 0; i < SCREEN_HEIGHT; i++) {
    if (memcmp(screen+i*SCREEN_WIDTH_BYTES, twocolor, SCREEN_WIDTH_BYTES) != 0) { 
      line_draw[i] = 255;
      memcpy(screen+i*SCREEN_WIDTH_BYTES, twocolor, SCREEN_WIDTH_BYTES);
    }
  }
// #endif
  espico.nlregx = 0;
  espico.regx = 0;
  espico.regy = 0;
  setClip(0,0,128,128);
}

inline void setImageFlipXY(uint8_t fxy) {
  espico.imageSize = ((espico.imageSize & 0x1f) | (fxy & 0xe0));
}

inline void setImageSize(uint8_t size){
  espico.imageSize = ((espico.imageSize & 0xe0) | (size & 0x1f));
}

inline void setActorPosition(int8_t n, int16_t x, int16_t y){
  actor_table[n].x = x;
  actor_table[n].y = y;
}

void actorSetDirectionAndSpeed(int8_t n, int16_t speed, int16_t dir){
  // coordshift here?
  actor_table[n].speedx = ((speed * getCos(dir)) / 128);
  actor_table[n].speedy = ((speed * getSin(dir)) / 128);
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
      return actor_table[n].hw;
    case 5:
      return actor_table[n].hh;
    case 6:
      return actor_table[n].opts_angle & 0x01FF;
    case 7:
      return actor_table[n].lives;
    case 8:
      return actor_table[n].refval;
    case 9:
      return actor_table[n].flags;
    case 10:
      return actor_table[n].gravity;
    case 14:
      return actor_table[n].opts_angle & 0xFE00;
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
      actor_table[n].hw = v;
      return;
    case 5:
      actor_table[n].hh = v;
      return;
    case 6:
      v = v % 360;
      if(v < 0)
        v += 360;
      actor_table[n].opts_angle &= 0xFE00;
      actor_table[n].opts_angle |= v;
      return;
    case 7:
      actor_table[n].lives = v;
      return;
    case 8:
      actor_table[n].refval = v;
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
    case 14:
      actor_table[n].opts_angle &= 0x01FF;
      actor_table[n].opts_angle |= (v & 0xFE00);
      return;
    case 13:
      actor_table[n].onanimate = (uint16_t)v;
      return;
    case 15:
      actor_table[n].sprite = (v & 0xFF); 
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

inline void drawSprite(int8_t n, int16_t x0, int16_t y0, int16_t w, int16_t h){
  drawImg(SPRITE_MEMMAP+SPRITE_ADDR(n), x0, y0, w, h);
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

void drawImgFlipXSafe(int16_t a, int16_t x, int16_t y, int16_t w, int16_t h, int add_next_row, int xissafe, int yissafe){
  uint8_t p, color;
  int add_x = 0, firstx = 0;
  int16_t xi = 0;
  a += (w >> 1) - 1; // end if image row
  if (!yissafe) {
    if (y < espico.clipy0) {
      a += (add_next_row - w/2) * (espico.clipy0 - y); 
      h = h - (espico.clipy0 - y);
      y = espico.clipy0;
    }
    h = ((espico.clipy1-y) < h) ? (espico.clipy1 - y) : h;
    if (h <= 0) return;
  }
  if (!xissafe) {
    if (x < espico.clipx0) {
      add_x = (espico.clipx0 - x);
      w = w - add_x;
      firstx = (add_x & 1);
      add_x >>= 1;
      x = espico.clipx0;
    }
    if ((espico.clipx1-x) < w) {
      // fe1 | -|-- | fs1
      // fe2 | -|-- | fs2
      // 64 + w2 + v2
      add_next_row -= (w-(espico.clipx1 - x))/2;
      w = (espico.clipx1 - x);
    }
    if (w <= 0) return;
  }
  for(int16_t yi = 0; yi < h; yi++) {
    a -= add_x;
    if (firstx) {
      p = readMem(a--);
      color = GET_PIX_LEFT(p);
      if(!IS_TRANSPARENT(color)){
        setPixNC(x, yi + y, drwpalette[color]);
      }
      xi = 1;
    } else {
      xi = 0;
    }
      for(; xi < w; xi++){
        p = readMem(a--);
        color = GET_PIX_RIGHT(p);
        if(!IS_TRANSPARENT(color)){
            setPixNC(xi + x, yi + y, drwpalette[color]);
        }
        xi++;
        if (xi < w) {
          color = GET_PIX_LEFT(p);
          if(!IS_TRANSPARENT(color)){
            setPixNC(xi + x, yi + y, drwpalette[color]);
          }
        }
      }
    a += add_next_row;
  }
}

void drawImgSafe(int16_t a, int16_t x, int16_t y, int16_t w, int16_t h, int add_next_row, int xissafe, int yissafe){
  uint8_t p, color;
  int add_x = 0, firstx = 0;
  int16_t xi = 0;
  if (!yissafe) {
    if (y < espico.clipy0) {
      a += (w/2 + add_next_row) * (espico.clipy0 - y);
      h = h - (espico.clipy0 - y);
      y = espico.clipy0;
    }
    h = ((espico.clipy1-y) < h) ? (espico.clipy1 - y) : h;
    if (h <= 0) return;
  }
  if (!xissafe) {
    if (x < espico.clipx0) {
      add_x = (espico.clipx0 - x);
      w = w - add_x;
      firstx = (add_x & 1);
      add_x >>= 1;
      x = espico.clipx0;
    }
    if ((espico.clipx1-x) < w) {
      add_next_row += (w-(espico.clipx1 - x))/2;
      w = (espico.clipx1 - x);
    }
    if (w <= 0) return;
  }
  for(int16_t yi = 0; yi < h; yi++) {
    a += add_x;
    if (firstx) {
      p = readMem(a++);
      color = GET_PIX_RIGHT(p);
      if(!IS_TRANSPARENT(color)){
        setPixNC(x, yi + y, drwpalette[color]);
      }
      xi = 1;
    } else {
      xi = 0;
    }
      for(; xi < w; xi++){
        p = readMem(a++);
        color = GET_PIX_LEFT(p);
        if(!IS_TRANSPARENT(color)){
            setPixNC(xi + x, yi + y, drwpalette[color]);
        }
        xi++;
        if (xi < w) {
          color = GET_PIX_RIGHT(p);
          if(!IS_TRANSPARENT(color)){
            setPixNC(xi + x, yi + y, drwpalette[color]);
          }
        }
      }
    a += add_next_row;
  }
}

#define ZOOM_IMG_SIZE (espico.imageSize & 0x1f)

void drawImg(int16_t adr, int16_t x, int16_t y, int16_t w, int16_t h){
  int add_next_row = 0;
  if (adr >= PRG_SIZE) {
    add_next_row = 64 - (w >> 1);
  }
  if (FLIP_Y_ON) adr += (add_next_row + (w >> 1)) * (h-1);
  if (FLIP_Y_ON ^ FLIP_X_ON) {
    add_next_row += (w >> 1)*2;
  }
  if (FLIP_Y_ON) {
    add_next_row = -add_next_row;
  }

  if(ZOOM_IMG_SIZE > 1){
    if (FLIP_X_ON) drawImgFlipXSize(adr, x, y, w, h, add_next_row);
    else drawImgSize(adr, x, y, w, h, add_next_row);
  } else {
    x -= espico.camx;
    y -= espico.camy;
    if (x + w > espico.clipx0 && x < espico.clipx1) {
      int xissafe = (x >= espico.clipx0 && x + w <= espico.clipx1);
      int yissafe = (y >= espico.clipy0 && y + h <= espico.clipy1);
      if (FLIP_X_ON) drawImgFlipXSafe(adr, x, y, w, h, add_next_row,xissafe,yissafe); 
      else drawImgSafe(adr, x, y, w, h, add_next_row,xissafe,yissafe);
    }
  }
}

void drawImageBit(int16_t adr, int16_t x1, int16_t y1, int16_t w, int16_t h){
  if(ZOOM_IMG_SIZE > 1){
    drawImageBitS(adr, x1, y1, w, h);
    return;
  }
  x1 -= espico.camx;
  y1 -= espico.camy;
  if (x1 + w > espico.clipx0 && x1 < espico.clipx1) {

  uint8_t ibit;
  int i = 0;
  const int8_t fgcolor = (IS_TRANSPARENT(espico.color)) ? -1 : drwpalette[espico.color];
  const int8_t bgcolor = (IS_TRANSPARENT(espico.bgcolor)) ? -1 : drwpalette[espico.bgcolor];
  // const int8_t bgcolor = drwpalette[espico.bgcolor];
  int xissafe = (x1 >= espico.clipx0 && x1 + w <= espico.clipx1);
  int yissafe = (y1 >= espico.clipy0 && y1 + h <= espico.clipy1);

  for(int16_t y = 0; y < h; y++) {
    if(yissafe || (y1+y >= espico.clipy0 && y1+y < espico.clipy1)){
    for(int16_t x = 0; x < w; x++){
      if(i % 8 == 0){
        ibit = readMem(adr);
        adr++;
      }
      const int8_t color = (ibit & 0x80) ? fgcolor : bgcolor;
      if (color >= 0)
        if(xissafe || (x1+x >= espico.clipx0 && x1+x < espico.clipx1))
          setPixNC(x1 + x, y1 + y, color);
      ibit = ibit << 1;
      i++;
    }
    } else {
      // increase adr
      i += w;
      adr += w/8;
    }
  }
  }
}

void drawImgFlipXSize(int16_t adr, int16_t x, int16_t y, int16_t w, int16_t h, int add_next_row){
  x -= espico.camx;
  y -= espico.camy;
  uint8_t p, color;
  const uint8_t s = ZOOM_IMG_SIZE;
  const int16_t ws = w * s;
  const int16_t w1 = (w & 1);
  const int16_t hs = h * s;

  if (x + ws > espico.clipx0 && x < espico.clipx1) {
  int xissafe = (x >= espico.clipx0 && x + ws <= espico.clipx1);
  int yissafe = (y >= espico.clipy0 && y + hs <= espico.clipy1);

  adr += (w >> 1) - 1;
  for(int16_t yi = 0; yi < hs; yi += s) {
    for(int16_t xi = 0; xi < ws; xi += s){
      p = readMem(adr--);
      color = GET_PIX_RIGHT(p);
      if(!IS_TRANSPARENT(color)){
        for(int jy = 0; jy < s; jy++)
         if(yissafe || (y+yi+jy >= espico.clipy0 && y+yi+jy < espico.clipy1))
          for(int jx = 0; jx < s; jx++)
            if(xissafe || (xi+x+jx >= espico.clipx0 && xi+x+jx < espico.clipx1))
              setPixNC(x + xi + jx, y + yi + jy, drwpalette[color]);
      }
      xi += s;
      color = GET_PIX_LEFT(p);
      if(!IS_TRANSPARENT(color)){
        for(int jy = 0; jy < s; jy++)
         if(yissafe || (y+yi+jy >= espico.clipy0 && y+yi+jy < espico.clipy1))
          for(int jx = 0; jx < s; jx++)
            if(xissafe || (xi+x+jx >= espico.clipx0 && xi+x+jx < espico.clipx1))
              setPixNC(x + xi + jx, y + yi + jy, drwpalette[color]);
      }
    }
    adr += add_next_row;
  }
  
  }
}

void drawImgSize(int16_t adr, int16_t x, int16_t y, int16_t w, int16_t h, int add_next_row){
  x -= espico.camx;
  y -= espico.camy;
  uint8_t p, color;
  const uint8_t s = ZOOM_IMG_SIZE;
  const int16_t ws = w * s;
  const int16_t w1 = (w & 1);
  const int16_t hs = h * s;

  if (x + ws > espico.clipx0 && x < espico.clipx1) {
  int xissafe = (x >= espico.clipx0 && x + ws <= espico.clipx1);
  int yissafe = (y >= espico.clipy0 && y + hs <= espico.clipy1);

  for(int16_t yi = 0; yi < hs; yi += s) {
    for(int16_t xi = 0; xi < ws; xi += s){
      p = readMem(adr);
      color = GET_PIX_LEFT(p);
      if(!IS_TRANSPARENT(color)){
        for(int jy = 0; jy < s; jy++)
         if(yissafe || (y+yi+jy >= espico.clipy0 && y+yi+jy < espico.clipy1))
          for(int jx = 0; jx < s; jx++)
            if(xissafe || (xi+x+jx >= espico.clipx0 && xi+x+jx < espico.clipx1))
              setPixNC(x + xi + jx, y + yi + jy, drwpalette[color]);
      }
      xi += s;
      color = GET_PIX_RIGHT(p);
      if(!IS_TRANSPARENT(color)){
        for(int jy = 0; jy < s; jy++)
         if(yissafe || (y+yi+jy >= espico.clipy0 && y+yi+jy < espico.clipy1))
          for(int jx = 0; jx < s; jx++)
            if(xissafe || (xi+x+jx >= espico.clipx0 && xi+x+jx < espico.clipx1))
              setPixNC(x + xi + jx, y + yi + jy, drwpalette[color]);
      }
      adr++;
    }
    adr += add_next_row;
  }
  
  }
}

void drawImageBitS(int16_t adr, int16_t x1, int16_t y1, int16_t w, int16_t h){
  x1 -= espico.camx;
  y1 -= espico.camy;
  const uint8_t s = ZOOM_IMG_SIZE;
  const int16_t ws = w * s;
  if (x1 + ws > espico.clipx0 && x1 < espico.clipx1) {

  uint8_t ibit;
  int i = 0;
  const int16_t hs = h * s; 
  const int8_t fgcolor = (IS_TRANSPARENT(espico.color)) ? -1 : drwpalette[espico.color];
  const int8_t bgcolor = (IS_TRANSPARENT(espico.bgcolor)) ? -1 : drwpalette[espico.bgcolor];
  // const int8_t bgcolor = drwpalette[espico.bgcolor];
  int xissafe = (x1 >= espico.clipx0 && x1 + ws <= espico.clipx1);
  int yissafe = (y1 >= espico.clipy0 && y1 + hs <= espico.clipy1);
  for(int16_t y = 0; y < hs; y += s)
    for(int16_t x = 0; x < ws; x += s){
      if(i % 8 == 0){
        ibit = readMem(adr);
        adr++;
      }
      const int8_t color = (ibit & 0x80) ? fgcolor : bgcolor;
      if (color >= 0)
        for(int jy = 0; jy < s; jy++)
         if(yissafe || (y1+y+jy >= espico.clipy0 && y1+y+jy < espico.clipy1))
          for(int jx = 0; jx < s; jx++)
            if(xissafe || (x1+x+jx >= espico.clipx0 && x1+x+jx < espico.clipx1))
              setPixNC(x1 + x + jx, y1 + y + jy, color);
      ibit = ibit << 1;
      i++;
    }
  }
}

void drawTileMap(int16_t celx, int16_t cely, int16_t xx, int16_t yy, int16_t celw, int16_t celh, uint8_t layer){
    int16_t nx = 0, ny = 0;
    uint8_t spr = 0;
    const int add_next_row = 64 - (8 >> 1);

    xx -= espico.camx;
    yy -= espico.camy;

    // int scx = 0;
    // int scy = 0;

    int scx = (espico.clipx0-xx)/8;
    int scy = (espico.clipy0-yy)/8;
    if (scx < 0) scx = 0;
    if (scy < 0) scy = 0;

    int ew = (espico.clipx1-xx)/8 + 1;
    if (ew < celw) celw = ew;
    int eh = (espico.clipy1-yy)/8 + 1;
    if (eh < celh) celh = eh;

    int yissafe = false;
    int xissafe = false;

    for(int16_t y = scy; y < celh; y++){
      ny = yy + y * 8;
      for(int16_t x = scx; x < celw; x++){
        nx = xx + x * 8;
        spr = getTile((celx+x), (cely+y));
        if (spr != 0) {
          if ((sprite_flags[spr] & layer) == layer) {
            drawImgSafe(SPRITE_MEMMAP+SPRITE_ADDR(spr), nx, ny, 8, 8, add_next_row, xissafe, yissafe);
          }
        } else if (espico.palt == 0) {
          for(int16_t jy = 0; jy < 8; jy++)
            drawFHLine(nx, nx+7, ny+jy, 0);
          // fillRect(nx,ny,nx+7,ny+7);
        }
        xissafe = (x < celw-2);
      }
      yissafe = (y < celh-2);
    }
}

inline void drawFVLine(int16_t x, int16_t y1, int16_t y2, uint8_t color){
  if (x < espico.clipx0 || x >= espico.clipx1) return;
  y1 = (y1 < espico.clipy0) ? espico.clipy0 : y1;
  y2 = (y2 >= espico.clipy1) ? espico.clipy1-1 : y2;

  for(int16_t  i = y1; i <= y2; i++)
    setPixNC(x, i, color);
}

inline void drawFHLine(int16_t x1, int16_t x2, int16_t y, uint8_t color){
  if (y < espico.clipy0 || y >= espico.clipy1) return;
  x1 = (x1 < espico.clipx0) ? espico.clipx0 : x1;
  x2 = (x2 >= espico.clipx1) ? espico.clipx1-1 : x2;

  for(int16_t  i = x1; i <= x2; i++)
    setPixNC(i, y, color);
}

void drwLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
  const uint8_t fgcolor = drwpalette[espico.color];
    x1 -= espico.camx;
    x2 -= espico.camx;
    y1 -= espico.camy;
    y2 -= espico.camy;
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
    setPixWC(x2, y2, fgcolor);
    while(x1 != x2 || y1 != y2){
      setPixWC(x1, y1, fgcolor);
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

inline void setPixNC(uint8_t x, uint8_t y, uint8_t c){
  uint8_t xi = x / 2;
  uint8_t b = screen[SCREEN_ADDR(xi, y)];
  uint8_t px = b;
  if(x & 1)
    SET_PIX_RIGHT(px, c);
  else
    SET_PIX_LEFT(px, c);
  if(b != px) {
    screen[SCREEN_ADDR(xi, y)] = px;
    line_draw[y] |= (1 << (x >> 4));
  }
/*
  if(x & 1)
    SET_PIX_RIGHT(screen[SCREEN_ADDR(xi, y)], c);
  else
    SET_PIX_LEFT(screen[SCREEN_ADDR(xi, y)], c);
  if(b != screen[SCREEN_ADDR(xi, y)])
    line_redraw[y] |= (1 << (x >> 4));
*/
}

inline void setSpritePix(uint8_t x, uint8_t y, uint8_t c){
  if (x < 128 && y < 128) {
    uint8_t xi = x / 2;
    uint8_t px = sprite_map[SPRITE_PIX(xi, y)];
    if(x & 1)
      SET_PIX_RIGHT(px, c);
    else
      SET_PIX_LEFT(px, c);
    sprite_map[SPRITE_PIX(xi, y)] = px;
  }
}

inline void setPixWC(int16_t x, int16_t y, uint8_t c){
  if(x >= espico.clipx0 && x < espico.clipx1 && y >= espico.clipy0 && y < espico.clipy1){
    setPixNC(x,y,c);
  }
}

inline void setPix(int16_t x, int16_t y, uint8_t c){
  x -= espico.camx;
  y -= espico.camy;
  setPixWC(x,y,c);
}

inline byte getPix(byte x, byte y){
  byte b = 0;
  byte xi = x / 2;
  if(x < 128 && y < 128){
    if(x & 1)
      b = GET_PIX_RIGHT(screen[SCREEN_ADDR(xi, y)]);
    else
      b = GET_PIX_LEFT(screen[SCREEN_ADDR(xi, y)]);
  }
  return b;
}

inline byte getSpritePix(byte x, byte y){
  byte b = 0;
  byte xi = x / 2;
  if(x < 128 && y < 128){
    if(x & 1)
      b = GET_PIX_RIGHT(sprite_map[SPRITE_PIX(xi, y)]);
    else
      b = GET_PIX_LEFT(sprite_map[SPRITE_PIX(xi, y)]);
  }
  return b;
}

inline void changePalette(uint8_t n, uint16_t c){
  c &= 0xf;
  if(n < 16){
    drwpalette[n] = c&0xf;
  } else if (n < 32) {
    n = n - 16;
    palette[n] = (uint16_t)pgm_read_word_near(epalette + c);
    for(int y = 0; y < 128; y++){
      for(int x = 0; x < 64; x++){
        if((GET_PIX_LEFT(redrawscreen[SCREEN_ADDR(x, y)]) == n || GET_PIX_RIGHT(redrawscreen[SCREEN_ADDR(x, y)]) == n))
          line_redraw[y] |= (1 << (x >> 3));
      }
    }
  }
}

inline void setPalT(uint16_t palt) {
  espico.palt = palt;
}

inline void setCharX(int8_t x){
  espico.nlregx = x;
  espico.regx = x;
}

inline void setCharY(int8_t y){
  espico.regy = y;
}

inline void prints(int16_t adr){
  int x = espico.regx;
  while (!(readMem(adr) == 0)) {
    char c = (char)(readMem(adr));

    if(c == '\n'){
      espico.regy += FONT_HEIGHT;
      x = espico.nlregx;
    } else{
      es_putchar(c, x, espico.regy);
      x += FONT_WIDTH+1;
    }
    adr++;
  }
  espico.regy += FONT_HEIGHT;
  espico.regx = espico.nlregx;
}

inline void printc(char c) {
  if (c == '\n') {
    espico.regy += FONT_HEIGHT;
    espico.regx = espico.nlregx;
  } else {
    es_putchar(c, espico.regx, espico.regy);
    espico.regx += FONT_WIDTH+1;
  }
}

inline void setColor(uint8_t c){
  espico.color = c & 0xf;
}

void drawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1){
  x0 -= espico.camx;
  y0 -= espico.camy;
  x1 -= espico.camx;
  y1 -= espico.camy;
  const uint8_t fgcolor = drwpalette[espico.color];
  drawFHLine(x0, x1, y0, fgcolor);
  drawFHLine(x0, x1, y1, fgcolor);
  drawFVLine(x0, y0, y1, fgcolor);
  drawFVLine(x1, y1, y1, fgcolor);
}

void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1){
  const uint8_t bgcolor = drwpalette[espico.bgcolor];
  x0 -= espico.camx;
  y0 -= espico.camy;
  x1 -= espico.camx;
  y1 -= espico.camy;
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
  x0 -= espico.camx;
  y0 -= espico.camy;
  int16_t  x  = 0;
  int16_t  dx = 1;
  int16_t  dy = r+r;
  int16_t  p  = -(r>>1);

  drawFHLine(x0 - r, x0 + r, y0, bgcolor);

  while(x<r){
    if(p>=0) {
      dy-=2;
      p-=dy;
      r--;
    }

    dx+=2;
    p+=dx;

    x++;

    drawFHLine(x0 - r, x0 + r, y0 + x, bgcolor);
    drawFHLine(x0 - r, x0 + r, y0 - x, bgcolor);
    drawFHLine(x0 - x, x0 + x, y0 + r, bgcolor);
    drawFHLine(x0 - x, x0 + x, y0 - r, bgcolor);
  }
}

void putStringUC(char s[], int8_t y){
  int8_t i = 0;
  int x = 0;
  while(s[i] != 0 && i < 32){
    es_putchar(toupper(s[i]), x, y);
    x += FONT_WIDTH+1;
    i++;
  }
}

void es_putchar(char c, uint8_t x, uint8_t y) {
  const uint8_t fgcolor = drwpalette[espico.color];
  if (c >= 32 && c < 156) {
    c -= 32;
    for(int i=0; i<3; i++ ) { // Char bitmap = 3 columns
      uint8_t line = pgm_read_byte_near(&font_a[c * 3 + i]);
      for(int j=0; j<6; j++, line >>= 1) {
        if(line & 1)
         setPix(x+i, y+j, fgcolor);
      }
    }
  }
}

#pragma GCC pop_options

