# CoAP Version

This version of the arduino code communicates with the CoAP protocol.

## How to test:

Start a CoAP test server inside a Docker container, exposing port 5863:

```
docker run --name coap-test-server -i --rm -p 5683:5683/udp aleravat/coap-test-server:latest
```