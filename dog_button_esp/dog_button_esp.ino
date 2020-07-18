// Revised dog button.

//#include "osc.h"
//OSCOUTGOINGQUEUE oscOutgoingQueue[4];

#include <ESP8266WiFi.h>
#include <Ticker.h>

#include <WiFiClient.h>

#include <ESP8266WebServer.h>   // Include the WebServer library

const char* ssid = "marshmallowPrime";
const char* password = "marshmarsh6";

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();
void handleAttention();
bool parseSerialBuffer();
bool addCharToSerialBuffer(char _newChar);
void switchMode(uint8_t _mode);
bool clearSerialBuffer();
void switchAddress(uint8_t address);
void playSequenceNote(uint8_t note);

IPAddress staticIP(10, 0, 0, 5); //ESP static ip
IPAddress gateway(10, 0, 0, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(8, 8, 8, 8);  //DNS


Ticker tenthsofasecond;



const char MAIN_page[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html>
  <body>
  <center>
  <h1>WiFi LED on off demo: 1</h1><br>
  Ciclk to turn <a href="ledOn">LED ON</a><br>
  Ciclk to turn <a href="ledOff">LED OFF</a><br>
  <hr>
  <a href="https://circuits4you.com">circuits4you.com</a>
  </center>
  
  </body>
  </html>
)=====";




// Device-specific settings

#define DEVICE_ADDRESS   3    // I don't think I'll ever need to change this in the field.


// I/O

#define PIN_SPEAKER  D2

#define PIN_BUTTON  D7

#define PIN_ONBOARDLED 0

#define PIN_LIGHT    D3

#define PIN_LEDRED   0

#define PIN_LEDGREEN 0

#define PIN_LEDBLUE  0




uint8_t victoryDebounce;
bool suppressInitialFire;
uint8_t ultrasonic = false;


// System

#define MODE_IDLE      0
#define MODE_ATTENTION 1
#define MODE_VICTORY   2
#define MODE_STARTUP   3

uint8_t myAddress, mode;


// Timer

int period = 2500;
uint8_t spinner, commTimer, standbyTimer;
bool communicationsUp;


// Debug console

#define console Serial
// #define VERBOSE 0


// Network

#define BUFFER_LENGTH 30
char buf[BUFFER_LENGTH];
uint8_t cur;
bool startSerial;


// Music

int musicalNotes[12] = {523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988};  // Starting with middle C
int n1, n2, n3;

#define ATTENTION_TONE_LENGTH 22
uint8_t attentionSequence[ATTENTION_TONE_LENGTH] = {7, 99, 8, 99, 9, 99, 10, 99, 9, 99, 8, 99, 7, 99, 0, 0, 0, 0, 0, 0, 0, 0};

#define VICTORY_TONE_LENGTH 27
uint8_t victorySequence[VICTORY_TONE_LENGTH] = {1, 2, 1, 2, 3, 2, 3, 4, 3, 4, 5, 4, 5, 6, 5, 6, 7, 6, 7, 8, 7, 8, 9, 8, 9, 10, 99};

#define STARTUP_TONE_LENGTH 9
uint8_t startupSequence[STARTUP_TONE_LENGTH] = {4, 5, 6, 7, 0, 99, 99, 99, 99};  // The extra 99's are so that we can test the lights

uint8_t victoryMusicTimer, attentionMusicTimer, startupMusicTimer, attentionCycles, attentionGap;
uint16_t attentionTimer = 0;


bool buttonTest = false;
bool silent = 0;



bool handleNewSerialChar(char _newChar)
{
  if (!communicationsUp)
  {
    communicationsUp = true;
    // tone(PIN_SPEAKER, n1 * 2, 50);
    commTimer = 50;
  }
  
  switch (_newChar)
  {
    case 0:
    case 10:
    case 13:
    case '.': parseSerialBuffer();             break;
    default:  addCharToSerialBuffer(_newChar); break;
  }
  
  return true;
}



bool addCharToSerialBuffer(char _newChar)
{
  if (cur < BUFFER_LENGTH)
  {
    buf[cur] = _newChar;
    cur++;
  }
  else
    return false;
    
  return true;
}


void printWithSpecialCharacters(char * _message)
{
  for (uint8_t chr = 0; chr < strlen(_message); chr++)
  {
    char thisChar = _message[chr];
    
    if (thisChar < 32 || thisChar > 127)
    {
      Serial.print("[");
      Serial.print(thisChar, DEC);
      Serial.print("]");
    }
    else
      Serial.print(thisChar);
  }
  
  Serial.println();
}


bool parseSerialBuffer()
{
  if (strcmp("attention", buf) == 0)
  {
    switchMode(MODE_ATTENTION);
  }
  else if (strcmp("victory", buf) == 0)
  {
    switchMode(MODE_VICTORY);
  }
  else if (strcmp("idle", buf) == 0)
  {
    switchMode(MODE_IDLE);
  }
  else if (strcmp("silent_on", buf) == 0)
  {
    silent = 1;
    Serial.println("#silent mode on.");
  }
  else if (strcmp("silent_off", buf) == 0)
  {
    silent = 0;
    Serial.println("#silent mode off.");
  }
  else if (strcmp("button_test_on", buf) == 0)
  {
    Serial.println("#button test on.");
    buttonTest = true;
  }
  else if (strcmp("button_test_off", buf) == 0)
  {
    Serial.println("#button test off.");
    buttonTest = false;
  }
  else if (strcmp("battery", buf) == 0)
  {
    Serial.print("#battery ");
    Serial.print(analogRead(0));
    Serial.println(".");
  }
  else if (strcmp("ultrasonic_on", buf) == 0)
  {
    ultrasonic = true;
    Serial.println("#ultrasonic on");
  }
  else if (strcmp("battery", buf) == 0)
  {
    ultrasonic = false;
    Serial.println("#ultrasonic off");
  }
  else if (strcmp("help", buf) == 0)
  {
    Serial.println("attention, victory, idle, silent_on, silent_off, battery, help, ping, ultrasonic_on, ultrasonic_off");
    buttonTest = false;
  }
  else if (strcmp("ping", buf) == 0)
    Serial.println("#pong.");
  else
  {
    Serial.print("#command_not_found: ");
    printWithSpecialCharacters(buf);
  }
    
  clearSerialBuffer();
}


bool clearSerialBuffer()
{
  for (uint8_t thisChar = 0; thisChar < BUFFER_LENGTH; thisChar++)
    buf[thisChar] = 0;
    
  cur = 0;
}




void switchAddress(uint8_t address)
{
  if (address < 12)
  {
    myAddress = address;
    address--;    // This is so we start at zero on the notes table
    
    n1 = musicalNotes[address];
    
    if ((address + 4) >= 12)
      n2 = musicalNotes[address - 8] * 2;
    else
      n2 = musicalNotes[address + 4];
      
    if ((address + 7) >= 12)
      n3 = musicalNotes[address - 5] * 2;
    else
      n3 = musicalNotes[address + 7];
  }
}






void playSequenceNote(uint8_t note)
{
  uint32_t freq = 0;
  
  switch(note)
  {
    case 0: noTone(PIN_SPEAKER); break; //analogWrite(PIN_SPEAKER,0); break;
  
    case 1: freq = n1 / 2; break;
    case 2: freq = n2 / 2; break;
    case 3: freq = n3 / 2; break;
    case 4: freq = n1; break;
    case 5: freq = n2; break;
    case 6: freq = n3; break;
    case 7: freq = n1 * 2; break;
    case 8: freq = n2 * 2; break;
    case 9: freq = n3 * 2; break;
    case 10: freq = n1 * 4; break;
    default: break;
  }

  if (freq && !silent)
  {
    if (ultrasonic)
    {
      tone(PIN_SPEAKER, freq * 40);
    }
    else
    {
      tone(PIN_SPEAKER, freq);
    }
  }
}





void printAddress()
{
  Serial.print("(");
  Serial.print(DEVICE_ADDRESS);
  Serial.print(")");
}



void switchMode(uint8_t _mode)
{
  if (mode != _mode) // mostly to avoid the 10x request bug as a hack but there isn't really a point in switching mode if it's the same
  {
    mode = _mode;
    
    switch (_mode)
    {
      case MODE_STARTUP:
      {
        startupMusicTimer = 0;
        break;
      }
      case MODE_IDLE:
      {
        Serial.print("#");
        printAddress();
        Serial.println(" idle.");
        digitalWrite(PIN_LIGHT, LOW);
        digitalWrite(PIN_ONBOARDLED, LOW);
        noTone(PIN_SPEAKER);
        //analogWrite(PIN_SPEAKER,0);
        break;
      }
      case MODE_ATTENTION:
      {
        Serial.print("#");
        printAddress();
        Serial.println(" attention.");
        digitalWrite(PIN_LEDRED, HIGH);
        digitalWrite(PIN_LEDGREEN, HIGH);
        attentionMusicTimer = 0;
        attentionGap = 0;
        attentionCycles = 0;
        attentionTimer = 0;
        break;
      }
      case MODE_VICTORY:
      {
        Serial.print("#");
        printAddress();
        Serial.print(" victory ");
        Serial.print(attentionTimer);
        Serial.println(".");
        
        digitalWrite(PIN_LEDRED, LOW);
        digitalWrite(PIN_LEDGREEN, HIGH);
        victoryMusicTimer = 0;
        break;
      }
      default: break;
    }
  }
}



void victory()
{
  Serial.println("victory function");
  
  if (buttonTest)
    Serial.println(digitalRead(PIN_BUTTON));
    
  if (!suppressInitialFire)
  {
    detachInterrupt(PIN_BUTTON);
    victoryDebounce = 10;
    
    if (mode == MODE_ATTENTION)
      switchMode(MODE_VICTORY);
  }
  else
  {
    suppressInitialFire = false;
  }
}



//void ICACHE_RAM_ATTR onTimerISR()
void tenthsfunction()
{
  // OCR1A = period;
  //timer1_write(500000);
  
  spinner++;    // Rotates around forever.  Used for pulses and sequences.
  
  if (mode == MODE_ATTENTION)
  {
    digitalWrite(PIN_LIGHT, bitRead(spinner, 2));
    digitalWrite(PIN_ONBOARDLED, bitRead(spinner, 2));
    
    attentionTimer++;
    
    if (attentionMusicTimer > ATTENTION_TONE_LENGTH + attentionGap)
    {
      noTone(PIN_SPEAKER);
      //analogWrite(PIN_SPEAKER,0);
      
      attentionMusicTimer = 0;
      attentionCycles++;
      
      if (attentionCycles >= 4)
      {
        if (attentionCycles >= 9)
          attentionGap = ATTENTION_TONE_LENGTH * 2;
        else
          attentionGap = ATTENTION_TONE_LENGTH;
      }
   }

   if (attentionMusicTimer < ATTENTION_TONE_LENGTH)
     playSequenceNote(attentionSequence[attentionMusicTimer]);

    
    attentionMusicTimer++;
  }
  else if (mode == MODE_VICTORY)
  { 
    digitalWrite(PIN_LIGHT, bitRead(spinner, 1));
    digitalWrite(PIN_ONBOARDLED, bitRead(spinner, 1));
    
    if (victoryMusicTimer >= VICTORY_TONE_LENGTH)
    {
      switchMode(MODE_IDLE);
      victoryMusicTimer = 0;
      noTone(PIN_SPEAKER);
      //analogWrite(PIN_SPEAKER,0);
    }
    else
    {
      playSequenceNote(victorySequence[victoryMusicTimer]);
    }
    
    victoryMusicTimer++;
  }
  else if (mode == MODE_STARTUP)
  {
    switch(startupMusicTimer)
    {
      case 0: digitalWrite(PIN_LEDGREEN, HIGH); break;
      case 2: digitalWrite(PIN_LEDGREEN, LOW);  digitalWrite(PIN_LEDBLUE, HIGH); break;
      case 4: digitalWrite(PIN_LEDBLUE, LOW);   digitalWrite(PIN_LIGHT, HIGH); digitalWrite(PIN_ONBOARDLED, HIGH);  break;
      case 6: digitalWrite(PIN_LIGHT, LOW); digitalWrite(PIN_ONBOARDLED, LOW);     digitalWrite(PIN_LEDRED, HIGH); break;
      default: break;
    }
    
    if (startupMusicTimer >= STARTUP_TONE_LENGTH)
    {
      switchMode(MODE_IDLE);
      startupMusicTimer = 0;
      noTone(PIN_SPEAKER);
      //analogWrite(PIN_SPEAKER,0);
    }
    else
    {
      playSequenceNote(startupSequence[startupMusicTimer]);
    }
    
    startupMusicTimer++;
  }
  
  if (victoryDebounce)
  {
    victoryDebounce--;
    
    if (!victoryDebounce)
    {
      if (digitalRead(PIN_BUTTON) == LOW)
      {
        victoryDebounce = 1;
      }
      else
      {
        attachInterrupt(PIN_BUTTON, victory, FALLING);
        suppressInitialFire = true;
      }
    }
  }
  
  if (commTimer)
  {
    commTimer--;
    
    if (!commTimer)
    {
      communicationsUp = false;
      // tone(PIN_SPEAKER, n1, 50);
    }
  }
}





void setup()
{
  
  Serial.begin(74880);
  Serial.println("setup");


  
  pinMode(PIN_SPEAKER, OUTPUT);
  pinMode(PIN_LEDRED, OUTPUT);
  pinMode(PIN_LEDGREEN, OUTPUT);
  pinMode(PIN_LEDBLUE, OUTPUT);
  pinMode(PIN_LIGHT, OUTPUT);
  pinMode(PIN_ONBOARDLED, OUTPUT);
  
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Serial.println("inputs set");


  
  //digitalWrite(PIN_BUTTON, HIGH);  // pull-up
  Serial.println("pull up");
  
    
  switchAddress(DEVICE_ADDRESS);
  Serial.println("switched address");
  
  // Set up the interrupts
  //attachInterrupt(PIN_BUTTON, victory, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), victory, FALLING);
  Serial.println("attached interrupt");
  suppressInitialFire = true;            // Prevents the interrupt from firing immediately after being attached

  tenthsofasecond.attach(0.04, tenthsfunction);
//  timer1_attachInterrupt(onTimerISR);
//  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
//  timer1_write(500000);
  
  mode = MODE_STARTUP;  // We have started up

  

  WiFi.begin(ssid, password);             // Start the access point

  ESP.eraseConfig();
  WiFi.disconnect();
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);

  
  WiFi.disconnect();
  WiFi.config(staticIP, subnet, gateway, dns);
  WiFi.begin(ssid, password);

  WiFi.mode(WIFI_STA); //WiFi mode station (connect to wifi router only

    Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");

  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  

  server.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/")
  server.on("/attention", handleAttention);

  server.begin();                  //Start server
  Serial.println("HTTP server started");
}


void handleRoot()
{
  String root = "<html><head><title>dog buttons</title><script type='text/javascript'>\n"
  "function loadXMLDoc(url,cfunc) {\n"
    "if (window.XMLHttpRequest) {\n"
          "xmlhttp=new XMLHttpRequest(); }\n"  // code for IE7+, Firefox, Chrome, Opera, Safari
    "xmlhttp.onreadystatechange=cfunc;\n"
    "xmlhttp.open('GET',url,true);\n"
    "xmlhttp.send(); }\n\n"
    "function sendAttention(_request) {\n"
    "loadXMLDoc(_request, function() {\n"
        "if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n"
            "document.getElementById(_item).innerHTML = xmlhttp.responseText;}});}\n"
     "</script></head><body>\n"
     "<button onclick=\"sendAttention('http://10.0.0.4/attention')\">4</button>\n"
     "<button onclick=\"sendAttention('http://10.0.0.5/attention')\">5</button>\n"
     "<button onclick=\"sendAttention('http://10.0.0.6/attention')\">6</button>\n "
     "</body></html>";
    
  
 server.send(200, "text/html", root); //Send web page
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}



uint32_t lastAttention = 0;

void handleAttention()
{
  Serial.println("handleAttention() called");
  if (millis() > lastAttention + 5000)  // HACK because I have no goddamn idea why /attention gets called exactly ten times
  {
    switchMode(MODE_ATTENTION);
    server.send(200, "text/plain", "attention");
    lastAttention = millis();
  }
}




void loop()
{
  if (Serial.available())
  {
    handleNewSerialChar(Serial.read());
    Serial.println("received serial");
  }

  server.handleClient();
}
