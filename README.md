# Thermostat
ESP8266 thermostat with web interface

# Schematics are here
https://oshwlab.com/executer/thyristor-ac-thermostat

Based on https://habr.com/ru/company/unwds/blog/390601/

# Main features:
* Stand alone, no external WiFi/Internet required
* Web server based configuration. No hardware indicators/buttons required
* High power load commutation in sealed enclosure, low heat generation (triac+relay in parallel solution)
* Pretty web pages with css and svg design
* Single side circuitboard, see project at easyeda/oshwlab portal

# Installation:

## Project's references:

Check out https://randomnerdtutorials.com/esp8266-nodemcu-async-web-server-espasyncwebserver-library/ "Installing Libraries – ESP Async Web Server" to install Async Web Server library. In Sloeber IDE you need to go to Arduino\"Add a source folder to the selected project" and refer to local folders with ESPAsyncWebServer and ESPAsyncTCP libraryes. If all done right, you should have it in project references:

![External references](Images/IDE_Externals.png?raw=true "External references")

## Libraryes:
Libraryes used for project:

![Libs](Images/IDE_Libs.png?raw=true "Libs")

## Board settings:
ESP8266 v2.7.4 platfoem used and works fine with following settings:

![Board and platform](Images/IDE_BoardSettings.png?raw=true "Board and platform")

## Last step:
Dont forget to open SecretsExample.h, set your own credentials and save it as Secrets.h.

Thats all. Compilation should runs sucesfully.

*Tips:
To get project in Sloeber IDE download ZIP and extract it manually or follow https://github.com/executer-uno/ESP-CAM_water_counter#how-to-get-that-repo-into-sloeber-ide to keep connection with repository.*

*To get project in Arduino IDE download zip or create new folder, and make "git clone https://github.com/executer-uno/Thermostat.git" from Git Bash.*

# Hardware
Some details are available on https://oshwlab.com/executer/thyristor-ac-thermostat portal. Some pictures for overview:

![Web main](Images/Main%20web%20page.jpg?raw=true "Web main")

![Web config](Images/Config%20web%20page.jpg?raw=true "Web config")

![Exterior 1](Images/External%201.jpg?raw=true "Exterior 1")

![Exterior 2](Images/External%202.jpg?raw=true "Exterior 2")

![Before enclosure close](Images/Before%20enclosure%20close.jpg?raw=true "Before enclosure close")

![Load test](Images/Load%20test%201.5kW.jpg?raw=true "Load test")
