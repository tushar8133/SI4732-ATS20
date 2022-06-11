/*
TODO:
show SNR
show BATT
Add reset into settings
Auto apply optimal settings as per selected band
AGC/ATT auto adjust rectify
Save station to EEPROM
Volume BAR
Skip unused band while manual tuning
auto fine adjust after seek
again setFrequency after seek
How to copy array to another array?
-Fix digit selection highlight
-Seek
-helper method to set settings value
-SIG and SNR addition in settings
*/

#include <SI4735.h>
#include <Tiny4kOLED.h>
#include <font8x16atari.h>
#include <Rotary.h>

volatile int KNOB = 0;     // -1, 0, 1
int KEY = 0;    // 1, 2, 3, 4
int SCREEN = 0; // 0, 1, 2
int CURRENT_SETTING = 0;
int FREQUENCY[5] = {0, 17, 8, 0, 0};
bool INIT = true;
long timer = millis();

SI4735 si4735;
Rotary encoder = Rotary(2, 3);

void sendCommand(String name, int val) {
  if (name == "SOFTMUTE")    si4735.setAmSoftMuteMaxAttenuation(val); else
  if (name == "AVC")         si4735.setAvcAmMaxGain(val); else
  if (name == "STEPS")       si4735.setFrequencyStep(val); else
  if (name == "CAPACITOR")   si4735.setTuneFrequencyAntennaCapacitor(val); else
  if (name == "VOLUME")      si4735.setVolume(val); else
  if (name == "AGC")         si4735.setAutomaticGainControl(val, getSettingValueByName("ATTENUATE")); else
  if (name == "ATTENUATE")   si4735.setAutomaticGainControl(getSettingValueByName("AGC"), val); else
  if (name == "BANDWIDTH")   si4735.setBandwidth(val, getSettingValueByName("LINENOISE")); else
  if (name == "LINENOISE")   si4735.setBandwidth(getSettingValueByName("BANDWIDTH"), val); else
  if (name == "SIGNAL-TH")   si4735.setSeekAmRssiThreshold(val); else
  if (name == "SNR-TH")      si4735.setSeekAmSNRThreshold(val); else
  if (name == "SEEK-STEP")   si4735.setSeekAmSpacing(val);
}

typedef struct {
  String name;
  int index;
  int items[20];
  void (*func)(String, int);
} Settings;

Settings settings[] = {
  { "VOLUME", 10, { 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 63 }, sendCommand},
  { "STEPS", 1, { 1, 5, 9, 10 }, sendCommand},
  { "AVC", 8, { 12, 15, 18, 20, 25, 30, 35, 40, 50, 70, 90 }, sendCommand},
  { "SOFTMUTE", 3, { 0, 1, 5, 8, 15, 20, 25, 32 }, sendCommand},
  { "AGC", 0, { 0, 1 }, sendCommand},
  { "ATTENUATE", 0, { 0, 1, 5, 10, 25, 36 }, sendCommand},
  { "BANDWIDTH", 2, { 0, 1, 2, 6, 3, 5, 4 }, sendCommand},
  { "CAPACITOR", 0, { 0, 1 }, sendCommand},
  { "LINENOISE", 1, { 0, 1 }, sendCommand},
  { "SIGNAL-TH", 12, { 0, 1, 2, 5, 8, 10, 12, 15, 18, 20, 30, 40, 50, 60, 70 }, sendCommand},
  { "SNR-TH", 5, { 0, 1, 2, 3, 4, 5, 8, 10, 12, 15, 18, 20, 23, 25 }, sendCommand},
  { "SEEK-STEP", 0, { 1, 5, 10 }, sendCommand}
};

void setSettingIndexByName(String name, int val) {
  int totalSize = sizeof(settings) / sizeof(Settings);
  int pos = 0;
  for (int i = 0; i < totalSize; i++) {
    if(settings[i].name == name) {
      pos = i;
      break;
    }
  }
  settings[pos].index = val;
}

int getSettingValueByName(String name) {
  int totalSize = sizeof(settings) / sizeof(Settings);
  int pos = 0;
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
  } else if (val < 0) {
    si4735.setFrequencyStep(settings[1].items[settings[1].index]);
    CURRENT_SETTING = 0;
  }
}

void updateFrequency() {
  if (FREQUENCY[0] != 0) {
    int pos = FREQUENCY[0];
    int oldDigit = FREQUENCY[pos];
    int newDigit = 0;
    if(pos == 1) {
      newDigit = digitHelper(oldDigit, 0, 30);
    } else {
      newDigit = digitHelper(oldDigit, 0, 9);
    }
    FREQUENCY[pos] = newDigit;
    si4735.setFrequency(convertDigitsToFreq());
  } else {
    if (KNOB == 1) {
      si4735.frequencyUp();
    } else if (KNOB == -1) {
      si4735.frequencyDown();
    }
    convertFreqToDigits(si4735.getFrequency());
  }
}

int digitHelper(int num, int mn, int mx) {
  int val = num + KNOB;
  if (val < mn) {
    return mx;
  } else if (val > mx) {
    return mn;
  } else {
    return val;
  }
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

void updateDisplay() {
  if (KEY != 0 || KNOB != 0) {
    String name = settings[CURRENT_SETTING].name;
    int index = settings[CURRENT_SETTING].index;
    int val = settings[CURRENT_SETTING].items[index];

    if (SCREEN == 0) {
      display1(true);
      display2("");
    } else if (SCREEN == 1) {
      display2("Settings >> " + name);
      display1(false);
    } else if (SCREEN == 2) {
      display2(String(name) + " >> " + val);
    }
  } else if (INIT) {
    display1(true);
    INIT = false;
  }

  if (SCREEN == 0 && timeLimit()) {
    si4735.getCurrentReceivedSignalQuality(); // use it before calling getCurrentRSSI() and getCurrentSNR()
    // si4735.getStatus();

    // int b = si4735.getStatusSNR(); // works with seek. remain constant. may be reflecting values of getCurrentSNR()
    // int c = si4735.getReceivedSignalStrengthIndicator(); // works with seek. remain constant. may be reflecting values of getCurrentRSSI()
    // int d = si4735.getStatusValid(); // works with seek. remain constant. returns bool if found
    // int e = si4735.getCurrentRSSI(); // works fine. To be used with getCurrentReceivedSignalQuality() ??
    // int f = si4735.getCurrentSNR(); // works fine. To be used with getCurrentReceivedSignalQuality() ??
    // int g = si4735.getCurrentRssiDetectLow(); // constant 0. Not sure if any method be called prior to it.
    // int h = si4735.getCurrentRssiDetectHigh(); // constant 0. Not sure if any method be called prior to it.
    // int i = si4735.getCurrentSnrDetectLow(); // constant 0. Not sure if any method be called prior to it.
    // int j = si4735.getCurrentSnrDetectHigh(); // constant 0. Not sure if any method be called prior to it.

    int sig = si4735.getCurrentRSSI();
    int snr = si4735.getCurrentSNR();

    String sigBars = getBars(sig, 80, 13);
    String snrBars = getBars(snr, 26, 13);

    String sigPad = getPadding(sig, 2);
    String snrPad = getPadding(snr, 2);

    display4("SIG "+sigPad+" "+sigBars);
    display2("SNR "+snrPad+" "+snrBars);
  }

}

String getBars(int val, int maxLimit, int barLimit) {
  int percent = maxLimit / barLimit;
  int bars = val/percent;
  String fnl = "";
  for(int i = 0; i < bars; ++i) {
    fnl = fnl + "=";
  }
  fnl = fnl + ">";
  return fnl;
}

String getPadding(int val, int padLength) {
  String str = String(val);
  int len = str.length();
  int zeroLength = padLength - len;
  String fnl = "";
  for (int i = 0; i < zeroLength; ++i) {
    fnl = fnl + "0";
  }
  fnl += str;
  return fnl;
}

bool timeLimit() {
  long currTime = millis();
  if( currTime > timer) {
    timer = millis() + 300;
    return true;
  } else {
    return false;
  }
}

void display0() {
  oled.begin();
  oled.clear();
  oled.on();
  oled.setFont(FONT8X16ATARI);
  oled.setCursor(0, 0);
  oled.print("LOADING...");
}

void display1(bool show) {
  int margin = 45;
  int inc = 8;
  int cursorPos = FREQUENCY[0];
  oled.setFont(FONT8X16ATARI);
  oled.setCursor(0, 0);
  oled.print("                ");
  if (show) {
    for (int i = 1; i <= 4; ++i)
    {
      if (cursorPos == i) oled.invertOutput(true);
      int cf = FREQUENCY[i];
      if (cf > 9) {
        oled.setCursor( margin, 0);
      } else {
        oled.setCursor( margin + (inc * i), 0);
      }
      oled.print(cf);
      if (cursorPos == i) oled.invertOutput(false);
    }
  }
}

void display2(String str) {
  oled.setFont(FONT6X8);
  oled.setCursor(0, 3);
  oled.print("                     ");
  oled.setCursor(0, 3);
  oled.print(str);
}

void display4(String str) {
  oled.setFont(FONT6X8);
  oled.setCursor(0, 2);
  oled.print("                     ");
  oled.setCursor(0, 2);
  oled.print(str);
}

void resetAll() {
  KEY = 0;
  KNOB = 0;
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

void reactToKeys() {
  /*
  |----|----|----|----|
  | 08 | 06 | 10 | 11 |   02      03
  |----|----|----|----|  <--- 14 --->
  | 09 | 07 | 05 | 04 |
  |----|----|----|----|
  */
  switch (KEY) {
    case 8: setScreen(-1); break;
    case 6: setScreen(1); break;
    case 9: frequencyJumper(-1); break;
    case 7: frequencyJumper(1); break;
    case 5: seekStation(-1); break;
    case 4: seekStation(1); break;
    default: break;
  }
  if(KEY != 0) {
    delay(200);
  }
}

void seekStation(int dir) {
  si4735.setFrequencyStep(1);
  // si4735.setSeekAmSpacing(1); // Default is 10 kHz spacing.
  // si4735.setSeekAmRssiThreshold(50); // Default is 25
  // si4735.setSeekAmSNRThreshold(1); // Default is 5
  si4735.setMaxSeekTime(600000); // Default is 8

  if (dir == 1) {
    display4("---------------------");
    display2("   >> SEARCHING >>   ");
    si4735.seekStationProgress(seekDisplay, seekStop, 1);
  } else {
    display4("---------------------");
    display2("   << SEARCHING <<   ");
    si4735.seekStationProgress(seekDisplay, seekStop, 0);
  }
}

void seekDisplay(int freq) {
  convertFreqToDigits(freq);
  display1(true);
}

bool seekStop()
{
  return (bool)(digitalRead(8) == LOW);
}

void detectKeys() {
  if (digitalRead(4) == LOW) KEY = 4; else
  if (digitalRead(5) == LOW) KEY = 5; else
  if (digitalRead(6) == LOW) KEY = 6; else
  if (digitalRead(7) == LOW) KEY = 7; else
  if (digitalRead(8) == LOW) KEY = 8; else
  if (digitalRead(9) == LOW) KEY = 9; else
  if (digitalRead(10) == LOW) KEY = 10; else
  if (digitalRead(11) == LOW) KEY = 11; else
  if (digitalRead(14) == LOW) KEY = 14;
}

void detectKnob() {
  uint8_t encoderStatus = encoder.process();
  if (encoderStatus) {
    KNOB = (encoderStatus == DIR_CW) ? 1 : -1;
  }
}

void addKeysListener() {
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), detectKnob, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), detectKnob, CHANGE);
}

void frequencyJumper(int dir) {
  int oldPos = FREQUENCY[0];
  int newPos = oldPos + dir;
  if (dir == 1 && newPos > 4) {
    newPos = 0;
  } else if (dir == -1 && newPos < 0) {
    newPos = 4;
  }
  FREQUENCY[0] = newPos;
}

void convertFreqToDigits(int freq) {
  FREQUENCY[4] = freq%10;
  FREQUENCY[3] = (freq/10)%10;
  FREQUENCY[2] = (freq/100)%10;
  FREQUENCY[1] = freq/1000;
}

int convertDigitsToFreq() {
  String str = String(FREQUENCY[1]) + String(FREQUENCY[2]) + String(FREQUENCY[3]) + String(FREQUENCY[4]);
  int freq = str.toInt();
  return freq;
}

void setup() {
  addKeysListener();
  display0();
  si4735.getDeviceI2CAddress(12); // Looks for the I2C bus address and set it.  Returns 0 if error
  si4735.setup(12, 1); //
  si4735.setAM(100, 30000, convertDigitsToFreq(), getSettingValueByName("STEPS"));
  si4735.setBandwidth(getSettingValueByName("BANDWIDTH"), getSettingValueByName("LINENOISE")); // BW 0=6kHz,  1=4kHz,  2=3kHz,  3=2kHz,  4=1kHz,  5=1.8kHz,  6=2.5kHz . The default bandwidth is 2 kHz. It works only in AM / SSB (LW/MW/SW) | Enables the AM Power Line Noise Rejection Filter.
  si4735.setSeekAmLimits(100, 30000);
  si4735.setAutomaticGainControl(getSettingValueByName("AGC"), getSettingValueByName("ATTENUATE")); // This param selects whether the AGC is enabled or disabled (0 = AGC enabled; 1 = AGC disabled) | AGC Index (0 = Minimum attenuation (max gain) 1 – 36 = Intermediate attenuation) if >greater than 36 - Maximum attenuation (min gain) )
  si4735.setTuneFrequencyAntennaCapacitor(1); // Related to VARACTOR. Official recommendation is 0, but PU2CLR has set to 1 for SW/MW and 0 for LW
  si4735.setTuneFrequencyAntennaCapacitor(getSettingValueByName("CAPACITOR")); // Related to VARACTOR. Official recommendation is 0, but PU2CLR has set to 1 for SW/MW and 0 for LW
  si4735.setAvcAmMaxGain(getSettingValueByName("AVC")); // Sets the maximum gain for automatic volume control on AM/SSB mode (between 12 and 90dB)
  si4735.setAmSoftMuteMaxAttenuation(getSettingValueByName("SOFTMUTE")); // This function can be useful to disable Soft Mute. The value 0 disable soft mute. Specified in units of dB. Default maximum attenuation is 8 dB. Goes til 32. It works for AM and SSB.
  si4735.setSeekAmRssiThreshold(getSettingValueByName("SIGNAL-TH"));
  si4735.setSeekAmSNRThreshold(getSettingValueByName("SNR-TH"));
  si4735.setSeekAmSpacing(getSettingValueByName("SEEK-STEP"));
  si4735.setVolume(getSettingValueByName("VOLUME"));
}

void loop() {
  detectKeys();
  reactToKeys();
  reactToKnob();
  updateDisplay();
  resetAll();
}
