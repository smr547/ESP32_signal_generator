/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com
*********/

#undef DEBUG

// Load Wi-Fi library
#include <WiFi.h>

#include "secrets.h"
// Replace with your network credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_WIFI_PASSWORD;

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

#define CHNLEN 16
struct PwmState {
  const int channel;
  char const * id;
  char channelName[CHNLEN];
  uint8_t pin;
  uint64_t frequency;
  uint16_t resolution;
  uint8_t duty;
};

struct PwmState pwmChannel[] = {
  {0, "H0", "unnamed", 23, 0, 8, 0},
  {2, "H2", "unnamed", 22, 0, 8, 0},
  {4, "H4", "unnamed", 1, 0, 8, 0},
  {6, "H6", "unnamed", 3, 0, 8, 0},
  {8, "L0", "unnamed", 4, 0, 8, 0},
  {10, "L2", "unnamed", 0, 0, 8, 0},
  {12, "L4", "unnamed", 2, 0, 8, 0},
  {14, "L6", "unnamed", 15, 0, 8, 0},
};

// Two DAC channels

struct DacState {
  const uint8_t channel;
  const uint8_t pin;
  uint8_t value;
};

struct DacState dacChannel[] = {
  {0, 25, 0},
  {1, 26, 0}
};


// Variable to store the HTTP request

#define BUFLEN 128
struct Buffer {
  char buff[BUFLEN];  // array of characters
  uint16_t maxlen;    // maximum length
  uint16_t len;       // current len
};

uint8_t appendChar(Buffer &aBuff, const char c) {
  if (aBuff.len < aBuff.maxlen) {
    aBuff.buff[aBuff.len] = c;
    aBuff.len++;
    aBuff.buff[aBuff.len] = '\0';
    return 0;
  }
  return 1;
}

int16_t indexOf(Buffer &aBuff, const char * aString) {
  const char * ptr = strstr(aBuff.buff, aString);

#ifdef DEBUG
  Serial.println("");
  Serial.printf("Checking for '%s' in '%s'\n", aString, aBuff.buff);
  Serial.printf("strstr return %d\n", (long) ptr);
#endif //DEBUG

  if (ptr == NULL) {
    return -1;
  }
  return (int16_t) (ptr - aBuff.buff);
}

void clearBuff(Buffer &aBuff) {
  aBuff.buff[0] = '\0';
  aBuff.len = 0;
}

void setDacPin(DacState &ds) {
  dacWrite(ds.pin, ds.value);
}

void setPwmPin(PwmState &pwms) {
  ledcSetup(pwms.channel, pwms.frequency, pwms.resolution);
  ledcAttachPin(pwms.pin, pwms.channel);
  ledcWrite(pwms.channel, pwms.duty);
}

int processPwmUpdate(Buffer &cmd, PwmState &pwms) {
  // locate all the required parameters
  // ensure they are present and in range
  // update the state table entry
  // adjust the hardware settings

  // return 0 if OK, or
  // return positive error number


  // locate all required paremeters

  char * chName;  // channel name
  uint64_t f;     // frequency
  uint16_t r;     // resolution
  uint8_t d;      // duty

#ifdef DEBUG
  Serial.print("Processing state change for PWM channel ");
  Serial.print(pwms.id);
  Serial.println("");
#endif //DEBUG

  char * ptr = cmd.buff;
  char * field[5];
  for (int i = 0; i < 5; i++) {
    // look for next "=" delimiter
    ptr = strstr(ptr, "=");
    if (ptr == NULL) {
      return i * 2 + 1;
    }
    ptr++;
    field[i] = ptr;
  }

  chName = field[1];

  f = atoi(field[2]);

  // 0 <= r <= 16

  r = atoi(field[3]);

  if (r > 16) {
    r = 16;
  }
  if (r < 0) {
    r = 0;
  }

  // 0 <= d <= 100

  d = atoi(field[4]);

  if (d < 0) {
    d = 0;
  }
  if (d > 100) {
    d = 100;
  }


  // copy channel name

  for (int i = 0; i < (CHNLEN - 1); i++) {
    if (chName[i] == '\0' || chName[i] == '&') {
      break;
    }
    pwms.channelName[i] = chName[i];
    pwms.channelName[i + 1] = '\0';
  }

  // copy in integer parameters

  pwms.frequency = f;
  pwms.resolution = r;
  pwms.duty = d;

  return 0;
}




// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


void sendResponseHTML(WiFiClient &client) {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

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
  for (int i = 0; i < 8; i++) {
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
  for (int i = 0; i < 8; i++) {
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
  client.print(F("</table><p>Full documentation and code at <a href=\"https://github.com/smr547/ESP32_signal_generator\">GitHub Repo</a></p></body></html>"));
  

  // The HTTP response ends with another blank line
  client.println();
}

void processRequest(Buffer &header) {
#ifdef DEBUG
  Serial.println(header.buff);
#endif //DEBUG

  char buf[16];
  for (int i = 0; i < 8; i++) {
    sprintf(buf, "GET /%s/toggle", switches[i].id);
    if (indexOf(header, buf) >= 0) {
      switches[i].state = switches[i].state ^ 0x01;
      digitalWrite(switches[i].pin, switches[i].state);
      break;
    }
    sprintf(buf, "GET /%s?id=", pwmChannel[i].id);
#ifdef DEBUG
    Serial.printf("Checking for: ");
    Serial.println(buf);
#endif //DEBUG
    if (indexOf(header, buf) >= 0) {
      if (processPwmUpdate(header, pwmChannel[i]) == 0) {
        // set PWM parameters
        setPwmPin(pwmChannel[i]);
      }
      break;
    }
  }
}

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif //DEBUG

  // set switches

  for (int i = 0; i < 8; i++) {
    pinMode(switches[i].pin, OUTPUT);
    digitalWrite(switches[i].pin, switches[i].state);
  }

  // Connect to Wi-Fi network with SSID and password
#ifdef DEBUG
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif //DEBUG
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif //DEBUG
  }
  // Print local IP address and start web server
#ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif //DEBUG
  server.begin();
}

void loop() {
  struct Buffer header = {"", BUFLEN - 1, 0};
  struct Buffer currentLine = {"", BUFLEN - 1, 0};

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
#ifdef DEBUG
    Serial.println("New Client.");          // print a message out in the serial port
#endif //DEBUG
    clearBuff(currentLine);                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
#ifdef DEBUG
        Serial.write(c);                    // print it out the serial monitor
#endif //DEBUG
        appendChar(header, c);
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.len == 0) {


            processRequest(header);
            sendResponseHTML(client);

            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            clearBuff(currentLine);
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          appendChar(currentLine, c);      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    clearBuff(header);
    // Close the connection
    client.stop();
#ifdef DEBUG
    Serial.println("Client disconnected.");
    Serial.println("");
#endif //DEBUG
  }
}
