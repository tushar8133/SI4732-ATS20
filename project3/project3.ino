int KEY = 0;    // 1, 2, 3, 4
int SCREEN = 0; // 0, 1, 2
int KNOB = 0;     // -1, 0, 1
int CURRENT_SETTING = 0;
int CURRENT_FREQUENCY = 12725;
int CURRENT_STEP = 5;

typedef struct {
  String name;
  int index;
  int items[20];
} Settings;

Settings settings[] = {
  {"VOLUME", 0, {11, 22, 33, 44} },
  {"STEPS", 0, {11, 22, 33, 44} },
  {"BANDWIDTH", 0, {11, 22, 33, 44} }
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
      case 1: chooseSetting(); break;
      case 2: updateSetting(); break;
      default: break;
    }
  }
}

void updateFrequency() {
  CURRENT_FREQUENCY = CURRENT_FREQUENCY + CURRENT_STEP * KNOB;
}

void chooseSetting() {
  int val = CURRENT_SETTING + KNOB;
  if (val >= 0 && val <= 2) {
    CURRENT_SETTING = val;
  }
}

void updateSetting() {
  settings[CURRENT_SETTING].index = settings[CURRENT_SETTING].index + KNOB;
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
