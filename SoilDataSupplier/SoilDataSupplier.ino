/*
 * Created on: 14.04.2023
*/
#include "arduino_secrets.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <WiFiManager.h>

#include <ArduinoJson.h>

#include <DHT.h>
#include <DHT_U.h>


const char endpoint[] = "/chili-app/v1/soilData";

// sensors
#define DHTPIN D2
#define DHTTYPE DHT11
DHT_Unified dht(DHTPIN, DHTTYPE);


#define moistureLevelPower D7
#define moistureLevelPin A0

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 300

void setup() {


  Serial.begin(74880);
  while (!Serial)
    ;


  WiFiManager wm;

  // if the accesspoint is online for 10 seconds, then go back to deepSleep and try again next time
  // This is done do avoid waiting indefinetly for connection.
  wm.setConfigPortalTimeoutCallback(deepSleep);
  wm.setConfigPortalTimeout(10);


  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("AutoConnectAP", "password");  // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    //ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }

  // sensor init
  pinMode(moistureLevelPower, OUTPUT);
  pinMode(moistureLevelPin, INPUT);
  digitalWrite(moistureLevelPower, LOW);
  digitalWrite(LED_BUILTIN, LOW);

  dht.begin();
  printDhtSensorData();

  sendSensorData();
  delay(100);

  deepSleep();
}

void loop() {
  // not needed because of deep sleep
}


void deepSleep() {
  ESP.deepSleep(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}


void alarm(int interval) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(interval);
  digitalWrite(LED_BUILTIN, LOW);
  delay(interval);
}

StaticJsonDocument<128> getSensorData() {
  StaticJsonDocument<128> doc;

  digitalWrite(moistureLevelPower, HIGH);

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  doc["temperature"] = event.temperature;

  dht.humidity().getEvent(&event);
  doc["relativeHumidity"] = event.relative_humidity;

  int sampleSize = 10;
  double val = 0;
  for (int i = 0; i < sampleSize; i++) {
    val += analogRead(moistureLevelPin);
  }

  doc["moistureLevel"] = val / sampleSize;
  digitalWrite(moistureLevelPower, LOW);

  return doc;
}

void sendSensorData() {
  StaticJsonDocument<128> doc = getSensorData();
  char out[128];
  serializeJson(doc, out);
  Serial.print("Sending ");
  Serial.println(out);

  if ((WiFi.status() == WL_CONNECTED)) {

    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    String url = String("http://");
    url.concat(SERVER_IP);
    url.concat(":");
    url.concat(port);
    url.concat(endpoint);
    http.begin(client, url);  // HTTP
    http.addHeader("Content-Type", "application/json");

    Serial.print("[HTTP] POST...\n");
    // start connection and send HTTP header and body
    int httpCode = http.POST(out);
    Serial.println(httpCode);
    if (httpCode > 0) {
      Serial.print("Request successful, code: ");
    }

  } else {
    Serial.println("Lost connection during sensor read.");
  }
}


void printDhtSensorData() {
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("°C"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("°C"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("%"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("%"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
}
