/* 
Prairie Avenue Live - Exterior Microphone Node
==============================================

The exterior mic node reads the volume data from each of the exterior
mics and transmits it to the microcontroller running the LED panel.

Parts
-----

- Arduino Fio: https://www.sparkfun.com/products/10116
- Electret mics with auto gain: http://www.adafruit.com/products/1713
- XBee Series 2: http://www.adafruit.com/products/968

*/

#include <XBee.h>
#include <SoftwareSerial.h>

// AUDIO
const int numberOfMics = 2;
const int micPins[] = {1, 2};
uint8_t sampleMaxes[numberOfMics];
const uint8_t sampleFloor = 100; // Use mic_check sketch to determine value.
const uint8_t sampleCeiling = 190; // Use mic_check sketch to determine value.

// XBEE
XBee xbee = XBee();
uint8_t payload[numberOfMics]; // payload, micPins, numberOfMics and sampleMaxes need to all correspond.
XBeeAddress64 addr64 = XBeeAddress64(0x0, 0x0); // coord 0x0013a200, 0x40ace8df
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));

// TIMERS
const int timeBetweenTransmissions = 40; // 25 FPS. (Panel runs at 12 FPS).
unsigned long prevTransmissionTime;

const int timeBetweenReports = 1000; // Report once per second.
unsigned long prevReportTime;

// REPORTING
SoftwareSerial softSerial(4, 5); // RX, TX
int numberOfTransmissions;

void setup() 
{
  Serial.begin(9600);
  xbee.setSerial(Serial);
  
  softSerial.begin(4800);
  
  // Initialize the timers and counters;
  prevTransmissionTime = millis();
  prevReportTime = millis();
  numberOfTransmissions = 0;
  for (int i = 0; i < numberOfMics; i++){
    sampleMaxes[i] = 0;
  }
}

void loop() 
{
  for (int i = 0; i < numberOfMics; i++) {
    uint8_t rawSample = getSample(micPins[i]);
    if (rawSample > sampleMaxes[i]) {
      sampleMaxes[i] = rawSample;
    }
  }
  
  if (millis() - prevTransmissionTime > timeBetweenTransmissions) {
    prevTransmissionTime = millis();
    for (int i = 0; i < numberOfMics; i++){
      uint8_t factoredSample = factorSample(sampleMaxes[i], sampleFloor, sampleCeiling);
      payload[i] = factoredSample;
    }
    xbee.send(zbTx);
    numberOfTransmissions++;
    for (int i = 0; i < numberOfMics; i++){
      sampleMaxes[i] = 0;
    }
  }
  
  if (millis() - prevReportTime > timeBetweenReports) {
    prevReportTime = millis();
    softSerial.println(numberOfTransmissions);
    numberOfTransmissions = 0;
  }
}

uint8_t getSample(int pin) {
  int sample = analogRead(pin);
  sample = map(sample, 0, 1023, 0, 255);
  return uint8_t(constrain(sample, 0, 255));
}

uint8_t factorSample(uint8_t rawSample, uint8_t thresholdLow, uint8_t thresholdHigh) {
  int factoredSample = map(rawSample, thresholdLow, thresholdHigh, 0, 255);
  return uint8_t(constrain(factoredSample, 0, 255));
}
