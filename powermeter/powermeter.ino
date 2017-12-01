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

Adafruit_BLE_UART BTLEserial = Adafruit_BLE_UART(ADAFRUITBLE_REQ, ADAFRUITBLE_RDY, ADAFRUITBLE_RST);

HX711 scale(A1, A0); // omit 'gain' parameter - defaults to 128

Adafruit_L3GD20_Unified gyro = Adafruit_L3GD20_Unified(20);


void setup() {
  Serial.begin(9600);

  Serial.println("HX711 Demo");

  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());      // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));   // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set) divided 
            // by the SCALE parameter (not set yet)  

  scale.set_scale(2280.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();        

  Serial.println("After setting up the scale:");

  Serial.print("read: \t\t");
  Serial.println(scale.read());                 // print a raw reading from the ADC

//  Serial.print("read average: \t\t");
//  Serial.println(scale.read_average(20));       // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));    // print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided 
            // by the SCALE parameter set with set_scale

  Serial.println("Gyroscope Test"); Serial.println("");
  
  /* Enable auto-ranging */
  gyro.enableAutoRange(true);
  
  /* Initialise the sensor */
  if(!gyro.begin())
  {
    /* There was a problem detecting the L3GD20 ... check your connections */
    Serial.println("Ooops, no L3GD20 detected ... Check your wiring!");
    while(1);
  }
  BTLEserial.begin();
  Serial.println(sizeof(float));

}

aci_evt_opcode_t laststatus = ACI_EVT_DISCONNECTED;

void loop() {

    // Tell the nRF8001 to do whatever it should be working on.
  BTLEserial.pollACI();
  sensors_event_t event; 
  gyro.getEvent(&event);

  float z = event.gyro.z;
  float f = scale.get_units();

  float power;

//  Serial.print("x: ");
//  Serial.print(x);
//  Serial.print(" y: ");
//  Serial.print(y);
  Serial.print(" z: ");
  Serial.print(z);
  Serial.print(" f: ");
  Serial.println(f);

  // temp until Dan figures out if he wired up the strain gauges correctly
  if(f < 0.0000) {
    f = -f;
  }


  scale.power_down();              // put the ADC in sleep mode
  
  // Ask what is our current status
  aci_evt_opcode_t status = BTLEserial.getState();
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
    Serial.println("HELP");
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
      memcpy(sendbuffer, (unsigned char *) &f, sizeof(f));
      char sendbuffersize = sizeof(sendbuffer);

      Serial.println(f);
      // write the data
      BTLEserial.write(sendbuffer, sendbuffersize);
  }
  scale.power_up();
  delay(300);
}
