#include <SI4735.h>
#include <EEPROM.h>
#include <Tiny4kOLED.h>
#include <font8x16atari.h> // Please, install the TinyOLED-Fonts library
#include <Rotary.h>

#define BTN_RST_PIN -1 // Define proper BTN_RST_PIN if required.
#define BTN_ENCODER_PIN_A 2
#define BTN_ENCODER_PIN_B 3
#define BTN_STEP_INCR 4      // Switch MODE (Am/LSB/USB)
#define BTN_STEP_DECR 5 // Used to select the banddwith.
#define BTN_VOLUME_BUTTON 6    // Volume Up
#define BTN_SEEK_NEXT 7     // **** Use thi button to implement a new function
#define BTN_BAND_BUTTON 8      // Next band
#define BTN_SEEK_PREV 9     // **** Use thi button to implement a new function
#define BTN_STEP_BUTTON 10     // Used to select the increment or decrement frequency step (see tabStep)
#define BTN_AGC_BUTTON 11      // Switch AGC ON/OF
#define BTN_RESET_PIN 12
#define BTN_ENCODER_BUTTON 14  // Used to select the enconder control (BFO or VFO) and SEEK function on AM and FM modes

#define MIN_ELAPSED_TIME 100
#define MIN_ELAPSED_RSSI_TIME 150

uint8_t seekDirection = 1;

bool cmdVolume = false;   // if true, the encoder will control the volume.
bool cmdAgcAtt = false;   // if true, the encoder will control the AGC / Attenuation
bool cmdStep = false;     // if true, the encoder will control the step frequency
bool cmdBw = false;       // if true, the encoder will control the bandwidth
bool cmdBand = false;     // if true, the encoder will control the band
bool cmdSoftMute = false; // if true, the encoder will control the Soft Mute attenuation
bool cmdAvc = false;      // if true, the encoder will control Automatic Volume Control

long countRSSI = 0;
long elapsedRSSI = millis();
long elapsedButton = millis();

// Encoder control variables
volatile int encoderCount = 0;

// Some variables to check the SI4735 status
uint16_t currentFrequency;
uint16_t previousFrequency;

// Atenua and AGC
int8_t agcIdx = 0;
uint8_t disableAgc = 1;
uint8_t agcNdx = 0;
int8_t smIdx = 32;
int8_t avcIdx = 90;

uint8_t rssi = 0;
uint8_t stereo = 1;
uint8_t volume = 20;

int tabStep[] = {1,5,9,10,100,1000};

const int lastStep = (sizeof tabStep / sizeof(int)) - 1;
int idxStep = 5;

// Devices class declarations
Rotary encoder = Rotary(BTN_ENCODER_PIN_A, BTN_ENCODER_PIN_B);
SI4735 si4735;

void setup()
{
  // Encoder pins

  pinMode(BTN_AGC_BUTTON, INPUT_PULLUP);
  pinMode(BTN_BAND_BUTTON, INPUT_PULLUP);
  pinMode(BTN_ENCODER_BUTTON, INPUT_PULLUP);
  pinMode(BTN_ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(BTN_ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(BTN_SEEK_NEXT, INPUT_PULLUP);
  pinMode(BTN_SEEK_PREV, INPUT_PULLUP);
  pinMode(BTN_STEP_BUTTON, INPUT_PULLUP);
  pinMode(BTN_STEP_DECR, INPUT_PULLUP);
  pinMode(BTN_STEP_INCR, INPUT_PULLUP);
  pinMode(BTN_VOLUME_BUTTON, INPUT_PULLUP);

  oled.begin();
  oled.clear();
  oled.on();
  oled.setFont(FONT6X8);
  oled.setCursor(40, 0);
  oled.print("SI4732");
  delay(2000);

  // If you want to reset the eeprom, keep the VOLUME_UP button pressed during statup
  if (digitalRead(BTN_BAND_BUTTON) == LOW)
  {
    oled.clear();
    EEPROM.write(0, 0);
    oled.setCursor(0, 0);
    oled.print("EEPROM RESETED");
    delay(2000);
    oled.clear();
  }

  // Encoder interrupt
  attachInterrupt(digitalPinToInterrupt(BTN_ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BTN_ENCODER_PIN_B), rotaryEncoder, CHANGE);

  si4735.getDeviceI2CAddress(BTN_RESET_PIN); // Looks for the I2C bus address and set it.  Returns 0 if error

  si4735.setup(BTN_RESET_PIN, 1); //
  si4735.setAvcAmMaxGain(48); // Sets the maximum gain for automatic volume control on AM/SSB mode (between 12 and 90dB)

  delay(500);

  si4735.setTuneFrequencyAntennaCapacitor(1); // Related to VARACTOR. Official recommendation is 0, but PU2CLR has set to 1 for SW and 0 for MW/LW
  si4735.setAM(100, 30000, 7300, 5);
  si4735.setAutomaticGainControl(0, 0); // This param selects whether the AGC is enabled or disabled (0 = AGC enabled; 1 = AGC disabled) | AGC Index (0 = Minimum attenuation (max gain) 1 – 36 = Intermediate attenuation) if >greater than 36 - Maximum attenuation (min gain) )
  si4735.setAmSoftMuteMaxAttenuation(32); // This function can be useful to disable Soft Mute. The value 0 disable soft mute. Specified in units of dB. Default maximum attenuation is 8 dB. Goes til 32. It works for AM and SSB.
  si4735.setBandwidth(3, 1); // BW 0=6kHz,  1=4kHz,  2=3kHz,  3=2kHz,  4=1kHz,  5=1.8kHz,  6=2.5kHz . The default bandwidth is 2 kHz. It works only in AM / SSB (LW/MW/SW) | Enables the AM Power Line Noise Rejection Filter.
  si4735.setSeekAmLimits(100, 30000);
  si4735.setSeekAmSpacing(10); // Selects frequency spacingfor AM seek. Default is 10 kHz spacing.
  
  delay(100);

  currentFrequency = previousFrequency = si4735.getFrequency();

  si4735.setVolume(volume);
  oled.clear();
  showStatus();
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
  unit = (char *)"kHz";
  convertToChar(currentFrequency, freqDisplay, 5, 0, '.');
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
  return (bool)encoderCount || (digitalRead(BTN_ENCODER_BUTTON) == LOW); // returns true if the user rotates the encoder or press the push button
}

/**
    Shows some basic information on display
*/
void showStatus()
{
  showFrequency();
  showVolume();
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

/**
   Changes the step frequency value based on encoder rotation
*/
void doStep(int8_t v)
{
  idxStep = (v == 1) ? idxStep + 1 : idxStep - 1;
  if (idxStep > lastStep)
    idxStep = 0;
  else if (idxStep < 0)
    idxStep = lastStep;

  si4735.setFrequencyStep(tabStep[idxStep]);
  si4735.setSeekAmSpacing((tabStep[idxStep] > 10) ? 10 : tabStep[idxStep]); // Max 10kHz for spacing
  oled.setCursor(93, 1);
  oled.print("     ");
  oled.setCursor(93, 1);
  oled.invertOutput(cmdStep);
  oled.print(":");
  oled.invertOutput(false);
  oled.print(tabStep[idxStep]);
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

  if (b != NULL) // rescues the last status of the last command only the parameter is not null
    *b = value;
  if (showFunction != NULL) //  show the desired status only if it is necessary.
    showFunction();

  elapsedRSSI = millis();
  countRSSI = 0;
}


void loop()
{
  // Check if the encoder has moved.
  if (encoderCount != 0)
  {
    if (cmdVolume)
      doVolume(encoderCount);
    else if (cmdStep)
      doStep(encoderCount);
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
    previousFrequency = 0; // if you moved the encoder, something was changed
    elapsedRSSI = millis();
    countRSSI = 0;
  }

  // Check button commands
  if ((millis() - elapsedButton) > MIN_ELAPSED_TIME) // Is that necessary?
  {
    // check if some button is pressed
    if (digitalRead(BTN_STEP_DECR) == LOW)
    {
      doStep(-1);
    }
    else if (digitalRead(BTN_STEP_INCR) == LOW)
    {
      doStep(1);
    }
    else if (digitalRead(BTN_SEEK_PREV) == LOW)
    {
      seekDirection = 0;
      encoderCount = 0;
      si4735.frequencyDown();
      si4735.seekStationProgress(showFrequencySeek, checkStopSeeking, seekDirection);
      delay(30);
      currentFrequency = si4735.getFrequency();
      showFrequency();
      delay(MIN_ELAPSED_TIME);
    }
    else if (digitalRead(BTN_SEEK_NEXT) == LOW)
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
    else if (digitalRead(BTN_VOLUME_BUTTON) == LOW)
    {
      cmdVolume = !cmdVolume;
      disableCommand(&cmdVolume, cmdVolume, showVolume);
      delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
    }
    elapsedButton = millis();
  }


  delay(10);
}