# Motion classification application on an arduino board supporting multiple IoT Protocols

Final project of the Enabling Technologies for IIoT Summer School by University of Pisa, 2022.

The objective is to develop a motion classification application on an arduino board  (using embedded ML capabilities of the hardware) and connect it to the external world either: 

- by sending the events through an MQTT topic
- by sending events with Coap protocol (pub/sub mode)
- by wrapping it as a "Web Thing" using the WebThings interoperability framework, and monitoring it via a WebThings gateway (available as Docker container, or installed on a Raspberry pi)


### Required hardware:
- [Arduino Nano RP2040 connect](https://docs.arduino.cc/hardware/nano-rp2040-connect)

### Milestones:

- [X] Sending the current motion status through an MQTT topic to a broker running on my personal DigitalOcean cloud server

    ![](./demo.PNG)


### References:

- [Using the IMU Machine Learning Core Features of the Nano RP2040 Connect](https://docs.arduino.cc/tutorials/nano-rp2040-connect/rp2040-imu-advanced)
- [WebThings Framework](https://webthings.io/framework/)
- [Coap Library for Arduino](https://www.arduino.cc/reference/en/libraries/coap-simple-library/)
- [My starred repositories for this project](https://github.com/stars/linomp/lists/iiot-summer-school-project)
