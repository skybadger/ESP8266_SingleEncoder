/*
 Program to talk to incremental rotary encoder and provide REST web interface and send output to MQTT topic /skybadger/device/encoder
 Supports REST web interface on port 80 returning application/json string
 Concept is an encoder measures rotation of its rotor. 
 Put a wheel of known diameter on that and it measures distance 
 That distance can be unbounded as far as the int32 position variable will rollover at _INT_MAX_. 
 Put a rollover value on it and it will measure a larger diameter/distance than just the wheel on the rotor
 Specify the rollover to be the same as the wheel diameter and the encoder just measures rotation at the resolution of the encoder.
 One of the things I want is to use the encoder as a compass replacement - the encoder is driven by a larger circle so the rollover and wheel dimater are important. 
 
To do:

Done: 
OTA update
EEPRom support
Status settings
REST API
Tested electronics output using 12v VSS and p-channel mosfet attached to pin 3 to control encoder power-up sequence. 

 Layout:
 GPIO 4,2 to SDA
 GPIO 5,0 to SCL 
 All 3.3v logic. 
 
 */
//Specify which pins - device dependent
//#define _ESP8266_12
#define _ESP8266_01
#define DEBUG

//Comment if you want interrupts and you have added the 'ICACHE_RAM_ATTR' to the 
//ISR handler functions in the encoder interrupts.h file. 

//#define ENCODER_DO_NOT_USE_INTERRUPTS

#include "DebugSerial.h"
#include <esp8266_peri.h> //register map and access
#include <ESP8266WiFi.h>
#include <Wire.h> //needed for common_funcs
#include <PubSubClient.h> //https://pubsubclient.knolleary.net/api.html
#include <EEPROM.h>
#include <Time.h>         //Look at https://github.com/PaulStoffregen/Time for a more useful internal timebase library
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoJson.h>  //https://arduinojson.org/v5/api/

#include <Encoder.h>      //http://www.pjrc.com/teensy/td_libs_Encoder.html
#include <Math.h>         //Used for PI.

//Ntp dependencies - available from ESP v2.4
#include <time.h>
#include <sys/time.h>
#include <coredecls.h>
#define TZ              0       // (utc+) TZ in hours
#define DST_MN          60      // use 60mn for summer time in some countries
#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
time_t now; //use as 'gmtime(&now);'

const int MAX_NAME_LENGTH = 25;

#include "SkybadgerStrings.h"
char* defaultHostname        = "espENC01";
char* thisID                 = nullptr;
char* myHostname             = nullptr;

WiFiClient espClient;
PubSubClient client(espClient);
volatile bool callbackFlag = false; //Flag for MQTT callback
volatile bool timerSet = false; 

// Create an instance of the web server
// specify the port to listen on as an argument
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

//Hardware device system functions - reset/restart etc
EspClass device;
ETSTimer timer, timeoutTimer;
volatile bool newDataFlag = false;
volatile bool timeoutFlag = false;

//Todo Add an interrupt to the home pin on GPIO3 to detect one rev complete if the encoder supports it

//Function definitions
void onTimer(void);
String& getTimeAsString(String& );
uint32_t inline ICACHE_RAM_ATTR myGetCycleCount(); //Processor instruction counter - very high res timing 
void callback(char* topic, byte* payload, unsigned int length) ; //MQTT callback handler

//Encoder setup 
bool encPresent = false;
int position;
int lastPosition;
int ppr;                    //List ppr * number of edges = number of pulses in a rev of the encoder wheel
float wheelDiameter;        //mm - may need to be a float. 
int ppRollover;             //Count at which we complete a revolution of the dome.
bool homeFlag = false;

#if defined _ESP8266_01
Encoder enc( 0, 2);
#elif defined _ESP8266_12
Encoder enc( 5, 6); //Connect to D1 and D2 respectively on the silkscreen
#endif 

#include "Skybadger_common_funcs.h"

//EEprom handler functions
#include "SnglEncEeprom.h"

//Web Handler function definitions
#include "ESP8266_SnglEncHandlers.h"

//Setup web page
#include "SnglEncSetupHandlers.h"

//#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
void setup_wifi()
{
  DEBUGSL1("Starting WiFi..");
  int zz=0;

  WiFi.mode(WIFI_STA);
  WiFi.hostname( myHostname );  
  WiFi.begin( ssid2, password2 );

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
      Serial.print(".");
    zz++;
    if ( zz >200 )
      device.restart();
  }
  Serial.println("WiFi connected");
  Serial.printf("SSID: %s, Signal strength %i dBm \n\r", WiFi.SSID().c_str(), WiFi.RSSI() );  
  Serial.printf("Hostname: %s\n\r",      WiFi.hostname().c_str() );
  Serial.printf("IP address: %s\n\r",    WiFi.localIP().toString().c_str() );
  Serial.printf("DNS address 0: %s\n\r", WiFi.dnsIP(0).toString().c_str() );
  Serial.printf("DNS address 1: %s\n\r", WiFi.dnsIP(1).toString().c_str() );

  //Setup sleep parameters
  //wifi_set_sleep_type(LIGHT_SLEEP_T);
  wifi_set_sleep_type(NONE_SLEEP_T);
  
  Serial.print("WiFi complete..");
  delay(5000);
}

void setup()
{
  Serial.begin( 115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println("ESP starting.");
  delay(2000);
  
  //Start NTP client
  configTime(TZ_SEC, DST_SEC, timeServer1, timeServer2, timeServer3 );
  DEBUGSL1("Time configured.");
  
  //Setup timestruct
  now = time(nullptr);

  //Setup defaults first - via EEprom. 
  EEPROM.begin(256);
  DEBUGSL1("EEProm begun.");
  
  setDefaults();  
  setupFromEeprom();
  
  // Connect to wifi 
  setup_wifi();                   
  
  //Pins mode and direction setup for encoder on ESP8266-01
  //Not actually necessary since Encoder class does this for us. 
  //Remember to use pullup resistors on the open-collector encoder types!
  //pinMode(0, INPUT_PULLUP );
  //pinMode(2, INPUT_PULLUP );
  //GPIO 3 is used to control the encoder power via a mosfet in order 
  //to prevent the low data signals on GPIO0 and GPIO2 stopping this device from booting!
  //GPIO 3 (normally RX on -01) 
  pinMode( 3, OUTPUT );
  digitalWrite( 3, 1 );
  delay(50);
  digitalWrite( 3, 0 ); //Turn on power to the encoder now we have booted, via the p-type mosfet by setting the output low.
  delay(50);
  
  //GPIO 1 (normally Tx on -01) swap the pin to a GPIO or PWM if you want to sacrifice serial output
  Serial.println("Pins setup & interrupts attached.");
    
  //Open a connection to MQTT
  client.setServer( mqtt_server, 1883 );
  client.connect( thisID, pubsubUserID, pubsubUserPwd ); 

  //Create a timer-based callback that causes this device to read the local i2C bus devices for data to publish.
  client.setCallback( callback );
  client.subscribe( inTopic, 1 );
  publishHealth();
  
  Serial.println("MQTT setup complete.");

  //Setup the sensors
  encPresent = true;
  
  //Setup webserver handler functions
  server.on("/", handleStatusGet);
  server.on( "/restart", handlerRestart );
  server.onNotFound(handleNotFound); 


  //handler code in separate file. 
  server.on("/encoder",                  HTTP_GET, handleEncoder );
  server.on("/encoder",                  HTTP_PUT, handleEncoder );
  server.on("/status",                   HTTP_GET, handleStatusGet );
  server.on("/encoder/bearing",          HTTP_GET, handleEncoderBearingGet );
  server.on("/encoder/bearing",          HTTP_PUT, handleEncoderBearingPut );
  
  //server.on("/encoder/distance",         HTTP_GET, handleEncoderAsDistance );
  
  //Device setup functions
  server.on("/setup",                  HTTP_GET, handleSetup );
  server.on("/setup/ppr",              HTTP_GET, handlePpr );
  server.on("/setup/ppr",              HTTP_PUT, handlePpr );
  server.on("/setup/wheelDiameter",    HTTP_GET, handleWheelDiameter );
  server.on("/setup/wheelDiameter",    HTTP_PUT, handleWheelDiameter );
  server.on("/setup/ppRollover",       HTTP_GET, handlePpRollover );   
  server.on("/setup/ppRollover",       HTTP_PUT, handlePpRollover );   
   
  //Remote wireless firmware upload support
  httpUpdater.setup(&server);
  server.begin();
    
  //Setup timers for measurement loop
  //setup interrupt-based 'soft' alarm handler for periodic acquisition/recording of new measurements.
  ets_timer_setfn( &timer, onTimer, NULL ); 
  ets_timer_setfn( &timeoutTimer, onTimeoutTimer, NULL ); 
  
  //Set the timer function first
  //fire timer every 1000 msec
  ets_timer_arm_new( &timer, 500, 1/*repeat*/, 1);
  //ets_timer_arm_new( &timeoutTimer, 2500, 0/*one-shot*/, 1);

  Serial.println( "Setup complete" );
}

//Timer handler for 'soft' interrupt timer. 
void onTimer( void * pArg )
{
  newDataFlag = true;
}

//Used to complete timeout actions. 
void onTimeoutTimer( void* pArg )
{
  //Read command list and apply. 
  timeoutFlag = true;
}

//Main processing loop
void loop()
{
  String timestamp;
  String output;
  float bearing = 0.0F;
  int mqttState = 0;
  
  DynamicJsonBuffer jsonBuffer(256);
  JsonObject& root = jsonBuffer.createObject();

  if( newDataFlag == true ) //every second
  {
    root["time"] = getTimeAsString( timestamp );
    //Serial.println( getTimeAsString( timestamp ) );

    position = enc.read();
    if ( position != lastPosition )
    {
      if( position < 0 ) 
      {  
        position = ppRollover + position;
        enc.write( position );
      }
      else if ( position > ppRollover ) 
      {
        position = position - ppRollover;
        enc.write( position);
      }
      
      lastPosition = position;
      bearing = ( position *360.0F / ppRollover) ;
      //Serial.printf( "New position: %i , bearing %03.1f \n\r", position, bearing );
      DEBUGSL1(".");
    }
  }  

  if ( client.connected() )
  {
    if (callbackFlag ) 
    {
      //publish results
      publishHealth();
      callbackFlag = false;
    }
    //Service MQTT keep-alives
    client.loop();
  }
  else
  {
    //reconnect();
    reconnectNB();
    client.subscribe(inTopic);
  }
  
  //Handle web requests
  server.handleClient();
}


/* MQTT callback for subscription and topic.
 * Only respond to valid states ""
 * Publish under ~/skybadger/sensors/<sensor type>/<host>
 * Note that messages have an maximum length limit of 18 bytes - set in the MQTT header file. 
 */
 void callback(char* topic, byte* payload, unsigned int length) 
 {  
  //set callback flag
  callbackFlag = true;  
 }

void publishHealth(void)
{
  String outTopic;
  String output;
  String timestamp;
  
  //checkTime();
  getTimeAsString2( timestamp );

  //Put a notice out regarding device health
  //publish to our device topic(s)
  DynamicJsonBuffer jsonBuffer(300);
  JsonObject& root = jsonBuffer.createObject();
  root["time"] = timestamp;
  root["hostname"] = myHostname;
  root["message"] = "Listening";
  root.printTo( output );
  outTopic = outHealthTopic;
  outTopic.concat( myHostname );  
  
  if ( client.publish( outTopic.c_str(), output.c_str(), true ) )
    Serial.printf( " Published health message: '%s' to %s\n",  output.c_str(), outTopic.c_str() );
  else
    Serial.printf( " Failed to publish health message: '%s' to %s\n",  output.c_str(), outTopic.c_str() );
  return;
}

/*
 * Had to do a lot of work to get this to work 
 * Mostly around - 
 * length of output buffer
 * reset of output buffer between writing json strings otherwise it concatenates. 
 * Writing to serial output was essential.
 */
 void publishStuff( void )
 {
  String outTopic;
  String output;
  String timestamp;
  
  //checkTime();
  getTimeAsString( timestamp );
  
  if (encPresent) 
  {
    //publish to our device topic(s)
    DynamicJsonBuffer jsonBuffer(300);
    JsonObject& root = jsonBuffer.createObject();

    output="";//reset
    root["sensor"]     = "Encoder";
    root["time"]       = timestamp;
    root["hostname"]   = myHostname;
    root["Position"]   = position;
    root["Bearing"]   = (position/ppRollover) * 360.0F;

    outTopic = outSenseTopic;
    outTopic.concat("Encoder/");
    outTopic.concat(myHostname);

    root.printTo( output );
    if ( client.publish( outTopic.c_str(), output.c_str(), true ) )        
      Serial.printf( "Published Encoder position sensor measurement %s to %s\n",  output.c_str(), outTopic.c_str() );
    else    
      Serial.printf( "Failed to publish Encoder position sensor measurement %s to %s\n",  output.c_str(), outTopic.c_str() );
  }
}

uint32_t inline ICACHE_RAM_ATTR myGetCycleCount()
{
    uint32_t ccount;
    __asm__ __volatile__("esync; rsr %0,ccount":"=a" (ccount));
    return ccount;
}
