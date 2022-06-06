int KEY = 0;    // 1, 2, 3, 4
int SCREEN = 0; // 0, 1, 2
int KNOB = 0;     // -1, 0, 1
int CURRENT_SETTING = 0;

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

void updateScreen(int dir) {
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
  } else {
    KNOB = 0;
  }
}

void reactToKeys() {
  if (KEY == 1) {
    updateScreen(-1);
    display();
  } else if (KEY == 2) {
    updateScreen(1);
    display();
  }
  delay(200);
}

void reactToKnob() {
  if (KNOB != 0) {
    if (SCREEN == 1) {
      int val = CURRENT_SETTING + KNOB;
      if (val >= 0 && val <= 2) {
        CURRENT_SETTING = val;
      }
    }
    if (SCREEN == 2) {
      settings[CURRENT_SETTING].index = settings[CURRENT_SETTING].index + KNOB;
    }
    display();
  }
}

void reactToVolume() {
  if (SCREEN == 1 && KNOB != 0) {
    settings[CURRENT_SETTING].index = settings[CURRENT_SETTING].index + KNOB;
  }
}

void display() {
  String name = settings[CURRENT_SETTING].name;
  int index = settings[CURRENT_SETTING].index;
  int val = settings[CURRENT_SETTING].items[index];
  Serial.println( "Sceen"+String(SCREEN) +" ~ "+ name + " ~ " + String(val) );
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
}

