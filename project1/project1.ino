#include <SI4735.h>
#include <EEPROM.h>
#include <Tiny4kOLED.h>
#include <font8x16atari.h>
#include "Rotary.h"

// Devices class declarations
Rotary encoder = Rotary(2, 3);
SI4735 si4735;

typedef struct
{
  const int pin;
  const char * const title;
  const bool complex;
  long tstamp;
} Btns;

Btns btns[] = {
  {8, "MODE_DOWN", true, 0},
  {6, "MODE_UP", true, 0},
  {10, "VOLUME_DOWN", false, 0},
  {11, "VOLUME_UP", false, 0},
  {9, "SEEK_DOWN", false, 0},
  {7, "SEEK_UP", false, 0},
  {5, "STEP_DOWN", false, 0},
  {4, "STEP_UP", false, 0},
};

const int btnTotal = (sizeof btns / sizeof(Btns));
short currentButton = -1;

typedef struct
{
  short idx;
  const char * const type;
  short temp;
} Mode;

Mode mode[] = {
  {1, "FREQUENCY", 0},
  {2, "BAND", 0},
  {3, "GAIN", 0},
  {4, "BANDWIDTH", 0},
  {5, "BATTERY", 0}
};

short currentMode = 0;
volatile int encoderCount = 0;

void setup() {

  Serial.begin(9600); // si4732 (comment kept here to quicky search and replace all the comments using this term, for the use of wokwi)

  for (int i = 0; i < btnTotal; i++) {
    pinMode(btns[i].pin, INPUT_PULLUP);
  }
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), rotaryEncoder, CHANGE);

  oled.begin();
  oled.clear();
  oled.on();
  display("READY", 0);

  si4735.getDeviceI2CAddress(12); // Looks for the I2C bus address and set it.  Returns 0 if error
  si4735.setup(12, 1); //
  si4735.setAvcAmMaxGain(48); // Sets the maximum gain for automatic volume control on AM/SSB mode (between 12 and 90dB)
  si4735.setTuneFrequencyAntennaCapacitor(1); // Related to VARACTOR. Official recommendation is 0, but PU2CLR has set to 1 for SW and 0 for MW/LW
  si4735.setAM(100, 30000, 7300, 5);
  si4735.setAutomaticGainControl(0, 0); // This param selects whether the AGC is enabled or disabled (0 = AGC enabled; 1 = AGC disabled) | AGC Index (0 = Minimum attenuation (max gain) 1 – 36 = Intermediate attenuation) if >greater than 36 - Maximum attenuation (min gain) )
  si4735.setAmSoftMuteMaxAttenuation(32); // This function can be useful to disable Soft Mute. The value 0 disable soft mute. Specified in units of dB. Default maximum attenuation is 8 dB. Goes til 32. It works for AM and SSB.
  si4735.setBandwidth(3, 1); // BW 0=6kHz,  1=4kHz,  2=3kHz,  3=2kHz,  4=1kHz,  5=1.8kHz,  6=2.5kHz . The default bandwidth is 2 kHz. It works only in AM / SSB (LW/MW/SW) | Enables the AM Power Line Noise Rejection Filter.
  si4735.setSeekAmLimits(100, 30000);
  si4735.setSeekAmSpacing(10); // Selects frequency spacingfor AM seek. Default is 10 kHz spacing.
  si4735.setVolume(20);
}


void loop() {
  long currentTime = millis();
  for (int i = 0; i < btnTotal; i++) {
    if (digitalRead(btns[i].pin) == LOW) {
      currentButton = i;
      btns[i].tstamp = currentTime;
      btnHandler(i);
      delay(50);
    }
  }
  if (encoderCount != 0) {
    modeHandler(encoderCount);
    btns[0].tstamp = millis();
    encoderCount = 0;
  }
  // if (currentButton > -1 && !(btns[currentButton].complex && btns[currentButton].tstamp + 5000 > currentTime)) {
  //   resetAll();
  // }
}


void btnHandler(short dir) {
  switch (dir) {
    case 0 : changeMode(-1); break;
    case 1 : changeMode(1); break;
    case 2 : volume(-1); break;
    case 3 : volume(1); break;
    case 4 : seek(-1); break;
    case 5 : seek(1); break;
    case 6 : step(-1); break;
    case 7 : step(1); break;
  }
}

void resetAll() {
  if (currentButton == -1) return;
  for (int i = 0; i < btnTotal; i++) {
    btns[i].tstamp = 0;
  }
  currentButton = -1;
  currentMode = 0;
  oled.clear();
}

void changeMode(short dir) {
  int totalModes = (sizeof mode / sizeof(Mode)) - 1;
  currentMode = currentMode + dir;
  if (dir < 0 && currentMode < 0) {
    currentMode = totalModes;
  }
  else if (dir > 0 && currentMode > totalModes) {
    currentMode = 0;
  }
  display(mode[currentMode].type, 0);
}

void rotaryEncoder()
{ // rotary encoder events
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

void modeHandler(short dir) {
  switch (mode[currentMode].idx) {
    case 1 : spinFrequency(dir); break;
    case 2 : spinBand(dir); break;
    case 3 : spinGain(dir); break;
    case 4 : spinBandwidth(dir); break;
    case 5 : spinBattery(dir); break;
  }
}

void spinFrequency(short dir) {
  if (dir == 1) {
    si4735.frequencyUp();
  } else {
    si4735.frequencyDown();
  }
  display("frequency", si4735.getFrequency());
}

void spinBand(short dir) {
  display("Band", dir);
}

void spinGain(short dir) {
  display("Gain", dir);
}

void spinBandwidth(short dir) {
  display("Bandwidth", dir);
}

void spinBattery(short dir) {
  display("Battery", 100);
}

void volume(short dir) {
  if (dir > 0) {
    si4735.volumeUp();
  }
  else if (dir < 0) {
    si4735.volumeDown();
  }
  display("Volume", si4735.getVolume());
}

void step(short dir) {
  if (dir > 0) {
    si4735.setFrequencyStep(100);
    si4735.setSeekAmSpacing(100);
  }
  else if (dir < 0) {
    si4735.setFrequencyStep(5);
    si4735.setSeekAmSpacing(5);
  }
  display("Step", dir);
}

void seek(short dir) {
  if (dir > 0) {
    si4735.frequencyUp();
    si4735.seekStationProgress(showFrequencySeek, 1);
  }
  else if (dir < 0) {
    si4735.frequencyDown();
    si4735.seekStationProgress(showFrequencySeek, 0);
  }
  display("Seek", dir);
}

void showFrequencySeek(uint16_t freq)
{
  oled.setFont(FONT8X16ATARI);
  oled.setCursor(0, 2);
  oled.print("                       ");
  oled.setCursor(0, 2);
  oled.print(freq);
}

void display(char* str, int val) {
  oled.setFont(FONT6X8);
  oled.setCursor(0, 0);
  oled.print("                       ");
  oled.setCursor(0, 0);
  oled.print(str);

  oled.setFont(FONT8X16ATARI);
  oled.setCursor(0, 2);
  oled.print("                       ");
  oled.setCursor(0, 2);
  oled.print(val);
}
