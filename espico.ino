#include <Ticker.h>
#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <coos.h>
#include <FS.h>

// #define DISPLAY_X_OFFSET 12
#define INFO_RIGHT
// #define USE_NUNCHUCK
#ifdef USE_NUNCHUCK
#include <NintendoExtensionCtrl.h>
// Use nunchuck
Nunchuk nchuk;
#endif

ADC_MODE(ADC_VCC);

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();
#define RAM_SIZE 21 * 1024
#define PRG_SIZE 16 * 1024

// RAM LAYOUT (accessible)
// 0
//   PROGRAM INCL VARS
// 16 K
//   SPRITE MAP
// 20 K
//   256 bytes SPRITE FLAGS
//   128 bytes LINE_IS_DRAW
//   Stack Pointer (going down from RAM_SIZE-1)
// 21 K

// ------------------begin ESP8266'centric----------------------------------
#define FREQUENCY    160                  // valid 80, 160
//
#include "ESP8266WiFi.h"
extern "C" {
  #include "user_interface.h"
}
// ------------------end ESP8266'centric------------------------------------
int voltaje=0;
uint8_t i2c_adress;
byte thiskey;
Ticker timer;
uint16_t cadr_count;
unsigned long timeF,timeR;
int timeCpu = 0,timeGpu = 0,timeSpr = 0,cpuOPS = 0;
byte fps;
volatile uint16_t timers[8] __attribute__ ((aligned));
uint8_t mem[RAM_SIZE] __attribute__ ((aligned));
static const uint8_t bootseq[] PROGMEM = {0x12,0x00,0x06,0x20,0x61,0x01,0x90,0x00,0x30,0x01,0x11,0x05,0xD4,0x11,0xD0,0x00,0x12,0x01,0x06,0x20,0x61,0x01,0x03,0x01,0x61,0x01,0x12,0x0C,0xC1,0x12,0xC2,0x12,0xB1,0x00,0x92,0x00,0x50,0x00,0x01,0x02,0x52,0x01,0x82,0x02,0x12,0x36,0x82,0x02,0x03,0x02,0x61,0x01,0x82,0x02,0x12,0x18,0x82,0x02,0x12,0x05,0x82,0x02,0x99,0x00,0x48,0x01,0xA8,0xA0,0x99,0x00,0x3E,0x01,0xA8,0x10,0x61,0x01,0x90,0x00,0x16,0x00,0x11,0x0D,0xD4,0x11,0x11,0x02,0xD4,0x51,0x12,0x0C,0x06,0x20,0x61,0x01,0x03,0x01,0x61,0x01,0x12,0x14,0xC1,0x12,0xC2,0x12,0xB1,0x00,0x92,0x00,0x98,0x00,0x01,0x02,0x52,0x01,0x82,0x02,0x12,0x2C,0x82,0x02,0x03,0x02,0x61,0x01,0x82,0x02,0x12,0x18,0x82,0x02,0x12,0x05,0x82,0x02,0x99,0x00,0x48,0x01,0xA8,0xA0,0x99,0x00,0x3E,0x01,0xA8,0x10,0x61,0x01,0x90,0x00,0x5E,0x00,0x11,0x06,0xD4,0x11,0x11,0x03,0xD4,0x51,0x12,0x14,0x06,0x20,0x61,0x01,0x03,0x01,0x61,0x01,0x12,0x1C,0xC1,0x12,0xC2,0x12,0xB1,0x00,0x92,0x00,0xE0,0x00,0x01,0x02,0x52,0x01,0x82,0x02,0x12,0x22,0x82,0x02,0x03,0x02,0x61,0x01,0x82,0x02,0x12,0x18,0x82,0x02,0x12,0x05,0x82,0x02,0x99,0x00,0x48,0x01,0xA8,0xA0,0x99,0x00,0x3E,0x01,0xA8,0x10,0x61,0x01,0x90,0x00,0xA6,0x00,0x11,0x07,0xD4,0x11,0x11,0x04,0xD4,0x51,0x01,0x02,0x52,0x01,0x82,0x02,0x12,0x18,0x82,0x02,0x03,0x02,0x61,0x01,0x82,0x02,0x12,0x18,0x82,0x02,0x12,0x05,0x82,0x02,0x99,0x00,0x48,0x01,0xA8,0xA0,0x12,0x14,0x06,0x20,0x61,0x01,0x03,0x01,0x61,0x01,0x12,0x28,0xC1,0x12,0xC2,0x12,0xB1,0x00,0x92,0x00,0x28,0x01,0x99,0x00,0x3E,0x01,0xA8,0x10,0x61,0x01,0x90,0x00,0x0C,0x01,0x11,0x00,0xD4,0x21,0xD0,0x00,0x9A,0x00,0x01,0x0F,0x00,0x00,0x06,0xF0,0x63,0x01,0x99,0x00,0x0A,0x00,0x50,0x00,0xC2,0x16,0xB1,0x00,0x92,0x00,0x3E,0x01,0x9A,0x00,0x07,0x10,0x12,0x02,0xA0,0x12,0xD4,0xA1,0x9A,0x00,0x00,0x00,0x00,0xE6,0xC9,0x80,0xC8,0xA2,0x10,0x82,0xCA,0x28,0xEC,0x89,0x90};


void loadFromSerial(){
  char c;
  // unsigned char n;
  int j = 0;
  memset(mem, 0, RAM_SIZE);
  while(c != '.'){
    if(Serial.available()){
      c = Serial.read();
      Serial.print(c);
      if(c == '$'){
        char a = Serial.read();
        char b = Serial.read();
        // n = hextobyte(a, b);
        mem[j++] = hextobyte(a, b);
      }
    }
  }
  Serial.print(F("load "));
  Serial.print(j);
  Serial.println(F(" byte"));
  Serial.print(F("free heap "));
  Serial.println(system_get_free_heap_size());
  cpuInit();
}

void coos_cpu(void){   
  while(1){
    COOS_DELAY(1);        // 1 ms
    timeR = millis();
    cpuOPS += 1;
    cpuRun(1100);  
    timeCpu += millis() - timeR;
  }
}

void coos_screen(void){
   while(1){
    yield();
    // timeR = millis();
    // wait 50 ms until new screeb redraw
    // timeSpr += millis() - timeR;
    COOS_DELAY(40);        // 40 ms
    timeR = millis();
    redrawScreen();
    // setRedraw();
    timeGpu += millis() - timeR;
    if(millis() - timeF > 1000){
      timeF = millis();
      fps = cadr_count;
      cadr_count = cadr_count % 2;
    }  
  } 
}

void timer_tick(void){
  for(int8_t i = 0; i < 8; i++){
    if(timers[i] >= 1)
      timers[i] --;
  }
}

void pause(){
  uint8_t prevKey = 128;
//  drawPause();
//  redrawScreen();
#if defined(INFO_RIGHT)
  tft.fillRect(270, 0, 32, 98, 0x0000);
  tft.setTextColor(0xF809);
  tft.fillRect(270, 10, 4, 10, 0xF809);
  tft.fillRect(276, 10, 4, 10, 0xF809);
#endif
  while(1){
    delay(100);
    getKey();
    if((thiskey & 128) && prevKey != 128){
      thiskey = 0;
      delay(800);
      return;
    }
    if(thiskey & 16){
      while(thiskey & 16){
        delay(100);
        getKey();
      }
      thiskey = 0;
      fileList("/");
      return;
    }
    prevKey = thiskey;
  }
}

void coos_key(void){   
  while(1){
    COOS_DELAY(80);        // 100 ms
    getKey();
    if (thiskey == 128) pause();
  }
}

void coos_info(void){   
  while(1){
    COOS_DELAY(1000);        // 1000 ms
    // voltaje = ESP.getVcc();
#if defined(INFO_RIGHT)
    tft.fillRect(270, 0, 32, 98, 0x0000);
    tft.setTextColor(0xFFFF);
    tft.setCursor(270, 0);
    tft.println("fps");
    tft.setCursor(270, tft.getCursorY());
    tft.println(fps);
    tft.setCursor(270, tft.getCursorY());
    tft.println("cpu");
    tft.setCursor(270, tft.getCursorY());
    tft.println(timeCpu, DEC);
    tft.setCursor(270, tft.getCursorY());
    tft.println("gpu");
    tft.setCursor(270, tft.getCursorY());
    tft.println(timeGpu, DEC);
    // tft.println("spr");
    // tft.println(timeSpr, DEC);
    tft.setCursor(270, tft.getCursorY());
    tft.println("kIPS");
    tft.setCursor(270, tft.getCursorY());
    tft.println(cpuOPS, DEC);
#elif defined(INFO_BOTTOM)
    tft.fillRect(0, 232, 320, 8, 0x0000);
    tft.setTextColor(0xFFFF);
    tft.setCursor(12, 232);
    tft.print("fps ");
    tft.print(fps);
    tft.print(" cpu ");
    tft.print(timeCpu, DEC);
    tft.print(" gpu ");
    tft.print(timeGpu, DEC);
    // tft.println("spr");
    // tft.println(timeSpr, DEC);
    tft.print(" kIPS ");
    tft.print(cpuOPS, DEC);
#endif
    timeCpu = 0;
    timeGpu = 0;
    // timeSpr = 0;
    cpuOPS = 0;
  }
}

void setup() {
  byte menuSelected = 3;
  // ------------------begin ESP8266'centric----------------------------------
  WiFi.forceSleepBegin();                  // turn off ESP8266 RF
  delay(1);                                // give RF section time to shutdown
  system_update_cpu_freq(FREQUENCY);
  // ------------------end ESP8266'centric------------------------------------
  tft.init();            // initialize LCD
  tft.setRotation(3);
  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  tft.setTextColor(0xffff);
  Serial.begin (115200);
#ifdef USE_NUNCHUCK
  nchuk.begin();
  while (!nchuk.connect()) {
    Serial.println("Nunchuk not detected!");
    delay(1000);
  }
#else
  Wire.begin(SDA, SCL);
  geti2cAdress();
  Serial.println(i2c_adress, HEX);
#endif

  memset(mem, 0, RAM_SIZE);
  for(int i = 0; i < sizeof(bootseq); i++){
    // mem[i] = bootseq[i];
    mem[i] = (int8_t)pgm_read_byte_near(bootseq + i);
  }

  Serial.println();
  cpuInit();
  //Initialize File System
  if(SPIFFS.begin()){
    Serial.println(F("SPIFFS Initialize....ok"));
    //fileList("/");
  }
  else{
    Serial.println(F("SPIFFS Initialization...failed"));
  }
  voltaje = ESP.getVcc();
  randomSeed(ESP.getVcc());
  // clearScr(0);
  // setColor(1);
  randomSeed(analogRead(0));
  timer.attach(0.001, timer_tick);
  coos.register_task(coos_cpu); 
  coos.register_task(coos_screen);   
  coos.register_task(coos_key); 
  coos.register_task(coos_info);
  coos.start();                     // init registered tasks
}

void loop() {
  coos.run();  // Coos scheduler
  if(Serial.available()){
    char c = Serial.read();
    Serial.print(c);
    if(c == 'm'){
      loadFromSerial();
      cpuInit();
      return;
    }
    else if(c == 'r'){
      ESP.reset();
      return;
    }
    else  if(c == 'd'){
      debug();
      return;
    }
  }
}
