

//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>

#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <AsyncTCP.h>
#include <SPI.h>
#include <SD.h>

#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>



#include <ESP8266SAM.h>
#include <AudioOutputI2S.h>

#include <Adafruit_NeoPixel.h>





// PIN  Definitions if usinng uart
// MTXO=17
// MRXI=16
//#define AUDIO_SERIAL Serial2
// These definitions aren't actually used but included for completeness
// #define PIN_AUDIO_MTXO 17
// #define PIN_AUDIO_MRXI 16

//Using software serial
// #define PIN_AUDIO_MTXO 26
// #define PIN_AUDIO_MRXI 25

// I2S PINs

#define PIN_AUDIO_BCLK 26 // Yellow
#define PIN_AUDIO_WCLK 25 // Orange
#define PIN_AUDIO_DOUT 14 // Blue

#define SERVO_MOVES


#define PIN_GOB_SERVO 13


#define PIN_NEOPIXEL 15
#define NUMPIXELS 2

Servo gob;
int pos=0;

AsyncWebServer server(80);

// const char* ssid = "Edmob";
// const char* password = "hgohgohgo";
const char* home_ssid = "spotlessman";
const char* home_password = "asurplusof99turnips";
const char* tethered_ssid = "Edmob";
const char* tethered_password = "hgohgohgo";

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_RGB + NEO_KHZ800);


AudioOutputI2S *out = NULL;
ESP8266SAM *sam;


bool speaking=false;

const char* PARAM_MESSAGE = "message";

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void move_servo(int pos)
{
    #ifdef SERVO_MOVES
    gob.write(pos);
    #endif
}

void set_eye_color(uint8_t r,uint8_t g,uint8_t b)
{
        pixels.setPixelColor(0, pixels.Color(r,g,b));
        pixels.setPixelColor(1,pixels.Color(r,g,b));
        pixels.show();   // Send the updated pixel colors to the hardware.
}

void speak(const char * words)
{
    speaking=true;
    for (pos = 0; pos <= 180; pos += 1)
    { // goes from 0 degrees to 180 degrees
            // in steps of 1 degree
            move_servo(pos);    // tell servo to go to position in variable 'pos'
            delay(3);             // waits 15ms for the servo to reach the position
    }


    sam->Say(out, words);

    for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
        move_servo(pos);    // tell servo to go to position in variable 'pos'
        delay(3);             // waits 15ms for the servo to reach the position
    }
    speaking=false;

}


void setup() {

    Serial.begin(115200);
    delay(500);
    Serial.println("Running...");


      // Initialize SPIFFS
    while(!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        delay(1000);
    }

    ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	gob.setPeriodHertz(50);    // standard 50 hz servo
	gob.attach(PIN_GOB_SERVO, 1000, 2000); // attaches the servo on pin 18 to the servo object
    move_servo(0);
    delay(1000);
    move_servo(90);
    delay(1000);
    move_servo(-90);
    pixels.begin();
    pixels.clear(); 
    while (false)
    {
        
        // Set all pixel colors to 'off'

  // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.
        for(int c=0; c<100; c++)
        { // For each pixel...

        // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
        // Here we're using a moderately bright green color:
        pixels.setPixelColor(0, pixels.Color(100-c,c, 0));
        pixels.setPixelColor(1,pixels.Color(0,c,100-c));

        pixels.show();   // Send the updated pixel colors to the hardware.

        delay(50); // Pause before next pass through loop
        }
    }
    set_eye_color(200,200,0);


    out = new AudioOutputI2S();
    out->SetPinout(PIN_AUDIO_BCLK,PIN_AUDIO_WCLK,PIN_AUDIO_DOUT);
    out->begin();

    sam = new ESP8266SAM;
    //sam->Say(out, "Can you hear me now?");
    // delay(500);
    
    






    // delay(2000);
    // speak("You humans are impossible.");
    // delay(200);
    // speak("taystee yes.");
    // speak("but still impossible.");



    while (true) // Loop to try each wifi in turn until it connects
    {

        WiFi.mode(WIFI_STA);
        WiFi.begin(tethered_ssid, tethered_password);
        uint8_t wifi_res=WiFi.waitForConnectResult();
        Serial.print("Result of trying to tether: ");
        Serial.println(wifi_res);
        if ( wifi_res!= WL_CONNECTED)
        {
            Serial.printf("WiFi tethered Failed!\n");
            
        } else {
            speak("I am tethered.");
            break;
        }
        delay(1000);
        WiFi.disconnect(true);
        delay(3000);
        WiFi.mode(WIFI_STA);
        WiFi.begin(home_ssid, home_password);
        wifi_res=WiFi.waitForConnectResult();
        Serial.print("Result of trying to connect at home: ");
        Serial.println(wifi_res);
        if (wifi_res != WL_CONNECTED)
        {
            Serial.printf("WiFi home Failed!\n");
        } else {
            speak("I am at home.");
            break;
        }
        delay(1000);
        WiFi.disconnect(true);
        delay(3000);


    }
    WiFi.setAutoReconnect(true);
    

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    IPAddress i;

    char say_ip_buffer[200];

    sprintf(say_ip_buffer,"%s.",WiFi.localIP().toString().c_str());
    speak(say_ip_buffer);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
        //request->send(200, "text/plain", "Hello, world");
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/speak", HTTP_GET, [] (AsyncWebServerRequest *request) {
        // while(speaking)
        // {
        //     Serial.println("Waiting for previous to finish");
        //     delay(600);
        // }
        String message;
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
            speak(message.c_str());
        } else {
            message = "No message sent";
        }
        
        request->send(200, "text/plain", "Hello, GET: " + message);
    });

    server.on("/speaking",
            HTTP_GET,
            [] (AsyncWebServerRequest * request)
            {
                request->send(200,"text/plain",speaking?"BUSY":"QUIET");
            });

    server.on("/set_eye_color",HTTP_GET,[](AsyncWebServerRequest * request)
            {
                int32_t red;
                int32_t green;
                int32_t blue;
                if (request->hasParam("red") && request->hasParam("green") && request->hasParam("blue"))
                {
                    red=request->getParam("red")->value().toInt();
                    green=request->getParam("green")->value().toInt();
                    blue=request->getParam("blue")->value().toInt();
                    set_eye_color(red,green,blue);
                    char tempbuffer[200];
                    sprintf(tempbuffer,"Eye color rgb set to: (%d,%d,%d)\n",red,green,blue);
                    request->send(200, "text/plain", tempbuffer);
                } else {
                    request->send(200, "text/plain", "red, green or blue parameter missing");
                }
            }
        );

    server.on("/set_mouth",HTTP_GET,[](AsyncWebServerRequest * request)
            {
                String angle_s;
                int32_t angle;
                if (request->hasParam("angle"))
                {
                    angle_s=request->getParam("angle")->value();
                    Serial.printf("Mouth angle requested: %s\n",angle_s.c_str());
                    delay(500);
                    angle=angle_s.toInt();
                    move_servo(angle);
                    request->send(200, "text/plain", "Mouth angle set to : " + angle_s);
                } else {
                    request->send(200, "text/plain", "Mouth angle not received in request");
                }
            }
        );



    // Send a POST request to <IP>/post with a form field message set to <message>
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
        String message;
        if (request->hasParam(PARAM_MESSAGE, true)) {
            message = request->getParam(PARAM_MESSAGE, true)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, POST: " + message);
    });

    server.onNotFound(notFound);

    server.begin();
}

void loop()
{


}