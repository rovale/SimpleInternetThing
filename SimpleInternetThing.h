/*
  SimpleInternetThing.h - Simplifies building an IoT thing.
*/
#ifndef SimpleInternetThing_h
#define SimpleInternetThing_h

#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <PubSubClient.h> // https://github.com/rovale/pubsubclient, a fork with some changes enabling MQTT OTA updates.
#include <ArduinoJson.h>  // Take a version 5, not the version 6 beta

#define SIMPLE_INTERNET_THING_RECONNECT_DELAY 5000
#define SIMPLE_INTERNET_THING_SYSTEM_MESSAGE_INTERVAL 60000

class SimpleInternetThing
{
public:
  SimpleInternetThing(
      const char *mqttTopicBase,
      const char *thingId, const char *thingName, const char *version,
      const char *wiFiSsid, const char *wiFiPassword,
      const char *mqttServer, uint16_t mqttPort,
      const char *caCert,
      const char *mqttUsername, const char *mqttPassword,
      int indicatorLedPin);

  void setup();
  void loop();
  void publishData(const char *subject, String data);
  void onCommand(std::function<void(String, JsonObject &)> callback);

private:
  const char *_wiFiSsid;
  const char *_wiFiPassword;
  const char *_mqttServer;
  uint16_t _mqttPort;
  const char *_caCert;
  const char *_thingId;
  const char *_thingName;
  const char *_mqttUsername;
  const char *_mqttPassword;
  const char *_mqttTopicBase;
  int _indicatorLedPin;
  const char *_version;

  WiFiClientSecure _wiFiClient;
  PubSubClient _mqttClient;

  String createTopic(const char *subject);

  unsigned long _lastReconnectAttemptAt;
  void stayConnected();
  unsigned long _sequenceNumber = 0;
  String createStatusMessage(bool isOnline);

  void turnIndicatorLedOn();
  void turnIndicatorLedOff();

  void subscribe(String topic, int qos);
  void publish(String topic, String message, bool retain);

  std::function<void(String, JsonObject &)> _commandCallback;
  void onReceive(char *topic, unsigned long length);
  void handleOtaUpdate(unsigned long length);
  String createOtaUpdateProgressMessage(String message);

  unsigned long _lastSystemMessageAt;
  String createSystemMessage();
};

#endif