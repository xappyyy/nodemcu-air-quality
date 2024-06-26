#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

WiFiClient espClient;
PubSubClient client(espClient);

double temperature;
double humidity;
double dust_density;
char msg[100];
unsigned long previousMillis = 0;
const unsigned long interval = 60000;
bool isReceived = false;

HardwareSerial stmSerial(2);

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
  stmSerial.begin(115200, SERIAL_8N1, 16, 17);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (stmSerial.available()) {
    Serial.println("reading");
    String rawdata = stmSerial.readStringUntil('\n');
    rawdata.trim();

    String _temp = split(rawdata, ' ', 0);
    String _humid = split(rawdata, ' ', 1);
    String _dust = split(rawdata, ' ', 2);
    Serial.println(rawdata);
    if((_temp != "") && (_humid != "") && (_dust != "")) {
      bool validData = true;
      double temp = _temp.toDouble();
      double humid = _humid.toDouble();
      double dust = _dust.toDouble();

      if (_temp != String(temp) || _humid != String(humid) || _dust != String(dust)) validData = false;

      if (validData) {
        Serial.println(temp);
        Serial.println(humid);
        Serial.println(dust);
        temperature = temp;
        humidity = humid;
        dust_density = dust;
        isReceived = true;
      }
    }
  }

  unsigned long currentMillis = millis();
  if (isReceived && (currentMillis - previousMillis >= interval || previousMillis == 0)) {
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
