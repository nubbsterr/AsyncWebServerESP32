# AsyncWebServerESP32
An implementation of the `ESPAsyncWebServer` library for one of my Computer Engineering projects. An example for anybody who needs it :)

# Project Info
The script I used here was to control 2 separate DC and Stepper motors to drive what can be summed up as a 2D printer. That is all that needs to be known if you ask me, but if you wanna learn how I made this, feel free to contact me thru email or my discord username: <strong>nubbieeee</strong>

The big ESP32 stuff happens in `void setup()`, which is where we connect to WiFi and launch our web server. Everything involving request sending and receiving is super simple once you think it through like I did. You guys can test this site out as long as you:

1) <strong>Change the SSID and Password
2) *Have an ESP32*</strong>

Das all you need to make a lovely HTML page that can control a massive 2D printer :)
