#define ENCODER_CLK 2
#define ENCODER_DT  3
=====================================

volatile int encoderCount = 0;

=====================================
typedef struct BTNGRP
{
  int pin1;
  const int pin2;
  const char * const title;
  const bool pressed;
  long timestamp;
};

BTNGRP btnGrp[] = {
  {12, 0, "MODE", false, 0},
  {11, 0, "VOL", false, 0},
  {4, 0, "STEP", false, 0}
};

const int btnGrpLen = (sizeof btnGrp / sizeof(BTNGRP));

=====================================

typedef struct
{
  int selected;
  const int fullSize;
  const char* option[10];
} Funk;

Funk funk[] = {
  { 0, 5, {"1", "5", "9", "10", "100", "1000"} },
  { 0, 3, {"SW", "MW", "LW", "FM"} }
};

enum funkionsss {
  STEPS,
  BANDS
};

void changeMode(funkionsss opt, int dir) {
  short fullSize = funk[opt].fullSize;
  short currentIndex = funk[opt].selected + dir;
}

int cs = funk[STEPS].selected;
changeMode(STEPS, -1);

=====================================

=====================================

WRONG CONCEPT

enum Settings {
  FREQUENCY,
  BAND,
  GAIN,
  BANDWIDTH,
  BATTERY
} settings;

if (settings == 4) {
  settings = 0;
} else {
  settings = (Settings)(settings + 1);
}

switch (settings) {
  case FREQUENCY: spinFrequency(); break;
  case BAND: spinBand(); break;
  case GAIN: spinGain(); break;
  case BATTERY: spinBattery(); break;
  default: break;
}

=====================================
//SWITCH CASE
//works with int or char 'A' not "String" (double quotes)
void modeHandler(short dir) {
  switch (settings) {
    case FREQUENCY: spinFrequency(dir); break;
    case BAND: spinBand(dir); break;
    case GAIN: spinGain(dir); break;
    case BANDWIDTH: spinBandwidth(dir); break;
    case BATTERY: spinBattery(dir); break;
  }
}

short currentButton = 1;
modeHandler(currentButton)
=====================================
// RECEVING USER INPUT FROM DEBUGGING WINDOW
String userinput = "";
void setup() {
  Serial.begin(9600);
}
void loop() {
  userinput = Serial.readString();
  if (userinput == "1\n") {
    Serial.println("received with CR LF and compared as it is");
  }
  
  userinput.trim();
  if (userinput == "1") {
    Serial.println("removed CR LF before comparing the string");
  }
  
  int mynumber = userinput.toInt();
  if (mynumber == 1) {
    Serial.println("converted to number before comparing");
  }

  int mynumber = 1;
  if (String(mynumber) == "1") {
    Serial.println("Number converted to String");
  }
  delay(500);
}
=====================================
// ARRAY
int _ATTUENATE[] = { 0, 1, 5, 10, 25, 36 };
int size = (sizeof(_ATTUENATE) / sizeof(_ATTUENATE[0]));
int val = _ATTUENATE[5];
=====================================
// ENUM EXAMPLE 1
enum State {
  SLEEP,
  AWAKE,
  CRAZY
};
void setup() {
  Serial.begin(9600);
  State state = CRAZY;
  Serial.println(state);
}
// ENUM EXAMPLE 2
enum State {
  SLEEP,
  AWAKE,
  CRAZY
} state;

void setup() {
  Serial.begin(9600);
  state = CRAZY;
  Serial.println(state);
}

// ENUM EXAMPLE 3
enum State {
  SLEEP,
  AWAKE,
  CRAZY
} state = CRAZY;

void setup() {
  Serial.begin(9600);
  Serial.println(state);
}

=====================================
