#include <SI4735.h>
#include <Tiny4kOLED.h>
#include <font8x16atari.h>
#include <Rotary.h>

int SCREEN = 0;
int BTN = -1;
String Btn = "";
int Jump = 0;
int n = 5985;
int dir = 1;

volatile int encoderCount = 0;

enum TT {
  FREQUENCY,      // 0
  VOLUME,         // 1
  LINENOISE,      // 2
  AGC,            // 3
  ATTENUATE,      // 4
  CAPACITOR,      // 5
  AVC,            // 6
  SOFTMUTE,       // 7
  BANDWIDTH,      // 8
  STEPS,          // 9
  JUMP,           // 10
  SEARCH          // 11
};

int _VOLUME[] = { 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 63 };
int _VOLUME_DEFAULT = 3;

int _STEPS[] = { 1, 5, 9 };
int _STEPS_DEFAULT = 1;

int _AGC[] = { 1, 0 }; // 1=AGC-Disabled, 0=AGC-Enabled
int _AGC_DEFAULT = 0;

int _AVC[] = { 12, 48, 90 };
int _AVC_DEFAULT = 2;

int _ATTENUATE[] = { 0, 1, 5, 10, 25, 36 };
int _ATTENUATE_DEFAULT = 0;

int _SOFTMUTE[] = { 0, 1, 5, 8, 15, 20, 25, 32 };
int _SOFTMUTE_DEFAULT = 0;

int _BANDWIDTH[] = { 4, 5, 3, 6, 2, 1, 0 };// 1kHz, 1.8kHz, 2kHz, 2.5kHz,  3kHz, 4kHz, 6kHz,
int _BANDWIDTH_DEFAULT = 6;

int _CAPACITOR[] = { 0, 1 };
int _CAPACITOR_DEFAULT = 0;

int _LINENOISE[] = { 0, 1 };
int _LINENOISE_DEFAULT = 0;



Rotary encoder = Rotary(2, 3);
SI4735 si4735;

void setup() {
  
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
  
  attachInterrupt(digitalPinToInterrupt(2), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), rotaryEncoder, CHANGE);

  display0();

  delay(4000);

  si4735.getDeviceI2CAddress(12); // Looks for the I2C bus address and set it.  Returns 0 if error
  si4735.setup(12, 1); //
  delay(500);
  si4735.setAvcAmMaxGain(_AVC[_AVC_DEFAULT]); // Sets the maximum gain for automatic volume control on AM/SSB mode (between 12 and 90dB)
  delay(500);
  si4735.setTuneFrequencyAntennaCapacitor(_CAPACITOR[_CAPACITOR_DEFAULT]); // Related to VARACTOR. Official recommendation is 0, but PU2CLR has set to 1 for SW and 0 for MW/LW
  delay(500);
  si4735.setAM(100, 30000, n, _STEPS[_STEPS_DEFAULT]);
  delay(500);
  si4735.setAutomaticGainControl(_AGC[_AGC_DEFAULT], _ATTENUATE[_ATTENUATE_DEFAULT]); // This param selects whether the AGC is enabled or disabled (0 = AGC enabled; 1 = AGC disabled) | AGC Index (0 = Minimum attenuation (max gain) 1 – 36 = Intermediate attenuation) if >greater than 36 - Maximum attenuation (min gain) )
  delay(500);
  si4735.setAmSoftMuteMaxAttenuation(_SOFTMUTE[_SOFTMUTE_DEFAULT]); // This function can be useful to disable Soft Mute. The value 0 disable soft mute. Specified in units of dB. Default maximum attenuation is 8 dB. Goes til 32. It works for AM and SSB.
  delay(500);
  si4735.setBandwidth(_BANDWIDTH[_BANDWIDTH_DEFAULT], _LINENOISE[_LINENOISE_DEFAULT]); // BW 0=6kHz,  1=4kHz,  2=3kHz,  3=2kHz,  4=1kHz,  5=1.8kHz,  6=2.5kHz . The default bandwidth is 2 kHz. It works only in AM / SSB (LW/MW/SW) | Enables the AM Power Line Noise Rejection Filter.
  delay(500);
  si4735.setSeekAmLimits(100, 30000);
  delay(500);
  si4735.setSeekAmSpacing(10); // Selects frequency spacingfor AM seek. Default is 10 kHz spacing.
  delay(500);
  si4735.setVolume(_VOLUME[_VOLUME_DEFAULT]);
  delay(100);
  si4735.setFrequencyStep(_STEPS[_STEPS_DEFAULT]);
  delay(100);
  si4735.setFrequency(5985);

  si4735.setFrequency(n);
  display1();
  display3();

}

void loop() {
  /*
  |----|----|----|----|
  | 08 | 06 | 10 | 11 |   02      03
  |----|----|----|----|  <--- 14 --->
  | 09 | 07 | 05 | 04 |
  |----|----|----|----|
  */

  if (digitalRead(8) == LOW && SCREEN == FREQUENCY) { // 8
    SCREEN = FREQUENCY;
    display3();
    delay(200);
  } else if (digitalRead(8) == LOW && SCREEN > FREQUENCY) { // 8
    SCREEN--;
    display3();
    delay(200);
  } else if (digitalRead(6) == LOW && SCREEN < STEPS) { // 6
    SCREEN++;
    display3();
    delay(200);
  } else if (digitalRead(9) == LOW) { // 9
    SCREEN = JUMP;
    setJumper();
    delay(200);
  } else if (digitalRead(7) == LOW) { // 7
    SCREEN = SEARCH;
    display3();
    setSearch(-1);
    delay(200);
    SCREEN = FREQUENCY;
    display3();
  } else if (digitalRead(5) == LOW) { // 5
    SCREEN = SEARCH;
    display3();
    setSearch(1);
    delay(200);
    SCREEN = FREQUENCY;
    display3();
  }

  if (encoderCount != 0) {
    knob(encoderCount);
    encoderCount = 0;
  }

  delay(10);
}

void rotaryEncoder()
{
  uint8_t encoderStatus = encoder.process();
  if (encoderStatus)
  {
    if (encoderStatus == DIR_CW)
    {
      encoderCount = 1;
    }
    else
    {
      encoderCount = -1;
    }
  }
}

void knob(int dir) {
    if (SCREEN == FREQUENCY) {
      knobFrequency(dir);
    } else if (SCREEN == JUMP) {
      knobJumpFrequency(dir);
    } else if (SCREEN == AGC) {
      knob_AGC(dir);
    } else if (SCREEN == AVC) {
      knob_AVC(dir);
    } else if (SCREEN == ATTENUATE) {
      knob_ATTENUATE(dir);
    } else if (SCREEN == SOFTMUTE) {
      knob_SOFTMUTE(dir);
    } else if (SCREEN == BANDWIDTH) {
      knob_BANDWIDTH(dir);
    } else if (SCREEN == CAPACITOR) {
      knob_CAPACITOR(dir);
    } else if (SCREEN == VOLUME) {
      knob_VOLUME(dir);
    } else if (SCREEN == LINENOISE) {
      knob_LINENOISE(dir);
    } else if (SCREEN == STEPS) {
      knob_STEPS(dir);
    }
}

void knobFrequency(int dir) {
  int currStep = _STEPS[_STEPS_DEFAULT] * dir;
  n = n + currStep;
  si4735.setFrequency(n);
  display1();
}

void knobJumpFrequency(int dir) {
  int u = n%10;
  int t = (n/10)%10;
  int h = (n/100)%10;
  int th = n/1000;
  if (Jump == 4) {
    u = helperJump(u, dir, 0, 9);
  } else if (Jump == 3) {
    t = helperJump(t, dir, 0, 9);
  } else if (Jump == 2) {
    h = helperJump(h, dir, 0, 9);
  } else if (Jump == 1) {
    th = helperJump(th, dir, 0, 30);
  }
  String full = String(th) + String(h) + String(t) + String(u);
  n = full.toInt();
  si4735.setFrequency(n);
  display1();
}

int helperJump(int num, int dir, int mn, int mx) {
  int val = num + dir;
  if (val < mn) {
    return mx;
  } else if (val > mx) {
    return mn;
  } else {
    return val;
  }
}

void setJumper() {
  Jump++;
  if (Jump > 4) {
    Jump = 0;
    SCREEN = FREQUENCY;
  }
  displayDashes(Jump);
  
}

void setSearch(int way) {
  if (way > 0) {
    oled.setCursor(115, 3);
    oled.print(">>");
    si4735.frequencyUp();
    si4735.seekStationProgress(showFrequencySeek, checkStopSeeking, 1);
  }
  else if (way < 0) {
    oled.setCursor(115, 3);
    oled.print("<<");
    si4735.frequencyDown();
    si4735.seekStationProgress(showFrequencySeek, checkStopSeeking, 0);
  }
}

void showFrequencySeek(uint16_t freq)
{
  n = freq;
  display1();
}

bool checkStopSeeking()
{
  return (bool) (digitalRead(4) == LOW);
}

void knob_AGC(int dir) {
  int val = _AGC_DEFAULT + dir;
  int size = (sizeof(_AGC) / sizeof(_AGC[0]));
  if (val >= 0 && val < size) {
    _AGC_DEFAULT = val;
  }
  check_AGC_AND_ATTENUATE();
  display5(_AGC[_AGC_DEFAULT]);
}

void knob_ATTENUATE(int dir) {
  int val = _ATTENUATE_DEFAULT + dir;
  int size = (sizeof(_ATTENUATE) / sizeof(_ATTENUATE[0]));
  if (val >= 0 && val < size) {
    _ATTENUATE_DEFAULT = val;
  }
  check_AGC_AND_ATTENUATE();
  display5(_AGC[_AGC_DEFAULT]);
}

void check_AGC_AND_ATTENUATE() {
  if (_AGC[_AGC_DEFAULT] == 0) { // 0=AGC-Enabled, 1=AGC-Disabled
    _ATTENUATE_DEFAULT = 0; // if AGC-Enabled, then ATT should always be disabled (0);
  }
  si4735.setAutomaticGainControl(_AGC[_AGC_DEFAULT], _ATTENUATE[_ATTENUATE_DEFAULT]);
}

void knob_AVC(int dir) {
  int val = _AVC_DEFAULT + dir;
  int size = (sizeof(_AVC) / sizeof(_AVC[0]));
  if (val >= 0 && val < size) {
    _AVC_DEFAULT = val;
  }
  si4735.setAvcAmMaxGain(_AVC[_AVC_DEFAULT]);
  display5(_AVC[_AVC_DEFAULT]);
}

void knob_SOFTMUTE(int dir) {
  int val = _SOFTMUTE_DEFAULT + dir;
  int size = (sizeof(_SOFTMUTE) / sizeof(_SOFTMUTE[0]));
  if (val >= 0 && val < size) {
    _SOFTMUTE_DEFAULT = val;
  }
  si4735.setAmSoftMuteMaxAttenuation(_SOFTMUTE[_SOFTMUTE_DEFAULT]);
  display5(_SOFTMUTE[_SOFTMUTE_DEFAULT]);
}

void knob_BANDWIDTH(int dir) {
  int val = _BANDWIDTH_DEFAULT + dir;
  int size = (sizeof(_BANDWIDTH) / sizeof(_BANDWIDTH[0]));
  if (val >= 0 && val < size) {
    _BANDWIDTH_DEFAULT = val;
  }
  si4735.setBandwidth(_BANDWIDTH[_BANDWIDTH_DEFAULT], _LINENOISE[_LINENOISE_DEFAULT]);
  display5(_BANDWIDTH[_BANDWIDTH_DEFAULT]);
}

void knob_LINENOISE(int dir) {
  int val = _LINENOISE_DEFAULT + dir;
  int size = (sizeof(_LINENOISE) / sizeof(_LINENOISE[0]));
  if (val >= 0 && val < size) {
    _LINENOISE_DEFAULT = val;
  }
  si4735.setBandwidth(_BANDWIDTH[_BANDWIDTH_DEFAULT], _LINENOISE[_LINENOISE_DEFAULT]);
  display5(_LINENOISE[_LINENOISE_DEFAULT]);
}

void knob_CAPACITOR(int dir) {
  int val = _CAPACITOR_DEFAULT + dir;
  int size = (sizeof(_CAPACITOR) / sizeof(_CAPACITOR[0]));
  if (val >= 0 && val < size) {
    _CAPACITOR_DEFAULT = val;
  }
  si4735.setTuneFrequencyAntennaCapacitor(_CAPACITOR[_CAPACITOR_DEFAULT]);
  display5(_CAPACITOR[_CAPACITOR_DEFAULT]);
}

void knob_VOLUME(int dir) {
  int val = _VOLUME_DEFAULT + dir;
  int size = (sizeof(_VOLUME) / sizeof(_VOLUME[0]));
  if (val >= 0 && val < size) {
    _VOLUME_DEFAULT = val;
  }
  si4735.setVolume(_VOLUME[_VOLUME_DEFAULT]);
  display5(_VOLUME[_VOLUME_DEFAULT]);
}

void knob_STEPS(int dir) {
  int val = _STEPS_DEFAULT + dir;
  int size = (sizeof(_STEPS) / sizeof(_STEPS[0]));
  if (val >= 0 && val < size) {
    _STEPS_DEFAULT = val;
  }
  si4735.setFrequencyStep(_STEPS[_STEPS_DEFAULT]);
  display5(_STEPS[_STEPS_DEFAULT]);
}

void display3() {
  if (SCREEN == FREQUENCY) {
    display4("FREQUENCY");
    display5(-1);
  } else if (SCREEN == VOLUME) {
    display4("VOLUME");
    display5(_VOLUME[_VOLUME_DEFAULT]);
  } else if (SCREEN == STEPS) {
    display4("STEPS");
    display5(_STEPS[_STEPS_DEFAULT]);
  } else if (SCREEN == AGC) {
    display4("AGC");
    display5(_AGC[_AGC_DEFAULT]);
  } else if (SCREEN == AVC) {
    display4("AVC");
    display5(_AVC[_AVC_DEFAULT]);
  } else if (SCREEN == ATTENUATE) {
    display4("ATTENUATE");
    display5(_ATTENUATE[_ATTENUATE_DEFAULT]);
  } else if (SCREEN == SOFTMUTE) {
    display4("SOFTMUTE");
    display5(_SOFTMUTE[_SOFTMUTE_DEFAULT]);
  } else if (SCREEN == BANDWIDTH) {
    display4("BANDWIDTH");
    display5(_BANDWIDTH[_BANDWIDTH_DEFAULT]);
  } else if (SCREEN == CAPACITOR) {
    display4("CAPACITOR");
    display5(_CAPACITOR[_CAPACITOR_DEFAULT]);
  } else if (SCREEN == LINENOISE) {
    display4("LINENOISE");
    display5(_LINENOISE[_LINENOISE_DEFAULT]);
  } else if (SCREEN == STEPS) {
    display4("STEPS");
    display5(_STEPS[_STEPS_DEFAULT]);
  } else if (SCREEN == SEARCH) {
    // display4("SEARCH");
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

void display1() {
  oled.setFont(FONT8X16ATARI);
  oled.setCursor(0, 0);
  oled.print("              ");
  oled.setCursor(0, 0);
  oled.print(n);
  oled.setFont(FONT6X8);
}

void display4(String str) {
  oled.setCursor(0, 3);
  oled.print("               ");
  oled.setCursor(0, 3);
  oled.print(str);
}

void display5(int val) {
  oled.setCursor(115, 3);
  oled.print("  ");
  oled.setCursor(115, 3);
  if (val > -1) {
    oled.print(val);
  } else {
    oled.print("");
  }
}

void displayDashes(int amt) {
  oled.setCursor(0, 2);
  oled.print("    ");
  oled.setCursor(0, 2);
  String dash = "";
  for (int i = 1; i <= amt; ++i)
  {
    dash = dash + "-";
  }
  oled.print(dash);
}