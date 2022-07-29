/*
  rp2040_webthing - final project of Enabling Technologies for IIoT Summer School, 2022
  
  This version of the arduino code exposes the functionality as a Web Thing

  Required hardware:
  - Arduino Nano rp2040 connect
  
*/

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

#define INT_1 INT_IMU

// Components
LSM6DSOXSensor AccGyr(&DEV_I2C, LSM6DSOX_I2C_ADD_L);

// MLC
ucf_line_t *ProgramPointer;
int32_t LineCounter;
int32_t TotalNumberOfLine;

volatile int mems_event = 0;

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

  // Interrupt to indicate MEMS event
  mems_event = 1;

}

void publishMqttMessage(uint8_t status){
    bool retained = false;
    int qos = 1;
    bool dup = false;

    String payload = "Activity: ";
    
    switch(status) {
      case 0:
        payload += "Stationary";
        break;
      case 1:
        payload += "Walking";
        break;
      case 4:
        payload += "Jogging";
        break;
      case 8:
        payload += "Biking";
        break;
      case 12:
        payload += "Driving";
        break;
      default:
        payload += "Unknown";
        break;
    }
    
    Serial.print("Sending payload - ");
    Serial.println(payload);

    mqttClient.beginMessage(topic, payload.length(), retained, qos, dup);
    mqttClient.print(payload);	  
    mqttClient.endMessage();
}

