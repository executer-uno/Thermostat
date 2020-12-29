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

#include "Secrets.h"

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

// Main sensor temperature
float tempMain = -127.00;

String readDSTemperatureC() {
	/*								// - sensor call crushes runtime
	// Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
	sensors.requestTemperatures();

	float tempC = sensors.getTempCByIndex(0);
	*/
	float tempC = tempMain;

	if(tempC == -127.00) {
	Serial.println("Failed to read from DS18B20 sensor");
	return "--";
	} else {
	Serial.print("Temperature Celsius: ");
	Serial.println(tempC);
	}
	return String(tempC);
}


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP DS18B20 Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Celsius</span> 
    <span id="temperaturec">%TEMPERATUREC%</span>
    <sup class="units">&deg;C</sup>
  </p>
</body>

<script>

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturec").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 10000) ;

</script>

</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATUREC"){
    return readDSTemperatureC();
  }
  return String();
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
  }
}

void setup(){

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


  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

/*
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "Hello, world");
  });
*/

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });


  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDSTemperatureC().c_str());
  });

  server.onNotFound(notFound);

  // Start server
  server.begin();
}

void loop(){

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
		  Serial.print("°C   ");

		  tempMain = tempC;
		}
	}
	Serial.println("");

	digitalWrite(LED_BUILTIN, HIGH);


	delay(5000);
	yield();

}
