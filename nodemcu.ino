#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);

int temperature = 0;
int humidity = 0;
int pm25 = 0;
char msg[100];
unsigned long previousMillis = 0;
const unsigned long interval = 60000;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  Serial.println("WiFi connected!");
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval || previousMillis == 0) {
    previousMillis = currentMillis;
    JsonDocument doc;
    JsonObject dataObject = doc.createNestedObject("data");
    dataObject["temperature"]=temperature;
    dataObject["humidity"]=humidity;
    dataObject["pm25"]=pm25;
    String data;
    serializeJson(doc, data);
    data.toCharArray(msg, (data.length()+1));
    Serial.println("Sending Data: " + data);
    client.publish("@shadow/data/update", msg);
    Serial.println("Sent");
    delay(1000);
  }
}

void reconnect() {
  while(!client.connected()) {
    Serial.println("Connecting to NETPIE");
    if (client.connect(MQTT_CLIENT, MQTT_TOKEN, MQTT_SECRET)) {
      Serial.println("Connected to NETPIE");
    } else {
      Serial.println("Failed, retrying");
      delay(5000);
    }
  }
}
