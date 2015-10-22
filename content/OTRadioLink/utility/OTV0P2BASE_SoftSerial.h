/*
The OpenTRV project licenses this file to you
under the Apache Licence, Version 2.0 (the "Licence");
you may not use this file except in compliance
with the Licence. You may obtain a copy of the Licence at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the Licence is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied. See the Licence for the
specific language governing permissions and limitations
under the Licence.

Author(s) / Copyright (s): Deniz Erbilgin 2015
*/

#ifndef OTSOFTSERIAL_OTV0P2BASE_SOFTSERIAL_H_
#define OTSOFTSERIAL_OTV0P2BASE_SOFTSERIAL_H_

#include "Arduino.h"
#include <stdint.h>

namespace OTV0P2BASE
{

/**
 * @class	OTSoftSerial
 * @brief	Blocking software serial that runs using no interrupts
 * 			Defaults to 2400 baud as this is what it runs at most reliably
 * @todo	replace digitalWrite with fastDigitalWrite
 */
class OTSoftSerial
{
public:
	// public interface:
	OTSoftSerial(uint8_t _rxPin, uint8_t _txPin);

	void begin(uint16_t baud = 2400);
	void end();

	uint8_t read();
	uint8_t read(uint8_t *buf, uint8_t len);
	void print(uint8_t c);
	void write(const char *buf, uint8_t len);
	uint8_t print(const char *buf);

private:
	const uint8_t rxPin;
	const uint8_t txPin;
	const uint16_t timeOut = 1000;	// length of timeout in millis
	// Used to tune delay cycle times
		// Compensates for time setting up registries in delay func
	const uint8_t tuningVal = 21;	// was 24 for arduino

	uint16_t baud;
	uint8_t halfDelay;
	uint8_t fullDelay;
};

} // OTV0P2BASE
#endif /* OTSOFTSERIAL_OTV0P2BASE_SOFTSERIAL_H_ */