#include "HX711.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_L3GD20_U.h>
#include <SPI.h>
#include "Adafruit_BLE_UART.h"

// HX711.DOUT    - pin #A1
// HX711.PD_SCK - pin #A0
// nrf8001.REQ  - pin #D10
// nrf8001.RST  - pin #D9
// nrf8001.RDY  - pin #D2

#define ADAFRUITBLE_REQ 10
#define ADAFRUITBLE_RDY 2     // This should be an interrupt pin, on Uno thats #2 or #3
#define ADAFRUITBLE_RST 9

const float CRANK_ARM_LENGTH = 0.175;
const float KG_CONVERSION = 0.45359237;
Adafruit_BLE_UART BTLEserial = Adafruit_BLE_UART(ADAFRUITBLE_REQ, ADAFRUITBLE_RDY, ADAFRUITBLE_RST);

HX711 scale(A0, A1); // omit 'gain' parameter - defaults to 128

Adafruit_L3GD20_Unified gyro = Adafruit_L3GD20_Unified(20);
float calibration_factor = 9500;


void setup() {
  Serial.begin(9600);

  scale.set_scale();                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();        
  
  /* Enable auto-ranging */
  gyro.enableAutoRange(true);
  
  /* Initialise the sensor */
  if(!gyro.begin())
  {
    /* There was a problem detecting the L3GD20 ... check your connections */
    Serial.println("Ooops, no L3GD20 detected ... Check your wiring!");
    while(1);
  }

  // begin serial connection
  BTLEserial.begin();
}

aci_evt_opcode_t laststatus = ACI_EVT_DISCONNECTED;

void loop() {

    // Tell the nRF8001 to do whatever it should be working on.
  BTLEserial.pollACI();
  aci_evt_opcode_t status = BTLEserial.getState();

  scale.set_scale(calibration_factor); //Adjust to this calibration factor


  sensors_event_t event; 
  gyro.getEvent(&event);

  float z = event.gyro.z;
  float x = event.gyro.x;
  float y = event.gyro.y;
  float f = scale.get_units();

  float power = 0.0;

  Serial.print("x: ");
  Serial.print(x);
  Serial.print(" y: ");
  Serial.print(y);
  Serial.print(" z: ");
  Serial.print(z);
  Serial.print(" f: ");

  // temp until Dan figures out if he wired up the strain gauges correctly
  if(f < 0.0000) {
    f = -f;
  }

  Serial.println(f);

  
  // Ask what is our current status
  // If the status changed....
  if (status != laststatus) {
    Serial.println("status: " + status);
    // print it out!
    if (status == ACI_EVT_DEVICE_STARTED) {
        Serial.println(F("* Advertising started"));
    }
    if (status == ACI_EVT_CONNECTED) {
        Serial.println(F("* Connected!"));
    }
    if (status == ACI_EVT_DISCONNECTED) {
        Serial.println(F("* Disconnected or advertising timed out"));
    }
    // OK set the last status change to this one
    laststatus = status;
  } 

  if(status == ACI_EVT_DISCONNECTED) {
    Serial.println("Bluetooth disconnected");
  }

  if (status == ACI_EVT_CONNECTED) {
    // Lets see if there's any data for us!
    if (BTLEserial.available()) {
      Serial.print("* "); Serial.print(BTLEserial.available()); Serial.println(F(" bytes available from BTLE"));
    }
    // OK while we still have something to read, get a character and print it out
    while (BTLEserial.available()) {
      char c = BTLEserial.read();
      if(c == 't') {
        scale.tare();
      }
      Serial.print(c);
    }

      uint8_t sendbuffer[sizeof(float)];
      power = 2*f*z*CRANK_ARM_LENGTH*KG_CONVERSION;
      memcpy(sendbuffer, (unsigned char *) &power, sizeof(f));
      char sendbuffersize = sizeof(sendbuffer);

      // write the data
      BTLEserial.write(sendbuffer, sendbuffersize);
  }
  delay(500);
}
