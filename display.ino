#include "font_a.c"

#define DISPLAY_X_OFFSET 32
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
#define SCREEN_MEMMAP 0xA000
 
// [high-n][y][low-n][x] [n << 4][y][n&0x0f][x]
// #define SPRITE_ARRAY_DEF 4][8][16][4
#define SPRITE_HEIGHT 8
#define SPRITE_WIDTH  8
#define SPRITE_WIDTH_BYTES 4
#define SPRITE_MAP_SIZE 8192
#define SPRITE_ARRAY_DEF 128][64
#define SPRITE_ADDR(n)  ((n & 0xf0) >> 2)][((n & 0xf) << 3)
#define SPRITEPIX_ADDR(x, y) ((int(y) << 6) + (x))
#define TILEMAP_ADDR(x, y) (4096 + (int(y) << 6) + (x))
#define SPRITE_MEMMAP 0x8000

#define PIX_LEFT_MASK(p)  ((p) & 0xf0)
#define GET_PIX_LEFT(p)  ((p) >> 4)
#define PIX_RIGHT_MASK(p) ((p) & 0x0f)
#define GET_PIX_RIGHT(p) PIX_RIGHT_MASK(p)
#define SET_PIX_LEFT(p,c)  p = (PIX_RIGHT_MASK(p) + ((c) << 4))
#define SET_PIX_RIGHT(p,c) p = (PIX_LEFT_MASK(p) + ((c) & 0x0f))

#define PARTICLE_COUNT 32

struct EspicoState {
  uint16_t onupdate;
  uint16_t ondraw;
  int8_t   updatedone;
  int8_t   drawdone;
  int8_t   color;
  int8_t   bgcolor;
  int8_t   imageSize;
  int8_t   regx;
  int8_t   regy;
};  

struct Actor {
  uint16_t address;
  int16_t x;
  int16_t y;
  uint8_t width;
  uint8_t height;
  int8_t speedx;
  int8_t speedy;
  int16_t angle;
  int8_t lives;
  int8_t collision;
  uint8_t flags; //0 0 0 0 0 0 scrolled solid
  int8_t gravity;
  uint16_t oncollision;
  uint16_t onexitscreen;
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

struct TileMap { 
  int16_t adr;
  uint8_t imgwidth;
  uint8_t imgheight;
  uint8_t width;
  uint8_t height;
  int16_t x;
  int16_t y;
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

uint8_t screen[SCREEN_ARRAY_DEF] __attribute__ ((aligned));
uint8_t sprite_map[SPRITE_MAP_SIZE] __attribute__ ((aligned));
uint8_t line_is_draw[128] __attribute__ ((aligned));
char charArray[340] __attribute__ ((aligned));
uint16_t pix_buffer[256] __attribute__ ((aligned));
struct Actor actor_table[32];
struct Particle particles[PARTICLE_COUNT];
struct Emitter emitter;
struct TileMap tiles;
struct EspicoState espico __attribute__ ((aligned));

#pragma GCC optimize ("-O2")
#pragma GCC push_options

int16_t getCos(int16_t g){
  g = g % 360;
  if(g < 0)
    g += 360;
  return (int16_t)(int8_t)pgm_read_byte_near(cosT + g);
}

int16_t getSin(int16_t g){
  g = g % 360;
  if(g < 0)
    g += 360;
  return (int16_t)(int8_t)pgm_read_byte_near(sinT + g);
}

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

void display_init(){
  initEspicoState();
  for(int i = 0; i < 16; i++){
    palette[i] = (uint16_t)pgm_read_word_near(bpalette + i);
    sprtpalette[i] = (uint16_t)pgm_read_word_near(bpalette + i);
  }
  for(int i = 0; i < 32; i++){
    actor_table[i].address = 0;
    actor_table[i].x = -255;
    actor_table[i].y = -255;
    actor_table[i].width = 8;
    actor_table[i].height = 8;
    actor_table[i].speedx = 0;
    actor_table[i].speedy = 0;
    actor_table[i].angle = 0;
    actor_table[i].lives = 0;
    actor_table[i].collision = -1;
    actor_table[i].flags = 2; //scrolled = 1 solid = 0
    actor_table[i].gravity = 0;
    actor_table[i].oncollision = 0;
  }
  emitter.time = 0;
  emitter.timer = 0;
  tiles.adr = 0;
  for(int i = 0; i < PARTICLE_COUNT; i++)
    particles[i].time = 0;
  for(int i = 0; i < 340; i++)
    charArray[i] = 0;
  clearScr(0);
}

void initTileMap() {
  tiles.adr = SPRITE_MEMMAP + TILEMAP_ADDR(0,0);
  tiles.imgwidth = 8;
  tiles.imgheight = 8;
  tiles.width = 128;
  tiles.height = 32;
}

void initEspicoState() {
  espico.onupdate = 0;
  espico.ondraw = 0;
  espico.updatedone = 0;
  espico.drawdone = 0;
  espico.color = 1;
  espico.bgcolor = 0;
  espico.imageSize = 1;
  espico.regx = 0;
  espico.regy = 0;
}

void setEspicoState(int16_t s, int16_t v) {
  switch (s) {
   case 0:
     espico.updatedone = 1;
     break;
   case 1:
     espico.onupdate = v;
     break;
   case 2:
     espico.drawdone = 1;
     break;
   case 3:
     espico.ondraw = v;
     break;
  }
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

void updateGame() {
  if (espico.onupdate > 0)
    setinterrupt(espico.onupdate, 0);
}

void drawBuffer() {
  if (espico.ondraw > 0)
    setinterrupt(espico.ondraw, 0);
}

void redrawScreen(){
  int i;
  if (espico.ondraw != 0 && !espico.drawdone) return;
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
}

void redrawParticles(){
  int16_t i, n;
  uint8_t x, y;
  if(emitter.timer > 0){
      emitter.timer -= 50;
      i = emitter.count;
      for(n = 0; n < PARTICLE_COUNT; n++){
        if(i == 0)
          break;
        if(particles[n].time <= 0){
          i--;
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
        x = (particles[n].x & 127) / 2;
        y = particles[n].y & 127;
        if(particles[n].x & 1)
          SET_PIX_RIGHT(screen[SCREEN_ADDR(x,y)],particles[n].color); 
        else
          SET_PIX_LEFT(screen[SCREEN_ADDR(x,y)],particles[n].color);
        line_is_draw[y] |= 1 + x / 32;
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
  return - 1;
}

void redrawActors(){
  for(int i = 0; i < 32; i++){
    if(actor_table[i].lives > 0){   
      actor_table[i].speedy += actor_table[i].gravity;
      actor_table[i].x += actor_table[i].speedx;
      actor_table[i].y += actor_table[i].speedy;
      if(actor_table[i].x + actor_table[i].width < 0 || actor_table[i].x > 127 || actor_table[i].y + actor_table[i].height < 0 || actor_table[i].y > 127){
        if(actor_table[i].onexitscreen > 0)
           setinterrupt(actor_table[i].onexitscreen, i);
      }
      else
        drawActor(i, actor_table[i].x, actor_table[i].y);
    }
  }
}

uint16_t getTileInXY(int16_t x, int16_t y){
  if(x < tiles.x || y < tiles.y || x > tiles.x + tiles.imgwidth * tiles.width || tiles.y > tiles.imgheight * tiles.height)
    return 0;
  return readInt(tiles.adr + (((x - tiles.x) / tiles.imgwidth) + ((y - tiles.y) / tiles.imgheight) * tiles.width) * 2);
}

uint16_t getTile(int16_t x, int16_t y){
  if(x < 0 || x >= tiles.width || y < 0 || y >= tiles.height)
    return 0;
  return readInt(tiles.adr + (x + y * tiles.width) * 2);
}

void testActorCollision(){
  byte n, i;
  int16_t x0, y0, newspeed;
  for(n = 0; n < 32; n++)
    actor_table[n].collision = -1;
  for(n = 0; n < 32; n++){
    if(actor_table[n].lives > 0){
      for(i = 0; i < n; i++){
        if(actor_table[i].lives > 0){
          if(actor_table[n].x < actor_table[i].x + actor_table[i].width && 
          actor_table[n].x + actor_table[n].width > actor_table[i].x &&
          actor_table[n].y < actor_table[i].y + actor_table[i].height && 
          actor_table[n].y + actor_table[n].height > actor_table[i].y){
            actor_table[n].collision = i;
            actor_table[i].collision = n;
            if(actor_table[n].oncollision > 0)
              setinterrupt(actor_table[n].oncollision, n);
            if(actor_table[i].oncollision > 0)
              setinterrupt(actor_table[i].oncollision, i);
            if((actor_table[n].flags & 1) != 0 && (actor_table[n].flags & 1) != 0){
              if((actor_table[n].speedx >= 0 && actor_table[i].speedx <= 0) || (actor_table[n].speedx <= 0 && actor_table[i].speedx >= 0)){
                newspeed = (abs(actor_table[n].speedx) + abs(actor_table[i].speedx)) / 2;
                if(actor_table[n].x > actor_table[i].x){
                  actor_table[n].speedx = newspeed;
                  actor_table[i].speedx = -newspeed;
                }
                else{
                  actor_table[n].speedx = -newspeed;
                  actor_table[i].speedx = newspeed;
                }
                actor_table[n].x -= 2;
              }
              if((actor_table[n].speedy >= 0 && actor_table[i].speedy <= 0) || (actor_table[n].speedy <= 0 && actor_table[i].speedy >= 0)){
                newspeed = (abs(actor_table[n].speedy) + abs(actor_table[i].speedy)) / 2;
                if(actor_table[n].y > actor_table[i].y){
                  actor_table[n].speedy = newspeed;
                  actor_table[i].speedy = -newspeed;
                }
                else{
                  actor_table[n].speedy = -newspeed;
                  actor_table[i].speedy = newspeed;
                }
                actor_table[n].y -=  2;
              }
            }
          }
        }
      }
      if((actor_table[n].flags & 2) != 0 && tiles.adr > 0){
          x0 = ((actor_table[n].x + actor_table[n].width / 2 - tiles.x) / (int16_t)tiles.imgwidth);
          y0 = ((actor_table[n].y + actor_table[n].height / 2 - tiles.y + tiles.imgheight) / (int16_t)tiles.imgheight) - 1;
          if(x0 >= -1 && x0 <= tiles.width && y0 >= -1 && y0 <= tiles.height){
            if(getTile(x0, y0) != 0){
              if(actor_table[n].speedx != 0){
                if(actor_table[n].speedx > 0){
                  actor_table[n].x = tiles.x + x0 * tiles.imgwidth - actor_table[n].width ;
                  actor_table[n].speedx /= 2;
                }
                else{
                  actor_table[n].x = tiles.x + (x0 + 1) * tiles.imgwidth;
                  actor_table[n].speedx /= 2;
                }
              }
              if(actor_table[n].speedy != 0){
                if(actor_table[n].speedy > 0){
                  actor_table[n].y = tiles.y + y0 * tiles.imgheight - actor_table[n].height ;
                  actor_table[n].speedy /= 2;
                }
                else{
                  actor_table[n].y = tiles.y + (y0 + 1) * tiles.imgheight;
                  actor_table[n].speedy /= 2;
                }
              }
            }
            else{
              if(actor_table[n].speedy > 0 && getTile(x0, y0 + 1) != 0){
                if((tiles.y + (y0 + 1) * tiles.imgheight) - (actor_table[n].y  + actor_table[n].height) < actor_table[n].speedy * 2){
                  actor_table[n].y = tiles.y + (y0 + 1) * tiles.imgheight - actor_table[n].height;  
                  actor_table[n].speedy = 0;
                }
              }
              else if(actor_table[n].speedy < 0 && getTile(x0, y0 - 1) != 0){
                if(actor_table[n].y - (tiles.y + y0 * tiles.imgheight) < actor_table[n].speedy * 2){
                  actor_table[n].y = tiles.y + y0 * tiles.imgheight;  
                  actor_table[n].speedy = 0;
                }
              }
              if(actor_table[n].speedx > 0  && getTile(x0 + 1, y0) != 0){
                if((tiles.x + (x0 + 1) * tiles.imgwidth - actor_table[n].width) - actor_table[n].x < actor_table[n].speedx * 2){
                  actor_table[n].x = tiles.x + (x0 + 1) * tiles.imgwidth - actor_table[n].width;  
                  actor_table[n].speedx = 0;
                }
              }
              else if(actor_table[n].speedx < 0 && getTile(x0 - 1, y0) != 0){
                if(actor_table[n].x - (tiles.x + x0 * tiles.imgwidth) < actor_table[n].speedx * 2){
                  actor_table[n].x = tiles.x + x0 * tiles.imgwidth; 
                  actor_table[n].speedx = 0;
                }
              } 
            } 
          }
        }
        
    }
  }
}

void clearScr(uint8_t color){
  /*
  for(uint8_t y = 0; y < 128; y ++){
    for(uint8_t x = 0; x < 128; x++)
      setPix(x, y, color);
  }
  */
  uint8_t twocolor = ((color << 4) | (color & 0x0f));
  memset(screen, twocolor, SCREEN_SIZE);
  memset(line_is_draw, 3, 128);
//  for (int y = 0; y < 128; y++) 
//    line_is_draw[y] |= 3;

}

void setImageSize(uint8_t size){
  espico.imageSize = size;
}

void setActorSprite(uint16_t n, uint16_t adr){
  actor_table[n].address = adr;
}

void setActorPosition(int8_t n, uint16_t x, uint16_t y){
  actor_table[n].x = x;
  actor_table[n].y = y;
}

void actorSetDirectionAndSpeed(int8_t n, uint16_t speed, int16_t dir){
  actor_table[n].speedx = ((speed * getCos(dir)) >> 6);
  actor_table[n].speedy = ((speed * getSin(dir)) >> 6);
}

void setActorWidth(int8_t n, uint8_t w){
  actor_table[n].width = w;
}

void setActorHeight(int8_t n, uint8_t w){
  actor_table[n].height = w;
}

void setActorSpeedx(int8_t n, int8_t s){
  actor_table[n].speedx = s;
}

void setActorSpeedy(int8_t n, int8_t s){
  actor_table[n].speedy = s;
}

int16_t angleBetweenActors(int8_t n1, int8_t n2){
  int16_t A = atan2_fp(actor_table[n1].y - actor_table[n2].y, actor_table[n1].x - actor_table[n2].x);
  A = (A < 0) ? A + 360 : A;
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
      return actor_table[n].flags & 2;
    case 10:
      return actor_table[n].gravity;
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
      actor_table[n].speedx = (int8_t) v;
      return;
    case 3:
      actor_table[n].speedy = (int8_t) v;
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
      return;
    case 9:
      if(v != 0)
        actor_table[n].flags |= 0x01;
      else
        actor_table[n].flags &= ~0x01;
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
      if(v != 0)
        actor_table[n].flags |= 0x02;
      else
        actor_table[n].flags &= ~0x02;
      return;
 }
}

void drawRotateSprPixel(int8_t pixel, int8_t x0, int8_t y0, int16_t x, int16_t y, int16_t hw, int16_t hh, int16_t c, int16_t s){
  int16_t nx = hw + (((x - hw) * c - (y - hh) * s) >> 6);
  int16_t ny = hh + (((y - hh) * c + (x - hw) * s) >> 6);
  int16_t nnx = nx / 2;
  int8_t nnx0 = x0 / 2;
  if(nnx0 + nnx >= 0 && nnx0 + nnx < 64 && y0 + ny >= 0 && y0 + ny < 128){
    if(nx & 1)
      SET_PIX_RIGHT(screen[SCREEN_ADDR(nnx0 + nnx, y0 + ny)], pixel);
    else
      SET_PIX_LEFT(screen[SCREEN_ADDR(nnx0 + nnx, y0 + ny)], pixel);
    line_is_draw[y0 + ny] |= 1 + (nnx0 + nnx) / 32;
  }
}

inline void drawSprPixel(int8_t pixel, int8_t x0, int8_t y0, int16_t x, int16_t y){
  if(x0 + x >= 0 && x0 + x < 128 && y0 + y >= 0 && y0 + y < 128){
    if((x0 + x) & 1)
      SET_PIX_RIGHT(screen[SCREEN_ADDR((x0 + x) / 2, y0 + y)], pixel);
    else
      SET_PIX_LEFT(screen[SCREEN_ADDR((x0 + x) / 2, y0 + y)], pixel);
    line_is_draw[y0 + y] |= 1 + (x0 + x) / 64;
  }
}

void drawActor(int8_t n, int16_t x, int16_t y){
  uint16_t adr = actor_table[n].address;
  uint8_t w = actor_table[n].width;
  uint8_t h = actor_table[n].height;
  int16_t c = getCos(actor_table[n].angle);
  int16_t s = getSin(actor_table[n].angle);
  uint8_t pixel;
  w = w / 2;
  if(actor_table[n].angle == 0){
    for(byte y1 = 0; y1 < h; y1 ++)
      if(y1 + y >= -h && y1 + y < 128 + h){
        for(byte x1 = 0; x1 < w; x1++){
          pixel = readMem(adr + x1 + y1 * w);
          if(PIX_LEFT_MASK(pixel) > 0)
            drawSprPixel(GET_PIX_LEFT(pixel), x, y, x1 * 2, y1);
          if(PIX_RIGHT_MASK(pixel) > 0)
            drawSprPixel(GET_PIX_RIGHT(pixel), x, y, x1 * 2 + 1, y1);
        }
      }
  }
  else{
    for(byte y1 = 0; y1 < h; y1 ++)
      if(y1 + y >= -h && y1 + y < 128 + h){
        for(byte x1 = 0; x1 < w; x1++)
          if(x1 + x >= -w && x1 + x < 128 + w){
            pixel = readMem(adr + x1 + y1 * w);
            if(PIX_LEFT_MASK(pixel) > 0)
              drawRotateSprPixel(GET_PIX_LEFT(pixel), x, y, x1 * 2, y1, w, h / 2, c, s);
            if(PIX_RIGHT_MASK(pixel) > 0)
              drawRotateSprPixel(GET_PIX_RIGHT(pixel), x, y, x1 * 2 + 1, y1, w, h / 2, c, s);
          }   
      }
  }
}

void drawImg(int16_t a, int16_t x, int16_t y, int16_t w, int16_t h){
  if(espico.imageSize > 1){
    drawImgS(a, x, y, w, h);
    return;
  }
  uint8_t p, color;
  for(int16_t yi = 0; yi < h; yi++)
    for(int16_t xi = 0; xi < w; xi++){
      p = readMem(a);
      color = GET_PIX_LEFT(p);
      if(color > 0){
        setPix(xi + x, yi + y, color);
      }
      xi++;
      color = GET_PIX_RIGHT(p);
      if(color > 0){
        setPix(xi + x, yi + y, color);
      }
      a++;
    }
}

void drawImgRLE(int16_t adr, int16_t x1, int16_t y1, int16_t w, int16_t h){
    if(espico.imageSize > 1){
      drawImgRLES(adr, x1, y1, w, h);
      return;
    }
    int16_t i = 0;
    byte repeat = readMem(adr);
    adr++;
    int8_t color1 = GET_PIX_LEFT(readMem(adr));
    int8_t color2 = GET_PIX_RIGHT(readMem(adr));
    while(i < w * h){
      if(repeat > 0x81){
        if(color1 > 0){
          setPix(x1 + i % w, y1 + i / w, color1);
        }
        if(color2 > 0){
          setPix(x1 + i % w + 1, y1 + i / w, color2);
        }
        i += 2;
        adr++;
        repeat--;
        color1 = GET_PIX_LEFT(readMem(adr));
        color2 = GET_PIX_RIGHT(readMem(adr));
      }
      else if(repeat == 0x81){
        repeat = readMem(adr);
        adr++;
        color1 = GET_PIX_LEFT(readMem(adr));
        color2 = GET_PIX_RIGHT(readMem(adr));
      }
      else if(repeat > 0){
        if(color1 > 0){
          setPix(x1 + i % w, y1 + i / w, color1);
        }
        if(color2 > 0){
          setPix(x1 + i % w + 1, y1 + i / w, color2);
        }
        i += 2;
        repeat--;
      }
      else if(repeat == 0){
        adr++;
        repeat = readMem(adr);
        adr++;
        color1 = GET_PIX_LEFT(readMem(adr));
        color2 = GET_PIX_RIGHT(readMem(adr));
      }
    }
  }

void drawImageBit(int16_t adr, int16_t x1, int16_t y1, int16_t w, int16_t h){
  if(espico.imageSize > 1){
    drawImageBitS(adr, x1, y1, w, h);
    return;
  }
  int16_t size = w * h / 8;
  int16_t i = 0;
  uint8_t ibit;
  for(int16_t y = 0; y < h; y++)
    for(int16_t x = 0; x < w; x++){
      if(i % 8 == 0){
        ibit = readMem(adr);
        adr++;
      }
      if(ibit & 0x80)
        setPix(x1 + x, y1 + y, espico.color);
      else
        setPix(x1 + x, y1 + y, espico.bgcolor);
      ibit = ibit << 1;
      i++;
    }
}

void drawImgS(int16_t a, int16_t x, int16_t y, int16_t w, int16_t h){
  uint8_t p, jx, jy, color, s;
  s = espico.imageSize;
  for(int16_t yi = 0; yi < h; yi++)
    for(int16_t xi = 0; xi < w; xi++){
      p = readMem(a);
      color = GET_PIX_LEFT(p);
      if(color > 0){
        for(jx = 0; jx < s; jx++)
              for(jy = 0; jy < s; jy++)
                setPix(xi * s + x + jx, yi * s + y + jy, color);
      }
      xi++;
      color = GET_PIX_RIGHT(p);
      if(color > 0){
        for(jx = 0; jx < s; jx++)
              for(jy = 0; jy < s; jy++)
                setPix(xi * s + x + jx, yi * s + y + jy, color);
      }
      a++;
    }
}

void drawImgRLES(int16_t adr, int16_t x1, int16_t y1, int16_t w, int16_t h){
    int16_t i = 0;
    uint8_t jx, jy;
    byte repeat = readMem(adr);
    adr++;
    int8_t color1 = GET_PIX_LEFT(readMem(adr));
    int8_t color2 = GET_PIX_RIGHT(readMem(adr));
    while(i < w * h){
      if(repeat > 0x81){
        if(color1 > 0){
          for(jx = 0; jx < espico.imageSize; jx++)
            for(jy = 0; jy < espico.imageSize; jy++)
              setPix(x1 + (i % w) * espico.imageSize + jx, y1 + i / w * espico.imageSize + jy, color1);
        }
        if(color2 > 0){
          for(jx = 0; jx < espico.imageSize; jx++)
            for(jy = 0; jy < espico.imageSize; jy++)
              setPix(x1 + (i % w) * espico.imageSize + espico.imageSize + jx, y1 + i / w * espico.imageSize + jy, color2);
        }
        i += 2;
        adr++;
        repeat--;
        color1 = GET_PIX_LEFT(readMem(adr));
        color2 = GET_PIX_RIGHT(readMem(adr));
      }
      else if(repeat == 0x81){
        repeat = readMem(adr);
        adr++;
        color1 = GET_PIX_LEFT(readMem(adr));
        color2 = GET_PIX_RIGHT(readMem(adr));
      }
      else if(repeat > 0){
        if(color1 > 0){
          for(jx = 0; jx < espico.imageSize; jx++)
                for(jy = 0; jy < espico.imageSize; jy++)
                  setPix(x1 + (i % w) * espico.imageSize + jx, y1 + i / w * espico.imageSize + jy, color1);
        }
        if(color2 > 0){
          for(jx = 0; jx < espico.imageSize; jx++)
                for(jy = 0; jy < espico.imageSize; jy++)
                  setPix(x1 + (i % w) * espico.imageSize + espico.imageSize + jx, y1 + i / w * espico.imageSize + jy, color2);
        }
        i += 2;
        repeat--;
      }
      else if(repeat == 0){
        adr++;
        repeat = readMem(adr);
        adr++;
        color1 = GET_PIX_LEFT(readMem(adr));
        color2 = GET_PIX_RIGHT(readMem(adr));
      }
    }
  }

void drawImageBitS(int16_t adr, int16_t x1, int16_t y1, int16_t w, int16_t h){
  int16_t size = w * h / 8;
  int16_t i = 0;
  uint8_t ibit, jx, jy;
  for(int16_t y = 0; y < h; y++)
    for(int16_t x = 0; x < w; x++){
      if(i % 8 == 0){
        ibit = readMem(adr);
        adr++;
      }
      if(ibit & 0x80){
        for(jx = 0; jx < espico.imageSize; jx++)
          for(jy = 0; jy < espico.imageSize; jy++)
          setPix(x1 + x * espico.imageSize + jx, y1 + y * espico.imageSize + jy, espico.color);
      } 
      else{
        for(jx = 0; jx < espico.imageSize; jx++)
          for(jy = 0; jy < espico.imageSize; jy++)
            setPix(x1 + x * espico.imageSize + jx, y1 + y * espico.imageSize + jy, espico.bgcolor);
      } 
      ibit = ibit << 1;
      i++;
    }
}

void loadTileMap(int16_t adr, uint8_t iwidth, uint8_t iheight, uint8_t width, uint8_t height){
    tiles.adr = adr;
    tiles.imgwidth = iwidth;
    tiles.imgheight = iheight;
    tiles.width = width;
    tiles.height = height;
  }

void drawTiles(int16_t x0, int16_t y0){
    int16_t x, y, nx, ny;
    uint16_t imgadr;
    tiles.x = x0;
    tiles.y = y0;
    for(x = 0; x < tiles.width; x++){
      nx = x0 + x * tiles.imgwidth;
      for(y = 0; y < tiles.height; y++){
        ny = y0 + y * tiles.imgheight;
        if(nx >= -tiles.width && nx < 128 && ny >= -tiles.height && ny < 128){
          imgadr = readInt(tiles.adr + (x + y * tiles.width) * 2);
          if(imgadr > 0)
            drawImg(imgadr, nx, ny, tiles.imgwidth, tiles.imgheight); 
        }
      }
    }
  }

void drawFVLine(int16_t x, int16_t y1, int16_t y2){
  for(int16_t  i = y1; i <= y2; i++)
    setPix(x, i, espico.color);
}

void drawFHLine(int16_t x1, int16_t x2, int16_t y){
  for(int16_t  i = x1; i <= x2; i++)
    setPix(i, y, espico.color);
}

void drwLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if(x1 == x2){
      if(y1 > y2)
        drawFVLine(x1, y2, y1);
      else
        drawFVLine(x1, y1, y2);
      return;
    }
    else if(y1 == y2){
      if(x1 > x2)
        drawFHLine(x2, x1, y1);
      else
        drawFHLine(x1, x2, y1);
      return;
    }
    int16_t deltaX = abs(x2 - x1);
    int16_t deltaY = abs(y2 - y1);
    int16_t signX = x1 < x2 ? 1 : -1;
    int16_t signY = y1 < y2 ? 1 : -1;
    int16_t error = deltaX - deltaY;
    int16_t error2;
    setPix(x2, y2, espico.color);
    while(x1 != x2 || y1 != y2){
      setPix(x1, y1, espico.color);
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

inline void setPix2(int16_t xi, int16_t y, uint8_t c){
  if(xi < 64 && y < 128){
    if (screen[SCREEN_ADDR(xi, y)] != c) {
      screen[SCREEN_ADDR(xi, y)] = c;
      line_is_draw[y] |= 1 + xi /32;
    }
  }
}

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
  if(n < 16){
    palette[n] = c;
    for(uint8_t y = 0; y < 128; y++){
      for(uint8_t x = 0; x < 64; x++){
        if((GET_PIX_LEFT(screen[SCREEN_ADDR(x, y)]) == n || GET_PIX_RIGHT(screen[SCREEN_ADDR(x, y)]) == n))
          line_is_draw[y] |= 1 + x / 32;
      }
    }
  }
  else if(n < 32)
    sprtpalette[n - 16] = c;
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
      for(uint8_t n = 0; n < 32; n++)
        if(actor_table[n].flags & 2)
          actor_table[n].x -= 2;
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
      for(uint8_t n = 0; n < 32; n++)
        if(actor_table[n].flags & 2)
          actor_table[n].y--;
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
      for(uint8_t n = 0; n < 32; n++)
        if(actor_table[n].flags & 2)
          actor_table[n].x += 2;
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
      for(uint8_t n = 0; n < 32; n++)
        if(actor_table[n].flags & 2)
          actor_table[n].y++;
    }
    if(tiles.adr > 0)
      tileMapDrawLine(step, direction);
}

void tileMapDrawLine(uint8_t step, uint8_t direction){
    int16_t x,y,x0,y0,y1,nx,ny;
    uint16_t imgadr;
    if(direction == 2){
      tiles.x -= step*2;
      x0 = tiles.x;
      y0 = tiles.y;
      x = (127 - x0) / tiles.imgwidth;
      nx = x0 + x * tiles.imgwidth;
      if(x < tiles.width && x >= -tiles.width){
        for(y = 0; y < tiles.height; y++){
          ny = y0 + y * tiles.imgheight;
          if(ny > -tiles.height && ny < 128){
            imgadr = readInt(tiles.adr + (x + y * tiles.width) * 2);
            if(imgadr > 0)
              drawImg(imgadr, nx, ny, tiles.imgwidth, tiles.imgheight); 
            else
              fillRect(nx, ny, tiles.imgwidth, tiles.imgheight, espico.bgcolor);
          }
        }
      }
      else if(tiles.width * tiles.imgwidth + x0 >= 0){
        y0 = (y0 > 0) ? y0 : 0;
        y1 = (tiles.y + tiles.height * tiles.imgheight < 128) ? tiles.y + tiles.height * tiles.imgheight - y0 : 127 - y0;
        if(y0 < 127 && y1 > 0)
          fillRect(127 - step * 2, y0, step * 2, y1, espico.bgcolor);
      }
    }
    else if(direction == 1){
      tiles.y -= step;
      x0 = tiles.x;
      y0 = tiles.y;
      y = (127 - y0) / tiles.imgheight;
      ny = y0 + y * tiles.imgheight;
      if(y < tiles.height  && y >= -tiles.height)
        for(x = 0; x < tiles.width; x++){
          nx = x0 + x * tiles.imgwidth;
          if(nx > -tiles.width && nx < 128){
            imgadr = readInt(tiles.adr + (x + y * tiles.width) * 2);
            if(imgadr > 0)
              drawImg(imgadr, nx, ny, tiles.imgwidth, tiles.imgheight); 
            else
              fillRect(nx, ny, tiles.imgwidth, tiles.imgheight, espico.bgcolor);
          }
        }
    }
    else if(direction == 0){
      tiles.x += step*2;
      x0 = tiles.x;
      y0 = tiles.y;
      x = (0 - x0) / tiles.imgwidth;
      nx = x0 + x * tiles.imgwidth;
      if(x0 < 0 && x >= -tiles.width){
        for(y = 0; y < tiles.height; y++){
          ny = y0 + y * tiles.imgheight;
          if(ny > -tiles.height && ny < 128){
            imgadr = readInt(tiles.adr + (x + y * tiles.width) * 2);
            if(imgadr > 0)
              drawImg(imgadr, nx, ny, tiles.imgwidth, tiles.imgheight); 
            else
              fillRect(nx, ny, tiles.imgwidth, tiles.imgheight, espico.bgcolor);
          }
        }
      }
      else if(x0 < 128){
        y0 = (y0 > 0) ? y0 : 0;
        y1 = (tiles.y + tiles.height * tiles.imgheight < 128) ? tiles.y + tiles.height * tiles.imgheight - y0 : 127 - y0;
        if(y0 < 127 && y1 > 0)
          fillRect(0, y0, step * 2, y1, espico.bgcolor);
      }
    }
    else if(direction == 3){
      tiles.y += step;
      x0 = tiles.x;
      y0 = tiles.y;
      y = (0 - y0) / tiles.imgheight;
      ny = y0 + y * tiles.imgheight;
      if(y0 < 0  && y >= -tiles.height)
        for(x = 0; x < tiles.width; x++){
          nx = x0 + x * tiles.imgwidth;
          if(nx > -tiles.width && nx < 128){
            imgadr = readInt(tiles.adr + (x + y * tiles.width) * 2);
            if(imgadr > 0)
              drawImg(imgadr, nx, ny, tiles.imgwidth, tiles.imgheight); 
            else
              fillRect(nx, ny, tiles.imgwidth, tiles.imgheight, espico.bgcolor);
          }
        }
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
    fillRect(espico.regx * 6, espico.regy * 8, 127 - espico.regx * 6, 8, espico.bgcolor);
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
      fillRect(espico.regx * 6, espico.regy * 8, 6, 8, espico.bgcolor);
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
    fillRect(espico.regx * 6, espico.regy * 8, 6, 8, espico.bgcolor);
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

void fillRect(int8_t x, int8_t y, uint8_t w, uint8_t h, uint8_t c){
   for(int16_t jx = x; jx < x + w; jx++)
     for(int16_t jy = y; jy < y + h; jy++)
      setPix(jx, jy, c);
}

void putString(char s[], int8_t y){
  int8_t i = 0;
  while(s[i] != 0 && i < 21){
    putchar(s[i], i * 6, y);
    i++;
  }
}

void putchar(char c, uint8_t x, uint8_t y) {
    for(int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
      uint8_t line = pgm_read_byte(&font_a[c * 5 + i]);
      for(int8_t j=0; j<8; j++, line >>= 1) {
        if(line & 1)
         setPix(x+i, y+j, espico.color);
      }
  }
}

#pragma GCC pop_options

