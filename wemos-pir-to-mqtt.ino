// Board: LOLIN(WEMOS) D1 mini (clone)

// red is on if either wifi on mqtt is not connected
// blue is wifi connection
// green is mqtt connection
// white is the indicator, goes high if motion is detected
// mqtt is sent every 10 seconds regardless
// mqtt is sent every state change too
// door pin uses magnetic sensor for door opening

#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const char* mqtt_server = MQTT_SERVER;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_client_name = MQTT_CLIENT_NAME;


StaticJsonDocument<200> doc;
String payload;

int delayMillis = 10000;
int millisNow = 0;

int doorPrevState = 2;
int pirPrevState = 2;

#define DOOR_PIN D1
#define PIR_PIN D2
#define WIFI_PIN D5
#define INDICATOR_PIN D6
#define MQTT_PIN D7
#define ERROR_PIN D8

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.print("MQTT_SERVER ");
  Serial.println(MQTT_SERVER);

  client.setServer(mqtt_server, 1883);

  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);
  pinMode(WIFI_PIN, OUTPUT);
  pinMode(INDICATOR_PIN, OUTPUT);
  pinMode(MQTT_PIN, OUTPUT);
  pinMode(ERROR_PIN, OUTPUT);

  digitalWrite(WIFI_PIN, LOW);
  digitalWrite(INDICATOR_PIN, LOW);
  digitalWrite(MQTT_PIN, LOW);
  digitalWrite(ERROR_PIN, HIGH);

  delay(250);
  setup_wifi();
  delay(1000);
}

void loop() {
  if (digitalRead(WIFI_PIN) == LOW && digitalRead(MQTT_PIN) == LOW) {
    digitalWrite(ERROR_PIN, HIGH);
  } else {
    digitalWrite(ERROR_PIN, LOW);
  }


  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int doorState = digitalRead(DOOR_PIN);
  String doorTopic = String(MQTT_TOPIC) + "/DOOR";

  int pirState = digitalRead(PIR_PIN);
  String pirTopic = String(MQTT_TOPIC) + "/PIR";

  if (pirState == 1) {
    digitalWrite(INDICATOR_PIN, HIGH);
  } else {
    digitalWrite(INDICATOR_PIN, LOW);
  }

  int published = 0;

  if (doorState != doorPrevState) {
    published = 1;
    client.publish(doorTopic.c_str(), returnJsonPayload("state", "DOOR", doorState).c_str());
    Serial.println("doorState: " + String(doorState) + ", doorPrevState: " + String(doorPrevState));
  }

  if (pirState != pirPrevState) {
    published = 1;
    client.publish(pirTopic.c_str(), returnJsonPayload("state", "PIR", pirState).c_str());
    Serial.println("pirState: " + String(pirState) + ", pirPrevState: " + String(pirPrevState));
  }


  if (millisNow >= delayMillis) {
    published = 1;
    millisNow = 0;
    client.publish(doorTopic.c_str(), returnJsonPayload("interval", "DOOR", doorState).c_str());
    delay(100);
    client.publish(pirTopic.c_str(), returnJsonPayload("interval", "PIR", pirState).c_str());
  }


  doorPrevState = doorState;
  pirPrevState = pirState;

  if (published == 1) {
    digitalWrite(ERROR_PIN, HIGH);
    delay(250);
    digitalWrite(ERROR_PIN, LOW);
    delay(750);
  } else {
    delay(1000);
  }

  millisNow += 1000;
  Serial.println("millisNow: " + String(millisNow));

}


String returnJsonPayload(String type, String key, int payload) {
  StaticJsonDocument<256> doc;
  doc["type"] = type;
  doc[key] = String(payload);

  String payloadSerialized;
  serializeJson(doc, payloadSerialized);
  return payloadSerialized;
}

void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.hostname("PIR");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFI_PIN, LOW);
    delay(300);
    digitalWrite(WIFI_PIN, HIGH);
    delay(300);
    Serial.print(".");
  }

  digitalWrite(WIFI_PIN, HIGH);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {

  digitalWrite(MQTT_PIN, LOW);
  // Loop until we're reconnected
  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client_name, mqtt_user, mqtt_password)) {
      digitalWrite(MQTT_PIN, HIGH);

      Serial.println("connected");

      String full_topic = String(MQTT_TOPIC) + "/CONNECTED";
      client.publish(full_topic.c_str(), "1");

    } else {

      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(MQTT_PIN, HIGH);
      delay(2500);
      digitalWrite(MQTT_PIN, LOW);
      delay(2500);
    }
  }
}
