/* 
  Prairie Avenue Live - Exterior Mote
  ===================================
  
  The exterior mic node reads the volume data from each of the exterior
  mics and transmits it to the microcontroller running the LED panel.

  Terminology
  -----------
  
  - Mote: Arduino Fio and XBee collects sound levels from mic and transmits to controller.
    - There are multiple interior motes with one mic each and one exterior mote with multiple mics.
  - Mic: the electret mics used by the motes.
  - Controller: Teensy 3.1 and XBee that receive the sound data from the motes and control the LEDs.
  - Node: the origin point (on the LED array) of the visualization for a particular mic.
    
  Parts
  -----
  
  - Arduino Fio: https://www.sparkfun.com/products/10116
  - Electret mics with auto gain: http://www.adafruit.com/products/1713
  - XBee Series 2: http://www.adafruit.com/products/968

*/

#include <XBee.h>
#include <SoftwareSerial.h>

// NETWORK PARAMS
const int numberOfMics = 3;
// Controller alternates between polling an interior mote and the exterior
// mote. GoL updates each time. Therefore, the number of samples needs to be
// twice the number of mics to provide enough data to last until the
// next time the exterior mote is polled.
const int samplesPerTx = numberOfMics * 2;

// AUDIO
const int micPins[] = {1, 2, 1}; // XXX: Need to actually add 3rd mic.
uint8_t sampleMaxes[numberOfMics]; // Holds max levels during the sampling period.
const uint8_t sampleFloor = 100; // Use mic_check sketch to determine value.
const uint8_t sampleCeiling = 190; // Use mic_check sketch to determine value.

// XBEE
XBee xbee = XBee();
uint8_t payload[samplesPerTx];
XBeeAddress64 addr64 = XBeeAddress64(0x0, 0x0); // Coord's actual address: 0x0013a200, 0x40ace8df
uint16_t addr16;
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));

XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse();

// TIMERS
const int timeBetweenReports = 5000; // Report once per second.
unsigned long prevReportTime;

const int timeBetweenSamples = 100; // 10Hz.
unsigned long prevSampleTime;

// REPORTING
SoftwareSerial softSerial(4, 5); // RX, TX
int numberOfTransmissions;

void setup() 
{
  // Set up XBee.
  Serial.begin(57600);
  xbee.setSerial(Serial);
  zbTx.setFrameId(0); // No ACK.

  for (int i = 0; i < samplesPerTx; i++) {
    payload[i] = 0;
  }
  
  // Set up audio.
  for (int i = 0; i < numberOfMics; i++) {
    sampleMaxes[i] = 0;
  }

  // Set up reporting.
  softSerial.begin(57600);
  numberOfTransmissions = 0;

  // Timers.
  prevSampleTime = millis();
  prevReportTime = millis();
}

void loop() 
{
  // Collect audio samples.
  for (int i = 0; i < numberOfMics; i++) {
    uint8_t rawSample = getSample(micPins[i]);

    if (rawSample > sampleMaxes[i]) {
      sampleMaxes[i] = rawSample;
    }
  }

  // If it's the end of the sampling period, update the payload.
  if (millis() - prevSampleTime > timeBetweenSamples) {
    prevSampleTime = millis();
    // Shift the payload values to the left by the number of mics and append
    // the new maxes on the right.
    memcpy(&payload[0], &payload[numberOfMics], (numberOfMics) * sizeof(*payload));

    for (int i = 0; i < numberOfMics; i++) {
      payload[numberOfMics + i] = factorSample(sampleMaxes[i], sampleFloor, sampleCeiling);
      sampleMaxes[i] = 0; // Zero maxes for next sampling period.
    }
  }
  
  // Check for a pull request and send the data if one has been received.
  if (lookForData(255)) {
    xbee.send(zbTx);
    numberOfTransmissions++;
  }

  // If it's the end of the reporting period, print a report.
  if (millis() - prevReportTime > timeBetweenReports) {
    prevReportTime = millis();
    printReport();
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

bool lookForData(uint8_t expectedData) {
  // Look for a packet containing the expected data until the buffer is empty.
  // Clear the buffer if the expected data is found.
  xbee.readPacket();

  if (xbee.getResponse().isAvailable()) {
    if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      xbee.getResponse().getZBRxResponse(rx);
      uint8_t data = rx.getData()[0];

      if (data == expectedData) {
        addr16 = rx.getRemoteAddress16();

        // Clear out the buffer before returning.
        do {
          xbee.readPacket();
        } while (xbee.getResponse().isAvailable() || xbee.getResponse().isError());
        
        return true;
      }
    } 
    // If we got a packet, but it wasn't correct, try again in case the packet
    // we're after is further back in the buffer.
    return lookForData(expectedData);
  }
  if (xbee.getResponse().isError()) {
    return lookForData(expectedData);
  }
  return false; // Did not get the data.
}

void printReport() {
  softSerial.print(numberOfTransmissions);
  softSerial.print(" transmissions in last ");
  softSerial.print(timeBetweenReports);
  softSerial.println(" millis.");
}
