/*
  SimpleInternetThing.h - Simplifies building an IoT thing.
*/
#ifndef SimpleInternetThing_h
#define SimpleInternetThing_h

#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>  // Take a version 5, not the version 6 beta

#define SIMPLE_INTERNET_THING_RECONNECT_DELAY 10000
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
  void inverseIndicatorLed();

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
  bool _inverseIndicatorLed;
  const char *_version;

  WiFiClientSecure _wiFiClient;
  PubSubClient _mqttClient;

  String createTopic(const char *subject);

  unsigned long _lastReconnectAttemptAt;
  void stayConnected();

  unsigned long _sequenceNumber = 0;
  unsigned long _wiFiDisconnects = -1;
  unsigned long _mqttDisconnects = -1;

  String createStatusMessage(bool isOnline);

  void turnIndicatorLedOn();
  void turnIndicatorLedOff();

  void subscribe(String topic, int qos);
  void publish(String topic, String message, bool retain);

  std::function<void(String, JsonObject &)> _commandCallback;
  void onReceive(char* topic, byte* payload, unsigned int length);

  unsigned long _lastSystemMessageAt;
  String createSystemMessage();
};

#endif