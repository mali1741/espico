void fileList(String path) {
  Dir dir = SPIFFS.openDir(path);
  char s[32];
  char thisF[32];
  int16_t lst = 1;
  int16_t pos = 0;
  int16_t startpos = 0;
  int16_t fileCount = 0;
  int16_t skip = 0;
  while (dir.next()) {
    File entry = dir.openFile("r");
    strcpy(s, entry.name());
    Serial.println(s);
    entry.close();
    fileCount++;
  }
  while(1){
    skip = startpos;
    lst = 1;
    dir = SPIFFS.openDir(path);
    setColor(1);
    while (dir.next() && lst < 14) {
      File entry = dir.openFile("r");
      if(skip > 0){
        skip--;
      }
      else{
        strcpy(s, entry.name());
        if(lst + startpos - 1 == pos)
          strcpy(thisF, entry.name());
        putString(s, lst * 8);
        lst++;
      }
      entry.close();   
    }
    if(lst + startpos - 1 < pos){
      if(fileCount > pos)
        startpos++;
      else
        pos--;
    }
    else if(startpos > pos){
      startpos = pos;
    }
    setColor(8);
    drwLine(2, (pos - startpos + 1) * 8, 124,  (pos - startpos + 1) * 8);
    drwLine(2, (pos - startpos + 1) * 8 + 7, 124,  (pos - startpos + 1) * 8 + 7);
    redrawScreen();
    clearScr(0);
    while(thiskey != 0){
      getKey();
      delay(100);
    }
    while(thiskey == 0){   
      getKey();
      delay(100);
      if(Serial.available()){
        char c = Serial.read();
        Serial.print(c);
        if(c == 'm'){
          loadFromSerial();
          cpuInit();
          return;
        }
      }
    }
    if(thiskey & 16){//ok
      cpuInit();
      int len = strlen(thisF);
      if (len > 4 && strcmp(&thisF[len-4], ".epo") == 0) {
        loadEPO(thisF);
        // setEspicoPalette();
        for(int i = 0; i < 16; i++){
          palette[i] = (uint16_t)pgm_read_word_near(epalette + i);
        }
      } else {
        loadFromSPIFS(thisF);
      }
      return;
    }
    else if(thiskey & 2){//down
      if(pos < fileCount - 1)
        pos++;
      if(pos - startpos > 12)
        startpos++;
    }
    else if(thiskey & 1){//up
      if(pos > 0)
        pos--;
      if(pos - startpos < 0)
        startpos--;
    }
    if(thiskey & 4){//left
      cpuInit();
      return;
    }
  }
}

inline byte hextoval(char h) {
  return ((h >= '0' && h <= '9') ? h - '0' : (h >= 'A' && h <= 'F') ? h - '7' : (h >= 'a' && h <= 'f') ? h - 'W' : 0);
}

inline byte hextobyte(char a, char b) {
  return ((hextoval(a) << 4) + hextoval(b));
}

void loadFromSPIFS(char fileName[]){
  int i;
  File f = SPIFFS.open(fileName, "r");
  if(f.size() < RAM_SIZE)
    for(i = 0; i < f.size(); i++){
      mem[i] = (uint8_t)f.read();
    }
  Serial.print(F("loaded "));
  Serial.print(i);
  Serial.println(F(" byte"));
  Serial.print(F("free heap "));
  Serial.println(system_get_free_heap_size());
  f.close();  //Close file
}

void loadEPO(char fileName[]){
  int i;
  char hex[8];
  uint8_t *store;
  int max_bytes = 0;
  int bytes_read = 0;
  int new_line = 1;
  Serial.print(F("loading EPO: "));
  Serial.println(fileName);
  File f = SPIFFS.open(fileName, "r");
  while (f.available()) {
    if (f.readBytesUntil('\n', hex, 2) == 2) {
      if (new_line) {
        if (hex[0] == '_' && hex[1] == '_') {
          // reset counters
          if (bytes_read > 0) {
            Serial.print(F("read "));
            Serial.print(bytes_read);
            Serial.println(F(" byte"));
          }
          max_bytes = 0;
          bytes_read = 0;
          if (f.readBytesUntil('\n', hex, 5) == 5) {
            hex[5] = 0;
            // set section if found
            if (strcmp(hex, "epo__") == 0) {
              Serial.println(F(" found epo header"));
              store = mem;
              max_bytes = PRG_SIZE;
              bytes_read = 0;
            } else if (strcmp(hex, "gfx__") == 0) {
              Serial.println(F(" found gfx header"));
              store = &sprite_map[0];
              max_bytes = SPRITE_MAP_SIZE;
              bytes_read = 0;
            } else if (strcmp(hex, "gff__") == 0) {
              Serial.println(F(" found gff header"));
              store = &sprite_flags[0];
              max_bytes = 256;
              bytes_read = 0;
            } else if (strcmp(hex, "map__") == 0) {
              Serial.println(F(" found map header"));
              store = tile_map;
              max_bytes = TILEMAP_SIZE;
              bytes_read = 0;
            } // no known section found
          }
        } else if (max_bytes > 0) {
          store[bytes_read++] = hextobyte(hex[0], hex[1]);
          max_bytes--;
        }
        new_line = 0;
      } else if (max_bytes > 0) {
        store[bytes_read++] = hextobyte(hex[0], hex[1]);
        max_bytes--;
      }
    } else { // not enough characters to build byte
      new_line = 1;
    }
  }
  if (bytes_read > 0) {
    Serial.print(F("read "));
    Serial.print(bytes_read);
    Serial.println(F(" byte"));
  }
  f.close();  //Close file
  Serial.println(F("EPO done"));
}
