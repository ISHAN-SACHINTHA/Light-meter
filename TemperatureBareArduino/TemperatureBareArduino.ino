    /////////////////////////////////////////////////////////////////
   //           Low Power Arduino Thermometer            v1.02    //
  //         Get the latest version of the code here:            //
 //      http://educ8s.tv/low-power-arduino-thermometer         //
/////////////////////////////////////////////////////////////////

#include <OneWire.h>
#include "LowPower.h"
#include <DallasTemperature.h>
#include <LCD5110_Graph.h>

#define ONE_WIRE_BUS 7

char mode = 'c';   //c for degrees Celsius, f for degrees Fahrenheit
int LDRpin = 0;
int LDRPowerPin = 6;
int lightIntensity = 0;

LCD5110 lcd(8,9,10,12,11);

extern unsigned char BigNumbers[];
extern uint8_t temperatureIcon[];
extern uint8_t splash[];

char temperatureF[6];

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

float tempC = 0;
float tempF = 0;

void setup(void)
{
  pinMode(LDRPowerPin, OUTPUT);
  lcd.InitLCD();
  lcd.clrScr();
  lcd.drawBitmap(0, 0, splash, 84, 48);
  lcd.update();
  delay(2000);
  lcd.setFont(BigNumbers);
  sensors.begin();
}

void loop(void)
{ 
   digitalWrite(LDRPowerPin, HIGH);   
   delay(10);              
   lightIntensity = readLightIntensity();
   digitalWrite(LDRPowerPin, LOW);
   
  //Turn On/Off Screen
  if(lightIntensity<20)
  {
   lcd.enableSleep();
  }else
  {
    lcd.disableSleep();
    lcd.clrScr();
    lcd.drawBitmap(0, 0, temperatureIcon, 84, 48);

    sensors.requestTemperatures();
  
    tempC = sensors.getTempCByIndex(0);
    tempF = sensors.toFahrenheit(tempC);
  
    if(mode == 'f')
    {
       convertToString(tempF);
    }else if(mode == 'c')
    {
      convertToString(tempC); 
    } 
 
    lcd.print(temperatureF,25,11);
  
    lcd.update();  
    delay(50);
 }
  
  sleepForTwoMinutes();
}

void convertToString(float number)
{
   dtostrf(number, 3, 1, temperatureF);
}

void sleepForTwoMinutes()
{
  for(int i=0;i<15;i++)
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}
int readLightIntensity()
{
   int intensity = analogRead(LDRpin);
   intensity = map(intensity, 700, 8, 0, 100);
   return intensity;
}

