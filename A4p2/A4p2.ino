
// Melody Xu
// HCDE 440 
// 5/6/19
// A4 part 2

// Recieving messages in the JSON format, calls API from http://api.icndb.com/jokes/random for a 
// random Chuck Norris joke if humdity and temperature values are below 20.  

//Including necessary libraries

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>   
#include <ArduinoJson.h>    
#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h>  // for WiFi  
#include <Adafruit_Sensor.h>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // initialize oled 

// A version of the Adafruit128x32 display with 64 lines of buffer.

#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

//#define wifi_ssid "University of Washington"   // Wifi Stuff
//#define wifi_password "" //

#define wifi_ssid "Fake Asians"   // Wifi Stuff
#define wifi_password "samesame" //

WiFiClient espClient;             // espClient
PubSubClient mqtt(espClient);     // tie PubSub (mqtt) client to WiFi client

char mac[6]; //A MAC address as the unique user ID!

char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array

// initialize variables for temperature, humidity, and joke
float tem;
float hum;
String joke;

const int led = 13;     // turns on when joke is recieved

int ledStatus = LOW;    // storing led status

void setup() {
  // put your setup code here, to run once:
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  Serial.begin(115200);
  setup_wifi();
  // System status
  while(! Serial);
  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay(); 
  mqtt.setServer(mqtt_server, 1883); // Start the mqtt
  mqtt.setCallback(callback); //Register the callback function
  display.clearDisplay();
  pinMode(led, OUTPUT);        // for when joke is told
  pinMode(LED_BUILTIN, OUTPUT); //for incoming message

  // Initiate sensor values 
  hum = 0;
  tem = 0;
  joke = "";
  
}

void loop() {
  if (!mqtt.connected()) {  // Try connecting again
    reconnect();
  }
  mqtt.loop(); //this keeps the mqtt connection 'active'
  digitalWrite(led, ledStatus);
  delay(2500);

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
  WiFi.macAddress().toCharArray(mac, 4);  // creating unique identifier for mqtt
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
      mqtt.subscribe("fromMel/weather");
      Serial.println("subscribing");
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
  
  if (String(topic) == "fromMel/weather") {
    display.clearDisplay(); 
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(10, 0);
    display.println(); // print temp and pressure
    Serial.println("printing message");
    Serial.print("Message arrived in topic: ");
    tem = root["temp"];
    hum = root["humidity"];
    String sHum = String(hum);
    String sTem = String(tem);  
    Serial.println("Humidity " + sHum);
    Serial.println("Temperature " + sTem);
  }  

  // if humidity and temp are below 20
  if (hum < 20 && tem < 20) {
    getJoke();
    ledStatus = HIGH; 
    digitalWrite(led, ledStatus);    // notify incoming message
    display.println(joke.substring(0, 63));    
    display.display();
    delay(4000); 
    display.println(joke.substring(64, joke.length())); // flip page
    Serial.println(joke.substring(64, joke.length()));
    display.display();   
    delay(3000);
    digitalWrite(led, LOW); 
  } else {
    ledStatus = LOW; 
    display.println("environment not cold and dry enough for a joke");
    Serial.println("environment not cold and dry enough for a joke"); 
    display.display();  
  }
  
  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }
}


void getJoke() {
  HTTPClient theClient; // Creating the client for calling the API
  String apiCall = "http://api.icndb.com/jokes/random";  // random chucknorris joke
  theClient.begin(apiCall); // Calling API using the URL
  int httpCode = theClient.GET(); // Getting http code
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {             // If the code is 200
      String payload = theClient.getString(); // Getting the string of information
      Serial.println(payload); 
      DynamicJsonBuffer jsonBuffer;                       // Storing JSON
      JsonObject& root = jsonBuffer.parseObject(payload); // Converting into JSON
     
      if (!root.success()) {                              // If parsing failed
        Serial.println("parseObject() failed in getJoke()."); 
        return;
      }     
          
      // Below is accessing the JSON library for the conditions
      joke = root["value"]["joke"].as<String>();
      Serial.println("joke: " + joke);     
    }
  }
  else {
    Serial.printf("Something went wrong with connecting to the endpoint in getJoke().");
  }
}


