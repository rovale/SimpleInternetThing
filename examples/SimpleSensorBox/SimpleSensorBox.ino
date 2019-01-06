#include <SimpleInternetThing.h> // SimpleInternetThing library (https://github.com/rovale/SimpleInternetThing).
#include <ArduinoJson.h>         // Take a version 5, not the version 6 beta.
#include <Secrets.h>             // Add a secrets file.

/* Example of Secrets.h:
const char ssid[] =             "SomeSsid";
const char wiFiPassword[] =     "SomeWiFiPassword";
const char mqttServer[] =       "SomeMqttServer";
const char mqttUsername[] =     "SomeMqttUserName";
const char mqttPassword[] =     "SomeMqttPassword";
*/

const char mqttTopicBase[] = "somebuilding/someroom";
const char id[] = "ssb1"; // This is added to the MQTT topic name.
const char name[] = "An example of a simple sensor box";
const char version[] = "0.0.1";

const int indicatorLedPin = 25;

SimpleInternetThing simpleSensorBox(
    mqttTopicBase, id, name, version,
    ssid, wiFiPassword,
    mqttServer, 1883, mqttUsername, mqttPassword,
    indicatorLedPin);

const int lightPin = 38;
const int rainPin = 35;

unsigned long telemetryInterval = 30000;
unsigned long lastTelemetryMessageAt = -1 * telemetryInterval;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("Sensor ");
  Serial.print(id);
  Serial.print(" version ");
  Serial.print(version);
  Serial.println(".");

  simpleSensorBox.setup();
  simpleSensorBox.onCommand(onCommand);

  pinMode(lightPin, INPUT);
}

void loop()
{
  simpleSensorBox.loop();

  if (millis() - lastTelemetryMessageAt >= telemetryInterval)
  {
    lastTelemetryMessageAt = millis();

    // Publishes telemetry data to 'somebuilding/someroom/ssb1/telemetry/climate'.
    // Example: '{"light":79,"rain":23}'.
    simpleSensorBox.publishData("telemetry/climate", getClimateMessage());
  }
}

String getClimateMessage()
{
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject &jsonObject = jsonBuffer.createObject();

  jsonObject["light"] = analogRead(lightPin);
  jsonObject["rain"] = analogRead(rainPin);

  String jsonString;
  jsonObject.printTo(jsonString);
  return jsonString;
}

// Triggered when a command message is sent to 'somebuilding/someroom/ssb1/command'.
// Example: '{"name": "updateSettings", "telemetryInterval": 15000}'.
void onCommand(String commandName, JsonObject &root)
{
  if (commandName == "updateSettings")
  {
    if (root.is<unsigned long>("telemetryInterval"))
    {
      telemetryInterval = root.get<unsigned long>("telemetryInterval");
      Serial.print("Changed telemetry interval to ");
      Serial.print(telemetryInterval);
      Serial.println(".");
    }
  }
  else
  {
    Serial.println("Unknown command.");
  }
}
