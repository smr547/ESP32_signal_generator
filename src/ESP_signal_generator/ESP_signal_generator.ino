/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include <WiFi.h>

// Replace with your network credentials
const char* ssid = "Bertie";
const char* password = "Ookie1234";

// Set web server port number to 80
WiFiServer server(80);

struct SwitchState {
  char const * id;
  uint8_t pin;
  byte state;
};

struct SwitchState switches[] = {
        {"sw0", 16, 0x00},
        {"sw1", 17, 0x00},
        {"sw2", 18, 0x00},
        {"sw3", 19, 0x00},
        {"sw4", 32, 0x00},
        {"sw5", 33, 0x00},
        {"sw6", 27, 0x00},
        {"sw7", 14, 0x00}
};

// PWM channels

struct PwmState {
  char const * id;
  char channelName[16];
  uint8_t pin;
  uint64_t frequency;
  uint16_t resolution;
  uint8_t duty;
};

struct PwmState pwmChannel[] = {
  {"H0","unnamed", 23, 0, 8, 0},
  {"H2","unnamed", 22, 0, 8, 0},
  {"H4","unnamed", 1, 0, 8, 0},
  {"H6","unnamed", 3, 0, 8, 0},
  {"L0","unnamed", 4, 0, 8, 0},
  {"L2","unnamed", 0, 0, 8, 0},
  {"L4","unnamed", 2, 0, 8, 0},
  {"L6","unnamed", 15, 0, 8, 0},
};
     

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";

// Assign output variables to GPIO pins
const int output26 = 26;
const int output27 = 27;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);

  // set switches

  for (int i=0; i < 8; i++) {
    pinMode(switches[i].pin, OUTPUT);
    digitalWrite(switches[i].pin, switches[i].state);
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
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
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            Serial.println(header);
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("GPIO 26 on");
              output26State = "on";
              digitalWrite(output26, HIGH);
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              output26State = "off";
              digitalWrite(output26, LOW);
            } else if (header.indexOf("GET /27/on") >= 0) {
              Serial.println("GPIO 27 on");
              output27State = "on";
              digitalWrite(output27, HIGH);
            } else if (header.indexOf("GET /27/off") >= 0) {
              Serial.println("GPIO 27 off");
              output27State = "off";
              digitalWrite(output27, LOW);
            } else {
              char buf[16];
              for (int i = 0; i< 8; i++) {
                sprintf(buf, "GET /%s/toggle", switches[i].id);
                if (header.indexOf(buf) >= 0) {
                  switches[i].state = switches[i].state ^ 0x01;
                  digitalWrite(switches[i].pin, switches[i].state);
                }
              }
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP32 Signal Generator</h1>");

            // display a table of switches

            client.println(F("<p>Switches</p>"));
            client.println(F("<table><tr><th>id</th><th>Pin</th><th>State</th><th>Action</th></tr>"));
            for (int i=0; i < 8; i++) {
              client.print(F("<tr><td>"));
              client.print(switches[i].id);
              client.print(F("</td><td>"));
              client.print(switches[i].pin);
              client.print(F("</td><td>"));
              if (switches[i].state == 0x00) {
                client.print(F("OFF"));
              } else {
                client.print(F("ON"));
              }

              // toggle button

              client.print(F("</td><td>"));
              client.print(F("<form action=\"/"));
              client.print(switches[i].id);
              client.print(F("/toggle\"><input type=\"submit\" value=\"Toggle\" formmethod=\"get\"></form>"));
              client.println(F("</td></tr>"));
            }
            client.print(F("</table>"));


            // display table of PWM channels
            client.println(F("<p>PWM Channels</p>"));
            client.println(F("<table><tr><th>id</th><th>Pin</th><th></th><th>Name</th><th>Frequency</th><th>Resolution</th><th>Duty</th><th>Action</th></tr>"));

            // char form[8] = "";
            for (int i=0; i < 8; i++) {
              char const * id = pwmChannel[i].id;
             // sprintf(form, "form%d", i);
              client.print(F("<tr><td>"));
              client.print(id);
              client.print(F("</td><td>"));
              client.print(pwmChannel[i].pin);
              client.print(F("</td><td>"));
              client.print(F("<form id =\""));
              client.print(id);
              client.print(F("\" formaction=\"get\" action=\"/"));
              client.print(id);
              client.print(F("\"><input type=\"hidden\" name=\"id\" value=\"1\" /></form></td><td>"));

              // channel name input
              client.print(F("<input form=\""));
              client.print(id);
              client.print(F("\" type=\"text\" name=\"chName\" value=\""));
              client.print(pwmChannel[i].channelName);
              client.print(F("\" /></td><td>"));

              
              // frequency input
              client.print(F("<input form=\""));
              client.print(id);
              client.print(F("\" type=\"text\" name=\"frequency\" value=\""));
              client.print(pwmChannel[i].frequency);
              client.print(F("\" /></td><td>"));

              
              // resolution input
              client.print(F("<input form=\""));
              client.print(id);
              client.print(F("\" type=\"text\" name=\"resolution\" value=\""));
              client.print(pwmChannel[i].resolution);
              client.print(F("\" /></td><td>"));

              
              // duty input
              client.print(F("<input form=\""));
              client.print(id);
              client.print(F("\" type=\"text\" name=\"duty\" value=\""));
              client.print(pwmChannel[i].duty);
              client.print(F("\" /></td><td>"));

              
              // Update button
              client.print(F("<input form=\""));
              client.print(id);
              client.print(F("\" type=\"submit\" value=\"Update\" /></td></tr>"));
            }
            client.print(F("</table>"));
            
            // Display current state, and ON/OFF buttons for GPIO 26  
            client.println("<p>GPIO 26 - State " + output26State + "</p>");
            // If the output26State is off, it displays the ON button       
            if (output26State=="off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 27  
            client.println("<p>GPIO 27 - State " + output27State + "</p>");
            // If the output27State is off, it displays the ON button       
            if (output27State=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            
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
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
