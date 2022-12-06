/*This code is developed by ISHAN_SACHINTHA*/

int INCIDENT_CALIBRATION = 330;

#define KNOB_MAX_ANALOG_READING 1023

const int ISO_TABLE[]           = {6, 12, 25, 50, 100, 160, 200, 400, 800, 1600, 3200, 6400};

const int SHUTTERSPEED_TABLE[]    = { 1, 2, 4, 8, 15, 30, 60, 125, 250, 500, 1000};

const char* label[] = {"EV:", "ISO:", "F-num:", "SS:"};

#include <math.h>
#include <Wire.h>A
#include <BH1750.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <SwitchManager.h>
#include <TimerOne.h>
#include "LowPower.h"
#include <EEPROM.h>

int16_t last, value;

SwitchManager modeSW;
const byte ModeSW = 3;

unsigned long previousMillis = 0;
const unsigned long wake_interval = 12000;

unsigned long ShortPress   = 3000UL;
unsigned long LongPress    = 6000UL;

#define KNOB_PIN       0

BH1750 bh1750;
SSD1306AsciiAvrI2c oled;

bool started = false;
bool Calibration_mode = false;
bool ISO_mode = false;
int EV, shutrerIndex, shutrerIndexOld, isoIndex, isoIndexOld, CalibrationIndex;
long lux;
double aperture;
int iso;
int old_iso;
int shutterspeed;
int old_shutterspeed;

bool pressed = false;
bool first_high = false;

uint8_t col0 = 0;
uint8_t col1 = 0;
uint8_t rows;

void eepromWriteCalibration () {
  EEPROM_writeAnything(1, INCIDENT_CALIBRATION);
  EEPROM.update(0, isoIndex);
}
void eepromDefaults () {
  EEPROM_readAnything(1, INCIDENT_CALIBRATION);;
  isoIndex = EEPROM.read(0);
}

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
  const byte* p = (const byte*)(const void*)&value;
  int i;
  for (i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
  return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
  byte* p = (byte*)(void*)&value;
  int i;
  for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
  return i;
}

void setup() {

  Serial.begin(9600);

  pinMode(10, OUTPUT); // OLED trigger
  digitalWrite(10, HIGH);

  Wire.begin();

  modeSW.begin (ModeSW, modeSwitchManager);

  bh1750.begin();

  bh1750.readLightLevel();

  oled.begin(&Adafruit128x64, 0x3c);
  oled.setFont(System5x7);
  oled.set2X();

  Serial.println("ArduMeter (Arduino incident light meter for camera) READY:");
  eepromDefaults ();
  iso = ISO_TABLE[isoIndex];
  Serial.println(INCIDENT_CALIBRATION);
  Serial.println(iso);

  oled.clear();
  oled.println("  Arduino");
  oled.println("   Light");
  oled.println("   Meter");
  delay(2000);

  oled.clear();
  oled.set2X();

  for (uint8_t i = 0; i < 4; i++) {
    oled.println(label[i]);
    uint8_t w = oled.strWidth(label[i]);
    col0 = col0 < w ? w : col0;
  }

  col1 = col0 + oled.strWidth("99.9") + 7;
  rows = oled.fontRows();

  delay(3000);
}


void clearValue(uint8_t row) {
  oled.clear(col0, col1, row, row + rows - 1);
}
void wakeUp() {

}
void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > wake_interval) {
    previousMillis = currentMillis;
    Serial.println("goint to sleep");
    digitalWrite(10, LOW);
    delay(100);
    attachInterrupt(1, wakeUp, FALLING);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    detachInterrupt(1);
    digitalWrite(10, HIGH);
    delay(100);
    oled.begin(&Adafruit128x64, 0x3c);
    oled.set2X();
    for (uint8_t i = 0; i < 4; i++) {
      oled.println(label[i]);
    }
  }

  while (pressed) {
    unsigned long currentMillis = millis();

    unsigned long timer = currentMillis - previousMillis;

    Serial.println(timer);

    if (timer > ShortPress) {
      first_high = false;
      Serial.print("first_high");
      Serial.println(first_high);
      Serial.println("ShortPress");
      if (ISO_mode == false) {
        ISO_mode = true;
        Serial.println("ISO mode ON");
        clearValue(rows);
        oled.print(iso);
        oled.print("*");
        clearValue(3 * rows);
        oled.print(shutterspeed);
        oled.print(" ");
      }
    }
    if (timer > LongPress)
    { Serial.println("LongPress");
      first_high = false;
      Serial.print("first_high");
      Serial.println(first_high);
      if (ISO_mode == true) {
        ISO_mode = false;
        Serial.println("ISO mode OFF");
      }
      if (Calibration_mode == false) {
        Calibration_mode = true;
        Serial.println("Calibration mode ON");
        oled.clear();
        oled.print("CALIBRATE");
        clearValue(rows);
        oled.print("(");
        oled.print(INCIDENT_CALIBRATION);
        oled.print(")");
        while (Calibration_mode == true) {
          modeSW.check();
          previousMillis = millis();
          INCIDENT_CALIBRATION = round(map(analogRead(KNOB_PIN), 0, KNOB_MAX_ANALOG_READING, 230, 430));
          //int x = round(map(analogRead(KNOB_PIN), 0, KNOB_MAX_ANALOG_READING, 200, 400));
          int y = INCIDENT_CALIBRATION - 330;
          INCIDENT_CALIBRATION = INCIDENT_CALIBRATION + y;
          clearValue(2 * rows);
          oled.print(y);
          delay(100);
        }

        eepromWriteCalibration ();
        Serial.println(INCIDENT_CALIBRATION);
      }
    }
    modeSW.check();
  }

  modeSW.check();
  getPotValue();
  calculate();

  clearValue(0);
  oled.print(EV);
  clearValue(rows);
  oled.print(iso);
  if (ISO_mode == true) {
    oled.print("*");
  }
  clearValue(2 * rows);
  oled.print(aperture, 1);
  clearValue(3 * rows);
  oled.print(shutterspeed);
  if (ISO_mode == false) {
    oled.print("*");
  }

  shutrerIndexOld = shutrerIndex;
  isoIndexOld = isoIndex;

  delay(100);
}

void modeSwitchManager (const byte newState, const unsigned long interval) {
  if (newState == LOW) {
    Serial.println("Button pressed");
    pressed = true;
    previousMillis = millis();
  }

  if (newState == HIGH) {
    Serial.println("Button relessed");
    previousMillis = millis();
    modeSW.check();
    pressed = false;
    Serial.print("first_high");
    Serial.println(first_high);


    if (Calibration_mode == true && first_high == true) {
      Calibration_mode = false;
      Serial.println("Calibration mode OFF");
      oled.clear();
      oled.set2X();
      for (uint8_t i = 0; i < 4; i++) {
        oled.println(label[i]);
      }
    }
    if (ISO_mode == true && first_high == true) {
      ISO_mode = false;
      Serial.println("ISO mode OFF");
    }

    if (!started) started = true;
    displayExposureSetting(true);
    first_high = true;
    Serial.print("first_high");
    Serial.println(first_high);
  }
}
void getPotValue() {

  if (ISO_mode == false) {
//    if (shutrerIndex <= 0) {
//      shutrerIndex = 0;
//    }
//    if (shutrerIndex >= sizeof(SHUTTERSPEED_TABLE) / sizeof(int)) {
//      shutrerIndex = 11;
//    }
    //shutrerIndex = round(map(analogRead(KNOB_PIN  ), 0, KNOB_MAX_ANALOG_READING, 0, sizeof(SHUTTERSPEED_TABLE) / sizeof(int)));
    int x = analogRead(KNOB_PIN );
    if ( x < 93) {
      shutrerIndex = 0;
    }
    else if (x > 93 && x < 186) {
      shutrerIndex = 1;
    }
    else if (x > 186 && x < 279) {
      shutrerIndex = 2;
    }
    else if (x > 279 && x < 372) {
      shutrerIndex = 3;
    }
    
    else if (x > 372 && x < 465) {
      shutrerIndex = 4;
    }
    
    else if (x > 465 && x < 558) {
      shutrerIndex = 5;
    }
    
    else if (x > 558 && x < 651) {
      shutrerIndex = 6;
    }
    
    else if (x > 651 && x < 774) {
      shutrerIndex = 7;
    }
    
    else if (x > 744 && x < 837) {
      shutrerIndex = 8;
    }
    
    else if (x > 837 && x < 930) {
      shutrerIndex = 9;
    }
    
    else if (x > 930) {
      shutrerIndex = 10;
    }
    shutterspeed = SHUTTERSPEED_TABLE[shutrerIndex];
  }
  if (ISO_mode == true) {
    previousMillis = millis();

    if (isoIndex <= 0) {
      isoIndex = 0;
    }
    if (isoIndex >= sizeof(ISO_TABLE) / sizeof(int)) {
      isoIndex = 10;
    }
    isoIndex = round(map(analogRead(KNOB_PIN  ), 0, KNOB_MAX_ANALOG_READING, 0, sizeof(ISO_TABLE) / sizeof(int)));
    iso = ISO_TABLE[isoIndex];
    
  }
}
void displayExposureSetting(bool measureNewEV) {

  if (measureNewEV) {
    lux = bh1750.readLightLevel();
    Serial.print("Measured illuminance = ");
    Serial.print(lux);
    Serial.println(" lux");
    Serial.print("Exposure settings: EV = ");
    Serial.print(EV);
    Serial.print(", ISO = ");
    Serial.print(iso);
    Serial.print(", aperture = f/");
    Serial.print(aperture, 1);
    Serial.print(", ");
    Serial.print("shutter speed = ");
    Serial.println(shutterspeed);
  }

}

void calculate() {

  shutterspeed = SHUTTERSPEED_TABLE[shutrerIndex];
  iso = ISO_TABLE[isoIndex];

  EV = log10(lux * iso / INCIDENT_CALIBRATION) / log10(2);

  if (isfinite(EV)) {

    aperture = sqrt(pow(2, EV) / shutterspeed);

    if (aperture < 1) {
      aperture = 0.7;
    }
    else if (aperture > 0.9 && aperture < 1.4) {
      aperture = 1.0;
    }
    else if (aperture > 1.3 && aperture < 2.0) {
      aperture = 1.4;
    }
    else if (aperture > 1.9 && aperture < 2.8) {
      aperture = 2.0;
    }
    else if (aperture > 2.7 && aperture < 4.0) {
      aperture = 2.8;
    }
    else if (aperture > 3.9 && aperture < 5.6) {
      aperture = 4.0;
    }
    else if (aperture > 5.5 && aperture < 8.0) {
      aperture = 5.6;
    }
    else if (aperture > 7.9 && aperture < 11) {
      aperture = 8.0;
    }
    else if (aperture > 10.9 && aperture < 16) {
      aperture = 11.0;
    }
    else if (aperture > 15.9 && aperture < 22) {
      aperture = 16.0;
    }
    else if (aperture > 21.9 && aperture < 32) {
      aperture = 22;
    }
    else if (aperture > 31.9 && aperture < 45) {
      aperture = 32;
    }
    else if (aperture > 44.9 && aperture < 64) {
      aperture = 45.0;
    }
    else if (aperture > 63.9) {
      aperture = 64;
    }

  } else {
    Serial.println("Exposure out of bounds");
  }
  delay(100);

  if (old_iso != iso || old_shutterspeed != shutterspeed) {
    previousMillis = millis();
    old_iso = iso;
    old_shutterspeed = shutterspeed;
    Serial.print("Exposure settings: EV = ");
    Serial.print(EV);
    Serial.print(", ISO = ");
    Serial.print(iso);
    Serial.print(", aperture = f/");
    Serial.print(aperture, 1);
    Serial.print(", ");
    Serial.print("shutter speed = ");
    Serial.println(shutterspeed);
  }
}
