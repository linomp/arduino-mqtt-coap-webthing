/*
  rp2040_webthing - final project of Enabling Technologies for IIoT Summer School, 2022
  
  This version of the arduino code exposes the functionality as a Web Thing

  Required hardware:
  - Arduino Nano rp2040 connect
  
*/

#define LARGE_JSON_BUFFERS 1

#include <Arduino.h>
#include "Thing.h"

#define ARDUINO_SAMD_NANO_33_IOT
#include "WebThingAdapter.h"

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

/* Components */

LSM6DSOXSensor AccGyr(&DEV_I2C, LSM6DSOX_I2C_ADD_L);

/* MLC */

ucf_line_t *ProgramPointer;
int32_t LineCounter;
int32_t TotalNumberOfLine;

volatile int mems_event = 0;

void INT1Event_cb();
void setMovementProp(uint8_t status);

/* WebThingAdapter configurations */

char ssid[] = SECRET_SSID;
char password[] = SECRET_PASS;

WebThingAdapter *adapter;

const char *sensorTypes[] = {nullptr};
ThingDevice activitySensor("activity-sensor-1", "Arduino Activity Sensor", sensorTypes);
ThingProperty stationary("stationary", "stationary", BOOLEAN, "OnOffProperty");
ThingProperty walking("walking", "walking", BOOLEAN, "OnOffProperty");
ThingProperty jogging("jogging", "jogging", BOOLEAN, "OnOffProperty");
ThingProperty biking("biking", "biking", BOOLEAN, "OnOffProperty");
ThingProperty driving("driving", "driving", BOOLEAN, "OnOffProperty");
ThingProperty unknown("unknown", "unknown", BOOLEAN, "OnOffProperty");


#if defined(LED_BUILTIN)
const int ledPin = LED_BUILTIN;
#else
const int ledPin = 13;
#endif

bool lastOn = false;

void setup() {

  /*** Initialize MLC stuff ***/
  uint8_t mlc_out[8];
  pinMode(LED_BUILTIN, OUTPUT);

  // Force INT1 of LSM6DSOX low in order to enable I2C
  pinMode(INT_1, OUTPUT);
  digitalWrite(INT_1, LOW);
  delay(200);

  // Initialize serial for output.
  Serial.begin(9600);
  
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
 
  /*** Initialize WebThingAdapter stuff ***/

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Connecting to \"");
  Serial.print(ssid);
  Serial.println("\"");

  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  bool blink = true;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, blink ? LOW : HIGH); // active low led
    blink = !blink;
  }
  digitalWrite(ledPin, LOW); // active low led

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  adapter = new WebThingAdapter("w25", WiFi.localIP());

  activitySensor.description = "A motion classifier sensor powered by the Arduino rp2040's ML Core";


  stationary.title = "stationary";
  stationary.readOnly = "true";

  walking.title = "walking";
  walking.readOnly = "true";

  jogging.title = "jogging";
  jogging.readOnly = "true";

  biking.title = "biking";
  biking.readOnly = "true";

  driving.title = "driving";
  driving.readOnly = "true";

  unknown.title = "unknown";
  unknown.readOnly = "true";

  activitySensor.addProperty(&stationary);
  activitySensor.addProperty(&walking);
  activitySensor.addProperty(&jogging);
  activitySensor.addProperty(&biking);
  activitySensor.addProperty(&driving);
  activitySensor.addProperty(&unknown);

  adapter->addDevice(&activitySensor);
  adapter->begin();

  Serial.println("HTTP server started");

  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(activitySensor.id);

  // Send the first status obtained 
  setMovementProp(mlc_out[0]);
  adapter->update();
}

void loop() {  
  adapter->update();

  if (mems_event) {
    resetPropertyValues();
    adapter->update();
    mems_event=0;
    LSM6DSOX_MLC_Status_t status;
    AccGyr.Get_MLC_Status(&status);
    if (status.is_mlc1) {
      uint8_t mlc_out[8];
      AccGyr.Get_MLC_Output(mlc_out);
      setMovementProp(mlc_out[0]);      
    }
  }
}

void INT1Event_cb() {
  // Interrupt to indicate MEMS event
  mems_event = 1;
}

void resetPropertyValues(){
    ThingPropertyValue defaultVal;
    defaultVal.boolean = false;

    stationary.setValue(defaultVal);
    walking.setValue(defaultVal);
    jogging.setValue(defaultVal);
    biking.setValue(defaultVal);
    driving.setValue(defaultVal);
    unknown.setValue(defaultVal);
}

void setMovementProp(uint8_t status){
    ThingPropertyValue stationaryVal;
    stationaryVal.boolean = (status==0);
    stationary.setValue(stationaryVal);

    ThingPropertyValue walkingVal;
    walkingVal.boolean = (status==1);
    walking.setValue(walkingVal);

    ThingPropertyValue joggingVal;
    joggingVal.boolean = (status==4);
    jogging.setValue(joggingVal);

    ThingPropertyValue bikingVal;
    bikingVal.boolean = (status==8);
    biking.setValue(bikingVal);

    ThingPropertyValue drivingVal;
    drivingVal.boolean = (status==12);
    driving.setValue(drivingVal);

    ThingPropertyValue unknownVal;
    unknownVal.boolean = !(stationaryVal.boolean || walkingVal.boolean || joggingVal.boolean || bikingVal.boolean || drivingVal.boolean);
    unknown.setValue(unknownVal);

    formatForSerialDebugging(status);
}

void formatForSerialDebugging(uint8_t status){
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

    Serial.print("Sending payload - ");
    Serial.println(payload);
}

