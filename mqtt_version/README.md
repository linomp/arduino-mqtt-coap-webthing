# MQTT Version

This version of the arduino code communicates through MQTT.

## How to test:

1. Download the [Mosquitto CLI](https://mosquitto.org/download/) (available for Windows, Linux, etc.).

2. Launch an mqtt client and connect to the predefined broker & topic like this:

    ```
    mosquitto_sub -h "apps.xmp.systems" -t "iiot-project/test"
    ```
