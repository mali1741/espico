#include "keys.h"

#ifdef _ODROID_GO_H_

void getKey() {
  lastkey = thiskey;

  GO.BtnA.read();
  GO.BtnB.read();
  GO.BtnMenu.read();
  GO.BtnVolume.read();
  GO.BtnSelect.read();
  GO.BtnStart.read();
  GO.JOY_Y.readAxis();
  GO.JOY_X.readAxis();

  thiskey = 0;
  if (GO.JOY_Y.isAxisPressed() == 2) thiskey |= KEY_UP;
  if (GO.JOY_Y.isAxisPressed() == 1) thiskey |= KEY_DOWN;
  if (GO.JOY_X.isAxisPressed() == 2) thiskey |= KEY_LEFT;
  if (GO.JOY_X.isAxisPressed() == 1) thiskey |= KEY_RIGHT;
  // if (GO.BtnMenu.isPressed() == 1) thiskey |= KEY_MENU;
  // if (GO.BtnVolume.isPressed() == 1) thiskey |= KEY_VOLUME;
  if (GO.BtnSelect.isPressed() == 1) thiskey |= KEY_SELECT;
  if (GO.BtnStart.isPressed() == 1)  thiskey |= KEY_START;
  if (GO.BtnB.isPressed() == 1)      thiskey |= KEY_B;
  if (GO.BtnA.isPressed() == 1)      thiskey |= KEY_A;
}

#else 

#ifdef USE_NUNCHUCK
#include <NintendoExtensionCtrl.h>

void getKey(){
  boolean success = nchuk.update();  // Get new data from the controller

  thiskey = 0;
  if (nchuk.joyY() > 192) {
    thiskey |= KEY_UP;
  } else if (nchuk.joyY() < 64) {
    thiskey |= KEY_DOWN;
  }
  if (nchuk.joyX() > 192) {
    thiskey |= KEY_RIGHT;
  } else if (nchuk.joyX() < 64) {
    thiskey |= KEY_LEFT;
  }
  if (nchuk.buttonZ()) thiskey |= KEY_A;
  if (nchuk.buttonC()) thiskey |= KEY_B;
  // KEY_SELECT and KEY_START omitted
}

#else

#include <Wire.h>

void geti2cAdress(){
  byte error,address;
  i2c_adress=0;
  for(address = 1; address < 127; address++ ) 
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0){
      i2c_adress=address;
      return;
    }
    yield();
  }
}

#ifdef ESPBOY
void scani2c(){
  byte error, address;
  int nDevices;
  nDevices = 0;
  for(address = 1; address < 127; address++ ){
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0){
      Serial.print(F("I2C device found at address 0x"));
      if (address<16)
        Serial.print(F("0"));
      Serial.print(address,HEX);
      Serial.println(F("  !"));
      nDevices++;
    }
    else if (error==4){
      Serial.print(F("Unknown error at address 0x"));
      if (address<16)
        Serial.print(F("0"));
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0)
    Serial.println(F("No I2C devices found\n"));
}

void getKey(){
  uint8_t keyread;
  thiskey = 0;
  keyread = mcp.readGPIOAB()&255;
  if(!(keyread&1)) thiskey |= KEY_LEFT;
  if(!(keyread&2)) thiskey |= KEY_UP;
  if(!(keyread&4)) thiskey |= KEY_DOWN;
  if(!(keyread&8)) thiskey |= KEY_RIGHT;
  if(!(keyread&16)) thiskey |= KEY_A;
  if(!(keyread&32)) thiskey |= KEY_B;
  if(!(keyread&64)) thiskey |= KEY_START;
  if(!(keyread&128)) thiskey |= KEY_SELECT;
}
#else
void getKey(){
  byte dio_in;
  Wire.beginTransmission(i2c_adress);
  Wire.write(B11111111); //Конфигурация всех портов PCF8574P на клавиатуре как входа
  Wire.endTransmission();
  Wire.requestFrom(i2c_adress,(uint8_t)1);
  dio_in = Wire.read();  //читаем состояние портов PCF8574P(кнопок)
  lastkey = thiskey;
  thiskey = 0;
  if((dio_in & 128) == 0)
    thiskey |= KEY_A;
  if((dio_in & 64) == 0)
    thiskey |= KEY_B;
  if((dio_in & 32) == 0)
    thiskey |= KEY_START;
  if((dio_in & 16) == 0)
    thiskey |= KEY_SELECT;
  if((dio_in & 8) == 0)
    thiskey |= KEY_RIGHT;
  if((dio_in & 4) == 0)
    thiskey |= KEY_DOWN;
  if((dio_in & 2) == 0)
    thiskey |= KEY_UP;
  if((dio_in & 1) == 0)
    thiskey |= KEY_LEFT;
}
#endif

#endif

#endif
