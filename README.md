# SimpleInternetThing
The goal of this library is to simplify building an IoT thing with MQTT connectivity. The focus is on the ESP32 using the Arduino framework. 

The library comes with a solution for the following aspects:
- Connecting to the WiFi network.
- Connecting MQTT server over TLS.
- Publishing retained online/offline status messages.
- Publishing telemetry messages.
- Command handling.
- OTA over MQTT.

## Dependencies
In order to use this library, also install:
 - A [fork](https://github.com/rovale/pubsubclient) of the well known [PubSubClient](https://github.com/knolleary/pubsubclient) which I updated to enable OTA updates over MQTT.
 - [ArduinoJson](https://github.com/bblanchon/ArduinoJson), take a version 5, not the version 6 beta.

## Usage
See the example of a [SimpleSensorBox](./examples/SimpleSensorBox/SimpleSensorBox.cpp). Customize the secrets and flash it on an ESP32. When everything is OK the the serial output should show what is going on.

```
Sensor ssb1 version 0.0.1.
Connecting to the SomeSsid network.......
Connected.
Connecting to the SomeMqttServer MQTT server.
Connected.
Published to: somebuilding/someroom/ssb1/status, content: {"online":true,"name":"An example of a simple sensor box","version":"0.0.1","mac":"30:AE:A4:39:1C:48","ip":"192.168.2.3"}.
Subscribed to: somebuilding/someroom/ssb1/command.
Subscribed to: somebuilding/someroom/ssb1/update.
Published to: somebuilding/someroom/ssb1/telemetry/system, content: {"sequence":0,"rssi":-44,"memory":226496}.
Published to: somebuilding/someroom/ssb1/telemetry/climate, content: {"light":1283,"rain":0}.
```
When starting up it will connect to the WiFi network and the MQTT server. If there is no connection yet or if the connection is lost it will indicate that by turning on the indicator LED.

Once connected it will publish a retained online status message with some extra info about the thing and it will subscribe to the `command` and `update` topics for command handling and OTA updates.

While running it will publish system telemetry messages to indicate the system health. This message is sent every minute. The SimpleSensorBox example also sends its own telemetry data, every 30 seconds.

## Command handling
To send a command to be executed by the thing, publish a message to the `command` topic. Commands should be in the JSON format with a `name` attribute for the command name and extra attributes for possible parameters. The library already supports the `reset` command. 
```
mosquitto_pub -h <mqtt server> -p <mqtt port> --caFile <CA file> -u <mqtt username> -P <mqtt password> -t somebuilding/someroom/ssb1/command -m {"name":"reset"}
```
```
Received on: somebuilding/someroom/ssb1/command, 14 bytes.
Command reset.
ets Jun  8 2016 00:22:57

rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
```

The SimpleSensorBox example handles specific commands, this is optional and easy to extend.
```
mosquitto_pub -h <mqtt server> -p <mqtt port> --caFile <CA file> -u <mqtt username> -P <mqtt password> -t somebuilding/someroom/ssb1/command -m {"name":"updateSettings","telemetryInterval":10000}
```
```
Received on: somebuilding/someroom/ssb1/command, 47 bytes.
Command updateSettings.
Changed telemetry interval to 10000.
```

## OTA over MQTT
To update, publish the firmware binary to the `update` topic. 

```
mosquitto_pub -h <mqtt server> -p <mqtt port> --caFile <CA file> -u <mqtt username> -P <mqtt password> -t somebuilding/someroom/ssb1/update -f firmware.bin
```

```
Received on: somebuilding/someroom/ssb1/update, 689072 bytes.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Updating. New firmware size is 689072 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Read 68907 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Read 137814 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Read 206721 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Read 275628 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Read 344535 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Read 413442 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Read 482349 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Read 551256 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Read 620163 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Read 689070 bytes."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Update successful."}.
Published to: somebuilding/someroom/ssb1/update/progress, content: {"message":"Restarting."}.
```
