/*
  rp2040_webthing - final project of Enabling Technologies for IIoT Summer School, 2022
  
  Required hardware:
  - Arduino Nano rp2040 connect

  Author: Lino Mediavilla
*/

#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>

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
#define SerialPort Serial

#define INT_1 INT_IMU

//Interrupts.
volatile int mems_event = 0;

// Components
LSM6DSOXSensor AccGyr(&DEV_I2C, LSM6DSOX_I2C_ADD_L);

// MLC
ucf_line_t *ProgramPointer;
int32_t LineCounter;
int32_t TotalNumberOfLine;

void INT1Event_cb();
void publishMqttMessage(uint8_t status);

// Wifi & Mqtt client configurations
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
const char broker[] = "apps.xmp.systems";
int        port     = 1883;
const char topic[]  = "iiot-project/test";

// Time interval settings for old mqtt example
const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;

void setup() {
  /*** Initialize MLC stuff ***/
  uint8_t mlc_out[8];
  pinMode(LED_BUILTIN, OUTPUT);
  // Force INT1 of LSM6DSOX low in order to enable I2C
  pinMode(INT_1, OUTPUT);
  digitalWrite(INT_1, LOW);
  delay(200);

  // Initialize serial for output.
  SerialPort.begin(115200);
  
  // Initialize I2C bus.
  DEV_I2C.begin();
  AccGyr.begin();
  AccGyr.Enable_X();
  AccGyr.Enable_G();

  /* Feed the program to Machine Learning Core */
  /* Activity Recognition Default program */  
  ProgramPointer = (ucf_line_t *)lsm6dsox_activity_recognition_for_mobile;
  TotalNumberOfLine = sizeof(lsm6dsox_activity_recognition_for_mobile) / sizeof(ucf_line_t);
  SerialPort.println("Activity Recognition for LSM6DSOX MLC");
  SerialPort.print("UCF Number Line=");
  SerialPort.println(TotalNumberOfLine);

  for (LineCounter=0; LineCounter<TotalNumberOfLine; LineCounter++) {
    if(AccGyr.Write_Reg(ProgramPointer[LineCounter].address, ProgramPointer[LineCounter].data)) {
      SerialPort.print("Error loading the Program to LSM6DSOX at line: ");
      SerialPort.println(LineCounter);
      while(1) {
        // Led blinking.
        digitalWrite(LED_BUILTIN, HIGH);
        delay(250);
        digitalWrite(LED_BUILTIN, LOW);
        delay(250);
      }
    }
  }

  SerialPort.println("Program loaded inside the LSM6DSOX MLC");
  pinMode(INT_1, INPUT);
  attachInterrupt(INT_1, INT1Event_cb, RISING);
  /* We need to wait for a time window before having the first MLC status */
  delay(3000);
  AccGyr.Get_MLC_Output(mlc_out);
 
  /*** Initialize Wifi & Mqtt Stuff ***/
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

  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  // mqttClient.setId("clientId");

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  // Send the first status obtained 
  publishMqttMessage(mlc_out[0]);
}

void loop() {
  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  mqttClient.poll();

  if (mems_event) {
    mems_event=0;
    LSM6DSOX_MLC_Status_t status;
    AccGyr.Get_MLC_Status(&status);
    if (status.is_mlc1) {
      uint8_t mlc_out[8];
      AccGyr.Get_MLC_Output(mlc_out);
      publishMqttMessage(mlc_out[0]);
    }
  }
}

void INT1Event_cb() {
  mems_event = 1;
}

void publishMqttMessage(uint8_t status){
    Serial.print("Sending status ");
    Serial.println(status);

    mqttClient.beginMessage(topic);
    switch(status) {
      case 0:
        mqttClient.print("Activity: Stationary");
        break;
      case 1:
        mqttClient.print("Activity: Walking");
        break;
      case 4:
        mqttClient.print("Activity: Jogging");
        break;
      case 8:
        mqttClient.print("Activity: Biking");
        break;
      case 12:
        mqttClient.print("Activity: Driving");
        break;
      default:
        mqttClient.print("Activity: Unknown");
        break;
    }	  
    mqttClient.endMessage();
}

