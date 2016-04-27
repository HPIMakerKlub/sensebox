/*
SenseBox Home - Citizen Sensingplatform
Version: 2.1
Date: 2015-09-09
Homepage: http://www.sensebox.de
Author: Jan Wirwahn, Institute for Geoinformatics, University of Muenster
Note: Sketch for SenseBox Home Kit
Email: support@sensebox.de 
*/

#include <Wire.h>
#include "HDC100X.h"
#include "BMP280.h"
#include <Makerblog_TSL45315.h>
#include <SPI.h>
#include <Ethernet.h>
#include <avr/pgmspace.h>

//SenseBox ID
#define SENSEBOX_ID "5719c4037514d05c121e317c"

//Sensor IDs
#define TEMPSENSOR_ID "5719c4037514d05c121e3182"
#define HUMISENSOR_ID "5719c4037514d05c121e3181"
#define PRESSURESENSOR_ID "5719c4037514d05c121e3180"
#define LUXSENSOR_ID "5719c4037514d05c121e317f"
#define UVSENSOR_ID "5719c4037514d05c121e317e"

//Configure ethernet connection
IPAddress myIp(192, 168, 137, 42);
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "www.opensensemap.org";
EthernetClient client;

//Load sensors
Makerblog_TSL45315 TSL = Makerblog_TSL45315(TSL45315_TIME_M4);
HDC100X HDC(0x43);
BMP280 BMP;

//measurement variables
float temperature = 0;
float humidity = 0;
double tempBaro, pressure;
uint32_t lux;
uint16_t uv;
int messTyp;
#define UV_ADDR 0x38
#define IT_1   0x1

const unsigned long postingInterval = 60000;
unsigned long oldTime = -postingInterval;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  // start the Ethernet connection:
  Serial.println(F("SenseBox Home software version 2.1"));
  Serial.println();
  Serial.print(F("Starting ethernet connection..."));
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    Ethernet.begin(mac, myIp);
  }
  Serial.print(F("IP: "));
  Serial.print(Ethernet.localIP());
  Serial.print(F(" Gateway: "));
  Serial.println(Ethernet.gatewayIP());
  delay(1000);
  //Initialize sensors
  Serial.print(F("Initializing sensors..."));
  Wire.begin();
  Wire.beginTransmission(UV_ADDR);
  Wire.write((IT_1<<2) | 0x02);
  Wire.endTransmission();
  delay(500);
  HDC.begin(HDC100X_TEMP_HUMI,HDC100X_14BIT,HDC100X_14BIT,DISABLE);
  TSL.begin();
  BMP.begin();
  BMP.setOversampling(3);
  Serial.println(F("done!"));
  Serial.println(F("Starting loop."));
  temperature = HDC.getTemp();
}

void loop()
{
  // if there are incoming bytes available
  // from the server, read them and print them:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  if (millis() - oldTime > postingInterval) {
    oldTime = millis();   
    //-----Pressure-----//
    Serial.println(F("Posting pressure"));
    messTyp = 2;
    char result = BMP.startMeasurment();
    if(result!=0){
      delay(result);
      result = BMP.getTemperatureAndPressure(tempBaro,pressure);
      postObservation(pressure, PRESSURESENSOR_ID, SENSEBOX_ID); 
      Serial.print(F("Temp_baro = "));Serial.println(tempBaro,2);
      Serial.print(F("Pressure  = "));Serial.println(pressure,2);
    } else {
      Serial.print(F("Error: "));Serial.println((int)BMP.getError());
    }
    delay(2000); 
    //-----Humidity-----//
    Serial.println(F("Posting humidity"));
    messTyp = 2;
    humidity = HDC.getHumi();
    Serial.print(F("Humidity = ")); Serial.println(humidity);
    postObservation(humidity, HUMISENSOR_ID, SENSEBOX_ID); 
    delay(2000);
    //-----Temperature-----//
    Serial.println(F("Posting temperature"));
    messTyp = 2;
    temperature = HDC.getTemp();
    //Serial.println(temperature,2);
    Serial.print(F("Temperature = "));Serial.println(temperature);
    postObservation(temperature, TEMPSENSOR_ID, SENSEBOX_ID); 
    delay(2000);  
    //-----Lux-----//
    Serial.println(F("Posting illuminance"));
    messTyp = 1;
    lux = TSL.readLux();
    Serial.print(F("Illumi = ")); Serial.println(lux);
    postObservation(lux, LUXSENSOR_ID, SENSEBOX_ID);
    delay(2000);
    //UV intensity
    messTyp = 1;
    uv = getUV();
    Serial.print(F("UV = ")); Serial.println(uv);
    postObservation(uv, UVSENSOR_ID, SENSEBOX_ID);
  }
}

void postObservation(float measurement, String sensorId, String boxId){ 
  char obs[10]; 
  if (messTyp == 1) dtostrf(measurement, 5, 0, obs);
  else if (messTyp == 2) dtostrf(measurement, 5, 2, obs);
  Serial.println(obs); 
  //json must look like: {"value":"12.5"} 
  //post observation to: http://opensensemap.org:8000/boxes/boxId/sensorId
  Serial.println(F("connecting...")); 
  String value = "{\"value\":"; 
  value += obs; 
  value += "}";
  if (client.connect(server, 8000)) 
  {
    Serial.println(F("connected")); 
    // Make a HTTP Post request: 
    client.print(F("POST /boxes/")); 
    client.print(boxId);
    client.print(F("/")); 
    client.print(sensorId); 
    client.println(F(" HTTP/1.1")); 
    // Send the required header parameters 
    client.println(F("Host:opensensemap.org")); 
    client.println(F("Content-Type: application/json")); 
    client.println(F("Connection: close"));  
    client.print(F("Content-Length: ")); 
    client.println(value.length()); 
    client.println(); 
    // Send the data
    client.print(value); 
    client.println(); 
  } 
  waitForResponse();
}

void waitForResponse()
{ 
  // if there are incoming bytes available 
  // from the server, read them and print them: 
  boolean repeat = true; 
  do{ 
    if (client.available()) 
    { 
      char c = client.read();
      Serial.print(c); 
    } 
    // if the servers disconnected, stop the client: 
    if (!client.connected()) 
    {
      Serial.println();
      Serial.println(F("disconnecting.")); 
      client.stop(); 
      repeat = false; 
    } 
  }
  while (repeat);
}

uint16_t getUV(){
  byte msb=0, lsb=0;
  uint16_t uvValue;

  Wire.requestFrom(UV_ADDR+1, 1); //MSB
  delay(1);
  if(Wire.available()) msb = Wire.read();

  Wire.requestFrom(UV_ADDR+0, 1); //LSB
  delay(1);
  if(Wire.available()) lsb = Wire.read();

  uvValue = (msb<<8) | lsb;

  return uvValue*5;
}

