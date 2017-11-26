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

  Serial.print("one reading:\t");
  Serial.print(scale.get_units(), 1);
  Serial.println();

  scale.power_down();              // put the ADC in sleep mode
  
  // Ask what is our current status
  aci_evt_opcode_t status = BTLEserial.getState();
  // If the status changed....
  if (status != laststatus) {
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

  if (status == ACI_EVT_CONNECTED) {
    // Lets see if there's any data for us!
    if (BTLEserial.available()) {
      Serial.print("* "); Serial.print(BTLEserial.available()); Serial.println(F(" bytes available from BTLE"));
    }
//    // OK while we still have something to read, get a character and print it out
//    while (BTLEserial.available()) {
//      char c = BTLEserial.read();
//      Serial.print(c);
//    }

    // Next up, see if we have any data to get from the Serial console

//    if (Serial.available()) {
      // Read a line from Serial
//      Serial.setTimeout(100); // 100 millisecond timeout
//      String s = Serial.readString();

      // We need to convert the line to bytes, no more than 20 at this time
      uint8_t sendbuffer[sizeof(float)];
//      String s = "hello";
//      s.getBytes(sendbuffer, 20);
//      float f = 1.2f;
      float f = event.gyro.x;
      memcpy(sendbuffer, &f, sizeof(f));
//      char sendbuffersize = min(20, s.length());
      char sendbuffersize = sizeof(sendbuffer);
      
//      uint8_t sendbuffer[6];
//      sendbuffer[0] = (uint8_t) (event.gyro.x & 0xFF00) >> 8;
//      sendbuffer[1] = (uint8_t) (event.gyro.x & 0x00FF) >> 8;
//      sendbuffer[2] = (uint8_t) (event.gyro.y & 0xFF00) >> 8;
//      sendbuffer[3] = (uint8_t) (event.gyro.y & 0x00FF) >> 8;
//      sendbuffer[4] = (uint8_t) (event.gyro.z & 0xFF00) >> 8;
//      sendbuffer[5] = (uint8_t) (event.gyro.z & 0x00FF) >> 8;


      Serial.print(F("\n* Sending -> \"")); Serial.print((char *)sendbuffer); Serial.println("\"");

      // write the data
      BTLEserial.write(sendbuffer, sendbuffersize);
//    }
  }
  

//
//  sensors_event_t event; 
//  gyro.getEvent(&event);
// 
//  /* Display the results (speed is measured in rad/s) */
//  Serial.print("X: "); Serial.print(event.gyro.x); Serial.print("  ");
//  Serial.print("Y: "); Serial.print(event.gyro.y); Serial.print("  ");
//  Serial.print("Z: "); Serial.print(event.gyro.z); Serial.print("  ");
//  Serial.println("rad/s ");
//  Serial.println();
//  
//  delay(500);
  scale.power_up();
}
