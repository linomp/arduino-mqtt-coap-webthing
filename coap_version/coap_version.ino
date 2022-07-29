/*
  rp2040_webthing - final project of Enabling Technologies for IIoT Summer School, 2022

  This version of the arduino code communicates through the CoAP protocol  

  Required hardware:
  - Arduino Nano rp2040 connect

*/

#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <WiFiNINA.h>

#include <coap-simple.h>

#include "arduino_secrets.h"

#include "LSM6DSOXSensor.h"
#include "lsm6dsox_activity_recognition_for_mobile.h"

#ifdef ARDUINO_SAM_DUE
#define DEV_I2C Wire1
#elif defined(ARDUINO_ARCH_STM32)
#define DEV_I2C Wire
#elif defined(ARDUINO_ARCH_AVR)
#define DEV_I2C Wire
#else
#define DEV_I2C Wire
#endif

#define INT_1 INT_IMU

// Components
LSM6DSOXSensor AccGyr(&DEV_I2C, LSM6DSOX_I2C_ADD_L);

// MLC
ucf_line_t *ProgramPointer;
int32_t LineCounter;
int32_t TotalNumberOfLine;

volatile int mems_event = 0;

void INT1Event_cb();
void coapPutRequest(uint8_t status);

// Wifi & CoAP client configurations
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
WiFiClient wifiClient;
WiFiUDP Udp;
Coap coap(Udp);

void callback_response(CoapPacket &packet, IPAddress ip, int port);

void setup() {

  /*** Initialize MLC stuff ***/
  uint8_t mlc_out[8];
  pinMode(LED_BUILTIN, OUTPUT);

  // Force INT1 of LSM6DSOX low in order to enable I2C
  pinMode(INT_1, OUTPUT);
  digitalWrite(INT_1, LOW);
  delay(200);

  // Initialize serial for output.
  Serial.begin(115200);
  
  // Initialize I2C bus.
  DEV_I2C.begin();
  AccGyr.begin();
  AccGyr.Enable_X();
  AccGyr.Enable_G();

  // Feed the program to Machine Learning Core
  // Activity Recognition Default program
  ProgramPointer = (ucf_line_t *)lsm6dsox_activity_recognition_for_mobile;
  TotalNumberOfLine = sizeof(lsm6dsox_activity_recognition_for_mobile) / sizeof(ucf_line_t);
  Serial.println("Activity Recognition for LSM6DSOX MLC");
  Serial.print("UCF Number Line=");
  Serial.println(TotalNumberOfLine);

  for (LineCounter=0; LineCounter<TotalNumberOfLine; LineCounter++) {
    if(AccGyr.Write_Reg(ProgramPointer[LineCounter].address, ProgramPointer[LineCounter].data)) {
      Serial.print("Error loading the Program to LSM6DSOX at line: ");
      Serial.println(LineCounter);
      while(1) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(250);
        digitalWrite(LED_BUILTIN, LOW);
        delay(250);
      }
    }
  }

  Serial.println("Program loaded inside the LSM6DSOX MLC");
  pinMode(INT_1, INPUT);
  attachInterrupt(INT_1, INT1Event_cb, RISING);

  // We need to wait for a time window before having the first MLC status
  delay(3000);
  AccGyr.Get_MLC_Output(mlc_out);
 
  /*** Initialize Wifi & CoAP Stuff ***/

  // attempt to connect to WiFi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }

  Serial.println("You're connected to the network");
  Serial.println();

  Serial.print("My IP address: ");
  Serial.print(WiFi.localIP());
  Serial.println();

  // client response callback, this endpoint is single callback.
  Serial.println("Setup Response Callback");
  coap.response(callback_response);

  // start coap server/client
  coap.start();

  // Send the first status obtained 
  coapPutRequest(mlc_out[0]);
}

void loop() {
  if (mems_event) {
    mems_event=0;
    LSM6DSOX_MLC_Status_t status;
    AccGyr.Get_MLC_Status(&status);
    if (status.is_mlc1) {
      uint8_t mlc_out[8];
      AccGyr.Get_MLC_Output(mlc_out);
      coapPutRequest(mlc_out[0]);
    }
  }

  coap.loop();
}

// Interrupt to indicate MEMS event
void INT1Event_cb() {
  mems_event = 1;
}

void coapPutRequest(uint8_t status){
    String payload;
    
    switch(status) {
      case 0:
        payload = "Stationary";
        break;
      case 1:
        payload = "Walking";
        break;
      case 4:
        payload = "Jogging";
        break;
      case 8:
        payload = "Biking";
        break;
      case 12:
        payload = "Driving";
        break;
      default:
        payload = "Unknown";
        break;
    }

    int str_len = payload.length() + 1; 
    char payload_as_char_array[str_len];
    payload.toCharArray(payload_as_char_array, str_len);
    
    Serial.print("Sending payload - ");
    Serial.println(payload_as_char_array);

    // Predefined IP corresponds to my personal server on DigitalOcean
    coap.put(IPAddress(165,227,107,127), 5683, "Activity", payload_as_char_array);
}

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  //Serial.println("[Coap Response]");

  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  Serial.println(p);
}
