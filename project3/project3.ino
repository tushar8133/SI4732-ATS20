int KEY = 0;    // 1, 2, 3, 4
int SCREEN = 0; // 0, 1, 2
int KNOB = 0;     // -1, 0, 1
int CURRENT_SETTING = 0;
int CURRENT_FREQUENCY = 12725;
int CURRENT_STEP = 5;

void sendCommand(String name, int param) {
  Serial.println("COMMAND - NOW THIS IS WHAT I CALL MUSIC");
  // int val = _BANDWIDTH_DEFAULT + dir;
  // int size = (sizeof(_BANDWIDTH) / sizeof(_BANDWIDTH[0]));
  // if (val >= 0 && val < size) {
  //   _BANDWIDTH_DEFAULT = val;
  // }
  // si4735.setBandwidth(_BANDWIDTH[_BANDWIDTH_DEFAULT], _LINENOISE[_LINENOISE_DEFAULT]);
  // display5(_BANDWIDTH[_BANDWIDTH_DEFAULT]);
}

typedef struct {
  String name;
  int index;
  int items[20];
  void (*func)(String, int);
} Settings;

Settings settings[] = {
  { "VOLUME", 3, { 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 63 }, sendCommand},
  { "STEPS", 1, { 1, 5, 9 }, sendCommand},
  { "AGC", 0, { 1, 0 }, sendCommand},
  { "AVC", 2, { 12, 48, 90 }, sendCommand},
  { "ATTENUATE", 0, { 0, 1, 5, 10, 25, 36 }, sendCommand},
  { "SOFTMUTE", 0, { 0, 1, 5, 8, 15, 20, 25, 32 }, sendCommand},
  { "BANDWIDTH", 6, { 4, 5, 3, 6, 2, 1, 0 }, sendCommand},
  { "CAPACITOR", 0, { 0, 1 }, sendCommand},
  { "LINENOISE", 0, { 0, 1 }, sendCommand}
};

void setScreen(int dir) {
  int val = SCREEN + dir;
  if (val >= 0 && val <= 2) {
    SCREEN = val;
  }
}

void detectKeys() {
  KEY = Serial.parseInt();
}

void detectKnob() {
  if (KEY == 3) {
    KNOB = -1;
  } else if (KEY == 4) {
    KNOB = 1;
  }
}

void reactToKeys() {
  switch (KEY) {
    case 1: setScreen(-1); break;
    case 2: setScreen(1); break;
    default: break;
  }
  delay(200);
}

void reactToKnob() {
  if (KNOB != 0) {
    switch (SCREEN) {
      case 0: updateFrequency(); break;
      case 1: spinSetting(); break;
      case 2: adjustSetting(); break;
      default: break;
    }
  }
}

void updateFrequency() {
  CURRENT_FREQUENCY = CURRENT_FREQUENCY + CURRENT_STEP * KNOB;
}

void spinSetting() {
  int val = CURRENT_SETTING + KNOB;
  int len = (sizeof(settings) / sizeof(Settings)) - 1;
  if (val >= 0 && val <= len) {
    CURRENT_SETTING = val;
  }
}

void adjustSetting() {
  int oldValue = settings[CURRENT_SETTING].index;
  int newValue = oldValue + KNOB;
  String name = settings[CURRENT_SETTING].name;
  settings[CURRENT_SETTING].index = newValue;
  settings[CURRENT_SETTING].func(name, newValue);
}

void displayOutput() {
  if (KEY != 0 || KNOB != 0) {
    String name = settings[CURRENT_SETTING].name;
    int index = settings[CURRENT_SETTING].index;
    int val = settings[CURRENT_SETTING].items[index];

    if (SCREEN == 0) {
      Serial.println(CURRENT_FREQUENCY);
    }
    
    if (SCREEN == 1) {
      Serial.println(name +"~"+val);
    }
    
    if (SCREEN == 2) {
      Serial.println(val);
    }
  }
}

void resetAll() {
  KEY = 0;
  KNOB = 0;
}

void setup() {
  Serial.begin(9600);
  Serial.println("START");
}

void loop() {
  detectKeys();
  detectKnob();
  reactToKeys();
  reactToKnob();
  displayOutput();
  resetAll();
}

