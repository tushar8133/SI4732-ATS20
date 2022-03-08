/*
  | Device name               | Device Pin / Description      |  Arduino Pin  |
  | ----------------          | ----------------------------- | ------------  |
  | Display OLED              |                               |               |
  |                           | SDA                           |     A4        |
  |                           | CLK                           |     A5        |
  |     (*1) SI4735           |                               |               |
  |                           | RESET (pin 15)                |     12        |
  |                           | SDIO (pin 18)                 |     A4        |
  |                           | SCLK (pin 17)                 |     A5        |
  |                           | SEN (pin 16)                  |    GND        |
  |     (*2) Buttons          |                               |               |
  |                           | Switch MODE (AM/LSB/AM)       |      4        |
  |                           | Banddwith                     |      5        |
  |                           | Volume                        |      6        |
  |                           | Custom button 1 (*3)          |      7        |
  |                           | Band                          |      8        |
  |                           | Custom button 2 (*3)          |      9        |
  |                           | Step                          |     10        |
  |                           | AGC / Attentuation            |     11        |
  |                           | VFO/VFO Switch (Encoder)      |     14 / A0   |
  |    Encoder (*4)           |                               |               |
  |                           | A                             |       2       |
  |                           | B                             |       3       |

   1 - You can use the SI4732-A10. Check on the SI4732 package the pins: RESET, SDIO, SCLK and SEN.

*/

#include <SI4735.h>
#include <EEPROM.h>
#include <Tiny4kOLED.h>
#include <font8x16atari.h> // Please, install the TinyOLED-Fonts library
#include <Rotary.h>

#include "patch_ssb_compressed.h" // Compressed SSB patch version (saving almost 1KB)

const uint16_t size_content = sizeof ssb_patch_content; // See ssb_patch_content.h
const uint16_t cmd_0x15_size = sizeof cmd_0x15;         // Array of lines where the 0x15 command occurs in the patch content.

#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3

// OLED Diaplay constants
#define RST_PIN -1 // Define proper RST_PIN if required.

#define RESET_PIN 12

// Enconder PINs - if the clockwise and counterclockwise directions are not correct for you, please, invert this settings.
#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3

// Buttons controllers
#define STEP_INCR 4      // Switch MODE (Am/LSB/USB)
#define STEP_DECR 5 // Used to select the banddwith.
#define VOLUME_BUTTON 6    // Volume Up
#define SEEK_NEXT 7     // **** Use thi button to implement a new function
#define BAND_BUTTON 8      // Next band
#define SEEK_PREV 9     // **** Use thi button to implement a new function
#define AGC_BUTTON 11      // Switch AGC ON/OF
#define STEP_BUTTON 10     // Used to select the increment or decrement frequency step (see tabStep)
#define ENCODER_BUTTON 14  // Used to select the enconder control (BFO or VFO) and SEEK function on AM and FM modes

#define MIN_ELAPSED_TIME 100
#define MIN_ELAPSED_RSSI_TIME 150

#define DEFAULT_VOLUME 20 // change it for your favorite sound volume

#define FM 0
#define LSB 1
#define USB 2
#define AM 3
#define LW 4

#define SSB 1

#define STORE_TIME 10000 // Time of inactivity to make the current receiver status writable (10s / 10000 milliseconds).

const uint8_t app_id = 43; // Useful to check the EEPROM content before processing useful data
const int eeprom_address = 0;
long storeTime = millis();

const char *bandModeDesc[] = {"FM ", "LSB", "USB", "AM "};
uint8_t currentMode = FM;
uint8_t seekDirection = 1;

bool bfoOn = false;

bool ssbLoaded = false;
bool fmStereo = true;

bool cmdVolume = false;   // if true, the encoder will control the volume.
bool cmdAgcAtt = false;   // if true, the encoder will control the AGC / Attenuation
bool cmdStep = false;     // if true, the encoder will control the step frequency
bool cmdBw = false;       // if true, the encoder will control the bandwidth
bool cmdBand = false;     // if true, the encoder will control the band
bool cmdSoftMute = false; // if true, the encoder will control the Soft Mute attenuation
bool cmdAvc = false;      // if true, the encoder will control Automatic Volume Control

long countRSSI = 0;

int currentBFO = 0;

long elapsedRSSI = millis();
long elapsedButton = millis();
long elapsedBatt = millis();

// Encoder control variables
volatile int encoderCount = 0;

// Some variables to check the SI4735 status
uint16_t currentFrequency;
uint16_t previousFrequency;
// uint8_t currentStep = 1;
uint8_t currentBFOStep = 25;

// Datatype to deal with bandwidth on AM, SSB and FM in numerical order.
// Ordering by bandwidth values.
typedef struct
{
  uint8_t idx;      // SI473X device bandwidth index value
  const char *desc; // bandwidth description
} Bandwidth;

int8_t bwIdxSSB = 4;
Bandwidth bandwidthSSB[] = {
  {4, "0.5"}, // 0
  {5, "1.0"}, // 1
  {0, "1.2"}, // 2
  {1, "2.2"}, // 3
  {2, "3.0"}, // 4  - default
  {3, "4.0"}  // 5
};              // 3 = 4kHz

int8_t bwIdxAM = 4;
const int maxFilterAM = 6;
Bandwidth bandwidthAM[] = {
  {4, "1.0"}, // 0
  {5, "1.8"}, // 1
  {3, "2.0"}, // 2
  {6, "2.5"}, // 3
  {2, "3.0"}, // 4 - default
  {1, "4.0"}, // 5
  {0, "6.0"}  // 6
};

int8_t bwIdxFM = 0;
Bandwidth bandwidthFM[] = {
  {0, "AUT"}, // Automatic - default
  {1, "110"}, // Force wide (110 kHz) channel filter.
  {2, " 84"},
  {3, " 60"},
  {4, " 40"}
};

// Atenua and AGC
int8_t agcIdx = 0;
uint8_t disableAgc = 1;
uint8_t agcNdx = 0;
int8_t smIdx = 32;
int8_t avcIdx = 90;

int tabStep[] = {1,    // 0
                 5,    // 1
                 9,    // 2
                 10,   // 3
                 100,
                 1000
                }; // 5

const int lastStep = (sizeof tabStep / sizeof(int)) - 1;
int idxStep = 3;

/*
   Band data structure
*/
typedef struct
{
  uint8_t bandType;        // Band type (FM, MW or SW)
  uint16_t minimumFreq;    // Minimum frequency of the band
  uint16_t maximumFreq;    // Maximum frequency of the band
  uint16_t currentFreq;    // Default frequency or current frequency
  uint16_t currentStepIdx; // Idex of tabStep:  Defeult frequency step (See tabStep)
  int8_t bandwidthIdx;    // Index of the table bandwidthFM, bandwidthAM or bandwidthSSB;
} Band;

/*
   Band table
   YOU CAN CONFIGURE YOUR OWN BAND PLAN. Be guided by the comments.
   To add a new band, all you have to do is insert a new line in the table below. No extra code will be needed.
   You can remove a band by deleting a line if you do not want a given band.
   Also, you can change the parameters of the band.
   ATTENTION: You have to RESET the eeprom after adding or removing a line of this table.
              Turn your receiver on with the encoder push button pressed at first time to RESET the eeprom content.
*/
Band band[] = {
  {FM_BAND_TYPE, 6400, 10800, 9830, 3, 0},
  {LW_BAND_TYPE, 100, 510, 300, 0, 4},
  {MW_BAND_TYPE, 531, 1701, 531, 2, 4},       // MW for Europe, Africa and Asia
  {SW_BAND_TYPE, 1700, 30000, 1700, 1, 2}
};

const int lastBand = (sizeof band / sizeof(Band)) - 1;
int bandIdx = 1;

uint8_t rssi = 0;
uint8_t stereo = 1;
uint8_t volume = DEFAULT_VOLUME;

// Devices class declarations
Rotary encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);
SI4735 si4735;

void setup()
{
  // Encoder pins
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  pinMode(STEP_DECR, INPUT_PULLUP);
  pinMode(BAND_BUTTON, INPUT_PULLUP);
  pinMode(SEEK_PREV, INPUT_PULLUP);
  pinMode(VOLUME_BUTTON, INPUT_PULLUP);
  pinMode(SEEK_NEXT, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);
  pinMode(AGC_BUTTON, INPUT_PULLUP);
  pinMode(STEP_BUTTON, INPUT_PULLUP);
  pinMode(STEP_INCR, INPUT_PULLUP);

  oled.begin();
  oled.clear();
  oled.on();
  oled.setFont(FONT6X8);

  // Splash - Change orit for your introduction text or remove the splash code.
  oled.setCursor(40, 0);
  oled.print("SI473X");
  oled.setCursor(20, 1);
  oled.print("Arduino Library");
  delay(500);
  oled.setCursor(15, 2);
  oled.print("All in One Radio");
  delay(500);
  oled.setCursor(10, 3);
  oled.print("** TUSHAR **");
  delay(2000);
  // end Splash

  // If you want to reset the eeprom, keep the VOLUME_UP button pressed during statup
  if (digitalRead(ENCODER_BUTTON) == LOW)
  {
    oled.clear();
    EEPROM.write(eeprom_address, 0);
    oled.setCursor(0, 0);
    oled.print("EEPROM RESETED");
    delay(2000);
    oled.clear();
  }

  // Encoder interrupt
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);

  si4735.getDeviceI2CAddress(RESET_PIN); // Looks for the I2C bus address and set it.  Returns 0 if error

  si4735.setup(RESET_PIN, MW_BAND_TYPE); //
  si4735.setAvcAmMaxGain(48); // Sets the maximum gain for automatic volume control on AM/SSB mode (between 12 and 90dB)

  delay(500);

  // Checking the EEPROM content
  if (EEPROM.read(eeprom_address) == app_id)
  {
    readAllReceiverInformation();
  }

  // Set up the radio for the current band (see index table variable bandIdx )
  useBand();

  currentFrequency = previousFrequency = si4735.getFrequency();

  si4735.setVolume(volume);
  oled.clear();
  showStatus();
  showBatt();
}

// Use Rotary.h and  Rotary.cpp implementation to process encoder via interrupt
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

/*
   writes the conrrent receiver information into the eeprom.
   The EEPROM.update avoid write the same data in the same memory position. It will save unnecessary recording.
*/
void saveAllReceiverInformation()
{
  int addr_offset;
  EEPROM.update(eeprom_address, app_id);                 // stores the app id;
  EEPROM.update(eeprom_address + 1, si4735.getVolume()); // stores the current Volume
  EEPROM.update(eeprom_address + 2, bandIdx);            // Stores the current band
  EEPROM.update(eeprom_address + 3, currentMode);        // Stores the current Mode (FM / AM / SSB)
  EEPROM.update(eeprom_address + 4, currentBFO >> 8);
  EEPROM.update(eeprom_address + 5, currentBFO & 0XFF);

  addr_offset = 6;
  band[bandIdx].currentFreq = currentFrequency;

  for (int i = 0; i <= lastBand; i++)
  {
    EEPROM.update(addr_offset++, (band[i].currentFreq >> 8));   // stores the current Frequency HIGH byte for the band
    EEPROM.update(addr_offset++, (band[i].currentFreq & 0xFF)); // stores the current Frequency LOW byte for the band
    EEPROM.update(addr_offset++, band[i].currentStepIdx);       // Stores current step of the band
    EEPROM.update(addr_offset++, band[i].bandwidthIdx);        // table index (direct position) of bandwidth
  }
}

/**
   reads the last receiver status from eeprom.
*/
void readAllReceiverInformation()
{
  int addr_offset;
  int bwIdx;
  volume = EEPROM.read(eeprom_address + 1); // Gets the stored volume;
  bandIdx = EEPROM.read(eeprom_address + 2);
  currentMode = EEPROM.read(eeprom_address + 3);
  currentBFO = EEPROM.read(eeprom_address + 4) << 8;
  currentBFO |= EEPROM.read(eeprom_address + 5);

  addr_offset = 6;
  for (int i = 0; i <= lastBand; i++)
  {
    band[i].currentFreq = EEPROM.read(addr_offset++) << 8;
    band[i].currentFreq |= EEPROM.read(addr_offset++);
    band[i].currentStepIdx = EEPROM.read(addr_offset++);
    band[i].bandwidthIdx = EEPROM.read(addr_offset++);
  }

  previousFrequency = currentFrequency = band[bandIdx].currentFreq;
  idxStep = tabStep[band[bandIdx].currentStepIdx];
  bwIdx = band[bandIdx].bandwidthIdx;

  if (currentMode == LSB || currentMode == USB)
  {
    loadSSB();
    bwIdxSSB = (bwIdx > 5) ? 5 : bwIdx;
    si4735.setSSBAudioBandwidth(bandwidthSSB[bwIdxSSB].idx);
    // If audio bandwidth selected is about 2 kHz or below, it is recommended to set Sideband Cutoff Filter to 0.
    if (bandwidthSSB[bwIdxSSB].idx == 0 || bandwidthSSB[bwIdxSSB].idx == 4 || bandwidthSSB[bwIdxSSB].idx == 5)
      si4735.setSBBSidebandCutoffFilter(0);
    else
      si4735.setSBBSidebandCutoffFilter(1);
  }
  else if (currentMode == AM)
  {
    bwIdxAM = bwIdx;
    si4735.setBandwidth(bandwidthAM[bwIdxAM].idx, 1);
  }
  else
  {
    bwIdxFM = bwIdx;
    si4735.setFmBandwidth(bandwidthFM[bwIdxFM].idx);
  }
}

/*
   To store any change into the EEPROM, it is needed at least STORE_TIME  milliseconds of inactivity.
*/
void resetEepromDelay()
{
  storeTime = millis();
  previousFrequency = 0;
}

/**
   Converts a number to a char string and places leading zeros.
   It is useful to mitigate memory space used by sprintf or generic similar function

   value  - value to be converted
   strValue - the value will be receive the value converted
   len -  final string size (in bytes)
   dot - the decimal or tousand separator position
   separator -  symbol "." or ","
*/
void convertToChar(uint16_t value, char *strValue, uint8_t len, uint8_t dot, uint8_t separator)
{
  char d;
  for (int i = (len - 1); i >= 0; i--)
  {
    d = value % 10;
    value = value / 10;
    strValue[i] = d + 48;
  }
  strValue[len] = '\0';

  if (strValue[0] == '0')
  {
    strValue[0] = ' ';
    if (strValue[1] == '0')
      strValue[1] = ' ';
  }
}

/**
  Show current frequency
*/
void showFrequency()
{
  char *unit;
  char freqDisplay[10];

  if (band[bandIdx].bandType == FM_BAND_TYPE)
  {
    convertToChar(currentFrequency, freqDisplay, 5, 3, ',');
    unit = (char *)"MHz";
  }
  else
  {
    unit = (char *)"kHz";
    if (band[bandIdx].bandType == MW_BAND_TYPE || band[bandIdx].bandType == LW_BAND_TYPE)
      convertToChar(currentFrequency, freqDisplay, 5, 0, '.');
    else
      convertToChar(currentFrequency, freqDisplay, 5, 2, '.');
  }

  oled.invertOutput(bfoOn);
  // oled.setFont(FONT8X16ATARI);
  oled.setCursor(34, 0);
  oled.print("      ");
  oled.setCursor(34, 0);
  oled.print(freqDisplay);
  // oled.setFont(FONT6X8);
  oled.invertOutput(false);

  oled.setCursor(95, 0);
  oled.print(unit);
}

/**
    This function is called by the seek function process.
*/
void showFrequencySeek(uint16_t freq)
{
  currentFrequency = freq;
  showFrequency();
}

/**
   Checks the stop seeking criterias.
   Returns true if the user press the touch or rotates the encoder during the seek process.
*/
bool checkStopSeeking()
{
  // Checks the touch and encoder
  return (bool)encoderCount || (digitalRead(ENCODER_BUTTON) == LOW); // returns true if the user rotates the encoder or press the push button
}

/**
    Shows some basic information on display
*/
void showStatus()
{
  showFrequency();
  showBandDesc();
  showStep();
  showBandwidth();
  showAttenuation();
  showRSSI();
  showVolume();
}

/**
   Shows band information
*/
void showBandDesc()
{
  char *bandMode;
  if (currentFrequency < 520)
    bandMode = (char *)"LW  ";
  else
    bandMode = (char *)bandModeDesc[currentMode];

  oled.setCursor(0, 0);
  oled.print("    ");
  oled.setCursor(0, 0);
  oled.invertOutput(cmdBand);
  oled.print(bandMode);
  oled.invertOutput(false);
}

/* *******************************
   Shows RSSI status
*/
void showRSSI()
{
  int bars = (rssi / 20.0) + 1;
  oled.setCursor(90, 3);
  oled.print("      ");
  oled.setCursor(90, 3);
  oled.print(".");
  for (int i = 0; i < bars; i++)
    oled.print('_');
  oled.print('|');

  if (currentMode == FM)
  {
    oled.setCursor(18, 0);
    oled.print("  ");
    oled.setCursor(18, 0);
    oled.invertOutput(true);
    if (si4735.getCurrentPilot())
    {
      oled.invertOutput(true);
      oled.print("s");
    }
    oled.invertOutput(false);
  }
}

/*
   Shows the volume level on LCD
*/
void showVolume()
{
  oled.setCursor(58, 3);
  oled.print("  ");
  oled.setCursor(58, 3);
  oled.invertOutput(cmdVolume);
  oled.print(' ');
  oled.invertOutput(false);
  oled.print(si4735.getCurrentVolume());
}

void showStep()
{
  if (bfoOn)
    return;
  oled.setCursor(93, 1);
  oled.print("     ");
  if (currentMode == FM)
    return;
  oled.setCursor(93, 1);
  oled.invertOutput(cmdStep);
  oled.print(":");
  oled.invertOutput(false);
  oled.print(tabStep[idxStep]);
}

/**
   Shows bandwidth on AM,SSB and FM mode
*/
void showBandwidth()
{
  char *bw;
  if (currentMode == LSB || currentMode == USB)
  {
    bw = (char *)bandwidthSSB[bwIdxSSB].desc;
    showBFO();
  }
  else if (currentMode == AM)
  {
    bw = (char *)bandwidthAM[bwIdxAM].desc;
  }
  else
  {
    bw = (char *)bandwidthFM[bwIdxFM].desc;
  }
  oled.setCursor(0, 3);
  oled.print("          ");
  oled.setCursor(0, 3);
  oled.invertOutput(cmdBw);
  oled.print("BW: ");
  oled.invertOutput(false);
  oled.print(bw);
}

/*
   Shows AGCC and Attenuation
*/
void showAttenuation()
{
  // Show AGC Information
  oled.setCursor(0, 1);
  oled.print("     ");
  oled.setCursor(0, 1);
  if ( currentMode != FM ) {
    oled.invertOutput(cmdAgcAtt);
    if (agcIdx == 0)
    {
      oled.print("AGC");
      oled.invertOutput(false);
    }
    else
    {
      oled.print("At");
      oled.invertOutput(false);
      oled.print(agcNdx);
    }
  }

}

void showSoftMute() {
  if ( currentMode != FM ) {
    oled.setCursor(0, 1);
    oled.print("     ");
    oled.setCursor(0, 1);
    oled.invertOutput(cmdSoftMute);
    oled.print("SM");
    oled.invertOutput(false);
    oled.print(smIdx);
  }
}


void showAvc() {
  if ( currentMode != FM ) {
    oled.setCursor(0, 1);
    oled.print("     ");
    oled.setCursor(0, 1);
    oled.invertOutput(cmdAvc);
    oled.print("AVC");
    oled.invertOutput(false);
    oled.print(avcIdx);
  }
}

/*
   Shows the BFO current status.
   Must be called only on SSB mode (LSB or USB)
*/
void showBFO()
{
  oled.setCursor(0, 2);
  oled.print("            ");
  oled.setCursor(0, 2);
  oled.print("BFO: ");
  oled.print(currentBFO);
  oled.print("Hz ");

  oled.setCursor(93, 2);
  oled.print("    ");
  oled.setCursor(93, 2);
  oled.invertOutput(cmdStep);
  oled.print(":");
  oled.invertOutput(false);
  oled.print(currentBFOStep);
}

char *stationName;
char bufferStatioName[20];
long rdsElapsed = millis();

char oldBuffer[15];

/*
   Clean the content of the third line (line 2 - remember the first line is 0)
*/
void cleanBfoRdsInfo()
{
  oled.setCursor(0, 2);
  oled.print("                    ");
}

/*
   Show the Station Name.
*/
void showRDSStation()
{
  char *po, *pc;
  int col = 0;

  po = oldBuffer;
  pc = stationName;
  while (*pc)
  {
    if (*po != *pc)
    {
      oled.setCursor(col, 2);
      oled.print(*pc);
    }
    *po = *pc;
    po++;
    pc++;
    col += 10;
  }
  // strcpy(oldBuffer, stationName);
  delay(100);
}

/*
   Checks the station name is available
*/
void checkRDS()
{
  si4735.getRdsStatus();
  if (si4735.getRdsReceived())
  {
    if (si4735.getRdsSync() && si4735.getRdsSyncFound() && !si4735.getRdsSyncLost() && !si4735.getGroupLost())
    {
      stationName = si4735.getRdsText0A();
      if (stationName != NULL /* && si4735.getEndGroupB()  && (millis() - rdsElapsed) > 10 */)
      {
        showRDSStation();
        // si4735.resetEndGroupB();
        rdsElapsed = millis();
      }
    }
  }
}

/*
   Goes to the next band (see Band table)
*/
void bandUp()
{
  // save the current frequency for the band
  band[bandIdx].currentFreq = currentFrequency;
  band[bandIdx].currentStepIdx = idxStep; // currentStep;

  if (bandIdx < lastBand)
  {
    bandIdx++;
  }
  else
  {
    bandIdx = 0;
  }
  useBand();
}

/*
   Goes to the previous band (see Band table)
*/
void bandDown()
{
  // save the current frequency for the band
  band[bandIdx].currentFreq = currentFrequency;
  band[bandIdx].currentStepIdx = idxStep;
  if (bandIdx > 0)
  {
    bandIdx--;
  }
  else
  {
    bandIdx = lastBand;
  }
  useBand();
}

/*
   This function loads the contents of the ssb_patch_content array into the CI (Si4735) and starts the radio on
   SSB mode.
*/
void loadSSB()
{
  oled.setCursor(0, 2);
  oled.print("  Switching to SSB  ");
  // si4735.setI2CFastModeCustom(700000); // It is working. Faster, but I'm not sure if it is safe.
  si4735.setI2CFastModeCustom(500000);
  si4735.queryLibraryId(); // Is it really necessary here? I will check it.
  si4735.patchPowerUp();
  delay(50);
  si4735.downloadCompressedPatch(ssb_patch_content, size_content, cmd_0x15, cmd_0x15_size);
  si4735.setSSBConfig(bandwidthSSB[bwIdxSSB].idx, 1, 0, 1, 0, 1);
  si4735.setI2CStandardMode();
  ssbLoaded = true;
  // oled.clear();
  cleanBfoRdsInfo();
}

/*
   Switch the radio to current band.
   The bandIdx variable points to the current band.
   This function change to the band referenced by bandIdx (see table band).
*/
void useBand()
{
  cleanBfoRdsInfo();
  if (band[bandIdx].bandType == FM_BAND_TYPE)
  {
    currentMode = FM;
    si4735.setTuneFrequencyAntennaCapacitor(0);
    si4735.setFM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, tabStep[band[bandIdx].currentStepIdx]);
    si4735.setSeekFmLimits(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq);
    si4735.setSeekFmSpacing(1);
    bfoOn = ssbLoaded = false;
    si4735.setRdsConfig(1, 2, 2, 2, 2);
    si4735.setFifoCount(1);
    bwIdxFM = band[bandIdx].bandwidthIdx;
    si4735.setFmBandwidth(bandwidthFM[bwIdxFM].idx);
  }
  else
  {
    if (band[bandIdx].bandType == MW_BAND_TYPE || band[bandIdx].bandType == LW_BAND_TYPE)
      si4735.setTuneFrequencyAntennaCapacitor(0);
    else
      si4735.setTuneFrequencyAntennaCapacitor(1);

    if (ssbLoaded)
    {
      si4735.setSSB(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, tabStep[band[bandIdx].currentStepIdx], currentMode);
      si4735.setSSBAutomaticVolumeControl(1);
      si4735.setSsbSoftMuteMaxAttenuation(0); // Disable Soft Mute for SSB
      bwIdxSSB = band[bandIdx].bandwidthIdx;
      si4735.setSSBAudioBandwidth(bandwidthSSB[bwIdxSSB].idx);
      si4735.setSSBBfo(currentBFO);
    }
    else
    {
      currentMode = AM;
      si4735.setAM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, tabStep[band[bandIdx].currentStepIdx]);
      si4735.setAutomaticGainControl(disableAgc, agcNdx);
      si4735.setAmSoftMuteMaxAttenuation(smIdx); // // Disable Soft Mute for AM
      bwIdxAM = band[bandIdx].bandwidthIdx;
      si4735.setBandwidth(bandwidthAM[bwIdxAM].idx, 1);
      bfoOn = false;
    }
    si4735.setSeekAmLimits(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq);                                       // Consider the range all defined current band
    si4735.setSeekAmSpacing((tabStep[band[bandIdx].currentStepIdx] > 10) ? 10 : tabStep[band[bandIdx].currentStepIdx]); // Max 10kHz for spacing
  }
  delay(100);
  currentFrequency = band[bandIdx].currentFreq;
  idxStep = band[bandIdx].currentStepIdx;
  showStatus();
  resetEepromDelay();
}

/**
   Changes the step frequency value based on encoder rotation
*/
void doStep(int8_t v)
{
  // This command should work only for SSB mode
  if ((currentMode == LSB || currentMode == USB) && bfoOn)
  {
    currentBFOStep = (currentBFOStep == 25) ? 10 : 25;
    showBFO();
  }
  else
  {
    idxStep = (v == 1) ? idxStep + 1 : idxStep - 1;
    if (idxStep > lastStep)
      idxStep = 0;
    else if (idxStep < 0)
      idxStep = lastStep;

    si4735.setFrequencyStep(tabStep[idxStep]);
    band[bandIdx].currentStepIdx = idxStep;
    si4735.setSeekAmSpacing((tabStep[idxStep] > 10) ? 10 : tabStep[idxStep]); // Max 10kHz for spacing
    showStep();
  }
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
}

/**
   Changes the volume based on encoder rotation
*/
void doVolume(int8_t v)
{
  if (v == 1)
    si4735.volumeUp();
  else
    si4735.volumeDown();
  showVolume();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
}

/**
   Switches the AGC/Attenuation based on encoder rotation
*/
void doAttenuation(int8_t v)
{
  if ( cmdAgcAtt) {
    agcIdx = (v == 1) ? agcIdx + 1 : agcIdx - 1;
    if (agcIdx < 0)
      agcIdx = 37;
    else if (agcIdx > 37)
      agcIdx = 0;

    disableAgc = (agcIdx > 0); // if true, disable AGC; esle, AGC is enable

    if (agcIdx > 1)
      agcNdx = agcIdx - 1;
    else
      agcNdx = 0;

    // Sets AGC on/off and gain
    si4735.setAutomaticGainControl(disableAgc, agcNdx);
  }

  showAttenuation();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
}

/**
   SoftMute control
*/
void doSoftMute(int8_t v)
{
  if ( currentMode != FM ) {
    smIdx = (v == 1) ? smIdx + 1 : smIdx - 1;
    if (smIdx > 32)
      smIdx = 0;
    else if (smIdx < 0)
      smIdx = 32;
    si4735.setAmSoftMuteMaxAttenuation(smIdx);

    showSoftMute();
    delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  }
}

/**
   Automatic Volume Control
*/
void doAvc(int8_t v)
{
  if ( currentMode != FM ) {
    avcIdx = (v == 1) ? avcIdx + 2 : avcIdx - 2;
    if (avcIdx > 90)
      avcIdx = 12;
    else if (avcIdx < 12)
      avcIdx = 90;

    si4735.setAvcAmMaxGain(avcIdx);
    showAvc();
    delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  }
}


/**
   Switches the bandwidth based on encoder rotation
*/
void doBandwidth(uint8_t v)
{
  if (currentMode == LSB || currentMode == USB)
  {
    bwIdxSSB = (v == 1) ? bwIdxSSB + 1 : bwIdxSSB - 1;

    if (bwIdxSSB > 5)
      bwIdxSSB = 0;
    else if (bwIdxSSB < 0)
      bwIdxSSB = 5;

    band[bandIdx].bandwidthIdx = bwIdxSSB;

    si4735.setSSBAudioBandwidth(bandwidthSSB[bwIdxSSB].idx);
    // If audio bandwidth selected is about 2 kHz or below, it is recommended to set Sideband Cutoff Filter to 0.
    if (bandwidthSSB[bwIdxSSB].idx == 0 || bandwidthSSB[bwIdxSSB].idx == 4 || bandwidthSSB[bwIdxSSB].idx == 5)
      si4735.setSBBSidebandCutoffFilter(0);
    else
      si4735.setSBBSidebandCutoffFilter(1);
  }
  else if (currentMode == AM)
  {
    bwIdxAM = (v == 1) ? bwIdxAM + 1 : bwIdxAM - 1;

    if (bwIdxAM > maxFilterAM)
      bwIdxAM = 0;
    else if (bwIdxAM < 0)
      bwIdxAM = maxFilterAM;

    band[bandIdx].bandwidthIdx = bwIdxAM;
    si4735.setBandwidth(bandwidthAM[bwIdxAM].idx, 1);
  }
  else
  {
    bwIdxFM = (v == 1) ? bwIdxFM + 1 : bwIdxFM - 1;
    if (bwIdxFM > 4)
      bwIdxFM = 0;
    else if (bwIdxFM < 0)
      bwIdxFM = 4;

    band[bandIdx].bandwidthIdx = bwIdxFM;
    si4735.setFmBandwidth(bandwidthFM[bwIdxFM].idx);
  }
  showBandwidth();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
}

/**
   disble command buttons and keep the current status of the last command button pressed
*/
void disableCommand(bool *b, bool value, void (*showFunction)())
{
  cmdVolume = false;
  cmdAgcAtt = false;
  cmdStep = false;
  cmdBw = false;
  cmdBand = false;
  cmdSoftMute = false;
  cmdAvc = false;
  showVolume();
  showStep();
  showAttenuation();
  showBandwidth();
  showBandDesc();

  if (b != NULL) // rescues the last status of the last command only the parameter is not null
    *b = value;
  if (showFunction != NULL) //  show the desired status only if it is necessary.
    showFunction();

  elapsedRSSI = millis();
  countRSSI = 0;
}

void showBatt()
{
  int sensorValue = analogRead(A2); //read the A2 pin value
//  float voltage = sensorValue * (3.30 / 1023.00 * 2); //convert the value to a true voltage.
  float perc = map(sensorValue, 500, 645, 0, 100);
  perc=constrain(perc,0,100);
  
  oled.setCursor(0, 2);
  oled.print("                ");
  oled.setCursor(0, 2);
  oled.print(perc); //Display battery percentage
  oled.print("%");
}

void loop()
{
  // Check if the encoder has moved.
  if (encoderCount != 0)
  {
    if (cmdVolume)
      doVolume(encoderCount);
    else if (cmdAgcAtt)
      doAttenuation(encoderCount);
    else if (cmdStep)
      doStep(encoderCount);
    else if (cmdBw)
      doBandwidth(encoderCount);
    else if (cmdSoftMute)
      doSoftMute(encoderCount);
    else if (cmdAvc)
      doAvc(encoderCount);
    else if (cmdBand)
    {
      if (encoderCount == 1)
        bandUp();
      else
        bandDown();
    }
    else if (bfoOn)
    {
      currentBFO = (encoderCount == 1) ? (currentBFO + currentBFOStep) : (currentBFO - currentBFOStep);
      si4735.setSSBBfo(currentBFO);
      previousFrequency = 0; // Forces eeprom update
      showBFO();
    }
    else
    {
      if (encoderCount == 1)
      {
        si4735.frequencyUp();
        seekDirection = 1;
      }
      else
      {
        si4735.frequencyDown();
        seekDirection = 0;
      }
      // Show the current frequency only if it has changed
      currentFrequency = si4735.getFrequency();
      showFrequency();
    }
    encoderCount = 0;
    resetEepromDelay(); // if you moved the encoder, something was changed
    elapsedRSSI = millis();
    countRSSI = 0;
  }

  // Check button commands
  if ((millis() - elapsedButton) > MIN_ELAPSED_TIME) // Is that necessary?
  {
    // check if some button is pressed
    if (digitalRead(STEP_DECR) == LOW)
    {
      doStep(-1);
    }
    else if (digitalRead(STEP_INCR) == LOW)
    {
      doStep(1);
    }
    else if (digitalRead(SEEK_PREV) == LOW)
    {
      seekDirection = 0;
      encoderCount = 0;
      si4735.frequencyDown();
      si4735.seekStationProgress(showFrequencySeek, checkStopSeeking, seekDirection);
      delay(30);
      currentFrequency = si4735.getFrequency();
      showFrequency();
      delay(MIN_ELAPSED_TIME);
      showBatt();
    }
    else if (digitalRead(SEEK_NEXT) == LOW)
    {
      seekDirection = 1;
      encoderCount = 0;
      si4735.frequencyUp();
      si4735.seekStationProgress(showFrequencySeek, checkStopSeeking, seekDirection);
      delay(30);
      currentFrequency = si4735.getFrequency();
      showFrequency();
      delay(MIN_ELAPSED_TIME);
    }
    else if (digitalRead(VOLUME_BUTTON) == LOW)
    {
      cmdVolume = !cmdVolume;
      disableCommand(&cmdVolume, cmdVolume, showVolume);
      delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
    }
    else if (digitalRead(ENCODER_BUTTON) == LOW)
    {
      if (currentMode == LSB || currentMode == USB)
      {
        bfoOn = !bfoOn;
        if (bfoOn)
          showBFO();
        showStatus();
        disableCommand(NULL, false, NULL); // disable all command buttons
      }
      else if (currentMode == FM || currentMode == AM)
      {
        // Jumps up or down one space
        if (seekDirection)
          si4735.frequencyUp();
        else
          si4735.frequencyDown();

        si4735.seekStationProgress(showFrequencySeek, checkStopSeeking, seekDirection);
        delay(30);
        if (currentMode == FM)
        {
          float f = round(si4735.getFrequency() / 10.0);
          currentFrequency = (uint16_t)f * 10; // adjusts band space from 1 (10kHz) to 10 (100 kHz)
          si4735.setFrequency(currentFrequency);
        }
        else
        {
          currentFrequency = si4735.getFrequency(); //
        }
        showFrequency();
      }
      delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
    }
    else if (digitalRead(AGC_BUTTON) == LOW)
    {
      if ( currentMode != FM) {
        cmdAgcAtt = !cmdAgcAtt;
        disableCommand(&cmdAgcAtt, cmdAgcAtt, showAttenuation);
      }
      delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
    }
    else if (digitalRead(STEP_BUTTON) == LOW)
    {
      if (currentMode != FM)
      {
        cmdStep = !cmdStep;
        disableCommand(&cmdStep, cmdStep, showStep);
      }
      delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
    }
    else if (digitalRead(BAND_BUTTON) == LOW)
    {
      cmdBand = !cmdBand;
      disableCommand(&cmdBand, cmdBand, showBandDesc);
      delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
    }
    elapsedButton = millis();
  }

  // Show RSSI status only if this condition has changed
  if ((millis() - elapsedRSSI) > MIN_ELAPSED_RSSI_TIME * 9)
  {
    si4735.getCurrentReceivedSignalQuality();
    int aux = si4735.getCurrentRSSI();
    if (rssi != aux)
    {
      rssi = aux;
      showRSSI();
    }

    if (countRSSI++ > 3)
    {
      disableCommand(NULL, false, NULL); // disable all command buttons
      countRSSI = 0;
    }
    elapsedRSSI = millis();
  }

  if (currentMode == FM)
  {
    if (currentFrequency != previousFrequency)
    {
      cleanBfoRdsInfo();
    }
    else
    {
      checkRDS();
    }
  }

  // Show the current frequency only if it has changed
  if (currentFrequency != previousFrequency)
  {
    if ((millis() - storeTime) > STORE_TIME)
    {
      saveAllReceiverInformation();
      storeTime = millis();
      previousFrequency = currentFrequency;
    }
  }

  // Update battery status every 5 minutes
  // if ( (millis() - elapsedBatt ) > 300000)
  // {
  //    showBatt();
  //    elapsedBatt = millis();
  // }

  delay(10);
}
