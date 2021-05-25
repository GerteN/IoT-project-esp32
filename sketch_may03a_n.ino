
#include <vector>
#include <string>
#include <WiFi.h>
#include "SPIFFS.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define FORMAT_SPIFFS_IF_FAILED true


const char* ssid = "<<INSERT SSID HERE>>";
const char* password = "<<INSERT PASSWORD HERE>>";



WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

String formattedDate;
String dayStamp;
String timeStamp;

// Static ip
//IPAddress ip(192, 168, 1, 200);
//IPAddress subnet(192, 168, 1, 1);
//IPAddress gateway(255, 255, 255, 0);

//web server port 80
WiFiServer server(80);

// Header della request http
String header;
String response;


// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;



  
void setup() {
  Serial.begin(115200);
  
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
    }
  File file = SPIFFS.open("/all.txt", FILE_WRITE);
  if(!file){
    Serial.println("There was an error opening the file for writing");
    return;
    }
  // random sensors data
  for(int i=0; i<10; i++){
      String val = String(25+i);
      String val2 = String((15+(2*i))/2);
      file.println(val);
      file.println("37.511520, 15.084343");
      file.println(val2);
    }
   file.close();
 

  // Connesione con il wifiConnect to Wi-Fi network with SSID and password
  Serial.print("Establishing connection with ");
  Serial.print(ssid);
  Serial.println("...");
  WiFi.begin(ssid, password);

  // Ogni punto è un delay di 250ms per la connessione
  // Finchè non si connette non vado avanti
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  
  // Stampo il local ip per la connessione (solo per testarlo)
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  timeClient.begin();
  timeClient.setTimeOffset(7200);
}

void loop(){
  
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, we've got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type: application/json");
            client.println("Connection: close");
            client.println();
            
            
            // send all data
            if (header.indexOf("GET /all") >= 0) {
              DynamicJsonDocument jsondata(1024);
              //creating json string to send
              //from the basic json object we've created 3 json array containing temp, speed and coords
              JsonArray temp = jsondata.createNestedArray("temperatures");
              JsonArray spee = jsondata.createNestedArray("speed");
              JsonArray coord = jsondata.createNestedArray("coordinates");
              //let's open the file containg all data and add everything inside the arrays
              Serial.println("getting all data");
              File file = SPIFFS.open("/all.txt", "r");
              while(file.available()){
                temp.add(file.readStringUntil('\r'));
                file.read();
                coord.add(file.readStringUntil('\r'));
                file.read();
                spee.add(file.readStringUntil('\r'));
                file.read();
              }
              //file ended, we'll format everything to send the json object as response
              file.close();
              timeClient.forceUpdate();
              formattedDate = timeClient.getFormattedDate();
              jsondata["timestamp"] = serialized(formattedDate);
              serializeJson(jsondata, response);
              Serial.println("json data: "+ response);
            }
            client.println(response);
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
  }
}
