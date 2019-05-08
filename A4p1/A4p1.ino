
// Melody Xu
// HCDE 440 
// 5/6/19
// A4 part 1

// Sending messages about temperature and humidity in JSON format. 

//Including necessary libraries
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <PubSubClient.h>   
#include <ArduinoJson.h>    
#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h>   
#include <Adafruit_Sensor.h>


#define DATA_PIN 12

DHT_Unified dht(DATA_PIN, DHT22);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

#define wifi_ssid "Fake Asians"   // Wifi Stuff
#define wifi_password "samesame" //


//#define wifi_ssid "University of Washington"   // Wifi Stuff
//#define wifi_password "" //

WiFiClient espClient;             // espClient
PubSubClient mqtt(espClient);     // tie PubSub (mqtt) client to WiFi client

char mac[6]; //A MAC address as the unique user ID!

char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array

unsigned long currentMillis, previousMillis; // hold the values of timers


// initialize variables for temperature + humidity
float tem;
float hum;


void setup() {
  // start the serial connection
  Serial.begin(115200);

  // System status
  while(! Serial);
  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));

  Serial.println("initializing dht");
  dht.begin(); // begin dht (non i2c) sensor

  setup_wifi();
  mqtt.setServer(mqtt_server, 1883); // Start the mqtt
  mqtt.setCallback(callback); //Register the callback function
 
  // Initiate sensor values 
  hum = 0;
  tem = 0;
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  Serial.println(WiFi.macAddress());  //.macAddress returns a byte array 6 bytes representing the MAC address
  WiFi.macAddress().toCharArray(mac, 6);  
}       


void loop() {
  if (!mqtt.connected()) {  // Try connecting again
    reconnect();
  }
  mqtt.loop(); //this keeps the mqtt connection 'active'
  getSensorData();
  delay(2500);
}


void getSensorData() {
  sensors_event_t event;       // prep a instance for grabbing data 
  dht.temperature().getEvent(&event);  // grabbing data
  tem = event.temperature;    // Storing temperature data

  Serial.print("Temperature: ");
  Serial.print(tem);
  Serial.println("C");

  dht.humidity().getEvent(&event);  // grabbing data
  hum = event.relative_humidity;  // Storing humidity data

  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println("%");
  
  // Creates type char storage spaces for different sensor values
  char str_tem[7];
  char str_hum[7];
  char message[60];
  
  
  // Converts sensor data to string
  dtostrf(tem, 4, 2, str_tem);
  dtostrf(hum, 4, 2, str_hum);
  sprintf(message, "{\"temp\": \"%s\", \"humidity\": \"%s\"}", str_tem, str_hum);    
  mqtt.publish("fromMel/weather", message);   // publishing data to MQTT
  Serial.println("publishing");
  delay(4000);
}

//function to reconnect if we become disconnected from the server
void reconnect() {
  // Loop until we're reconnected
  while (!espClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // Attempt to connect
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //the connction
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //blah blah blah a DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }
}


