#include <Arduino.h> 
#include <EEPROM.h> 
#define USE_SERIAL Serial
#include <ESP8266WiFi.h> 
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <FirebaseArduino.h>
#include <time.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"


//#define FIREBASE_HOST "hack1-7b82c.firebaseio.com"
//#define FIREBASE_AUTH "wPE4tUJ7pBJk8VeqistgsOK5nwvzBuutxCumnG9t"
#define FIREBASE_HOST "summa-5bd10.firebaseio.com"
#define FIREBASE_AUTH "JmZJe9K8xIIC4I7IkvjNukc3ZN9Ed4V8sgt1aaFs"

//adafruit is to control the light using google assistant
#define MQTT_SERV "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_NAME "rogeeeth"
#define MQTT_PASS "124b68d3147b429eb78520f4cf014003"

const char* host = "maker.ifttt.com";
const int httpsPort = 443;

String url="/trigger/ESP/with/key/kOtb5-C0MaSjsJLp6CiinqrrXAe9lR559mvoqRrpzRq";

//Set up MQTT and WiFi clients
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS);

//Set up the feed you're subscribing to
Adafruit_MQTT_Subscribe onoff = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/f/onoff");

// Variable init
const int buttonPin = D2; // variable for D2 pin
const int ll = D0;
const int ledPin = D7;
char push_data[200]; //string used to send info to the server ThingSpeak
int addr = 0; //endereço eeprom
byte sensorInterrupt = 0; // 0 = digital pin 2
int timezone = 3;
int dst = 0;
int flag=0;

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;

//SSID and PASSWORD for the AP (swap the XXXXX for real ssid and password )
const char * ssid = "RPI14";
const char * password = "summa1234";

//HTTP client init

void setup() {
    
    Serial.begin(115200); // Start the Serial communication to send messages to the computer
    delay(10);
    Serial.println('\n');

    startWIFI();
  
    // Initialization of the variable “buttonPin” as INPUT (D2 pin)
    pinMode(buttonPin, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    
  
    // Two types of blinking
    // 1: Connecting to Wifi
    // 2: Push data to the cloud
    pinMode(ll, OUTPUT); 
    
    pulseCount = 0;
    flowRate = 0.0;
    flowMilliLitres = 0;
    totalMilliLitres = 0;
    oldTime = 0;
    mqtt.subscribe(&onoff);

    digitalWrite(buttonPin, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(ll, LOW);
    
    
    attachInterrupt(digitalPinToInterrupt(buttonPin), pulseCounter, RISING);
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("\nWaiting for time");
    while (!time(nullptr)) {
      Serial.print(".");
      delay(1000);
    }

}

int n;
void loop() {
    StaticJsonBuffer<200> jsonbuffer;
    JsonObject& root =jsonbuffer.createObject();
    if (WiFi.status() == WL_CONNECTED && (millis() - oldTime) > 1000) // Only process counters once per second
    {
        // Disable the interrupt while calculating flow rate and sending the value to
        // the host
        detachInterrupt(sensorInterrupt);

        // Because this loop may not complete in exactly 1 second intervals we calculate
        // the number of milliseconds that have passed since the last execution and use
        // that to scale the output. We also apply the calibrationFactor to scale the output
        // based on the number of pulses per second per units of measure (litres/minute in
        // this case) coming from the sensor.
        flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;

        // Note the time this processing pass was executed. Note that because we've
        // disabled interrupts the millis() function won't actually be incrementing right
        // at this point, but it will still return the value it was set to just before
        // interrupts went away.
        oldTime = millis();

        // Divide the flow rate in litres/minute by 60 to determine how many litres have
        // passed through the sensor in this 1 second interval, then multiply by 1000 to
        // convert to millilitres.
        flowMilliLitres = (flowRate / 60) * 1000;

        // Add the millilitres passed in this second to the cumulative total
        totalMilliLitres += flowMilliLitres;

        unsigned int frac;

        
        

        if (flowRate > 0) {
            // Print the flow rate for this second in litres / minute
        Serial.print("Flow rate: ");
        Serial.print(int(flowRate)); // Print the integer part of the variable
        Serial.print("."); // Print the decimal point
        // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
        frac = (flowRate - int(flowRate)) * 10;
        Serial.print(frac, DEC); // Print the fractional part of the variable
        Serial.print("L/min");
        // Print the number of litres flowed in this second
        Serial.print("  Current Liquid Flowing: "); // Output separator
        Serial.print(flowMilliLitres);
        Serial.print("mL/Sec");

        // Print the cumulative total of litres flowed since starting
        Serial.print("  Output Liquid Quantity: "); // Output separator
        Serial.print(totalMilliLitres);
        Serial.println("mL");
           
            delay(100);          

            digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
            delay(100);              
            time_t now = time(nullptr);
            Serial.println(ctime(&now));
            String name;
            String temp;
            root["time"]=ctime(&now);
            root["value"]=int(flowRate);
            root["no"]=n++;
              //if(n==80){
                  name =Firebase.push("Data",root);
                  temp =Firebase.push("temp",root);
                  Firebase.setInt("Main",totalMilliLitres);
                  //n=0;
                //}
                if (Firebase.failed()) {
                  Serial.print("pushing /logs failed:");
                  Serial.println(Firebase.error());  
                  return;
                    }
                    if(totalMilliLitres>300)
                    {
                      digitalWrite(ll, HIGH); 
                      flag=1;
                        WiFiClientSecure client;
                        Serial.print("connecting to ");
                        Serial.println(host);
                        if (!client.connect(host, httpsPort)) {
                          Serial.println("connection failed");
                          return;
                          }
                         client.print(String("GET ") + url + " HTTP/1.1\r\n" +"Host: " + host + "\r\n" +"User-Agent: BuildFailureDetectorESP\r\n" +"Connection: close\r\n\r\n");
                           Serial.println("request sent");
                           while (client.connected()) {
                            String line = client.readStringUntil('\n');
                            if (line == "\r") {
                              Serial.println("headers received");
                              break;
                              }
                              }
                              String line = client.readStringUntil('\n');
                              Serial.println("reply was:");
                              Serial.println("==========");
                              Serial.println(line);
                              Serial.println("==========");
                              Serial.println("closing connection");
                          }

        }
        
        // Reset the pulse counter so we can start incrementing again
        pulseCount = 0;
        delay(800);

        // Enable the interrupt again now that we've finished sending output
        attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
    } 
    else if (WiFi.status() != WL_CONNECTED) {
        startWIFI();
    }


    if(flag==1){
      MQTT_connect();
      Adafruit_MQTT_Subscribe * subscription;
      while ((subscription = mqtt.readSubscription(500)))
      {
        //If we're in here, a subscription updated...
        if (subscription == &onoff)
        {
          //Print the new value to the serial monitor
          Serial.print("onoff: ");
          Serial.println((char*) onoff.lastread);
          
          //If the new value is  "ON", turn the light on.
          //Otherwise, turn it off.
          if (!strcmp((char*) onoff.lastread, "ON"))
          {
            //Active low logic
            digitalWrite(ll, HIGH);
          }
          else
          {
            digitalWrite(ll, LOW);
          }
        }
      }

      // ping the server to keep the mqtt connection alive
      if (!mqtt.ping())
      {
        mqtt.disconnect();
      }
      
    }
  
}

/*
Insterrupt Service Routine
 */
void pulseCounter() {
    // Increment the pulse counter
    pulseCount++;
}


/* Function to start WiFi incase if it gets disconnected */
void startWIFI(void) {
    digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(100);              
    
    WiFi.begin(ssid, password); // Connect to the network
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println(" ...");
    oldTime = 0;
    int i = 0;
    digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
    delay(100);         
    
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(2000);
        Serial.print(++i);
        Serial.print('.');
        digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
        delay(100);  
    }
    delay(2000);
    Serial.print('\n');
    Serial.print("Connection established!");
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer

}


void MQTT_connect() 
{
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) 
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) // connect will return 0 for connected
  { 
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) 
       {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
