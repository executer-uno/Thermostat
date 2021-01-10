/*
 * EEPROM_param.cpp
 *
 *  Created on: Jan 10, 2021
 *      Author: E_CAD
 */

#define VER	1100		// change if EEPROM storage layout changed

#include <Arduino.h>
#include "EEPROM_param.h"
#include "EEPROM.h"


// thanks to http://cppstudio.com/en/post/5188/ for templates tutorial
template <typename T>
EEPROM_param<T>::EEPROM_param(T default_value, int &offset, unsigned long cycle_ms){
	int	check;
	T	TValue;

	this->offset	= offset;
	this->changed	= false;
	this->value		= default_value;
	this->cycle_ms	= cycle_ms;

	EEPROM.begin(512);
	EEPROM.get(offset,	check);
	EEPROM.get(offset + sizeof(check),	TValue);
	EEPROM.end();

	if(check == VER){
		this->value = TValue;
		this->timestamp = millis();
	}
	else
	{
		Serial.print("EEPROM reading error. Offset=");
		Serial.print(offset);
		Serial.print(" check=");
		Serial.println(check);
	}

	offset+=sizeof(int)+sizeof(this->value);
}


template <typename T>
T EEPROM_param<T>::Get(){
	return this->value;
}

template <typename T>
void EEPROM_param<T>::Set(T setVal){
	if(setVal != this->value){
		this->changed 	= true;
		this->timestamp = millis();
		this->value		= setVal;
	}
}

template <typename T>
void EEPROM_param<T>::Store(bool force){
	bool storeEn = force;

	if((millis() - this->timestamp > this->cycle_ms) && this->changed){
		storeEn = true;
	}

	if(storeEn){
		int check = VER;

		EEPROM.begin(512);
		EEPROM.put(this->offset,	check);
		EEPROM.put(this->offset + sizeof(check),	this->value);
		EEPROM.commit();
		EEPROM.end();

		Serial.print("EEPROM updated. Offset=");
		Serial.println(offset);
	}
}


