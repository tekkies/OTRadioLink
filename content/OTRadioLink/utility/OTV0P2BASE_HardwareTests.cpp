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

Author(s) / Copyright (s): Damon Hart-Davis 2016
*/

/*
 Hardware tests for general POST (power-on self tests)
 and for detailed hardware diagnostics.

 Some are generic such as testing clock behaviour,
 others will be very specific to some board revisions
 (eg looking for shorts or testing expected attached hardware).

 Most should return true on success and false on failure.

 Some may require being passed a Print reference
 (which will often be an active hardware serial connection)
 to dump diagnostics to.
 */

#ifndef OTV0P2BASE_HARDWARETESTS_H
#define OTV0P2BASE_HARDWARETESTS_H

#ifdef ARDUINO_ARCH_AVR
#include "util/atomic.h"
#endif

#include "OTV0P2BASE_Entropy.h"
#include "OTV0P2BASE_Sleep.h"

#include "OTV0P2BASE_HardwareTests.h"
#include "OTV0P2BASE_Serial_IO.h"


namespace OTV0P2BASE
{
namespace HWTEST
{


#ifdef ARDUINO_ARCH_AVR
// Returns true if the 32768Hz low-frequency async crystal oscillator appears to be running.
// This means the the Timer 2 clock needs to be running
// and have an acceptable frequency compared to the CPU clock (1MHz).
// Uses nap, and needs the Timer 2 to have been set up in async clock mode.
// In passing gathers some entropy for the system.
bool check32768HzOsc()
    {
    // Check that the 32768Hz async clock is actually running at least somewhat.
    const uint8_t initialSCT = OTV0P2BASE::getSubCycleTime();

    // Allow time for 32768Hz crystal to start reliably, see: http://www.atmel.com/Images/doc1259.pdf
#if 0 && defined(DEBUG)
    DEBUG_SERIAL_PRINTLN_FLASHSTRING("Sleeping to let 32768Hz clock start...");
#endif
    // Time spent here should not be a whole multiple of basic cycle time
    // to avoid a spuriously-stationary async clock reading!
    // Allow several seconds (~3s+) to start.
    // Attempt to capture some entropy while waiting,
    // implicitly from oscillator start-up time if nothing else.
    for(uint8_t i = 255; --i > 0; )
        {
        const uint8_t sct = OTV0P2BASE::getSubCycleTime();
        OTV0P2BASE::addEntropyToPool(sct, 0);
        // If counter has incremented/changed (twice) then assume probably OK.
        if((sct != initialSCT) && (sct != (uint8_t)(initialSCT+1))) { return(true); }
        // Ensure lower bound of ~3s until loop finishes.
        OTV0P2BASE::nap(WDTO_15MS);
        }

#if 0 && defined(DEBUG)
    DEBUG_SERIAL_PRINTLN_FLASHSTRING("32768Hz clock may not be running!");
#endif
    return(false); // FAIL // panic(F("Xtal")); // Async clock not running.
    }
#endif
#endif

#ifdef ARDUINO_ARCH_AVR
// Returns true if the 32768Hz low-frequency async crystal oscillator appears to be running and sane.
// Performs an extended test that the CPU (RC) and crystal frequencies are in a sensible ratio.
// This means the the Timer 2 clock needs to be running
// and have an acceptable frequency compared to the CPU clock (1MHz).
// Uses nap, and needs the Timer 2 to have been set up in async clock mode.
// In passing gathers some entropy for the system.
bool check32768HzOscExtended()
    {
    // Check that the slow clock appears to be running.
    if(!check32768HzOsc()) { return(false); }

    // Test low frequency oscillator vs main CPU clock oscillator (at 1MHz).
    // Tests clock frequency between 15 ms naps between for up to 30 cycles and fails if not within bounds.
    // As of 2016-02-16, all working REV7s give count >= 120 and that fail to program via bootloader give count <= 119
    // REV10 gives 119-120 (only one tested though).
    static const uint8_t optimalLFClock = 122; // May be optimal...
    static const uint8_t errorLFClock = 4; // Max drift from allowed value.
    uint8_t count = 0;
    for(uint8_t i = 0; ; i++)
        {
        ::OTV0P2BASE::nap(WDTO_15MS);
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            {
            // Wait for edge on xtal counter edge.
            // Start counting cycles.
            // On next edge, stop.
            const uint8_t t0 = TCNT2;
            while(t0 == TCNT2) {}
            const uint8_t t01 = TCNT0;
            const uint8_t t1 = TCNT2;
            while(t1 == TCNT2) {}
            const uint8_t t02 = TCNT0;
            count = t02-t01;
            }
      // Check end conditions.
      if((count < optimalLFClock+errorLFClock) & (count > optimalLFClock-errorLFClock)) { break; }
      if(i > 30) { return(false); } // FAIL { panic(F("xtal")); }
      // Capture some entropy from the (chaotic?) clock wobble, but don't claim any.  (TODO-800)
      OTV0P2BASE::addEntropyToPool(count, 0);

#if 0 && defined(DEBUG)
      // Optionally print value to debug.
      DEBUG_SERIAL_PRINT_FLASHSTRING("Xtal freq check: ");
      DEBUG_SERIAL_PRINT(count);
      DEBUG_SERIAL_PRINTLN();
#endif
      }

    return(true); // Success!
    }
#endif // ARDUINO_ARCH_AVR

#ifdef ARDUINO_ARCH_AVR

/**
 * @brief	Calibrate the internal RC oscillator against and external 32786 Hz crystal oscillator or resonator. The target frequency is 1 MHz.
 * @param   todo do we want settable stuff, e.g. ext osc rate, internal osc rate, etc?
 * @retval  True on calibration success. False if Xtal not running or calibration fails.
 * @note    OSCCAL register is cleared on reset so changes are not persistent.
 * @note     cf4:   f8 94           cli                                                         CLEAR INTERRUPTS!!!!!!!!
             cf6:   30 91 b2 00     lds r19, 0x00B2 ; 0x8000b2 <__data_load_end+0x7ff25e>
             cfa:   80 91 b2 00     lds r24, 0x00B2 ; 0x8000b2 <__data_load_end+0x7ff25e>
             cfe:   8f 5f           subi    r24, 0xFF   ; 255
             d00:   90 91 b2 00     lds r25, 0x00B2 ; 0x8000b2 <__data_load_end+0x7ff25e>
             d04:   39 17           cp  r19, r25
             d06:   e1 f3           breq    .-8         ; 0xd00 <main+0x15c>                    WAIT FOR TCNT2?
             d08:   e1 2c           mov r14, r1                                                 MEASUREMENT LOOP HERE
             d0a:   e3 94           inc r14                                              1
             d0c:   91 2f           mov r25, r17                                         1
             d0e:   9a 95           dec r25                                              *
             d10:   01 f0           breq    .+0         ; 0xd12 <main+0x16e>             *
             d12:   e9 f7           brne    .-6         ; 0xd0e <main+0x16a>             4     4 instructions * 8 = 32
             d14:   90 91 b2 00     lds r25, 0x00B2 ; 0x8000b2 <__data_load_end+0x7ff25e>2?
             d18:   98 17           cp  r25, r24                                         1
             d1a:   b9 f3           breq    .-18        ; 0xd0a <main+0x166>             2
             I make this 7 + 4 * <no delay cycles>
 * @note    Final OSCCAL register values for all the REV7s I have that I could get working. (DE20161209)
 *          hh
            81
            91
            9E
            A1
            9E
            8D
            96
            92
 */
bool calibrateInternalOscWithExtOsc()
{
    // todo these should probably go somewhere else but not sure where.
	const constexpr uint8_t maxTries = 128;  // Maximum number of values to attempt.
	const constexpr uint8_t initOscCal = 0;  // Initial oscillator calibration value to start from.
	// TCNT2 overflows every 2 seconds. One tick is 2000/256 = 7.815 ms, or 7815 clock cycles at 1 MHz.
	// Minimum number of cycles we want per count is (7815*1.1)/255 = 34, to give some play in case the clock is too fast.
	const constexpr uint16_t cyclesPerTick = 7815;
	const constexpr uint8_t innerLoopTime = 39;  // the number of cycles the inner loop takes to execute.
	const constexpr uint8_t targetCount = cyclesPerTick/innerLoopTime;  // The number of counts we are aiming for.

    // Check that the slow clock appears to be running.
    if(!check32768HzOsc()) { return(false); }

    // Set initial calibration value and wait to settle.
//    OSCCAL = initOscCal;
    _delay_x4cycles(2); // > 8 us. max oscillator settling time is 5 us.

    // Calibration routine
    for(uint8_t i = 0; i < maxTries; i++)
	{
    	uint8_t count = 0;
#if 0
        OTV0P2BASE::serialPrintAndFlush("OSCCAL: "); // 10000001 on my test version
        OTV0P2BASE::serialPrintAndFlush(OSCCAL, HEX);
#endif // 1
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			// Wait for edge on xtal counter edge.
			const uint8_t t0 = TCNT2;
			const uint8_t t1 = TCNT2 + 1;
			while(t0 == TCNT2) {}
			// Start counting cycles.
			do {
				count++; // 2 cycles?
				// 8*4 = 32 cycles per count.
				_delay_x4cycles(8);
            // Repeat loop until TCNT2 increments.
			} while (TCNT2 == t1); // 2 cycles?
		}
#if 0
        OTV0P2BASE::serialPrintAndFlush("\t count: ");
        OTV0P2BASE::serialPrintAndFlush(count);
        OTV0P2BASE::serialPrintAndFlush("\t mismatch: ");
        OTV0P2BASE::serialPrintAndFlush(count - targetCount);
        OTV0P2BASE::serialPrintAndFlush(F("\tUUUUU"));
        OTV0P2BASE::serialPrintlnAndFlush();
//        delay(1000);
#endif // 1
        // Set new calibration value.
        if ((OSCCAL == 0x80) || (OSCCAL == 0xFF)) return false;  // Return false if OSCCAL is at limits.
        if(count > targetCount) OSCCAL--;
        else if(count < targetCount) OSCCAL++;
        else {
            return true;
#if 0
            while (true) {
                OTV0P2BASE::serialPrintAndFlush("\t count: ");
                OTV0P2BASE::serialPrintAndFlush(count);
                OTV0P2BASE::serialPrintAndFlush("\t OSCCAL: ");
                OTV0P2BASE::serialPrintAndFlush(OSCCAL, HEX);
                OTV0P2BASE::serialPrintAndFlush(F("\tUUUUU"));
                OTV0P2BASE::serialPrintlnAndFlush();
                delay(1000);
            }
#endif // 0
        }
        // Wait for oscillator to settle.
        _delay_x4cycles(2);
	}
    return false;
}
#endif // ARDUINO_ARCH_AVR

}
}
