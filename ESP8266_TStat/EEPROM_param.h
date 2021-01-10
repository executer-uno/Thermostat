/*
 * EEPROM_param.h
 *
 *  Created on: Jan 10, 2021
 *      Author: E_CAD
 */

#ifndef EEPROM_PARAM_H_
#define EEPROM_PARAM_H_



	// EEPROM stored parameter
	template <typename T >
	class EEPROM_param {
	private:
		int 			offset		= 0;		// Parameter's adress in EEPROM
		unsigned long 	timestamp	= 0;		// last parameter sync with EEPROM
		unsigned long 	cycle_ms	= 0;		// EEPROM update minimum cycle
		bool			changed		= false;	// Parameter updated and not saved
		T				value;
	public:

		T		Get();				// Get value
		void	Set(T setVal);		// Set value
		void	Store(bool force);	// Store parameter in EEPROM

		EEPROM_param(T default_value, int &offset, unsigned long cycle_ms = 5*60*1000);	// constructor
	};


#endif /* EEPROM_PARAM_H_ */
