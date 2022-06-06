int KEY = 0;    // 1, 2, 3, 4
int SCREEN = 0; // 0, 1, 2
int KNOB = 0;     // -1, 0, 1
int CURRENT_SETTING = 0;
int CURRENT_FREQUENCY = 12725;
int CURRENT_STEP = 5;

void sendCommand(String name, int val) {
  Serial.println("NOW+! THIS IS WHAT I CALL MUSIC");
  Serial.println(name + "~" + String(val));

  // switch (name) {
  //   case: "AVC": si4735.setAvcAmMaxGain(val); break;
  //   case: "SOFTMUTE": si4735.setAmSoftMuteMaxAttenuation(val); break;
  //   case: "CAPACITOR": si4735.setTuneFrequencyAntennaCapacitor(val); break;
  //   case: "VOLUME": si4735.setVolume(val); break;
  //   case: "STEPS": si4735.setFrequencyStep(val); break;
  //   default: break;
  // }

  // settings[CURRENT_SETTING].items[index];
  // case: "AGC": si4735.setAutomaticGainControl(val, _ATTENUATE[_ATTENUATE_DEFAULT]);
  // case: "BANDWIDTH": si4735.setBandwidth(val, _LINENOISE[_LINENOISE_DEFAULT]);
}

typedef struct {
  String name;
  int index;
  int items[20];
  void (*func)(String, int);
} Settings;

Settings settings[] = {
  { "VOLUME", 3, { 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 63 }, sendCommand},
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
  int totalSize = sizeof (settings[CURRENT_SETTING].items) / sizeof (settings[CURRENT_SETTING].items[0]);
  int blankSize = 0;
  for (int i = (totalSize-1); i >= 0; i--) {
    if(settings[CURRENT_SETTING].items[i] == 0) {
      blankSize++;
    } else {
      break;
    }
  }
  int actualSize = (totalSize - blankSize);

  int oldIndex = settings[CURRENT_SETTING].index;
  int newIndex = oldIndex + KNOB;

  if (newIndex >= 0 && newIndex < actualSize ) {
    settings[CURRENT_SETTING].index = newIndex;
    String name = settings[CURRENT_SETTING].name;
    int val = settings[CURRENT_SETTING].items[newIndex];
    settings[CURRENT_SETTING].func(name, val);
  }

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

