/*
*
* Server rack security system
*
* To use this sketch make sure you change the broker IP and IP to the correct settings.
* Code is tested and used on a arduino UNO.
*/

// TODO: 

#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/* ====================Variables==================== */

// Ethernet configs
IPAddress ip (10,10,0,99);
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// MQTT / ethernet
IPAddress broker(10,10,0,67);
int brokerPort = 1883;
EthernetClient ethclient;
PubSubClient client(ethclient);
const char* statusTopic = "rack/sensors";
const char* statusTopicTemp = "rack/sensors/temp";
const char* statusTopicDoor = "rack/sensors/door";
char clientBuffer[50];

// Sensor pins
static byte tempSensorPins[2] = { 5, 7 };
static byte doorSensorPins[3] = { 2, 3, 8 };

// Sensor results
String tempSensorResult;

// MISC
const int msDelay = 10000; // The delay timer for when to send the next report to MQTT

/* ================================================= */

void publishMQTT(const char* topic, String message){ // Publish message to MQTT
  String clientString = String(message);
  clientString.toCharArray(clientBuffer, clientString.length() + 1);
  client.publish(topic, clientBuffer);
}

void reConnect(){ // Connect to the network and MQTT server.
  Ethernet.begin(mac, ip);
  Serial.print("Assigned IP: ");
  Serial.println(Ethernet.localIP());
  client.setServer(broker, brokerPort);

  while (!client.connected()) { // Checks if connected to network. Keeps looping when not connected
    Serial.print("Connecting to MQTT broker: ");
    if (client.connect(clientBuffer)) {
      Serial.println("connected");
      publishMQTT(statusTopic, String("connecting Arduino-" + String(Ethernet.localIP())));
    } else {
      Serial.println("failed, rc= " + client.state() );
      Serial.println("Failed to connect. Trying in 10sec");
      delay(10000);
    }
  }
}

void doorSensor(byte sensorPin){ // get magnetic sensor state
  int sensorState = digitalRead(sensorPin);
  if (sensorState == HIGH){ publishMQTT(statusTopicDoor, String("OPEN " + String(sensorPin))); }
  else if (sensorState == LOW){ publishMQTT(statusTopicDoor, String("CLOSED " + String(sensorPin))); }
  else{ publishMQTT(statusTopicDoor, String("ERROR " + String(sensorPin))); }
}

int tempSensor(byte sensorPin) { // Get tempature from the sensor
  OneWire oneWire(sensorPin);
  DallasTemperature sensors(&oneWire);
  sensors.begin();
  sensors.requestTemperatures();
  float Celcius = sensors.getTempCByIndex(0);
  return Celcius;
}

void setup() { // Basic arduino setup
  Serial.begin(9600); // Start serial communication
  reConnect();

  for (size_t i = 0; i < sizeof(doorSensorPins); i++){ // Set pullups for sensors
    pinMode(doorSensorPins[i], INPUT_PULLUP);
  }
}

void loop() { // Basic arduino loop
  if (!client.connected()) { // Check network connectivity.
    Serial.println("Not connected! Attempting reconnect");
    reConnect();
  }

  for (byte i = 0; i < sizeof(tempSensorPins); i++){ // Getting the temperature from defined pins and report them to MQTT.
    Serial.println("Getting temps");
    tempSensorResult = tempSensor(tempSensorPins[i]);
    publishMQTT(statusTopicTemp, tempSensorResult);
  }
  
  for (size_t i = 0; i < sizeof(doorSensorPins); i++){ // Gets sensor status and report's it to MQTT
    Serial.println("Getting sensor state");
    doorSensor(doorSensorPins[i]);
  }

  delay(msDelay); // A small delay just because.
}
