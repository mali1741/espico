// #include "settings.h"
// #define SOUNDPIN 2

#define NEXT_CHAR rtttl.startposition++; c = (char)readMem(rtttl.startposition); if(c == 0) return 0;
#define NEXT_CHAR_IN_P_END rtttl.position++; c = (char)readMem(rtttl.startposition + rtttl.position);
#define NEXT_CHAR_IN_P NEXT_CHAR_IN_P_END if(c == 0) return 0;
#define OCTAVE_OFFSET 0

int notes[] = { 0,
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
  523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
  1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
  2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951
};

struct RTTTL {
  uint16_t address;
  uint16_t position;
  uint16_t startposition;
  uint16_t bpm;
  uint8_t loop;
  uint8_t play;
  uint8_t default_dur;
  uint8_t default_oct;
  uint32_t wholenote;
};

struct PLAY_TONE {
  uint16_t freq;
  uint16_t time;
};

struct RTTTL rtttl;
struct PLAY_TONE play_tone;
int sound_enabled = 1;

inline void addTone(uint16_t f, uint16_t t){
  play_tone.freq = f;
  play_tone.time = t;
}

uint16_t loadRtttl(){
  int num;
  char c;
  rtttl.default_dur = 4;
  rtttl.default_oct = 6;
  rtttl.bpm = 63;
  rtttl.startposition = rtttl.address;
  c = readMem(rtttl.startposition);
  while(c != ':'){
    // ignore name
    NEXT_CHAR
  }
  NEXT_CHAR                     // skip ':'
  // get default duration
  if(c == 'd'){
    NEXT_CHAR
    NEXT_CHAR// skip "d="
    num = 0;
    while(isdigit(c)){
      num = (num * 10) + (c - '0');
      NEXT_CHAR
    }
    if(num > 0) 
      rtttl.default_dur = num;
    NEXT_CHAR                   // skip comma
  }
  // get default octave
  if(c == 'o'){
    NEXT_CHAR
    NEXT_CHAR// skip "o="
    num = c - '0';
    NEXT_CHAR
    if(num >= 3 && num <=7) 
      rtttl.default_oct = num;
    NEXT_CHAR                   // skip comma
  }
  // get BPM
  if(c == 'b'){
    NEXT_CHAR
    NEXT_CHAR// skip "b="
    num = 0;
    while(isdigit(c)){
      num = (num * 10) + (c - '0');
      NEXT_CHAR
    }
    rtttl.bpm = num;
  }
  rtttl.wholenote = (60 * 1000L / (uint32_t)rtttl.bpm) * 4;
  NEXT_CHAR
  rtttl.position = 0;
  return 1;
}

void setRtttlAddress(uint16_t adr){
  rtttl.address = adr;
  loadRtttl();
}

void setRtttlLoop(int16_t loop){
  rtttl.loop = loop;
}

void setRtttlPlay(int16_t play){
  if (play == -1) {
    sound_enabled = !sound_enabled;
    if (!sound_enabled) SOUND_OFF();
  } else if (play == 0) {
    rtttl.play = 0;
    SOUND_OFF();
  } else if(play == 1) {
    rtttl.play = 1;
#ifdef _ODROID_GO_H_
    if (sound_enabled) GO.Speaker.setVolume(11);
#endif
  } else if (play == 2) {
    rtttl.play = 0;
    rtttl.position = 0;
  }
}

uint16_t playRtttl(){
  int num;
  uint32_t duration = 0;
  int note;
  int scale;
  char c;
  //play single tone
  if(play_tone.time){
    if (sound_enabled) es_tone(play_tone.freq, play_tone.time);
    num = play_tone.time;
    play_tone.time = 0;
    return num;
  }
  //player
  if(rtttl.play == 0)
    return 0;
  //first, get note duration, if available
  num = 0;
  c = (char)readMem(rtttl.startposition + rtttl.position);
  if(c == 0){
    if(!rtttl.loop)
      rtttl.play = 0;
    rtttl.position = 0;
    c = (char)readMem(rtttl.startposition + rtttl.position);
  }
  while(isdigit(c)){
    num = (num * 10) + (c - '0');
    NEXT_CHAR_IN_P
  }
  if(num) 
    duration = rtttl.wholenote / num;
  else 
    duration = rtttl.wholenote / (uint32_t)rtttl.default_dur;  // we will need to check if we are a dotted note after
  //now get the note
  note = 0;
  switch(c){
    case 'c':
    case 'C':
      note = 1;
      break;
    case 'd':
    case 'D':
      note = 3;
      break;
    case 'e':
    case 'E':
      note = 5;
      break;
    case 'f':
    case 'F':
      note = 6;
      break;
    case 'g':
    case 'G':
      note = 8;
      break;
    case 'a':
    case 'A':
      note = 10;
      break;
    case 'b':
    case 'B':
      note = 12;
      break;
    case 'p':
    case 'P':
    default:
      note = 0;
  }
  NEXT_CHAR_IN_P_END
  // now, get optional '#' sharp
  if(c == '#'){
    note++;
    NEXT_CHAR_IN_P_END
  }
  // now, get optional '.' dotted note
  if(c == '.'){
    duration += duration/2;
    NEXT_CHAR_IN_P_END
  }
  // now, get scale
  if(isdigit(c)){
    scale = c - '0';
    NEXT_CHAR_IN_P_END
  }
  else{
    scale = rtttl.default_oct;
  }
  scale += OCTAVE_OFFSET;
  if(c == ',') {
    NEXT_CHAR_IN_P_END       // skip comma for next note (or we may be at the end)
  }
  // now play the note
  if(note){
    if (sound_enabled) es_tone(notes[(scale - 4) * 12 + note], duration);
  }
  return duration;
}
