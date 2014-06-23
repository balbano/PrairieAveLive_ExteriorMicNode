/***********************************************
Microphone Node, Arduino Fio, Electret mic, XBEE
************************************************/

#include <XBee.h>
#include <SoftwareSerial.h>

SoftwareSerial softSerial(4, 5);

// SET UP XBEE
XBee xbee = XBee();

uint8_t payload[] = {0, 0}; // payload, micPins, numberOfMics and sampleMaxes need to all correspond.

XBeeAddress64 addr64 = XBeeAddress64(0x0, 0x0); // coord 0x0013a200, 0x40ace8df
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));

// SET UP AUDIO
const int numberOfMics = 2; // payload, micPins, numberOfMics and sampleMaxes need to all correspond.
const int micPins[] = {1, 2}; // payload, micPins, numberOfMics and sampleMaxes need to all correspond.
uint8_t sampleMaxes[2]; // payload, micPins, numberOfMics and sampleMaxes need to all correspond.

const uint8_t sampleFloor = 100; // Use mic_check sketch to determine value.
const uint8_t sampleCeiling = 190; // Use mic_check sketch to determine value.
const int sampleWindow = 40; // 25 FPS. (Panel runs at 12 FPS).
unsigned long prevMillis;

void setup() 
{
  Serial.begin(9600);
  xbee.setSerial(Serial);
  
  softSerial.begin(4800);
  
  prevMillis = millis();
}

void loop() 
{
  for (int i = 0; i < numberOfMics; i++){
    sampleMaxes[i] = 0;
  }
  
  // Always take at least one set of samples.
  do {
    for (int i = 0; i < numberOfMics; i++) {
      uint8_t rawSample = getSample(micPins[i]);
      if (rawSample > sampleMaxes[i]) {
        sampleMaxes[i] = rawSample;
      }
    }
  } while (millis() - prevMillis < sampleWindow);
  
  prevMillis = millis();
  
  for (int i = 0; i < numberOfMics; i++){
    uint8_t factoredSample = factorSample(sampleMaxes[i], sampleFloor, sampleCeiling);
    payload[i] = factoredSample;
  }
  
  softSerial.print(payload[0]);
  softSerial.print(", ");
  softSerial.println(payload[1]);
  
  xbee.send(zbTx);
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
