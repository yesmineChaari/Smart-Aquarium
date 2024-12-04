#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>
#include "DHT.h" 

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.emqx.io";

const char* mqtt_client_id = "mqtt_iyoti_1Dp8Ot7H1K";
const int mqtt_port = 1883;
const char* mqtt_topic_subscribe = "smart_aquarium/commands";
const char* mqtt_topic_publish_light = "smart_aquarium/light";
const char* mqtt_topic_publish_water = "smart_aquarium/water";
const char* mqtt_topic_publish_temp = "smart_aquarium/temp";
WiFiClient espClient;
PubSubClient client(espClient);
Servo myservo;

const int trigPin = 18;
const int echoPin = 17;
const int ldrPin = 34;
const int servoPin = 4;
const int neoLength = 16;
const int neoDIN = 12;
const int SwitchPin = 22;
const int DHTPin = 25;

Adafruit_NeoPixel strip(neoLength, neoDIN, NEO_GRB + NEO_KHZ800);
DHT dht(DHTPin, DHT22); 

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (String(topic) == mqtt_topic_subscribe && message == "feed") {
    Serial.println("Feeding the fish...");
    myservo.write(180); 
    delay(2000);  
    myservo.write(90);  
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic_subscribe);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  myservo.attach(servoPin);
  strip.begin();
  strip.show();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(SwitchPin, INPUT);
  dht.begin(); }

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  long duration, distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  String distanceString = String(distance);
  client.publish(mqtt_topic_publish_water, distanceString.c_str());

  Serial.println("Checking water level...");
  Serial.print("Water level: ");
  Serial.print(distance);
  Serial.println(" cm");

  int ldrValue = analogRead(ldrPin);
  float ldrPercentage =100-( (ldrValue / 4095.0) * 100.0); 
  String ldrStringValue = String(ldrPercentage, 1) + "%"; 
  client.publish(mqtt_topic_publish_light, ldrStringValue.c_str());
  
  Serial.print("Real Light Intensity (LDR Percentage): ");
  Serial.print(ldrPercentage, 1);
  Serial.println("%");


  int neoBrightness = map(ldrValue, 0, 4095, 0, 255); 
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(neoBrightness, neoBrightness, neoBrightness));
  }
  strip.show();

 
  float temperature = dht.readTemperature();
  if (!isnan(temperature)) { 
    String tempString = String(temperature);
    client.publish(mqtt_topic_publish_temp, tempString.c_str());
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  } else {
    Serial.println("Failed to read temperature from DHT sensor!");
  }

  delay(1000);
}
