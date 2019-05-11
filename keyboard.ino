#include "keys.h"

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

void getKey(){
  byte dio_in;
  Wire.beginTransmission(i2c_adress);
  Wire.write(B11111111); //Конфигурация всех портов PCF8574P на клавиатуре как входа
  Wire.endTransmission();
  Wire.requestFrom(i2c_adress,(uint8_t)1);
  dio_in = Wire.read();  //читаем состояние портов PCF8574P(кнопок)
  thiskey = 0;
  if((dio_in & 128) == 0)
    thiskey |= KEY_LEFT;
  if((dio_in & 64) == 0)
    thiskey |= KEY_RIGHT;
  if((dio_in & 32) == 0)
    thiskey |= KEY_DOWN;
  if((dio_in & 16) == 0)
    thiskey |= KEY_UP; //up
  if((dio_in & 8) == 0)
    thiskey |= KEY_SELECT;
  if((dio_in & 4) == 0)
    thiskey |= KEY_B;
  if((dio_in & 2) == 0)
    thiskey |= KEY_A; 
  if((dio_in & 1) == 0)
    thiskey |= KEY_START; 
}

#endif
