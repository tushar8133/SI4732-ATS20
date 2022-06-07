#include <SI4735.h>
SI4735 si4735;

int KEY = 0;    // 1, 2, 3, 4
int SCREEN = 0; // 0, 1, 2
int KNOB = 0;     // -1, 0, 1
int CURRENT_SETTING = 0;
int CURRENT_FREQUENCY = 12725;
int CURRENT_STEP = 5;
bool INIT = true;

void sendCommand(String name, int val) {
  if (name == "SOFTMUTE")    si4735.setAmSoftMuteMaxAttenuation(val); else
  if (name == "AVC")         si4735.setAvcAmMaxGain(val); else
  if (name == "STEPS")       si4735.setFrequencyStep(val); else
  if (name == "CAPACITOR")   si4735.setTuneFrequencyAntennaCapacitor(val); else
  if (name == "VOLUME")      si4735.setVolume(val); else
  if (name == "AGC")         si4735.setAutomaticGainControl(val, getSettingValueByName("ATTENUATE")); else
  if (name == "ATTENUATE")   si4735.setAutomaticGainControl(getSettingValueByName("AGC"), val); else
  if (name == "BANDWIDTH")   si4735.setBandwidth(val, getSettingValueByName("LINENOISE")); else
  if (name == "LINENOISE")   si4735.setBandwidth(getSettingValueByName("BANDWIDTH"), val);
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
  { "BANDWIDTH", 0, { 0, 1, 2, 6, 3, 5, 4 }, sendCommand},
  { "CAPACITOR", 0, { 0, 1 }, sendCommand},
  { "LINENOISE", 0, { 0, 1 }, sendCommand}
};

int getSettingValueByName(String name) {
  int totalSize = sizeof(settings) / sizeof(Settings);
  int pos;
  for (int i = 0; i < totalSize; i++) {
    if(settings[i].name == name) {
      pos = i;
      break;
    }
  }
  int selectedIndex = settings[pos].index;
  int selectedValue = settings[pos].items[selectedIndex];
  return selectedValue;
}

void setScreen(int dir) {
  int val = SCREEN + dir;
  if (val >= 0 && val <= 2) {
    SCREEN = val;
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

void detectKeys() {
  KEY = Serial.parseInt();
}

void detectKnob() {
  if (KEY == 3) KNOB = -1; else
  if (KEY == 4) KNOB = 1;
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

void displayOutput() {
  if (KEY != 0 || KNOB != 0) {
    String name = settings[CURRENT_SETTING].name;
    int index = settings[CURRENT_SETTING].index;
    int val = settings[CURRENT_SETTING].items[index];

    switch (SCREEN) {
      case 0: Serial.println(CURRENT_FREQUENCY); break;
      case 1: Serial.println("Settings >> " + name); break;
      case 2: Serial.println(String(name) + " >> " + val); break;
      default: break;
    }
  } else if (INIT) {
    Serial.println(CURRENT_FREQUENCY);
    INIT = false;
  }
}

void resetAll() {
  KEY = 0;
  KNOB = 0;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Loading...");
  delay(100);
}

void loop() {
  detectKeys();
  detectKnob();
  reactToKeys();
  reactToKnob();
  displayOutput();
  resetAll();
}
