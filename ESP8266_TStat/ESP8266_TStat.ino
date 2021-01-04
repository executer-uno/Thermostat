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

#include "Secrets.h"
#include "HTML.h"

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

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = own_ssid;


// Main sensor temperature
float tempMain = -127.00;

/* Don't set this parameters. They are configurated at runtime and stored on EEPROM */
/* _prev parameters are for detection of EEPROM update is necessary */
float 	PAR_limit = 10;
float 	PAR_limit_prev = PAR_limit;
int		PAR_index = 0;
int		PAR_index_prev = PAR_index;

// last update
unsigned long lastPARUpdate = millis();

// Loop tasks calls rescaler
int rescale = 0;

/** Load parameters from EEPROM */
bool loadParameters() {

  float 	temp_PAR_limit = 0;
  int		temp_PAR_index = 0;

  EEPROM.begin(512);
  EEPROM.get(0, temp_PAR_limit);
  EEPROM.get(0 + sizeof(temp_PAR_limit), temp_PAR_index);
  char ok[2 + 1];
  EEPROM.get(0 + sizeof(temp_PAR_limit) + sizeof(temp_PAR_index), ok);
  EEPROM.end();
  if ((String(ok) != String("OK"))||(PAR_index < 0)) {

	  Serial.println("EEPROM parameters reading error");
	  return false;
  }

  PAR_limit = temp_PAR_limit;
  PAR_index = temp_PAR_index;

  PAR_limit_prev = PAR_limit;		// Save values to compare later for EEPROM update
  PAR_index_prev = PAR_index;

  Serial.println("Parameters from EEPROM:");
  Serial.println(PAR_limit);
  Serial.println(PAR_index);
  return true;
}

/** Store parameters to EEPROM */
void saveParameters() {
  EEPROM.begin(512);
  EEPROM.put(0, PAR_limit);
  EEPROM.put(0 + sizeof(PAR_limit), PAR_index);
  char ok[2 + 1] = "OK";
  EEPROM.put(0 + sizeof(PAR_limit) + sizeof(PAR_index), ok);
  EEPROM.commit();
  EEPROM.end();

  PAR_limit_prev = PAR_limit;		// Save values to compare later for EEPROM update
  PAR_index_prev = PAR_index;
  Serial.println("parameters saved to EEPROM");

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
    return String((int)PAR_limit);
  }
  else if(var == "INDEX"){
    return String(PAR_index);
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
  if (!isIp(request->host()) && request->host() != (String(myHostname) + ".local")) {
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
void handleSaveParams(AsyncWebServerRequest *request) {
  Serial.println("parameters updated");

  PAR_limit = request->arg("limit").toFloat();

  AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");

  response->addHeader("Location", "config");
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");

  request->send(response);
  request->client()->stop();	// Stop is needed because we sent no content length

  lastPARUpdate	 = millis();	// Parameters was updated from web server
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

  // Rise own wifi point
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(own_ssid, own_password);
  delay(500); // Without delay I've seen the IP address blank
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  /* Setup web pages*/
  // Route for root / web page
  server.on("/", handleRoot);
  server.on("/config", handleConfigPage);
  server.on("/generate_204", handleRoot);  	//Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  		//Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/saveparams", handleSaveParams);

  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDSTemperatureC().c_str());
  });


  server.on("/reboot", handleReboot);
  server.onNotFound(handleNotFound);

  // Start server
  server.begin();
  Serial.println("HTTP server started");


  loadParameters(); // Load parameters from EEPROM


  // OTA firmware update *************************************


  // Port defaults
  ArduinoOTA.setPort(8266);		// Use 8266 port if you are working in Sloeber IDE, it is fixed there and not adjustable
  //ArduinoOTA.setPort(3232); 	// for ArduinoIDE

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(own_ssid);

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

}

void loop(){

	if ( rescale > 100 ){

		rescale = 0;

		digitalWrite(LED_G, LOW);										// LED ON
		sensors.requestTemperatures(); // Send the command to get temperatures
		digitalWrite(LED_G, HIGH);										// LED OFF

		digitalWrite(LED_BUILTIN, LOW);

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
			  Serial.print("�C   ");

			  tempMain = tempC;
			}
		}
		Serial.println("");

		// Temperature control
		if(tempMain < PAR_limit){
			digitalWrite(LOAD, HIGH);
		}
		if(tempMain > PAR_limit + 1.0){
			digitalWrite(LOAD, LOW);
		}

		digitalWrite(LED_BUILTIN, HIGH);

		// Check if parameters to be saved in EEPROM
		if(((PAR_limit_prev != PAR_limit) || (PAR_index_prev != PAR_index)) && (millis() - lastPARUpdate > 20* 1000)){
			saveParameters();
		}
	}

	rescale++;
	delay(100);
	dnsServer.processNextRequest();
	ArduinoOTA.handle();
	yield();

}
