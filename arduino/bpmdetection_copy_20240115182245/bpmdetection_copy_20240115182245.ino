/*
  Optical Heart Rate Detection (PBA Algorithm) using the MAX30105 Breakout
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 2nd, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  This is a demo to show the reading of heart rate or beats per minute (BPM) using
  a Penpheral Beat Amplitude (PBA) algorithm.

  It is best to attach the sensor to your finger using a rubber band or other tightening
  device. Humans are generally bad at applying constant pressure to a thing. When you
  press your finger against the sensor it varies enough to cause the blood in your
  finger to flow differently which causes the sensor readings to go wonky.

  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected

  The MAX30105 Breakout can handle 5V or 3.3V I2C logic. We recommend powering the board with 5V
  but it will also run at 3.3V.
*/

//QUANDO MANDAR A MENSAGEM COM O BPM AVG ???

#include <Wire.h>
#include "MAX30105.h"

#include "heartRate.h"

MAX30105 particleSensor;

enum State {
  IDLE,
  READING,
  ERROR
};

const byte RATE_SIZE = 5;         //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];            //Array of heart rates
byte rateIndex = 0;

int fingerTimeout = 2000;            // 2 second timeout for finger detection (2000 milli seconds)
int irReportInterval = 1000;          // send IR value every 500ms when finger is detected (IR > 50000)

unsigned long lastBeat = 0;                //Time at which the last beat occurred
unsigned long lastFingerDetected = 0;      //Time at which the last finger contact detected
unsigned long lastIrReported = 0;

State lastState = IDLE;

float beatsPerMinute;
int beatAvg;

int beatCounter = 0;

bool isFirstBeat = true;


void setup() {
  Serial.begin(115200);
  while (!Serial); //wait for comm to stablish

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) { //Use default I2C port, 400kHz speed 
    Serial.print("error");
    Serial.print('\n');
    while (1);
  }
  
  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  Serial.print("init");
  Serial.print('\n');
}

void reset() {
  for (byte i = 0; i < RATE_SIZE; i++) {
    rates[i] = 0;
  }
  isFirstBeat = true;
  beatCounter = 0;
  rateIndex = 0;
  lastFingerDetected = 0;
  lastBeat = 0;
  lastIrReported = 0;
  lastState = IDLE;
}

void loop() {
  long irValue = particleSensor.getIR();

  //ENDING CONDITIONAL time elapsed > 2000 milli seconds 
  if(lastState == READING){
    unsigned long timeElapsed = millis() - lastFingerDetected;

    if (timeElapsed > fingerTimeout) {
      reset();
      Serial.print("end");
      Serial.print('\n');
    }
  }

  // if finger not detected, return
  if (irValue < 50000) {
    //Serial.print("ir < 50000, returning");
    //Serial.print('\n');
    return;
  }

  // finger detected, update state, if necessary
  lastFingerDetected = millis();
  if(lastState == IDLE) {
    Serial.print("start");
    Serial.print('\n');
    lastState = READING;
    isFirstBeat = true;
  }

  if ((millis() - lastIrReported) > irReportInterval) {
    if (lastIrReported != 0) {
      Serial.print("ir_" + String(irValue));
      Serial.print("\n");
    }
    lastIrReported = millis();
  }
  
  // if beat not detected, return
  if (!checkForBeat(irValue)) {
    return;
  }

  // beat detected
  unsigned long delta = millis() - lastBeat;
  lastBeat = millis();

  // ignore first reading as it's always way off
  if (isFirstBeat) {
    Serial.print("first beat, ignoring");
    Serial.print('\n');
    isFirstBeat = false;
    return;
  }
  
  beatsPerMinute = 60 / (delta / 1000.0);
  Serial.print("Single: " + String(beatsPerMinute) + "\n");
  

  // ignore off readings
  if (beatsPerMinute < 30 || beatsPerMinute > 170) {
    Serial.print("individual beat out of range, returning: " + String(beatsPerMinute));
    Serial.print('\n');
    return;
  }

  // valid reading
  beatCounter++;
  //Serial.print(String(beatCounter));
  //Serial.print('\n');
  rates[rateIndex++] = (byte)beatsPerMinute;       //Store this reading in the array
  rateIndex %= RATE_SIZE;                          //Wrap variable

  // if not enough readings for an average, return
  if (beatCounter < RATE_SIZE) {
    return;
  }

  //Take the moving average of readings
  beatAvg = 0;
  for (byte x = 0 ; x < RATE_SIZE ; x++) {
    beatAvg += rates[x];
  }

  beatAvg /= RATE_SIZE;
  Serial.print("bpm_" + String(beatAvg));
  Serial.print("\n");
}











//Serial.print("rangeError");
//Serial.print('\n');


//  Serial.print("IR=");
//  Serial.print(irValue);
//  Serial.print(", BPM=");
//  Serial.print(beatsPerMinute);
//  Serial.print(", Avg BPM=");
// Serial.print(beatAvg);