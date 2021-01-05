/*
 * HTML.h
 *
 *  Created on: Jan 4, 2021
 *      Author: E_CAD
 */

#ifndef HTML_H_
#define HTML_H_



	const char index_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE HTML><html>
	<head>
		<title>Термостат</title>
		<meta name="viewport" content="width=device-width, initial-scale=1">
		<link rel="icon" href="data:,">
	  <style>
		html {
		 font-family: Arial;
		 display: inline-block;
		 margin: 0px auto;
		 text-align: center;
		}
		h2 { font-size: 3.0rem; }
		p { font-size: 3.0rem; }
		.units { font-size: 2.0rem; }
		.ds-labels{
		  font-size: 1.5rem;
		  vertical-align:middle;
		  padding-bottom: 15px;
		}
		.icon {
			width: 3.0rem;
			height: 3.0rem;
			fill: #059e8a;
		}
		button[type=submit] {
			width: 15.0rem;  height: 3.0rem;
			font-size: 2.0rem;
		}
	  </style>
	  <svg style="display: none;">
	  <symbol id="thermometer-half">
		<path d="M192 384c0 35.346-28.654 64-64 64s-64-28.654-64-64c0-23.685 12.876-44.349 32-55.417V224c0-17.673 14.327-32 32-32s32 14.327 32 32v104.583c19.124 11.068 32 31.732 32 55.417zm32-84.653c19.912 22.563 32 52.194 32 84.653 0 70.696-57.303 128-128 128-.299 0-.609-.001-.909-.003C56.789 511.509-.357 453.636.002 383.333.166 351.135 12.225 321.755 32 299.347V96c0-53.019 42.981-96 96-96s96 42.981 96 96v203.347zM208 384c0-34.339-19.37-52.19-32-66.502V96c0-26.467-21.533-48-48-48S80 69.533 80 96v221.498c-12.732 14.428-31.825 32.1-31.999 66.08-.224 43.876 35.563 80.116 79.423 80.42L128 464c44.112 0 80-35.888 80-80z"/>
	  </symbol>
	  </svg>
	
	</head>
	<body>
	  <h2>Термостат</h2>
	  <h2>(%LIMIT% &deg;C)</h2>
	  <p>
		<span class="ds-icon">
			<svg class="icon" viewBox="0 0 300 550" ><use class="icon" xlink:href="#thermometer-half" x="0" y="0" /></svg>
		</span> 
		<span class="ds-labels"></span> 
		<span id="temperaturec">%TEMPERATUREC%</span>
		<sup class="units">&deg;C</sup>
	  </p>
	  <p>
		<form action="/config" method="GET">
			<button type="submit" >Настройки</button>
		</form>
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
	
	</html>
)rawliteral";




	const char settings_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE HTML><html>
	<head>
		<title>Термостат</title>
		<meta name="viewport" content="width=device-width, initial-scale=1">
		<link rel="icon" href="data:,">
		<style>
			html {
				font-family: Arial;
				display: inline-block;
				margin: 0px auto;
				text-align: center;
			}
			h2 { font-size: 3.0rem; }
			p { font-size: 2.0rem; }
			.units { font-size: 2.0rem; }
			.ds-labels{
				font-size: 1.5rem;
				vertical-align:middle;
				padding-bottom: 15px;
			}
			button {
				font-size: 2.0rem;
			}
			button[type=submit] {
				width: 15.0rem;  height: 3.0rem;
				font-size: 2.0rem;
			}
	
	
			input[type="number"] {
			  -webkit-appearance: textfield;
			  -moz-appearance: textfield;
			  appearance: textfield;
			}
	
			input[type=number]::-webkit-inner-spin-button,
			input[type=number]::-webkit-outer-spin-button {
			  -webkit-appearance: none;
			}
	
			.number-input {
			  border: 2px solid #ddd;
			  display: inline-flex;
			}
	
			.number-input,
			.number-input * {
			  box-sizing: border-box;
			}
	
			.number-input button {
			  outline:none;
			  -webkit-appearance: none;
			  background-color: transparent;
			  border: none;
			  align-items: center;
			  justify-content: center;
			  width: 3rem;
			  height: 3rem;
			  cursor: pointer;
			  margin: 0;
			  position: relative;
			}
	
			.number-input input[type=number] {
			  font-family: sans-serif;
			  max-width: 5rem;
			  padding: .5rem;
			  border: solid #ddd;
			  border-width: 0 2px;
			  font-size: 2rem;
			  height: 3rem;
			  font-weight: bold;
			  text-align: center;
			}

			body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
			.switch {position: relative; display: inline-block; width: 68px; height: 120px} 
			.switch input {display: none}
			.slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #0000b3; border-radius: 6px}
			.slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
			input:checked+.slider {background-color: #b30000}
			input:checked+.slider:before {-webkit-transform: translateY(-52px); -ms-transform: translateY(-52px); transform: translateY(-52px)}
	
		</style>
	</head>
	<body>
		<form method='POST' action='saveparams'><h2>Настройки</h2>
			<div class="number-input">
				<button onclick="this.parentNode.querySelector('input[name=limit]').stepDown()" >-</button>
				<input class="quantity" type="number" id="limit" name="limit" min="-55" max="125" value="%LIMIT%">
				<button onclick="this.parentNode.querySelector('input[name=limit]').stepUp()">+</button>
			</div>
			<sup class="units">&deg;C</sup>
		</form>
		<p>
<!--
			<span>Нагрев<br></span>
			<label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="heater" %HEATER%><span class="slider"></span></label>
			<span><br>Охлаждение</span>
			<script>
				function toggleCheckbox(element) {
					var xhr = new XMLHttpRequest();
					if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
					else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
					xhr.send();
				}
			</script>
-->
		</p>
		<form action="/" method="GET">
			<button type="submit">Вернуться</button>
		</form>
	</body>
	</html>
)rawliteral";




#endif /* HTML_H_ */
