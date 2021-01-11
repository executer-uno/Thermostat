/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com
*********/

// Import required libraries
#ifdef ESP32
  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
#else
  #include <Arduino.h>
  #include <ESP8266WiFi.h>
  #include <Hash.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
#endif
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

#include <DNSServer.h>

#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

#include <Ticker.h>  //Ticker Library

#include "HTML.h"
#include "EEPROM.h"

#include "EEPROM_param.h"

// Pinouts configuration
	// Data wire is connected to GPIO 4
	#define ONE_WIRE_BUS	4		// D2

	// Button is on D1
	#define BUTTON		 	5		// D1

	// RGB Led indicator (common anode)
	#define LED_G			13		// D7 // Freen
	#define LED_B			12		// D6 // Blue
	#define LED_R			14		// D5 // Red

	// Load switch control output
	#define LOAD			15		// D8

	#define LED_PERIOD		50		// in ms

// EEPROM layout
#define	EEPROM_PARAM	0
#define	EEPROM_SSID		300
#define	EEPROM_STAT		400
#define EEPROM_SIZE		512			// !! also defined in EEPROM_param.tpp


// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Number of temperature devices found
int numberOfDevices;
// We'll use this variable to store a found device address
DeviceAddress tempDeviceAddress;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
/* Soft AP network parameters */
IPAddress apIP(192, 168, 111, 111);
//IPAddress apIP(8, 8, 8, 8);
IPAddress netMsk(255, 255, 255, 0);


const byte DNS_PORT = 53;
DNSServer dnsServer;

// Main sensor temperature
float tempMain = -127.00;

// Temperature statistics
/*
float tempMain_MAX = -MAXFLOAT;
float tempMain_MIN =  MAXFLOAT;
float tempMain_MAX_prev = tempMain_MAX;
float tempMain_MIN_prev = tempMain_MIN;
*/

/* Don't set this parameters. They are configurated at runtime and stored on EEPROM */
/* _prev parameters are for detection of EEPROM update is necessary */

/*
float 	PAR_limit = 10;
float 	PAR_limit_prev = PAR_limit;
int		PAR_index = 0;
int		PAR_index_prev = PAR_index;
int		PAR_heater = 0;									// Load type. Heater = 1 (load on if temperature less than set point). Cooler = 2 (load on if temperature higher than set point). 0 = not defined, load will be switched off
int		PAR_heater_prev = PAR_heater;
*/


int		offsetPAR=EEPROM_PARAM;									// required for EEPROM_param constructors
EEPROM_param <float> PARlimit(10.0	,offsetPAR, 20*1000);		// Temperature set point
EEPROM_param <int> PARheater(0		,offsetPAR, 20*1000);		// Load type. Heater = +1 (load on if temperature less than set point). Cooler = -1 (load on if temperature higher than set point). 0 = (initial) not defined, load will be switched off

int		offsetSTAT=EEPROM_STAT;									// required for EEPROM_param constructors for statistics
EEPROM_param <float> T_max(-300.0	,offsetSTAT, 10*60*1000);	// Temperature maximum
EEPROM_param <float> T_min(+300.0	,offsetSTAT, 10*60*1000);	// Temperature minimum


Ticker Sensor_Call;
Ticker LED_Call;
Ticker Param_Call;
Ticker Button_Call;
Ticker Statistics_Call;

struct ButtonStruct {
	uint capFilter;				// input signal jitter filter
	uint capPress;				// pressed timer
	uint capRelease;			// released timer
	bool Pressed;				// Button pressed state
	bool ShortPress_RS;			// Short button press. Set once on release
	bool LongPress_RS;			// Long button press. Set once on release
};
ButtonStruct Button;

bool Load_ON = false;

bool Sensor_Call_Flag = false;	// Sensors can not be called inside ticker interrupt

uint16_t LED_Counter=0;

String APSSID = "";				// parameters are dynamic and stored in EEPROM
String APPASS = "";				// adjusted from web page

/** Store parameters to EEPROM */
void saveParameters() {

	PARlimit.Store();
	PARheater.Store();

}


String readDSTemperatureC() {
	/*								// - sensor call crushes runtime
	// Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
	sensors.requestTemperatures();

	float tempC = sensors.getTempCByIndex(0);
	*/
	float tempC = tempMain;

	if(tempC == -127.00) {
		//Serial.println("Failed to read from DS18B20 sensor");
	return "--";
	} else {
		//Serial.print("Temperature Celsius: ");
		//Serial.println(tempC);
	}
	return String(tempC);
}


// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATUREC"){
    return readDSTemperatureC();
  }
  else if(var == "LIMIT"){
    return String((int)PARlimit.Get());
  }
  else if(var == "T_MAX"){
    return String(T_max.Get());
  }
  else if(var == "T_MIN"){
    return String(T_min.Get());
  }
  else if(var == "APSSID"){
    return APSSID;
  }
  else if(var == "HEATER"){
	  if(PARheater.Get()==1){
	    return "checked";
	  }
	  else {
	    return "";
	  }
  }

  return String();
}

/** Is this an IP? */
boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

/** Handle the reboot request from web server */
void handleReboot(AsyncWebServerRequest *request) {
  Serial.println("ESP Reboot from web server");
  server.end(); // Stop is needed because we sent no content length
  ESP.restart();
}
/** Handle the root page from web server */
void handleRoot(AsyncWebServerRequest *request) {
  Serial.println("Root page from web server");
  request->send_P(200, "text/html", index_html, processor);
}
/** Handle the root page from web server */
void handleConfigPage(AsyncWebServerRequest *request) {
  Serial.println("Config page from web server");
  request->send_P(200, "text/html", settings_html, processor);
}

void handleNotFound(AsyncWebServerRequest *request) {
  if (captivePortal(request)) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += request->url();
  message += F("\nMethod: ");
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += request->args();
  message += F("\n");

  for (uint8_t i = 0; i < request->args(); i++) {
    message += String(F(" ")) + request->argName(i) + F(": ") + request->arg(i) + F("\n");
  }

  AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", message); //Sends 404 File Not Found
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");

  request->send(response);

}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal(AsyncWebServerRequest *request) {
  if (!isIp(request->host()) && request->host() != (APSSID + ".local")) {
    Serial.println("Request redirected to captive portal");

    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
    response->addHeader("Location", String("http://") + toStringIp(request->client()->localIP()));
    request->send(response);
    request->client()->stop();// Stop is needed because we sent no content length

    return true;
  }
  return false;
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleSaveSSID(AsyncWebServerRequest *request) {
  Serial.println("handleSaveSSID called");

  Serial.print("SSID: ");
  Serial.println(request->arg("APSSID"));
  Serial.print("PASS: ");
  Serial.println(request->arg("APPASS1"));
  Serial.print("PASS: ");
  Serial.println(request->arg("APPASS2"));

  bool 		ErrNo[4];

  ErrNo[3]	=	(request->arg("APSSID").length()	< 4 );
  ErrNo[2]	=	(request->arg("APPASS1").length()	< 8 );
  ErrNo[1]	=	request->arg("APPASS1").compareTo(request->arg("APPASS2")) != 0;

  if(!(ErrNo[1] || ErrNo[2] || ErrNo[3])){

	  APSSID = "SSID"+request->arg("APSSID");
	  APPASS = "PASS"+request->arg("APPASS1");

	  int offset = EEPROM_SSID;				// AP SSID and PASS EEPROM offset
	  EEPROM.begin(EEPROM_SIZE);
	  {
		  char	 TEMP[32+4+1]="";

		  // Save SSID
		  APSSID.toCharArray(TEMP, sizeof(TEMP));
		  EEPROM.put(offset,TEMP);	offset += sizeof(TEMP);
		  // Save PASSWORD
		  APPASS.toCharArray(TEMP, sizeof(TEMP));
		  EEPROM.put(offset,TEMP);	offset += sizeof(TEMP);

	  }
	  if (EEPROM.commit()) {
		  Serial.println("EEPROM successfully committed");

		  EEPROM.end();

		  delay(1000);
		  Serial.println("SSID updated in EEPROM. Reboots...");
		  ESP.restart();
	  }
	  else {
		  Serial.println("ERROR! EEPROM commit failed");
	  }

	  EEPROM.end();
  }
  else{

	  String ErrorResponse = "<!DOCTYPE HTML><html><body><h2>Ошибка</h2><h3>";

	  if(ErrNo[1])	ErrorResponse += "Поля паролей не совпадают<br>";
	  if(ErrNo[2])	ErrorResponse += "Пароль меньше 8 символов<br>";
	  if(ErrNo[3])	ErrorResponse += "Имя сети меньше 4 символов<br>";

	  ErrorResponse += "</h3></body></html>";

	  request->send(200, "text/html", ErrorResponse);

  }
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleSaveParams(AsyncWebServerRequest *request) {
  Serial.println("handleSaveParams called");

  PARlimit.Set((float)(request->arg("limit").toFloat()));

  AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");

  response->addHeader("Location", "config");
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");

  request->send(response);
  request->client()->stop();	// Stop is needed because we sent no content length
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleUpdate(AsyncWebServerRequest *request) {
  Serial.println("handleUpdate called");

  String inputMessage1;
  String inputMessage2;

  // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  if (request->hasParam("output") && request->hasParam("state")) {
    inputMessage1 = request->getParam("output")->value();
    inputMessage2 = request->getParam("state")->value();

    if(inputMessage1 == "heater"){
    	if(inputMessage2.toInt()==1){
    		PARheater.Set((int)+1);
    	}
    	else if(inputMessage2.toInt()==0){
    		PARheater.Set((int)-1);
    	}
    }
  }
  else {
    inputMessage1 = "No message sent";
    inputMessage2 = "No message sent";
  }
  Serial.print("Update parameter: ");
  Serial.print(inputMessage1);
  Serial.print(" - Set to: ");
  Serial.println(inputMessage2);
  request->send(200, "text/plain", "OK");

}


/** Handle the WLAN save form and redirect to WLAN config page again */
void handleRstStat(AsyncWebServerRequest *request) {
  Serial.println("handleRstStat called");

	// Statistics reset
  	T_max.Set(tempMain);
  	T_max.Store(true);

  	T_min.Set(tempMain);
  	T_min.Store(true);

	AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");

	response->addHeader("Location", "/");
	response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	response->addHeader("Pragma", "no-cache");
	response->addHeader("Expires", "-1");

	request->send(response);
	request->client()->stop();	// Stop is needed because we sent no content length
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
  }
}

void setup(){
  delay(1000);

  // Serial port for debugging purposes
  Serial.begin(76800);
  Serial.println("Serial output started.");


  // Setup pins *************************************
  Serial.println("Pins configuration.");
  pinMode(BUTTON,	INPUT_PULLUP);//INPUT_PULLUP);

  pinMode(LED_R,	OUTPUT_OPEN_DRAIN);//OUTPUT_OPEN_DRAIN);
  pinMode(LED_G,	OUTPUT_OPEN_DRAIN);//OUTPUT_OPEN_DRAIN);
  pinMode(LED_B,	OUTPUT_OPEN_DRAIN);//OUTPUT_OPEN_DRAIN);


  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);


  pinMode(LOAD,		OUTPUT);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Pins configured.");


  // Start up the DS18B20 library
  sensors.begin();
  // Grab a count of devices on the wire
  numberOfDevices = sensors.getDeviceCount();
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");
  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
  sensors.requestTemperatures(); // Send the command to get temperatures


  // access point name
  char	 TEMP[32+4+1]="";
  int offset = EEPROM_SSID;				// AP SSID and PASS EEPROM offset

  EEPROM.begin(EEPROM_SIZE);
  {
	  // Restore SSID
	  EEPROM.get(offset,	TEMP);	offset += sizeof(TEMP);
	  TEMP[32+4] = 0;				// To be sure null-terminated
	  APSSID = String(TEMP);

	  if(APSSID.startsWith("SSID")){
		APSSID = APSSID.substring(4);
	  }
	  else{
		APSSID = WiFi.macAddress();
		APSSID.replace(":", "");
		APSSID = "SOCKET" + APSSID;
	  }

	  // Restore PASSWORD
	  EEPROM.get(offset,	TEMP);	offset += sizeof(TEMP);
	  TEMP[32+4] = 0;				// To be sure null-terminated
	  APPASS = String(TEMP);

	  if(APPASS.startsWith("PASS")){
		APPASS = APPASS.substring(4);
	  }
	  else{
		APPASS = "";
	  }
  }
  EEPROM.end();

  Serial.print("AP SSID: ");
  Serial.println(APSSID);
  Serial.print("AP PASS: ");
  Serial.println(APPASS);

  // Rise own wifi point
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(APSSID.c_str(), APPASS.c_str());
  delay(500); // Without delay I've seen the IP address blank
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  /* Setup web pages*/
  // Route for root / web page
  server.on("/",			 handleRoot);
  server.on("/config", 		 handleConfigPage);
  server.on("/generate_204", handleRoot);  	//Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", 		 handleRoot);  		//Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/saveparams", 	 handleSaveParams);
  server.on("/saveSSID", 	 handleSaveSSID);
  server.on("/update", 		 handleUpdate);
  server.on("/rststat", 	 handleRstStat);	// Reset statistics

  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDSTemperatureC().c_str());
  });
  server.on("/temperaturemax", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(T_max.Get()).c_str());
  });
  server.on("/temperaturemin", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(T_min.Get()).c_str());
  });

  server.on("/reboot", handleReboot);
  server.onNotFound(handleNotFound);

  // Start server
  server.begin();
  Serial.println("HTTP server started");


  // OTA firmware update *************************************


  // Port defaults
  ArduinoOTA.setPort(8266);		// Use 8266 port if you are working in Sloeber IDE, it is fixed there and not adjustable
  //ArduinoOTA.setPort(3232); 	// for ArduinoIDE

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(APSSID.c_str());

  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready");

  // Attach cycles
  Param_Call.attach( 20.0, saveParameters);
  Sensor_Call.attach(10.3, CallSensorsHandler);
  LED_Call.attach_ms(LED_PERIOD, CallLED);
  Button_Call.attach_ms(2, CallButtons);
  Statistics_Call.attach(60 * 5.0, SaveStatistics);

  digitalWrite(LED_BUILTIN, HIGH);
}


/** Store statistics to EEPROM */
void SaveStatistics() {

	T_max.Store();
	T_min.Store();
}


void CallButtons(){												// Button control
	bool ButIn = !digitalRead(BUTTON);							// Input pulled up, button to ground
	if(Button.capFilter < 5 && ButIn) 	Button.capFilter++;		// 10 ms capacitor
	if(Button.capFilter > 0	&& !ButIn) 	Button.capFilter--;

	if(Button.capFilter >= 5) 			Button.Pressed = true;
	if(Button.capFilter <= 0) 			Button.Pressed = false;
}


void CallSensorsHandler(){
	Sensor_Call_Flag=true;
}


void CallLED(){
	if(Button.Pressed  && Button.capPress < UINT16_MAX ){
		Button.capPress += LED_PERIOD;
		Button.capRelease =0;
	}
	if(!Button.Pressed && Button.capRelease < UINT16_MAX ){
		if(Button.capPress){
			if(Button.capPress > 1000){
				Button.LongPress_RS 		= true;
				Serial.println("Button: Long press");
			}
			else if(Button.capPress){
				Button.ShortPress_RS 		= true;
			    Serial.println("Button: Short press");
			}
		}
		Button.capPress = 0;
		Button.capRelease += LED_PERIOD;
	}

	digitalWrite(LED_R, HIGH);	// Switch off all colors
	digitalWrite(LED_G, HIGH);
	digitalWrite(LED_B, HIGH);

	if(LED_Counter < 5000/LED_PERIOD){

		if(Load_ON){
			if(PARheater.Get()== +1) 	digitalWrite(LED_R, LOW);		// Heating
			if(PARheater.Get()== -1) 	digitalWrite(LED_B, LOW);		// Cooling
		}
		LED_Counter++;
	}
	else {

		digitalWrite(LED_G, LOW);	// Alive blink

		LED_Counter = 0;
	}
}


void CallSensors(){
	digitalWrite(LED_BUILTIN, LOW);
	sensors.requestTemperatures(); // Send the command to get temperatures

	// Loop through each device, print out temperature data
	Serial.print("Temperatures: ");
	for(int i=0;i<numberOfDevices; i++){
		// Search the wire for address
		if(sensors.getAddress(tempDeviceAddress, i)){
		  // Output the device ID
		  Serial.print("device: ");
		  Serial.print(i,DEC);
		  // Print the data
		  float tempC = sensors.getTempC(tempDeviceAddress);

		  Serial.print(" Temp : ");
		  Serial.print(tempC);
		  Serial.print("°C   ");

		  tempMain = tempC;
		}
	}
	Serial.println("");

	// Temperature control
	if(PARheater.Get()==	+1){												// Load - Heater
		if(tempMain < PARlimit.Get()) 		Load_ON	= true;
		if(tempMain > PARlimit.Get() + 1.0)	Load_ON	= false;
	}
	else if(PARheater.Get()==	-1){											// Load - Cooler
		if(tempMain > PARlimit.Get()) 		Load_ON	= true;
		if(tempMain < PARlimit.Get() - 1.0)	Load_ON	= false;
	}
	else{
		Load_ON	= false;
	}

	// Statistics
	if(tempMain > T_max.Get() && tempMain !=-127.0)		T_max.Set(tempMain);
	if(tempMain < T_min.Get() && tempMain !=-127.0)		T_min.Set(tempMain);

	digitalWrite(LED_BUILTIN, HIGH);
}


void loop(){

	// Sensors data request *****************************************************
	if(Sensor_Call_Flag){
		CallSensors();
		Sensor_Call_Flag = false;
	}

	// Load control		*********************************************************
	if(Load_ON){
		digitalWrite(LOAD, HIGH);
	}
	else{
		digitalWrite(LOAD, LOW);
	}

	// Aux tasks		*********************************************************
	dnsServer.processNextRequest();
	ArduinoOTA.handle();
	yield();
}
