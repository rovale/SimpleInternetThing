#include <SimpleInternetThing.h>
#include <Update.h>

SimpleInternetThing::SimpleInternetThing(
    const char *mqttTopicBase,
    const char *thingId, const char *thingName, const char *version,
    const char *wiFiSsid, const char *wiFiPassword,
    const char *mqttServer, uint16_t mqttPort,
    const char *caCert,
    const char *mqttUsername, const char *mqttPassword,
    int indicatorLedPin)
{
  _wiFiSsid = wiFiSsid;
  _wiFiPassword = wiFiPassword;
  _mqttServer = mqttServer;
  _mqttPort = mqttPort;
  _caCert = caCert;
  _thingId = thingId;
  _thingName = thingName;
  _mqttUsername = mqttUsername;
  _mqttPassword = mqttPassword;
  _mqttTopicBase = mqttTopicBase;
  _indicatorLedPin = indicatorLedPin;
  _version = version;

  _lastReconnectAttemptAt = SIMPLE_INTERNET_THING_RECONNECT_DELAY * -1;
  _lastSystemMessageAt = SIMPLE_INTERNET_THING_SYSTEM_MESSAGE_INTERVAL * -1;
};

void SimpleInternetThing::setup()
{
  pinMode(_indicatorLedPin, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(_wiFiSsid, _wiFiPassword, 0, NULL, false);

  _wiFiClient = WiFiClientSecure();
  _wiFiClient.setCACert(_caCert);

  _mqttClient = PubSubClient(_wiFiClient);
  _mqttClient.setServer(_mqttServer, _mqttPort);
  _mqttClient.setCallback(std::bind(&SimpleInternetThing::onReceive, this, std::placeholders::_1, std::placeholders::_2));
}

void SimpleInternetThing::loop()
{
  stayConnected();
  _mqttClient.loop();

  if (millis() - _lastSystemMessageAt >= SIMPLE_INTERNET_THING_SYSTEM_MESSAGE_INTERVAL)
  {
    _lastSystemMessageAt = millis();
    publish(createTopic("telemetry/system"), createSystemMessage(), false);
  }
}

void SimpleInternetThing::publishData(const char *subject, String data)
{
  publish(createTopic(subject), data, false);
}

void SimpleInternetThing::onCommand(std::function<void(String, JsonObject &)> callback)
{
  _commandCallback = callback;
}

String SimpleInternetThing::createTopic(const char *subject)
{
  return String(_mqttTopicBase) + "/" + String(_thingId) + "/" + String(subject);
}

void SimpleInternetThing::onReceive(char *topic, unsigned long length)
{
  Serial.print("Received on: ");
  Serial.print(topic);
  Serial.print(", ");
  Serial.print(length);
  Serial.println(" bytes.");

  if (String(topic) == createTopic("update"))
  {
    handleOtaUpdate(length);
  }

  DynamicJsonBuffer jsonBuffer(1024);
  JsonObject &root = jsonBuffer.parseObject(_wiFiClient);
  if (root.success())
  {
    String commandName = root["name"];
    Serial.print("Command ");
    Serial.print(commandName);
    Serial.println(".");

    if (commandName == "reset")
    {
      ESP.restart();
    }
    else
    {
      if (_commandCallback)
      {
        _commandCallback(commandName, root);
      }
      else
      {
        Serial.println("Unknown command.");
      }
    }
  }
  else
  {
    Serial.println("Unable to parse payload.");
  }
}

void SimpleInternetThing::handleOtaUpdate(unsigned long length)
{
  String progressTopic = createTopic("update/progress");
  publish(progressTopic.c_str(), createOtaUpdateProgressMessage("Updating. New firmware size is " + String(length) + " bytes."), false);

  if (Update.begin(length))
  {
    unsigned long tenPercent = length / 10;
    byte buffer[1];
    unsigned long actual;
    for (actual = 0; actual < length; actual++)
    {
      byte b;
      if (!_mqttClient.readByte(&b))
      {
        break;
      }
      buffer[0] = b;
      Update.write(buffer, 1);

      if ((actual > 0) && (actual % tenPercent == 0))
      {
        publish(progressTopic.c_str(), createOtaUpdateProgressMessage("Read " + String(actual) + " bytes."), false);
      }
    }

    if (Update.end())
    {
      if (Update.isFinished())
      {
        publish(progressTopic.c_str(), createOtaUpdateProgressMessage("Update successful."), false);
      }
      else
      {
        publish(progressTopic.c_str(), createOtaUpdateProgressMessage("Update not finished."), false);
      }
    }
    else
    {
      publish(progressTopic.c_str(), createOtaUpdateProgressMessage("Error occurred, #: " + String(Update.getError()) + "."), false);
    }
  }
  else
  {
    publish(progressTopic.c_str(), createOtaUpdateProgressMessage("Not enough space to update."), false);
  }

  publish(progressTopic.c_str(), createOtaUpdateProgressMessage("Restarting."), false);
  delay(1000);
  ESP.restart();
}

String SimpleInternetThing::createOtaUpdateProgressMessage(String message)
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &jsonObject = jsonBuffer.createObject();
  jsonObject["message"] = message;
  String jsonString;
  jsonObject.printTo(jsonString);

  return jsonString;
}

void SimpleInternetThing::stayConnected()
{
  if (_mqttClient.connected())
  {
    return;
  }

  turnIndicatorLedOn();

  if (!WiFi.isConnected())
  {
    unsigned long wifiConnectAttemptAt = millis();

    delay(10);
    Serial.print("Connecting to the ");
    Serial.print(_wiFiSsid);
    Serial.print(" network.");

    WiFi.reconnect();
    while (!WiFi.isConnected())
    {
      delay(500);
      Serial.print(".");

      if (millis() - wifiConnectAttemptAt >= 5 * 60 * 1000)
      {
        ESP.restart();
      }
    }

    Serial.println();
    Serial.println("Connected.");
}

  if (millis() - _lastReconnectAttemptAt >= SIMPLE_INTERNET_THING_RECONNECT_DELAY)
  {
    _lastReconnectAttemptAt = millis();

    Serial.print("Connecting to the ");
    Serial.print(_mqttServer);
    Serial.println(" MQTT server.");

    if (_mqttClient.connect(
            _thingId,
            _mqttUsername,
            _mqttPassword,
            createTopic("status").c_str(),
            1,
            true,
            createStatusMessage(false).c_str()))
    {
      turnIndicatorLedOff();
      Serial.println("Connected.");

      publish(createTopic("status"), createStatusMessage(true), true);
      subscribe(createTopic("command"), 1);
      subscribe(createTopic("update"), 0);
    }
    else
    {
      Serial.print("Failed, state is ");
      Serial.print(_mqttClient.state());
      Serial.println(".");
    }
  }
}

void SimpleInternetThing::subscribe(String topic, int qos) {
  Serial.print("Subscribed to: ");
  Serial.print(topic);
  Serial.println(".");
  _mqttClient.subscribe(topic.c_str(), qos);
}

void SimpleInternetThing::publish(String topic, String message, bool retained)
{
  Serial.print("Published to: ");
  Serial.print(topic);
  Serial.print(", content: ");
  Serial.print(message);
  Serial.println(".");

  if (!_mqttClient.publish(topic.c_str(), message.c_str(), retained))
  {
    Serial.println("Publish failed, maybe check MQTT_MAX_PACKET_SIZE, patch PubSubClient.h.");
    Serial.println("https://pubsubclient.knolleary.net/api.html");
    Serial.print("Message length is ");
    Serial.print(message.length());
    Serial.println(".");
  }
}

void SimpleInternetThing::turnIndicatorLedOn()
{
  digitalWrite(_indicatorLedPin, HIGH);
}

void SimpleInternetThing::turnIndicatorLedOff()
{
  digitalWrite(_indicatorLedPin, LOW);
}

String SimpleInternetThing::createStatusMessage(bool isOnline)
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &jsonObject = jsonBuffer.createObject();
  jsonObject["online"] = isOnline;
  if (isOnline)
  {
    jsonObject["name"] = _thingName;
    jsonObject["version"] = _version;
    jsonObject["mac"] = WiFi.macAddress();
    jsonObject["ip"] = WiFi.localIP().toString();
  }
  String jsonString;
  jsonObject.printTo(jsonString);
  return jsonString;
}

String SimpleInternetThing::createSystemMessage()
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &jsonObject = jsonBuffer.createObject();
  jsonObject["sequence"] = _sequenceNumber;
  jsonObject["rssi"] = WiFi.RSSI();
  jsonObject["memory"] = ESP.getFreeHeap();
  String jsonString;
  jsonObject.printTo(jsonString);

  _sequenceNumber++;

  return jsonString;
}
