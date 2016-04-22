/*
 * Jan Wirwahn, Institute for Geoinformatics, University of Muenster
 * January 2016 
 * Test for SenseBox Home Sensors
 */ 

#include <Wire.h>
#include <HDC100X.h>
#include <Makerblog_TSL45315.h>
#include "BMP280.h"

#define I2C_ADDR 0x38
#define IT_1   0x1 //1T

HDC100X HDC1(0x43);
BMP280 bmp;
Makerblog_TSL45315 luxsensor = Makerblog_TSL45315(TSL45315_TIME_M4);

uint32_t lux;

void setup() {
  Serial.begin(9600);
  Serial.println("Starte Sensortest...");
  
  HDC1.begin(HDC100X_TEMP_HUMI,HDC100X_14BIT,HDC100X_14BIT,DISABLE);
  
  luxsensor.begin();
  bmp.begin();
  bmp.setOversampling(4);
  
  Wire.begin();
  Wire.beginTransmission(I2C_ADDR);
  Wire.write((IT_1<<2) | 0x02);
  Wire.endTransmission();
  delay(500);
}

void loop() {
  Serial.print("Luftfeuchte:         ");Serial.print(HDC1.getHumi());Serial.println(" %");
  delay(200);
  
  Serial.print("Temperature:         ");Serial.print(HDC1.getTemp());Serial.println(" C");
  delay(200);
  
  double T,P;
  char result = bmp.startMeasurment();
  delay(result);
  bmp.getTemperatureAndPressure(T,P);
  Serial.print("Luftdruck:           ");Serial.print(P,2); Serial.println(" hPa");
  delay(200);

  lux = luxsensor.readLux();
  Serial.print("Beleuchtungsstaerke: ");
  Serial.print(lux, DEC);
  Serial.println(" lx");
  
  byte msb=0, lsb=0;
  uint16_t uv;

  Wire.requestFrom(I2C_ADDR+1, 1); //MSB
  delay(1);
  if(Wire.available())
    msb = Wire.read();

  Wire.requestFrom(I2C_ADDR+0, 1); //LSB
  delay(1);
  if(Wire.available())
    lsb = Wire.read();

  uv = (msb<<8) | lsb;
  uv = uv * 5.625;
  Serial.print("UV-Strahlung:        ");Serial.print(uv, DEC);Serial.println(" uW/cm2"); //output in steps (16bit)
  
  Serial.println();
  delay(2000);
}
