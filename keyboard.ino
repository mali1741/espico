#include <NintendoExtensionCtrl.h>
#include "keys.h"

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

