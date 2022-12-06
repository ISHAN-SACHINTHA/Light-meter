
// Set this to true if you wish to get "standard" shutter settings
#define STANRARD_SHUTTER_SPEED  false

// Calibration constant (C) for incident light meters.
// see https://en.wikipedia.org/wiki/Light_meter#Calibration_constants
#define INCIDENT_CALIBRATION    330


// Pre-defined aperture and ISO settings. Modify them for your own preference.
// Elements in the array have to be sorted from min to max.
const double APERATURE_TABLE[]  = {1.0, 1.4, 1.8, 2.0, 2.8, 3.5, 4.0, 4.5, 5.6, 6.3, 8.0, 11.0, 12.7, 16.0, 22.0, 32.0};
const int ISO_TABLE[]           = {6, 12, 25, 50, 100, 160, 200, 400, 800, 1600, 3200};

// The "shutterspeed table" array is used when STANRARD_SHUTTER_SPEED is set to true.
// 1/2 (0.5) seconds is expressed as 2, and 2 seconds is expressed as -2.
//double SHUTTERSPEED_TABLE[]     = {-60, -30, -15, -8, -4, -2, 1, 2, 4, 8, 15, 30, 60, 125, 250, 500, 1000, 2000, 4000};

// If you are using older cameras which have different shutter speed settings,
// you can switch to the folloing array:
const int SHUTTERSPEED_TABLE[]    = { 1, 2, 4, 8, 15, 30, 60, 125, 250, 500, 1000};

/* ------------------------------ */

#include <math.h>
#include <Wire.h>
#include <BH1750.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <ClickEncoder.h>
#include <TimerOne.h>
#include "LowPower.h"

ClickEncoder *encoder;
int16_t last, value;

int16_t rawValue, debounceEncoder;

BH1750 bh1750;
SSD1306AsciiAvrI2c oled;

bool started = false;
bool setting_mode = false;
bool setting = false;
int EV, shutrerIndex, shutrerIndexOld, isoIndex, isoIndexOld;
long lux;
int iso;
double aperature;
double shutterspeed;

uint8_t col0 = 0;  // First value column
uint8_t col1 = 0;  // Last value column.
uint8_t rows;      // Rows per line.

void setup() {

  const char* label[] = {"EV:", "ISO:", "F-num:", "SS:"};
  // initialize serial port
  Serial.begin(9600);
  encoder = new ClickEncoder(A1, A0, 3, 4);
  encoder->setAccelerationEnabled(true);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);
  Wire.begin();

  //  // prepare the shutterspeed table array if needed
  //  if (STANRARD_SHUTTER_SPEED) {
  //    for (int i = 0; i < (sizeof(SHUTTERSPEED_TABLE) / sizeof(double)); i++) {
  //      if (SHUTTERSPEED_TABLE[i] < 0) {
  //        SHUTTERSPEED_TABLE[i] = 1 / abs(SHUTTERSPEED_TABLE[i]);
  //      }
  //    }
  //  }

  // setup button````
  pinMode(3, INPUT_PULLUP);

  // set the BH1750 in BH1750_ONE_TIME_HIGH_RES_MODE_2
  bh1750.begin();

  // "initialize" BH1750 by using it once
  bh1750.readLightLevel();

  // initialize OLED
  oled.begin(&Adafruit128x64, 0x3c);
  oled.setFont(System5x7);
  oled.set2X();

  Serial.println("ArduMeter (Arduino incident light meter for camera) READY:");
  //  Serial.println("- Press button to measure light.");
  //  Serial.println("- Turn knobs to change aperature/ISO.");

  oled.clear();
  oled.println("  Arduino");
  oled.println("   Light");
  oled.println("   Meter");
  delay(2000);

  oled.clear();
  oled.set2X();

  // Setup form and find longest label.
  for (uint8_t i = 0; i < 4; i++) {
    oled.println(label[i]);
    uint8_t w = oled.strWidth(label[i]);
    col0 = col0 < w ? w : col0;
  }
  // Three pixels after label.
  //col0 += 3;
  // Allow two or more pixels after value.
  col1 = col0 + oled.strWidth("99.9") + 2;
  // Line height in rows.
  rows = oled.fontRows();

  delay(3000);


}

void timerIsr() {
  encoder->service();
}

void clearValue(uint8_t row) {
  oled.clear(col0, col1, row, row + rows - 1);
}

void loop() {

  getKnobIndex();

  clearValue(0);
  oled.print(EV, 1);
  clearValue(rows);
  oled.print(iso);
  clearValue(2 * rows);
  oled.print(aperature, 1);
  clearValue(3 * rows);
  oled.print(shutterspeed, 0);
  delay(1000);

  rawValue = encoder->getValue();
  debounceEncoder += abs(rawValue);

  if (debounceEncoder >= 2) {
    debounceEncoder = 0;
  } else {
    rawValue = 0;
  }

  if (rawValue == 1) {
    debounceEncoder += 1;
  }

  //  if (buttonPressed()) { //measure light
  //    if (!started) started = true;
  //    displayExposureSetting(true);
  //    delay(150);
  //
  //  } else { //change aperture/ISO settings
  //    if ((apertureIndex != apertureIndexOld || isoIndex != isoIndexOld) && started) {
  //      displayExposureSetting(false);
  //      delay(150);
  //    }
  //  }

  shutrerIndexOld = shutrerIndex;
  isoIndexOld = isoIndex;

  delay(50);

  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    switch (b) {
      case ClickEncoder::Pressed:
        break;
      case ClickEncoder::Clicked:
        if (!started) started = true;
        displayExposureSetting(true);
        delay(150);
        break;
      case ClickEncoder::DoubleClicked:
        if (setting_mode == false) {
          setting_mode = true;
          Serial.println("ISO");
        }
        else {
          setting_mode = false;
          Serial.println("Shutter speed");
        }
        break;
      case ClickEncoder::Released:

        if (setting == false) {
          setting = true;
          Serial.println("setting mode ON");
        }
        else {
          setting = false;
          Serial.println("setting mode OFF");
        }
        if ((shutrerIndex != shutrerIndexOld || isoIndex != isoIndexOld) && started) {
          displayExposureSetting(false);
          delay(150);

        }
        break;

    }
  }
}


// map knob analog readings to array indexes
void getKnobIndex() {
  if (setting == true && setting_mode == false) {
    if (shutrerIndex <= 0) {
      shutrerIndex = 0;
    }
    if (shutrerIndex >= sizeof(SHUTTERSPEED_TABLE) / sizeof(int)) {
      shutrerIndex = 10;
    }
    shutrerIndex += rawValue;
    Serial.print("shutter speed=");
    Serial.println(shutrerIndex);
    shutterspeed = SHUTTERSPEED_TABLE[shutrerIndex];
    clearValue(2 * rows);
    oled.print(shutterspeed, 1);

  }
  if (setting == true && setting_mode == true) {

    if (isoIndex <= 0) {
      isoIndex = 0;
    }
    if (isoIndex >= sizeof(ISO_TABLE) / sizeof(int)) {
      isoIndex = 10;
    }
    isoIndex += rawValue;
    Serial.print("ISO=");
    Serial.println(isoIndex);
    iso = ISO_TABLE[isoIndex];
    clearValue(rows);
    oled.print(iso);
  }
}

// measure/calculate and display exposure settings
void displayExposureSetting(bool measureNewEV) {

  shutterspeed = SHUTTERSPEED_TABLE[shutrerIndex];
  iso = ISO_TABLE[isoIndex];

  // measure light level (illuminance) and get a new lux value
  if (measureNewEV) {
    lux = bh1750.readLightLevel();
    Serial.print("Measured illuminance = ");
    Serial.print(lux);
    Serial.println(" lux");
  }

  //calculate EV
  EV = log10(lux * iso / INCIDENT_CALIBRATION) / log10(2);

  if (isfinite(EV)) { //calculate shutter speed if EV is neither NaN nor infinity

    // calculate shutter speed
    //shutterspeed = (pow(2, EV) / pow(aperature, 2));

    //calculate aperture
    aperature = sqrt(pow(2, EV) / shutterspeed);

    // choose standard shutter speed if needed
    //    if (STANRARD_SHUTTER_SPEED) {
    //      for (int i = 0; i < (sizeof(SHUTTERSPEED_TABLE) / sizeof(double)) - 1; i++) {
    //        if (shutterspeed >= SHUTTERSPEED_TABLE[i] && shutterspeed <= SHUTTERSPEED_TABLE[i + 1]) {
    //          if (abs(shutterspeed - SHUTTERSPEED_TABLE[i]) <= abs(shutterspeed) - SHUTTERSPEED_TABLE[i + 1]) {
    //            shutterspeed = SHUTTERSPEED_TABLE[i];
    //          } else {
    //            shutterspeed = SHUTTERSPEED_TABLE[i + 1];
    //          }
    //          break;
    //        }
    //      }
    //    }

    // output result to serial port
    Serial.print("Exposure settings: EV = ");
    Serial.print(EV);
    Serial.print(", ISO = ");
    Serial.print(iso);
    Serial.print(", aperture = f/");
    Serial.print(aperature, 1);
    Serial.print(", ");
    Serial.print("shutter speed = ");
    Serial.println(shutterspeed);

    //    // output result to OLED
    //    oled.clear();
    //    //oled.print("EV: ");
    //    oled.println(EV, 1);
    //    //oled.print("ISO: ");
    //    oled.println(iso);
    //    //oled.print("-- f/");
    //    oled.println(aperature, 1);
    //    //oled.print("-- ");
    //    if (shutterspeed > 1) {
    //      oled.print("1/");
    //      oled.print(shutterspeed, 0);
    //    } else {
    //      oled.print((1 / shutterspeed), 0);
    //    }
    //    oled.println("s");

  } else {
    Serial.println("Exposure out of bounds");
    //    oled.clear();
    //    oled.println("Exposure");
    //    oled.println("value");
    //    oled.println("out of");
    //    oled.println("bounds");
  }
}
