# MQTT Version

This version of the arduino code communicates through MQTT.

## Required Libraries:
- [WiFiNINA](https://www.arduino.cc/reference/en/libraries/wifinina/)
- [ArduinoMqttClient](https://www.arduino.cc/reference/en/libraries/arduinomqttclient/)
- [STM32duino X-NUCLEO-IKS01A3](https://www.arduino.cc/reference/en/libraries/stm32duino-x-nucleo-iks01a3/3)


## How to test:

1. Download the [Mosquitto CLI](https://mosquitto.org/download/) (available for Windows, Linux, etc.).

2. Launch an mqtt client and connect to the predefined broker & topic like this:

    ```
    mosquitto_sub -h "apps.xmp.systems" -t "iiot-project/test"
    ```
