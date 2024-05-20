#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

WiFiClient espClient;
PubSubClient client(espClient);

double temperature = 2.5;
double humidity = 3.66;
double dust_density = 4.7;
char msg[100];
unsigned long previousMillis = 0;
const unsigned long interval = 60000;

EspSoftwareSerial::UART stmSerial(D7, D8);

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
  stmSerial.begin(115200);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (stmSerial.available()) {
    String rawdata = Serial.readString();
    String _temp = split(rawdata, ' ', 0);
    String _humid = split(rawdata, ' ', 1);
    String _dust = split(rawdata, ' ', 2);
    
    if((_temp != "") && (_humid != "") && (_dust != "")) {
      double temp = _temp.toDouble();
      double humid = _humid.toDouble();
      double dust = _dust.toDouble();
      temperature = temp;
      humidity = humid;
      dust_density = dust;
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval || previousMillis == 0) {
    previousMillis = currentMillis;
    JsonDocument doc;
    JsonObject dataObject = doc.createNestedObject("data");
    dataObject["temperature"]=round(temperature * 10)/10.0;
    dataObject["humidity"]=round(humidity * 10)/10.0;
    dataObject["dust_density"]=round(dust_density * 10)/10.0;
    String data;
    serializeJson(doc, data);
    data.toCharArray(msg, (data.length()+1));
    Serial.println("Sending Data: " + data);
    client.publish("@shadow/data/update", msg);
    Serial.println("Sent");
    delay(1000);
  }
  yield();
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
    yield();
  }
}

String split(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
