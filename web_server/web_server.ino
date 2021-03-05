#include <Arduino.h>
#include <ESP8266WiFi.h> //ESP8266 Arduino library with built in functions
#include "fauxmoESP.h"


#define ssid "USSR24" // Replace with your network name
#define password "Nirishka2@" // Replace with your network password

WiFiServer server(443); // Web Server on port 443

String header;
String relayState = "Off";
int RELAY = 0;
#define RELAY_ON()  digitalWrite(RELAY, LOW);
#define RELAY_OFF() digitalWrite(RELAY, HIGH);


#define LAMP_1 "garage lamp"

fauxmoESP fauxmo;

void setup() { // only executes once

    Serial.begin(115200); // Initializing serial port
    
    pinMode(RELAY, OUTPUT);
    RELAY_OFF();
    
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
  WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password); // Connecting to WiFi network
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    // By default, fauxmoESP creates it's own webserver on the defined port
  // The TCP port must be 80 for gen3 devices (default is 1901)
  // This has to be done before the call to enable()
  fauxmo.createServer(true); // not needed, this is the default value
  fauxmo.setPort(80); // This is required for gen3 devices

  // You have to call enable(true) once you have a WiFi connection
  // You can enable or disable the library at any moment
  // Disabling it will prevent the devices from being discovered and switched
  fauxmo.enable(true);
  // You can use different ways to invoke alexa to modify the devices state:
  // "Alexa, turn lamp two on"

  // Add virtual devices
  fauxmo.addDevice(LAMP_1);

fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    // Callback when a command from Alexa is received. 
    // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
    // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
    // Just remember not to delay too much here, this is a callback, exit as soon as possible.
    // If you have to do something more involved here set a flag and process it in your main loop.
        
    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
    if ( (strcmp(device_name, LAMP_1) == 0) ) {
      // this just sets a variable that the main loop() does something about
      Serial.println("RELAY 1 switched by Alexa");
      //digitalWrite(RELAY_PIN_1, !digitalRead(RELAY_PIN_1));
      if (state) {
        RELAY_ON();
      } else {
        RELAY_OFF();
      }
    }

  });
        
    
    server.begin(); // Starting the web server
    Serial.println("Web server Initiated. Waiting for the ESP IP...");
    delay(10000);
    
    Serial.println(WiFi.localIP()); // Printing the ESP IP address
}

// runs over and over again
void loop() {
  // fauxmoESP uses an async TCP server but a sync UDP server
  // Therefore, we have to manually poll for UDP packets
  fauxmo.handle();
  
    // Searching for new clients
    WiFiClient client = server.available();
    
    if (client) {
        Serial.println("New client");
        boolean blank_line = true; // boolean to locate when the http request ends
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                header += c;
                
                if (c == '\n' && blank_line) {
                    Serial.print(header);

                    // Finding the right credential string
                    if (header.indexOf("YWRtaW46YWRkbWlu") >= 0) { // 
                        //successful login
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-Type: text/html");
                        client.println("Connection: close");
                        client.println();
                        // turns the LED on and off
                        if (header.indexOf("GET / HTTP/1.1") >= 0) {
                            Serial.println("Main Web Page");
                        }
                        else if (header.indexOf("GET /RELAYon HTTP/1.1") >= 0) {
                            Serial.println("RELAY On");
                            relayState = "On";
                            RELAY_ON();
                        }
                        else if (header.indexOf("GET /RELAYoff HTTP/1.1") >= 0) {
                            Serial.println("RELAY Off");
                            relayState = "Off";
                            RELAY_OFF();
                        }
    
                        // Web page
                        client.println("<!DOCTYPE HTML>");
                        client.println("<html>");
                        client.println("<head>");
                        client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css\">");
                        client.println("</head><div class=\"container\">");
                        client.println("<h1>MicrocontrollersLab</h1>");
                        client.println("<h2>WebServer</h2>");
                        client.println("<h3>RELAY: State: " + relayState);
                        client.println("<div class=\"row\">");
                        client.println("<div class=\"col-md-2\"><a href=\"/RELAYon\" class=\"btn btn-block btn-lg btn-primary\" role=\"button\">ON</a></div>");
                        client.println("<div class=\"col-md-2\"><a href=\"/RELAYoff\" class=\"btn btn-block btn-lg btn-info\" role=\"button\">OFF</a></div>");
                        client.println("</div></div></html>");
                    }
                    else { // Http request fails for unauthorized users
                        client.println("HTTP/1.1 401 Unauthorized");
                        client.println("WWW-Authenticate: Basic realm=\"Secure\"");
                        client.println("Content-Type: text/html");
                        client.println();
                        client.println("<html>Authentication failed</html>");
                    }
                    header = "";
                    break;
                }
                
                if (c == '\n') { // starts reading a new line
                    blank_line = true;
                }
                else if (c != '\r') { // finds a character on the current line
                    blank_line = false;
                }
            }
        }
        delay(1);
        client.stop(); // ending the client connection
        Serial.println("Client disconnected.");
    }
}
