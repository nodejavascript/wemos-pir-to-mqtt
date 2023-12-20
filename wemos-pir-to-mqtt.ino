// Board: LOLIN(WEMOS) D1 mini (clone)

// red is on if either wifi on mqtt is not connected
// blue is wifi connection
// green is mqtt connection
// mqtt is sent every 10 seconds regardless
// mqtt is sent every state change too

#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Replace with your MQTT credentials
const char* mqtt_server = MQTT_SERVER;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_client_name = MQTT_CLIENT_NAME;

int delayMillis = 10000;
int millisNow = 0;

int prevState = 2;

#define PIR_PIN D2
#define WIFI_PIN D5
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
  pinMode(PIR_PIN, INPUT);
  pinMode(WIFI_PIN, OUTPUT);
  pinMode(MQTT_PIN, OUTPUT);
  pinMode(ERROR_PIN, OUTPUT);

  digitalWrite(WIFI_PIN, LOW);
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

  int state = digitalRead(PIR_PIN);
  Serial.print("state ");
  Serial.println(state);
  Serial.print("prevState ");
  Serial.println(prevState);

  String full_topic = String(mqtt_client_name) + "/PIR";
  String state_value = String(state);




  if (state != prevState) {
    Serial.println("state send");
    
    // Create a JSON object
    StaticJsonDocument<200> doc;
    doc["type"] = "state";
    doc["PIR"] = state;
    String payload;
    serializeJson(doc, payload);
    client.publish(full_topic.c_str(), payload.c_str());
  }

  prevState = state;

  if (millisNow >= delayMillis) {
    Serial.println("reg send");
    millisNow = 0;

    // Create a JSON object
    StaticJsonDocument<200> doc;
    doc["type"] = "interval";
    doc["PIR"] = state;
    String payload;
    serializeJson(doc, payload);
    client.publish(full_topic.c_str(), payload.c_str());
  }
  
  
  delay(1000);
  millisNow += 1000;
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.hostname("PIR");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFI_PIN, LOW);
    delay(500);
    delay(250);
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

      String full_topic = String(mqtt_client_name) + "/CONNECTED";
      client.publish(full_topic.c_str(), "1");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
