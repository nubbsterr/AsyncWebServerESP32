/* Some code and suggested libraries were taken from: 
 * https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/
 * The only bits that were used from the above link are the suggested libraries, the <head> section of the html code
 * and the basic GET request sending logic. The rest of the code is of my doing and is purpose-built for this project :)
 */

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ENA is the enable pin to control DC motor speed (x-axis), set to GPIO 16
// STEPDELAY is in milliseconds to delay stepper and DC motor
#define ENA 16
#define STEPDELAY 100

// pin definitions and other variables
// bools are used to call stepX/stepY functions in loop() so they can run continuously and at the same time
volatile bool xMotorDir { false };
volatile bool yMotorDir { false };
volatile bool xMotorStop { true };
volatile bool yMotorStop { true };

const int IN1_DC { 17 };               // IN1 pin to control OUT1 for DC motor (x-axis)
const int IN2_DC { 18 };               // IN2 pin to control OUT2 for DC motor (x-axis)

// STEP and DIRECTION pins for stepper driver for Nema 17/SM24240 motor
const int DIR { 20 };
const int STEP { 21 };

// ssid and password for local network to host web server on, also parameters to send with GET requests to ESP
const char* ssid { "nubblocal" };
const char* password { "fgzd4823" };

const char* param_1 { "motor" }; // 0 = x-axis/DC, 1 = y-axis/Stepper
const char* param_2 { "dir" };   // same rules as stepX/Y functions, 1 on dc/step = right/down, 0 on dc/step = left/up, 2 = stop motor

AsyncWebServer server(80);       // start web server on port 80 for request/response to clients and web servers

const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
  <html>
    <head>
      <title>2DPrinter Local ESP32 Web Server</title>
      <meta name="viewport" content="width=device-width, inital-scale=1">
      <link rel="icon" href="data:,">
      <style>
        h1 {font-family: Courier New}
        button {font-family: Courier New}
        p {font-family: Courier New}
      </style>
    </head>
    <body>
      <h1>2DPrinter async web server!!!</h1>
      <!-- direction buttons below!-->
      <button type="button" onclick="sendRequest(1,0)">Move Up</button>
      <button type="button" onclick="sendRequest(1,1)">Move Down</button>
      <button type="button" onclick="sendRequest(0,1)">Move Right</button>
      <button type="button" onclick="sendRequest(0,0)">Move Left</button>
      <button type="button" onclick="sendRequest(0,2)">Turn Off DC</button>
      <button type="button" onclick="sendRequest(1,2)">Turn Off Stepper</button>
      <p>Peak website design right here</p>
      <!-- Below function opens new GET request then sends request with motor and dir after button presses above -->
      <!-- Need to test for function scope, whether or not we need to move it around -->
      <script>
      function sendRequest(motor, dir)
      {
        var xhr = new XMLHttpRequest(); 
        xhr.open("GET", "/update?motor="+motor+"&dir="+dir, true);
        xhr.send();
      }
      </script>
    </body>
  </html>
)rawliteral";

void serverNotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Page Not Found");
}

// controls DC motor of gantry belt (x-axis) in small steps to mimic stepper motor (micro/millisecond delay)
void stepX(bool direction)
{
  if (!(direction))   // drive left
  {
    digitalWrite(IN1_DC, LOW);
    digitalWrite(IN2_DC, HIGH);
    delay(STEPDELAY);
  }
  else if (direction) // drive right
  {
    digitalWrite(IN1_DC, HIGH);
    digitalWrite(IN2_DC, LOW);
    delay(STEPDELAY);
  }
}

// controls stepper motor of screw/nut + metal rod mechanism
void stepY(bool direction)
{
  if (!(direction))   // drive clockwise/up
  {
    digitalWrite(DIR, LOW);
    digitalWrite(STEP, HIGH);
    delay(STEPDELAY);
    digitalWrite(STEP, LOW);
    delay(STEPDELAY);
  }
  else if (direction) // drive counterclockwise/down
  {
    digitalWrite(DIR, HIGH);
    digitalWrite(STEP, HIGH);
    delay(STEPDELAY);
    digitalWrite(STEP, LOW);
    delay(STEPDELAY);
  }
}

void setup()
{
  pinMode(IN1_DC, OUTPUT);
  pinMode(IN2_DC, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(DIR, OUTPUT);
 
  Serial.begin(9600);

  // set speed for DC motor and shut it off on startup
  analogWrite(ENA, 150);
  digitalWrite(IN1_DC, LOW);
  digitalWrite(IN2_DC, LOW);
  
  // start wifi connection of esp32 to local network, 
  WiFi.begin(ssid, password);
  Serial.print("Connecting to local network...");

  while (WiFi.status() != WL_CONNECTED) // not connected to wifi yet
  {
    Serial.print(",");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected successfully at IP: ");
  Serial.println(WiFi.localIP());

  // send first request to server and turn it on, server set at root directory, GET requests used handled by AsyncServer. functio
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
  {
    request->send(200, "text/html", index_html); // sends html code above as web servers' webpage
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    // construct two instances of String class for both params on GET request received by esp32
    String motorParam;
    String dirParam;

    if (request->hasParam(param_1) && request->hasParam(param_2))
    {
      // get parameters sent from GET request
      motorParam = request->getParam(param_1)->value();
      dirParam = request->getParam(param_2)->value();

      // determine the motor to control and move it accordingly
      if (motorParam.toInt() == 0)
      {
        if (dirParam.toInt() == 2)
        {
          // shut off DC motor
          xMotorStop = true;
          Serial.println("DC Motor Off!");
        }
        else
        {
          // compare val of motorParam to drive either left or right if 1 or 0
          // 1 = right, 0 = left
          xMotorStop = false;
          dirParam.toInt() ? xMotorDir = true : xMotorDir = false;
        }
      }
      if (motorParam.toInt() == 1)
      {
        if (dirParam.toInt() == 2)
        {
          yMotorStop = true;
          Serial.println("Stepper Motor Off!");
        }
        else
        {
          // 1 = cc/down, 0 = clockwise/up 
          yMotorStop = false;
          dirParam.toInt() ? yMotorDir = true : yMotorDir = false; 
        }
      }
    }
    request->send(200, "text/plain", "OK"); // send response after successful request
  });
 
  server.onNotFound(serverNotFound);
  server.begin(); // starts serving web page index_html on web server
}

void loop() // drive motors continuously depending on request info from web server
{
  if (xMotorStop) // 1 = right, 0 = left
  {
    digitalWrite(IN1_DC, LOW);
    digitalWrite(IN2_DC, LOW);
  }
  else if (!(xMotorStop))
  {
    stepX(xMotorDir);
  }
  
  if (yMotorStop) // 1 = cc/down, 0 = clockwise/up
  {
    stepY(yMotorDir);
  }
  else if (!(yMotorStop))
  {
    stepY(yMotorDir);
  }
}
