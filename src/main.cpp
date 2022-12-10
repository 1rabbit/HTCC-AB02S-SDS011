// testprogram to check basic functions to get data from SDS011 pm sensor
// to work with Heltec softSerial function SDS011 source must be modified
// default SoftwareSerial must be replaced with softSerial

// Heltec Cubecell HTCC AB01: https://heltec.org/project/htcc-ab01/
#include "SdsDustSensor.h" // https://github.com/lewapek/sds-dust-sensors-arduino-library
#define __DEBUG_SDS_DUST_SENSOR__

#define _BoardRxPin GPIO7
#define _BoardTxPin GPIO6

// SDS011 with softSerial based communication for Heltec cubecell boards: 
softSerial     softwareSerial(_BoardTxPin,_BoardRxPin); // sds(-> SDS011-RXD, -> SDS011-TXD)
SdsDustSensor  sds(softwareSerial);  


void setup() {
  Serial.begin(115200);

}

void loop() {

  sds.begin(); // initalize SDS011 sensor
  sds.wakeup();
  sds.setQueryReportingMode(); // ensures sensor is in 'query' reporting mode
  
  delay(5000); // working some seconds to clean the internal sensor (recommended 30 seconds= 30000)
  PmResult pm = sds.queryPm();
  if (pm.isOk()) {
    Serial.print("PM2.5 = ");
    Serial.print(pm.pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm.pm10);
  }

  sds.sleep();
  detachInterrupt(_BoardRxPin); // prevents system hang after switching power off

  delay(5000);

}