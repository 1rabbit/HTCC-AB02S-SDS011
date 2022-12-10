//VER 22.12.10

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "SdsDustSensor.h"
#define __DEBUG_SDS_DUST_SENSOR__

#include "LoRaWan_APP.h"
#include "Arduino.h"

#define _BoardRxPin GPIO6
#define _BoardTxPin GPIO7

// SDS011 with softSerial based communication for Heltec cubecell boards: 
softSerial     softwareSerial(_BoardTxPin,_BoardRxPin); // sds(-> SDS011-RXD, -> SDS011-TXD)
SdsDustSensor  sds(softwareSerial);  

#define ADC1_BATTERY_CALIBRATION 15.806 //1.181
#define DEVICE_RESTART_PERIOD 7 //Days
#define TX_DUTY_CYCLE 30 //x10s

uint8_t devEui[] = {  };
uint8_t appEui[] = {  };
uint8_t appKey[] = {  };

uint8_t nwkSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint32_t devAddr =  ( uint32_t )0x00000000;
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t  loraWanClass = LORAWAN_CLASS;
bool overTheAirActivation = LORAWAN_NETMODE;
bool loraWanAdr = LORAWAN_ADR;
bool keepNet = LORAWAN_NET_RESERVE;
bool isTxConfirmed = LORAWAN_UPLINKMODE;
uint8_t confirmedNbTrials = 4;
bool sleepMode = false;

uint32_t appTxDutyCycle = TX_DUTY_CYCLE;  
uint8_t appPort = 50;
int frameCount = 0;

int voltageBattMV = 0;
int pm25encoded = 0;
int pm10encoded = 0;

/////////// HANDLE DOWNLINKS ////////////
void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
  frameCount = 0; //Auto-restart Reset
  uint8_t cmd = mcpsIndication->Buffer[0];

  if (mcpsIndication->Port == 3) appTxDutyCycle = cmd;
  if (mcpsIndication->Port == 5) LoRaWAN.setDataRateForNoADR(cmd);
}


static void prepareTxFrame()
{
  appDataSize = 0;

  int analogReadX = 0;
  for(int i=0;i<50;i++) analogReadX+=analogRead(ADC1);
	voltageBattMV = ( analogReadX / 50 ) * ADC1_BATTERY_CALIBRATION;

  sds.begin(); // initalize SDS011 sensor
  sds.wakeup();
  sds.setQueryReportingMode(); // ensures sensor is in 'query' reporting mode
  delay(10000); // working some seconds to clean the internal sensor (recommended 30 seconds= 30000)
  PmResult pm = sds.queryPm();
  if (pm.isOk()) {
    pm25encoded = pm.pm25 * 10;
    pm10encoded = pm.pm10 * 10;
  }
  sds.sleep();
  detachInterrupt(_BoardRxPin); // prevents system hang after switching power off

  appData[appDataSize++] = (uint8_t)(voltageBattMV >> 8);
  appData[appDataSize++] = (uint8_t)(voltageBattMV);

  appData[appDataSize++] = (uint8_t)(pm25encoded >> 8);
  appData[appDataSize++] = (uint8_t)(pm25encoded);

  appData[appDataSize++] = (uint8_t)(pm10encoded >> 8);
  appData[appDataSize++] = (uint8_t)(pm10encoded);

}
void setup() {
  boardInitMcu();
}

void loop() {
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
      LoRaWAN.init(loraWanClass,loraWanRegion);
      
      deviceState = DEVICE_STATE_JOIN;
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      int cyclesPerDay = (8640 / (appTxDutyCycle));

      frameCount++;
      if (frameCount > (cyclesPerDay * DEVICE_RESTART_PERIOD)) CySoftwareReset();

      prepareTxFrame();
      LoRaWAN.send();
      
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      if (!sleepMode)
      {
         LoRaWAN.cycle(appTxDutyCycle * 10000);
      }
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep();   
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}