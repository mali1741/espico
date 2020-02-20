#define FIFO_MAX_SIZE 32*4

#pragma GCC optimize ("-O2")
#pragma GCC push_options

#define toInt16(n) ((int16_t)n)

int16_t reg[16] __attribute__ ((aligned));
int16_t shadow_reg[16] __attribute__ ((aligned));
uint16_t pc = 0;
uint16_t interrupt = 0;
int32_t n = 0;
int32_t shadow_n = 0;
volatile byte redraw = 0;
uint16_t ticks = 0;
char s_buffer[16];

struct Fifo_t {
  uint16_t el[FIFO_MAX_SIZE];
  uint8_t position_read;
  uint8_t position_write;
  uint8_t size;
};

struct Fifo_t interruptFifo __attribute__ ((aligned));

inline void fifoClear(){
  interruptFifo.position_read = 0;
  interruptFifo.position_write = 0;
  interruptFifo.size = 0;
}

void pushInFifo(int16_t n){
  if(interruptFifo.size < FIFO_MAX_SIZE){
    interruptFifo.el[interruptFifo.position_write] = n;
    interruptFifo.position_write++;
    if(interruptFifo.position_write >= FIFO_MAX_SIZE)
      interruptFifo.position_write = 0;
    interruptFifo.size++;
  }
}

uint16_t popOutFifo(){
  uint16_t out = 0;
  if(interruptFifo.size > 0){
    interruptFifo.size--;
    out = interruptFifo.el[interruptFifo.position_read];
    interruptFifo.position_read++;
    if(interruptFifo.position_read >= FIFO_MAX_SIZE)
      interruptFifo.position_read = 0;
  }
  return out;
}

inline void nextinterrupt() {
    reg[0] -= 2;
    writeInt(reg[0], popOutFifo());
    reg[0] -= 2;
    writeInt(reg[0], popOutFifo());
    reg[0] -= 2;
    writeInt(reg[0], pc);
    interrupt = pc;
    pc = popOutFifo();
}

void setinterrupt(uint16_t adr, int16_t param1, int16_t param2){
  if(interrupt == 0 && adr != 0){
    shadow_n = n;
    for(int8_t j = 1; j <= 15; j++){
      shadow_reg[j] = reg[j];
    }
    reg[0] -= 2;
    writeInt(reg[0], param1);
    reg[0] -= 2;
    writeInt(reg[0], param2);
    reg[0] -= 2;
    writeInt(reg[0], pc);
    interrupt = pc;
    pc = adr;
  } else{
    pushInFifo(param1);
    pushInFifo(param2);
    pushInFifo(adr);
  }
}

void cpuInit(){
  for(byte i = 1; i < 16; i++){
    reg[i] = 0;
  }
  interrupt = 0;
  fifoClear();
  // LOCK_DRAWING();
  display_init();
  redraw = 0;
  // UNLOCK_DRAWING();
  reg[0] = PRG_SIZE - 1;//stack pointer
  setCharX(0);
  setCharY(0);
  pc = 0;
  n = 0;
//  carry = 0;
//  zero = 0;
//  negative = 0;
  // tft.setTextColor(palette[espico.color]);
}

void debug(){
  for(byte i = 0; i < 16; i++){
    Serial.print(" R");
    Serial.print(i);
    Serial.print(':');
    Serial.print(reg[i]);
  }
  Serial.print(F(" OP:"));
  Serial.print(readMem(pc),HEX);
  Serial.print(F(" PC:"));
  Serial.println(pc);
  Serial.print(F("carry: "));
  Serial.print((n > 0xffff) ? 1 : 0);
  Serial.print(F(" zero: "));
  Serial.print((n == 0) ? 1 : 0);
  Serial.print(F(" negative: "));
  Serial.print((n < 0) ? 1 : 0);
  Serial.print(F(" interrupt: "));
  Serial.print(interrupt);
  Serial.print('/');
  Serial.println(interruptFifo.size);
}

inline void writeInt(uint16_t adr, int16_t n){
  int8_t *nPtr;
  nPtr = (int8_t*)&n;
  writeMem(adr, *nPtr);
  nPtr++;
  adr++;
  writeMem(adr, *nPtr);
}

inline int16_t readInt(uint16_t adr){
  int16_t n;
  int8_t *nPtr;
  nPtr = (int8_t*)&n;
  *nPtr = readMem(adr);
  nPtr++;
  adr++;
  *nPtr = readMem(adr);
  return n;
}

inline void writeMem(uint16_t adr, int16_t n){
//  if(adr < RAM_SIZE) mem[adr] = n;
  mem[adr&0x7fff] = n;
}

inline byte readMem(uint16_t adr){
  // if(adr < RAM_SIZE) return mem[adr];
  return mem[adr&0x7fff];
//  return (adr < RAM_SIZE) ? mem[adr] : 0;
}

inline void setRedraw(){
  redraw = 1;
}

inline byte getRedraw(){
  if (redraw) {
    // LOCK_DRAWING();
    redraw = 0;
    // memset(line_redraw, 0, 128);
    // UNLOCK_DRAWING();
    return 1;
  }
  return 0;
}


/*
inline int16_t setFlags(int32_t n){
  carry = (n > 0xffff) ? 1 : 0;
  zero = (n == 0) ? 1 : 0;
  negative = (n < 0) ? 1 : 0;
  return (int16_t)n;
}

inline int16_t setFlagsC(int16_t n){
  carry = (n > 0xff) ? 1 : 0;
  zero = (n == 0) ? 1 : 0;
  negative = (n < 0) ? 1 : 0;
  return (uint16_t)n;
}
*/

int16_t isqrt(int16_t n) {
  int g = 0x8000;
  int c = 0x8000;
  for (;;) {
    if (g*g > n) {
      g ^= c;
    }
    c >>= 1;
    if (c == 0) {
      return g;
    }
    g |= c;
  }
}

int16_t distancepp(int16_t x1, int16_t y1, int16_t x2, int16_t y2){
  return isqrt((x2 - x1)*(x2 - x1) + (y2-y1)*(y2-y1));
}

inline uint16_t cpuRun(uint16_t n){
  uint16_t i;
  ticks = n;
  for(i=0; i < ticks; i++)
    cpuStep();
  return i;
}

void cpuStep(){
  byte op1 = readMem(pc++);
  byte op2 = readMem(pc++);
  byte reg1 = 0;
  byte reg2 = 0;
  byte reg3 = 0;
  uint16_t adr;
  uint16_t j;
  //if(isDebug)
  //  debug();
  switch(op1 >> 4){
    case 0x0:
      switch(op1){ 
        case 0x01: 
          //LDI R,int   01 0R XXXX
          reg1 = (op2 & 0xf);
          n = reg[reg1] = readInt(pc);
          pc += 2;
          break;
        case 0x02: 
          //LDI R,(R)   02 RR
          reg1 = ((op2 & 0xf0) >> 4);
          reg2 = (op2 & 0xf);
          n = reg[reg1] = readInt(reg[reg2]);
          break;
        case 0x03: 
          //LDI R,(adr) 03 0R XXXX
          reg1 = (op2 & 0xf);
          n = reg[reg1] = readInt(readInt(pc));
          pc += 2;
          break;
        case 0x04: 
          //LDI R,(int+R) 04 RR XXXX
          reg1 = ((op2 & 0xf0) >> 4);
          reg2 = (op2 & 0xf);
          n = reg[reg1] = readInt(reg[reg2] + readInt(pc));
          pc += 2;
          break;
        case 0x05: 
          //STI (R),R   05 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          writeInt(reg[reg1],reg[reg2]);
          break;
        case 0x06:
          if((op2 & 0x0f) == 0){
            //STI (adr),R 06 R0 XXXX
            reg1 = (op2 & 0xf0) >> 4;
            writeInt(readInt(pc),reg[reg1]);
            pc += 2;
          }
          else{
            //STI (adr+R),R 06 RR XXXX
            reg1 = (op2 & 0xf0) >> 4;
            reg2 = op2 & 0xf;
            writeInt(readInt(pc) + reg[reg1],reg[reg2]);
            pc += 2;
          }
          break;
        case 0x07:
          //MOV R,R   07 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          reg[reg1] = reg[reg2];
          break;
        case 0x08:
          //LDIAL R,(int+R*2) 08 RR XXXX
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = reg[reg1] = readInt(reg[reg2] * 2 + readInt(pc));
          pc += 2;
          break;
        case 0x09:
          //STIAL (adr+R*2),R   09 RR XXXX
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          writeInt(readInt(pc) + reg[reg1] * 2,reg[reg2]);
          pc += 2;
          break;
        default:
          pc++;
      }
      break;
    case 0x1:
      // LDC R,char 1R XX
      reg1 = (op1 & 0xf);
      n = reg[reg1] = op2;
      break;
    case 0x2:
      if(op1 == 0x20){
        // LDC R,(R)  20 RR
        reg1 = ((op2 & 0xf0) >> 4);
        reg2 = (op2 & 0xf);
        n = reg[reg1] = readMem(reg[reg2]);
      }
      else{
        // LDC R,(R+R)  2R RR
        reg1 = (op1 & 0xf);
        reg2 = ((op2 & 0xf0) >> 4);
        reg3 = (op2 & 0xf);
        n = reg[reg1] = readMem(reg[reg2] + reg[reg3]);
      }
      break;
    case 0x3: 
      switch(op1){
        case 0x30:
          // LDC R,(int+R)30 RR XXXX
          reg1 = ((op2 & 0xf0) >> 4);
          reg2 = (op2 & 0xf);
          n = reg[reg1] = readMem(reg[reg2] + readInt(pc));
          pc += 2;
          break;
        case 0x31:
          // LDC R,(adr)  31 0R XXXX
          reg1 = (op2 & 0xf);
          n = reg[reg1] = readMem(readInt(pc));
          pc += 2;
          break;
        case 0x32:
          // STC (adr),R  32 0R XXXX
          reg1 = (op2 & 0xf0) >> 4;
          writeMem(readInt(pc),reg[reg1]);
          pc += 2;
          break;
        case 0x33:
          // STC (int+R),R33 RR XXXX
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          writeMem(readInt(pc) + reg[reg1],reg[reg2]);
          pc += 2;
          break;
      }
      break;
    case 0x4:
      if(op1 == 0x40){
          // STC (R),R  40 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          writeMem(reg[reg1], reg[reg2]);
        }
        else{
          // STC (R+R),R  4R RR 
          reg1 = (op1 & 0xf);
          reg2 = ((op2 & 0xf0) >> 4);
          reg3 = (op2 & 0xf);
          writeMem(reg[reg1] + reg[reg2], reg[reg3]);
        }
      break;
    case 0x5:
      switch(op1){ 
        case 0x50:
          //HLT       5000
          //FLIP      5050
          if (op2 == 0x50) {
            if (redraw) {
            // LOCK_DRAWING();
              redraw = 0;
                // memset(line_redraw, 0, 128);

            // UNLOCK_DRAWING();
              // memset(line_redraw, 0, 128);
              break;
            }
            // disable cpuRun
            ticks = 0;
            pc -= 2;
          } else {
            fileList("/games");
          }
          break;
        case 0x51:
          // STIMER R,R   51RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          timers[reg[reg1] & 0x7] = reg[reg2];
          break;
        case 0x52:
          // GTIMER R   520R
          reg1 = op2 & 0xf;
          n = reg[reg1] = timers[reg[reg1] & 0x7];
          break;
        case 0x55:
          // EPSTAT R   55XR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          if (reg1 == 0 ) {
            // LOCK_DRAWING();
            setEspicoState(reg1,reg[reg2]);
            // UNLOCK_DRAWING();
          } else {
            setEspicoState(reg1,reg[reg2]);
          }
          break;
      }
      break;
    case 0x6:
      // LDIAL R,(R+R*2)  6R RR
      reg1 = (op1 & 0xf);
      reg2 = ((op2 & 0xf0) >> 4);
      reg3 = (op2 & 0xf);
      n = reg[reg1] = readInt(reg[reg2] + reg[reg3]*2);
      break;
    case 0x7:
      // STIAL (R+R*2),R  7R RR
      reg1 = (op1 & 0xf);
      reg2 = ((op2 & 0xf0) >> 4);
      reg3 = (op2 & 0xf);
      writeInt(reg[reg1] + reg[reg2]*2, reg[reg3]);
      break;  
    case 0x8:
      switch(op1){
        case 0x80:
          // POP R    80 0R
          reg1 = (op2 & 0xf);
          reg[reg1] = readInt(reg[0]);
          reg[0] += 2;
          break;
        case 0x81:
          // POPN R   81 0R
          reg1 = (op2 & 0xf);
          for(j = reg1; j >= 1; j--){
            reg[j] = readInt(reg[0]);
            reg[0] += 2;
          }
          break;
        case 0x82:
          // PUSH R   82 0R
          reg1 = (op2 & 0xf);
          reg[0] -= 2;
          writeInt(reg[0], reg[reg1]);
          break;
        case 0x83:
          // PUSHN R    83 0R
          reg1 = (op2 & 0xf);
          for(j = 1; j <= reg1; j++){
            reg[0] -= 2;
            writeInt(reg[0], reg[j]);
          }
          break;
        case 0x84:
          // PUSH int   84 00 XXXX
          reg[0] -= 2;
          writeInt(reg[0], readInt(pc));
          pc += 2;
          break;
        case 0x88:
        {
          // MEMSET R             88 0R
          // MEMCPY R             88 1R
          reg1 = (op2 & 0xf0);
          reg2 = (op2 & 0x0f);
          adr = reg[reg2];
          uint16_t ptr1 = (readInt(adr + 4) & 0x7fff);
          uint16_t ptr2 = (readInt(adr + 2) & 0x7fff);
          uint16_t numb = readInt(adr);
          if (numb > 0x8000) numb = 0x8000;
            if (ptr1 + numb > 0x8000) numb = 0x8000 - ptr1;
            if (reg1 == 0x00) {
              memset(&mem[ptr1], ptr2 & 0xff, numb);
            } else if (reg1 == 0x10) {
              if (ptr2 + numb > 0x8000) numb = 0x8000 - ptr2;
              memcpy(&mem[ptr1], &mem[ptr2], numb);
            }
        }
          break;
        case 0x8C:
          {// MEMCONV R             8C 0R
          reg1 = (op2 & 0xf0);
          reg2 = (op2 & 0x0f);
          if (reg1 == 0x00) {
            adr = reg[reg2];
            uint16_t ptr1 = (readInt(adr + 4) & 0x7fff);
            uint16_t ptr2 = (readInt(adr + 2) & 0x7fff);
            uint16_t numb = readInt(adr);
            if (numb > 0x8000) numb = 0x8000;
            if (ptr1 + numb > 0x8000) numb = 0x8000 - ptr1;
            if (ptr2 + 256 > 0x8000) break;
            for(int i = 0; i < numb; i++)
              mem[ptr1+i] = mem[ptr2+mem[ptr1+i]];
          }
          }
          break;
      }
      break;
    case 0x9:
      switch(op1){
        case 0x90:
          // JMP adr    90 00 XXXX
          pc = readInt(pc);
          break;
        case 0x91:
          // JNZ adr    91 00 XXXX
          if(n != 0)
            pc = readInt(pc);
          else 
            pc += 2;
          break;
        case 0x92:
          // JZ adr   92 00 XXXX
          if(n == 0)
            pc = readInt(pc);
          else 
            pc += 2;
          break;
        case 0x93:
          // JNP adr    93 00 XXXX
          if(n < 0)
            pc = readInt(pc);
          else 
            pc += 2;
          break;
        case 0x94:
          // JP adr   94 00 XXXX
          if(n >= 0)
            pc = readInt(pc);
          else 
            pc += 2;
          break;
        case 0x95:
          // JNC adr    95 00 XXXX
          if(n <= 0xffff)
            pc = readInt(pc);
          else 
            pc += 2;
          break;
        case 0x96:
          // JC adr   96 00 XXXX
          if(n > 0xffff)
            pc = readInt(pc);
          else 
            pc += 2;
          break;
        case 0x97:
          // JZR R,adr  97 0R XXXX
          reg1 = op2 & 0xf;
          if(reg[reg1] == 0)
            pc = readInt(pc);
          else 
            pc += 2;
          break;
        case 0x98:
          // JNZR R,adr 98 0R XXXX
          reg1 = op2 & 0xf;
          if(reg[reg1] != 0)
            pc = readInt(pc);
          else 
            pc += 2;
          break;
        case 0x99:
          // CALL adr   99 00 XXXX
          // CALL (adr)   99 10 XXXX
          // CALL (adr+R)   99 2R XXXX
          reg1 = op2 & 0xf; 
          reg[0] -= 2;
          // if(reg[0] < 0)
          //  reg[0] += 0xffff;
          writeInt(reg[0], pc + 2);
          if ((op2 & 0xf0) == 0x20) pc = readInt(readInt(pc) + reg[reg1]);
          else if ((op2 & 0xf0) == 0x10) pc = readInt(readInt(pc));
          else pc = readInt(pc);
          break;
        case 0x9A:
          // RET      9A 00
          if(interrupt == 0){
            pc = readInt(reg[0]);
            reg[0] += 2;
          }
          else{
            pc = readInt(reg[0]);
            if(pc == interrupt){
              reg[0] += 6;
              if(interruptFifo.size > 0) {
                nextinterrupt();
              } else {
                for(int8_t j = 15; j >= 1; j--){
                  reg[j] = shadow_reg[j];
                }
                n = shadow_n;
                interrupt = 0;
              }
            } else {
              reg[0] += 2;
            }
          }
      }
      break;
    case 0xA:
      switch(op1){
        case 0xA0:
          // ADD R,R    A0 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (int32_t)reg[reg1] + reg[reg2];
          reg[reg1] = toInt16(n);
          break;
        case 0xA1:
          // ADC R,R    A1 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (int32_t)reg[reg1] + reg[reg2] + ((n > 0xffff) ? 1 : 0);
          reg[reg1] = toInt16(n);
          break;
        case 0xA2:
          // SUB R,R    A2 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (int32_t)reg[reg1] - reg[reg2];
          reg[reg1] = toInt16(n);
          break;
        case 0xA3:
          // SBC R,R    A3 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (int32_t)reg[reg1] - reg[reg2] - ((n > 0xffff) ? 1 : 0);
          reg[reg1] = toInt16(n);
          break;
        case 0xA4:
          // MUL R,R    A4 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (int32_t)reg[reg1] * (int32_t)reg[reg2];
          reg[reg1] = toInt16(n);
          break;
        case 0xA5:
          // DIV R,R    A5 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          if(reg[reg1] == 0) {
            n = 0;
            reg[reg2] = 0;
          } else if(reg[reg2] == 0) {
            n = (reg[reg1] > 0)? 0x7fff : 0x8000;
            reg[reg2] = 0;
          } else {
            n = (int32_t)reg[reg1] / reg[reg2];
            int m = abs(reg[reg1] % reg[reg2]);
            reg[reg2] = (reg[reg2] < 0)? -m : m;
          }
          reg[reg1] = toInt16(n);
          break;
        case 0xA6:
          // AND R,R    A6 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (reg[reg1] & reg[reg2]);
          reg[reg1] = toInt16(n);
          break;
        case 0xA7:
          // OR R,R   A7 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (reg[reg1] | reg[reg2]);
          reg[reg1] = toInt16(n);
          break;
        case 0xA8:
          if(op2 == 0x10){
            // INC adr    A8 10 XXXX
            reg1 = op2 & 0xf;
            n = readInt(readInt(pc)) + 1;
            writeInt(readInt(pc), n);
            pc += 2;
          }
          else if(op2 > 0x10){
            // INC R,n    A8 nR
            reg1 = op2 & 0xf;
            n = (int32_t)reg[reg1] + (op2 >> 4);
            reg[reg1] = toInt16(n);
          }
          else{
            // INC R    A8 0R       
            reg1 = op2 & 0xf;
            n = (int32_t)reg[reg1] + 1;
            reg[reg1] = toInt16(n);
          }
          break;
        case 0xA9:
          if(op2 == 0x10){
            // DEC adr    A9 10 XXXX
            reg1 = op2 & 0xf;
            n = readInt(readInt(pc)) - 1;
            writeInt(readInt(pc), n);
            pc += 2;
          }
          else if(op2 > 0x10){
            // DEC R,n    A9 nR
            reg1 = op2 & 0xf;
            n = (int32_t)reg[reg1] - (op2 >> 4);
            reg[reg1] = toInt16(n);
          }
          else{
            // DEC R    A9 0R
            reg1 = op2 & 0xf;
            n = (int32_t)reg[reg1] - 1;
            reg[reg1] = toInt16(n);
          }
          break;
        case 0xAA:
          // XOR R,R    AA RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (reg[reg1] ^ reg[reg2]);
          reg[reg1] = toInt16(n);
          break;
        case 0xAB:
          // SHL R,R    AB RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (int32_t)reg[reg1] << reg[reg2];
          reg[reg1] = toInt16(n);
          break;
        case 0xAC:
          // SHR R,R    AC RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (int32_t)reg[reg1] >> reg[reg2];
          reg[reg1] = toInt16(n);
          break;
        case 0xAD:
          reg1 = op2 & 0xf;
          reg2 = op2 & 0xf0;
          // RAND R,R   AD 0R
          if(reg2 == 0x00){
            n = random(0, reg[reg1] + 1);
            reg[reg1] = toInt16(n);
          }
          // SQRT R    AD 1R
          else if(reg2 == 0x10){
            reg[reg1] = isqrt(reg[reg1]);
            n = reg[reg1];
          }
          // COS R    AD 2R
          else if(reg2 == 0x20){
            reg[reg1] = getCos(reg[reg1]);
            n = reg[reg1];
          }
          // SIN R    AD 3R
          else if(reg2 == 0x30){
            reg[reg1] = getSin(reg[reg1]);
            n = reg[reg1];
          }
          // ABS R    AD 4R
          else if(reg2 == 0x40){
            n = abs(reg[reg1]);
            reg[reg1] = n;
          }
           // NOTL R    AD 5R
          else if(reg2 == 0x50){
            n = (reg[reg1]) ? 0 : 1;
            reg[reg1] = n;
          }
           // NOT R    AD 6R
          else if(reg2 == 0x60){
            n = ~(reg[reg1]);
            reg[reg1] = n;
          }
          break;
        case 0xAE:
          // ANDL R,R   AE RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (reg[reg1] != 0 && reg[reg2] != 0) ? 1 : 0;
          reg[reg1] = n;
          break;
        case 0xAF:
          // ORL R,R    AF RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (reg[reg1] != 0 || reg[reg2] != 0) ? 1 : 0;
          reg[reg1] = n;
          break;
      }
      break;
    case 0xB:
      //CMP R,CHR   BR XX
      reg1 = (op1 & 0x0f);
      n = (int32_t)reg[reg1] - op2;
      break;
    case 0xC:
      switch(op1){
        case 0xC0:
          //CMP R,INT   C0 R0 XXXX
          reg1 = (op2 & 0xf0) >> 4;
          n = (int32_t)reg[reg1] - readInt(pc);
          pc += 2;
          break;
        case 0xC1:
          //CMP R,R   C1 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (int32_t)reg[reg1] - (int32_t)reg[reg2];
          break;
        case 0xC2:
          //LDF R,F   C2 RF
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          if(reg2 == 0)
            reg[reg1] = (n > 0xffff) ? 1 : 0;
          else if(reg2 == 1)
            reg[reg1] = (n == 0) ? 1 : 0;
          else if(reg2 == 2)
            reg[reg1] = (n < 0) ? 1 : 0;
          else if(reg2 == 3){ //pozitive
            if(n > 0)
              reg[reg1] = 1;
            else
              reg[reg1] = 0;
          }
          else if(reg2 == 4){ //not pozitive
            if(n > 0)
              reg[reg1] = 0;
            else
              reg[reg1] = 1;
          }
          else if(reg2 == 5)
            reg[reg1] = (n == 0) ? 0 : 1;
          else if(reg2 == 6){
              reg[reg1] = redraw;
              redraw = 0;
            }
          else
            reg[reg1] = 0;
          break;
        case 0xC3:
          //LDRES X,R   C3 XR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          reg[reg2] = toInt16(n >> reg1);
          break;
        case 0xC4:
          // MULRES X,R    C4 XR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = (int32_t)reg[reg2] * (int32_t)(1 << reg1);
          reg[reg2] = toInt16(n);
          break;
        case 0xC5:
          // DIVRES X,R    C5 XR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          if (reg1 == 0) {
            if(reg[reg2] != 0)
              n = n / (int32_t)reg[reg2];
            else
              n = 0;//error
          } else {
            n = n / (int32_t)(1 << reg1);
          }
          reg[reg2] = toInt16(n);
          break;
        case 0xCA:
          // ATAN2 R,R   CA RR
          reg1 = (op2 & 0xf0) >> 4;//y
          reg2 = op2 & 0xf;//x
          n = reg[reg1] = atan2_rb(reg[reg1], reg[reg2]);
          break;
      }
      break;
    case 0xD:
      switch(op1){ 
        case 0xD0:
          //CLS   D000
          if (op2 == 0x00) {
            clearScr(espico.bgcolor);
          } else if ((op2 & 0xf0) == 0x10) {
            //DRECT D0 1R
            reg1 = (op2 & 0xf);
            adr = reg[reg1];
            drawRect(readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
          } else if ((op2 & 0xf0) == 0x20) {
            //FRECT D0 2R
            reg1 = (op2 & 0xf);
            adr = reg[reg1];
            fillRect(readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
          } else if ((op2 & 0xf0) == 0x30) {
            //DCIRC D0 3R
            reg1 = (op2 & 0xf);
            adr = reg[reg1];
            drawCirc(readInt(adr + 4), readInt(adr + 2), readInt(adr));
          } else if ((op2 & 0xf0) == 0x40) {
            //FCIRC D0 4R
            reg1 = (op2 & 0xf);
            adr = reg[reg1];
            fillCirc(readInt(adr + 4), readInt(adr + 2), readInt(adr));
          }
          break;
        case 0xD1:
          switch(op2 & 0xf0){
            case 0x00:
              //PUTC R  D10R
              reg1 = (op2 & 0xf);
              printc((char)reg[reg1]);
              break;
            case 0x10:
              //PUTS R  D11R
              reg1 = (op2 & 0xf);
              prints(reg[reg1]);
              break;
            case 0x20:
              //PUTN R D12R
              reg1 = (op2 & 0xf);
              if(reg[reg1] < 32768)
                itoa(reg[reg1], s_buffer, 10);
              else
                itoa(reg[reg1]-0x10000, s_buffer, 10);
              j = 0;
              while (s_buffer[j]){
                printc(s_buffer[j++]);
              }
              break;
            case 0x30:
              //SETX R      D13R
              reg1 = (op2 & 0xf);
              setCharX(reg[reg1] & 0xff);
              break;
            case 0x40:
              //SETY R      D14R
              reg1 = (op2 & 0xf);
              setCharY(reg[reg1] & 0xff);
              break;
            case 0x50:
              //CLIP R      D15R
              reg1 = (op2 & 0xf);
	      adr = reg[reg1];
              setClip(readInt(adr+6), readInt(adr+4), readInt(adr+2), readInt(adr));
              break;
            case 0xA0:
              //DRWADR R      D1AR
              //RSTDAD        D1A0
              reg1 = (op2 & 0xf);
              if (reg1 == 0) {
                resetDrawAddr();
              } else {
                setDrawAddr(reg[reg1]);
              }
              break;
            case 0xB0:
              //PUTRES X                      D1BX
              {
              const int8_t deci[] = {0,1,2,3,3,3, 3,3,3,3,3, 4,4,5,5,5};
              reg1 = (op2 & 0xf);
              int16_t numi = toInt16(n);
              if (numi < 0) {
                printc('-');
		            numi = 0 - numi;
              }
              itoa(numi >> reg1, s_buffer, 10);
              j = 0;
              while (s_buffer[j]) {
                printc(s_buffer[j++]);
              }
              if (reg1 > 0) {
                const uint16_t fracmask = (1 << reg1) - 1; 
                int32_t numf = numi; 
                printc('.');
		            for(int j = 0; j < deci[reg1]; j++) {
                  numf &= fracmask;
                  numf *= 10;
                  printc((uint8_t)(numf >> reg1) + '0');
                }
              }
              }
              break;
            case 0xD0:
              //RSTLRD R      D1D0
              reg1 = (op2 & 0xf);
              if (reg1 == 0) {
                memset(line_draw, 0, LINE_REDRAW_SIZE);
              }
              break;
          }
          break;
        case 0xD2: 
          switch(op2 & 0xf0){
            case 0x00:
              // GETK R     D20R
              reg1 = (op2 & 0xf);
              if(Serial.available())
                reg[reg1] = Serial.read();
              else
                pc -= 2;
              break;
            case 0x10:
              // GETJ R     D21R
              reg1 = (op2 & 0xf);
              n = reg[reg1] = thiskey;
              break;
            case 0x20:
              // BTN R                        D22R
              reg1 = (op2 & 0xf);
              n = thiskey;
              if (reg[reg1] != -1) {
                reg[reg1] &= 7;
                n &= (1 << reg[reg1]);
              }
              reg[reg1] = n;
              break;
            case 0x30:
              // BTNP R                       D23R
              reg1 = (op2 & 0xf);
              n = thiskey & (~lastkey);
              if (reg[reg1] != -1) {
                reg[reg1] &= 7;
                n &= (1 << reg[reg1]);
              }
              reg[reg1] = n;
              break;

          }
          break;
        case 0xD3:
          // PPIX R,R   D3RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          setPix(reg[reg1], reg[reg2], drwpalette[espico.color]);
          break;
        case 0xD4:
          switch(op2 & 0xf0){
              case 0x00:
                // DRWIM R      D40R
                reg1 = op2 & 0xf;
                adr = reg[reg1];//the register contains the address of the values located sequentially h, w, y, x, аdr
                drawImg(readInt(adr + 8), readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
                break;
              case 0x10:
                // SFCLR R      D41R
                reg1 = op2 & 0xf;
                espico.color = reg[reg1] & 0xf;
                break;
              case 0x20:
                // SBCLR R      D42R
                reg1 = op2 & 0xf;
                espico.bgcolor = reg[reg1] & 0xf;
                break;
              case 0x30:
                // GFCLR R      D43R
                reg1 = op2 & 0xf;
                n = reg[reg1] = espico.color;
                break;
              case 0x40:
                // GBCLR R      D44R
                reg1 = op2 & 0xf;
                n = reg[reg1] = espico.bgcolor;
                break;
              case 0x50:
                // IMOPTS       D45R
                reg1 = op2 & 0xf;
                if (reg[reg1] & 31) setImageSize(reg[reg1] & 31);
		            setImageFlipXY(reg[reg1]);
                break;
              case 0x60:
                // DLINE      D46R
                reg1 = op2 & 0xf;
                adr = reg[reg1];//the register contains the address of the values located sequentially y1, x1, y, x
                drwLine(readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
                break;
              case 0x70:
                // // DRWSPR R   D47R
                reg1 = op2 & 0xf;
                adr = reg[reg1];//the register contains the address of the values located sequentially h, w, y, x, аdr
                drawSprite(readInt(adr + 8), readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
                break;
              case 0x80:
                // SMAPXY R   D4 8R
                reg1 = op2 & 0xf;
                adr = reg[reg1];////the register contains the address of the values located sequentially height, width, iheight, iwidth, adr
                setTile(readInt(adr + 4), readInt(adr + 2), readInt(adr));
                break;
              case 0x90:
                // ACTDS R*2 D4 9R
                reg1 = op2 & 0xf;
                adr = reg[reg1];////the register contains the address of the values located sequentially direction, speed, n
                actorSetDirectionAndSpeed(readInt(adr + 4), readInt(adr + 2), readInt(adr));
                break;
              case 0xA0:
                // DRWBIT R  D4 AR
                reg1 = op2 & 0xf;
                adr = reg[reg1];//the register contains the address of the values located sequentially h, w, y, x, аdr
                drawImageBit(readInt(adr + 8), readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
                break;
              case 0xB0:
                // DRWMAP R   D4 BR
                reg1 = op2 & 0xf;
                adr = reg[reg1]; // stack adr, for layer, celh, celw, y0, x0, cely, celx 
                // drawTileMap(0, 0, 0, 8, readInt(adr + 4), readInt(adr + 2), readInt(adr));
                drawTileMap(readInt(adr + 12), readInt(adr + 10), readInt(adr + 8), readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
                break;
              case 0xC0:
                // MVACT R    D4 CR
                reg1 = op2 & 0xf;
                moveActor(reg[reg1]);
                break;
              case 0xD0:
                // DRWACT R    D4 DR
                reg1 = op2 & 0xf;
                drawActor(reg[reg1]);
                break;
              case 0xE0:
                // TACTC R    D4 ER
                reg1 = op2 & 0xf;
                testActorCollision();
                break;
              case 0xF0:
                // TACTM R    D4 FR
                reg1 = op2 & 0xf;
                adr = reg[reg1];
                testActorMap(readInt(adr + 12), readInt(adr + 10), readInt(adr + 8), readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
                break;
            }
            break;
        case 0xD5:
          // FSET R,R   D5RR
          // FGET R     D50R
          reg1 = (op2 & 0xf0) >> 4; //sprite
          reg2 = op2 & 0xf; // flag
          if (reg1 == 0) n = reg[reg2] = getSpriteFlag(reg[reg2]);
          else setSpriteFlag(reg[reg1], reg[reg2]);
          break;
        case 0xD6:
          // RPALET       D600
          if (op2 == 0x00) {
            resetPalette(); 
          } else {
          // SPALET R,R   D6 RR
            reg1 = (op2 & 0xf0) >> 4;//palette color
            reg2 = op2 & 0xf; // color
	          if (reg1 == 0) {  // PALT  D6 0R
              setPalT(reg[reg2]);
	          } else {
              changePalette(reg[reg1] & 15, reg[reg2]);
	          }
          }
          break;
        case 0xD7:
            reg1 = op2 & 0xf;
            adr = reg[reg1];
            switch (op2 & 0xf0) {
              case 0x00:
              // SPART R     D7 0R
              // the register contains the address of the values located sequentially  count, time, gravity
              setParticleTime(readInt(adr + 2), readInt(adr));
              break;
              case 0x10:
              // the register contains the address of the values located sequentially  speed, direction2, direction1, time
              setEmitter(readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
              break;
              case 0x20:
              // the register contains the address of the values located sequentially  color, y, x
              drawParticles(readInt(adr + 8), readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
              break;
              case 0x50:
              // the register contains the address of the values located sequentially  y2,x2,y1,x1
              n = reg[reg1] = distancepp(readInt(adr + 6), readInt(adr + 4), readInt(adr + 2), readInt(adr));
              break;
              case 0x60:
              animateParticles();
              break;
              case 0x70:
              n = reg[reg1] = makeParticleColor(readInt(adr + 6),readInt(adr + 4), readInt(adr + 2), readInt(adr));
              break;
            }
            break;
        case 0xD8:
          // SGET R,R   D8 RR
          reg1 = (op2 & 0xf0) >> 4;//x
          reg2 = op2 & 0xf;//y
          n = reg[reg1] = getSpritePix(reg[reg1], reg[reg2]);
          break;
        case 0xD9:
          // GETPIX R,R   D9RR
          reg1 = (op2 & 0xf0) >> 4;//x
          reg2 = op2 & 0xf;//y
          n = reg[reg1] = getPix(reg[reg1], reg[reg2]);
          break;
        case 0xDA:
          // SSET R,R   DA RR
          reg1 = (op2 & 0xf0) >> 4;//x
          reg2 = op2 & 0xf;//y
          setSpritePix(reg[reg1], reg[reg2], drwpalette[espico.color]);
          break;
        case 0xDB:
          // GACTXY R,R   D8 RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = reg[reg1] = getActorInXY(reg[reg1], reg[reg2]);
          break;
        case 0xDC:
          // ACTGET R,R   DC RR
          reg1 = (op2 & 0xf0) >> 4;//num
          reg2 = op2 & 0xf;//value
          n = reg[reg1] = getActorValue(reg[reg1] & 31, reg[reg2]);
          break;
        case 0xDE:
          // AGBACT R,R     DE RR
          reg1 = (op2 & 0xf0) >> 4;//n1
          reg2 = op2 & 0xf;//n2
          n = reg[reg1] = angleBetweenActors(reg[reg1], reg[reg2]);
          break;
        case 0xDF:
          // GMAPXY R,R      DF RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          n = reg[reg1] = getTile(reg[reg1], reg[reg2]);
          break;
      }
      break;
    case 0xE:
      // SOUND
      switch (op1) {
        case 0xE0:
          switch (op2) {
            // PLAYRT               E000
            case 0x00:
              setRtttlPlay(1);
              break;
            // PAUSERT              E001
            case 0x01:
              setRtttlPlay(0);
              break;
            // STOPRT               E002
            case 0x02:
              setRtttlPlay(2);
              break;
          }
          break;
        case 0xE1:
          // LOADRT               E1RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          setRtttlAddress(reg[reg1]);
          setRtttlLoop(reg[reg2]);
          break;
        case 0xE2:
          // PLAYTN               E2RR
          reg1 = (op2 & 0xf0) >> 4;
          reg2 = op2 & 0xf;
          addTone(reg[reg1], reg[reg2]);
          break;
      }
      break;
    case 0xF:
      // ACTSET R,R,R FR RR
      reg1 = (op1 & 0xf);//sprite number
      reg2 = (op2 & 0xf0) >> 4;//type
      reg3 = op2 & 0xf;//value
      if (reg[reg2] == -1) resetActor(reg[reg1]);
      else setActorValue(reg[reg1] & 0x1f, reg[reg2], reg[reg3]); 
      break;
  }
}

#pragma GCC pop_options
