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

Author(s) / Copyright (s): Damon Hart-Davis 2013--2015
*/

/*
 * Driver for FTH8V wireless valve actuator (and FS20 protocol encode/decode).
 */

#ifndef ARDUINO_LIB_OTRADVALVE_FHT8VRADVALVE_H
#define ARDUINO_LIB_OTRADVALVE_FHT8VRADVALVE_H


#include <stddef.h>
#include <stdint.h>
#include <OTV0p2Base.h>
#include "OTRadValve_AbstractRadValve.h"


// Use namespaces to help avoid collisions.
namespace OTRadValve
    {


//// FHT8V radio-controlled radiator valve, using FS20 protocol.
//// preambleBytes specifies the space to leave for preabmble bytes for remote receiver sync.
//class FHT8VRadValveBase : public OTRadValve::AbstractRadValve
//  {
//  protected:
//    // Radio link usually expected to be RFM23B; non-NULL when available.
//    OTRadioLink::OTRadioLink *radio;
//
//    // TX buffer (non-null) and size (non-zero).
//    uint8_t * const buf;
//    const uint8_t bufSize;
//    // Construct an instance, providing TX buffer details.
//    FHT8VRadValveBase(uint8_t *_buf, uint8_t _bufSize)
//      : radio(NULL),
//        buf(_buf), bufSize(_bufSize),
//        halfSecondCount(0)
//      {
//      clearHC(); // Cleared housecodes will prevent any attempt to sync with FTH8V.
//      FHT8VSyncAndTXReset(); // Clear state ready to attempt sync with FHT8V.
//      }
//
//    // Sync status and down counter for FHT8V, initially zero; value not important once in sync.
//    // If syncedWithFHT8V = 0 then resyncing, AND
//    //     if syncStateFHT8V is zero then cycle is starting
//    //     if syncStateFHT8V in range [241,3] (inclusive) then sending sync command 12 messages.
//    uint8_t syncStateFHT8V;
//
//    // Count-down in half-second units until next transmission to FHT8V valve.
//    uint8_t halfSecondsToNextFHT8VTX;
//
//    // Half second count within current minor cycle for FHT8VPollSyncAndTX_XXX().
//    uint8_t halfSecondCount;
//
//    // True once/while this node is synced with and controlling the target FHT8V valve; initially false.
//    bool syncedWithFHT8V;
//
//    // True if FHT8V valve is believed to be open under instruction from this system; false if not in sync.
//    bool FHT8V_isValveOpen;
//
//    // Send current (assumed valve-setting) command and adjust FHT8V_isValveOpen as appropriate.
//    // Only appropriate when the command is going to be heard by the FHT8V valve itself, not just the hub.
//    void valveSettingTX(bool allowDoubleTX);
//
//    // Run the algorithm to get in sync with the receiver.
//    // Uses halfSecondCount.
//    // Iff this returns true then a(nother) call FHT8VPollSyncAndTX_Next() at or before each 0.5s from the cycle start should be made.
//    bool doSync(const bool allowDoubleTX);
//
//    #if defined(V0P2BASE_TWO_S_TICK_RTC_SUPPORT)
//    static const uint8_t MAX_HSC = 3; // Max allowed value of halfSecondCount.
//    #else
//    static const uint8_t MAX_HSC = 1; // Max allowed value of halfSecondCount.
//    #endif
//
//    // Sends to FHT8V in FIFO mode command bitstream from buffer starting at bptr up until terminating 0xff,
//    // then reverts to low-power standby mode if not in hub mode, RX for OpenTRV FHT8V if in hub mode.
//    // The trailing 0xff is not sent.
//    //
//    // Returns immediately without transmitting if the command buffer starts with 0xff (ie is empty).
//    // (If doubleTX is true, sends the bitstream twice, with a short (~8ms) pause between transmissions, to help ensure reliable delivery.)
//    //
//    // Note: single transmission time is up to about 80ms (without extra trailers), double up to about 170ms.
//    void FHT8VTXFHTQueueAndSendCmd(uint8_t *bptr, const bool doubleTX);
//
//    // Call just after TX of valve-setting command which is assumed to reflect current TRVPercentOpen state.
//    // This helps avoiding calling for heat from a central boiler until the valve is really open,
//    // eg to avoid excess load on (or energy wasting by) the circulation pump.
//    // FIXME: compare against own threshold and have NominalRadValve look at least open of all vs minPercentOpen.
//    void setFHT8V_isValveOpen();
//
//    // House codes part 1 and 2 (must each be <= 99 to be valid).
//    // Starts as '0xff' as unset EEPROM values would be to indicate 'unset'.
//    uint8_t hc1, hc2;
//
//  public:
//    static inline bool isValidFHTV8HouseCode(const uint8_t hc) { return(hc <= 99); }
//
//    // Clear both housecode parts (and thus disable use of FHT8V valve).
//    void clearHC() { hc1 = ~0, hc2 = ~0; }
//    // Set (non-volatile) HC1 and HC2 for single/primary FHT8V wireless valve under control.
//    // Both parts must be <= 99 for the house code to be valid and the valve used.
//    void setHC1(uint8_t hc) { hc1 = hc; }
//    void setHC2(uint8_t hc) { hc2 = hc; }
//    // Get (non-volatile) HC1 and HC2 for single/primary FHT8V wireless valve under control (will be 0xff until set).
//    // Both parts must be <= 99 for the house code to be valid and the valve used.
//    uint8_t getHC1() const { return(hc1); }
//    uint8_t getHC2() const { return(hc2); }
//    // Check if housecode is valid for controlling an FHT8V.
//    bool isValidHC() const { return(isValidFHTV8HouseCode(hc1) && isValidFHTV8HouseCode(hc2)); }
//
//    // Set radio to use (if non-NULL) or clear access to radio (if NULL).
//    void setRadio(OTRadioLink::OTRadioLink *r) { radio = r; }
//
//    // Type for information content of FHT8V message.
//    // Omits the address field unless it is actually used.
//    typedef struct
//      {
//      uint8_t hc1;
//      uint8_t hc2;
//#ifdef OTV0P2BASE_FHT8V_ADR_USED
//      uint8_t address;
//#endif
//      uint8_t command;
//      uint8_t extension;
//      } fht8v_msg_t;
//
//    // Decode raw bitstream into non-null command structure passed in; returns true if successful.
//    // Will return non-null if OK, else NULL if anything obviously invalid is detected such as failing parity or checksum.
//    // Finds and discards leading encoded 1 and trailing 0.
//    // Returns NULL on failure, else pointer to next full byte after last decoded.
//    static uint8_t const *FHT8VDecodeBitStream(uint8_t const *bitStream, uint8_t const *lastByte, fht8v_msg_t *command);
//
//    // Minimum and maximum FHT8V TX cycle times in half seconds: [115.0,118.5].
//    // Fits in an 8-bit unsigned value.
//    static const uint8_t MIN_FHT8V_TX_CYCLE_HS = (115*2);
//    static const uint8_t MAX_FHT8V_TX_CYCLE_HS = (118*2+1);
//
//    // Compute interval (in half seconds) between TXes for FHT8V given house code 2 (HC2).
//    // (In seconds, the formula is t = 115 + 0.5 * (HC2 & 7) seconds, in range [115.0,118.5].)
//    inline uint8_t FHT8VTXGapHalfSeconds(const uint8_t hc2) { return((hc2 & 7) + 230); }
//
//    // Compute interval (in half seconds) between TXes for FHT8V given house code 2
//    // given current halfSecondCountInMinorCycle assuming all remaining tick calls to _Next
//    // will be foregone in this minor cycle,
//    inline uint8_t FHT8VTXGapHalfSeconds(const uint8_t hc2, const uint8_t halfSecondCountInMinorCycle)
//      { return(FHT8VTXGapHalfSeconds(hc2) - (MAX_HSC - halfSecondCountInMinorCycle)); }
//
//    // For longest-possible encoded FHT8V/FS20 command in bytes plus terminating 0xff.
//    static const uint8_t MIN_FHT8V_200US_BIT_STREAM_BUF_SIZE = 46;
//    // Create stream of bytes to be transmitted to FHT80V at 200us per bit, msbit of each byte first.
//    // Byte stream is terminated by 0xff byte which is not a possible valid encoded byte.
//    // On entry the populated FHT8V command struct is passed by pointer.
//    // On exit, the memory block starting at buffer contains the low-byte, msbit-first bit, 0xff terminated TX sequence.
//    // The maximum and minimum possible encoded message sizes are 35 (all zero bytes) and 45 (all 0xff bytes) bytes long.
//    // Note that a buffer space of at least 46 bytes is needed to accommodate the longest-possible encoded message plus terminator.
//    // This FHT8V messages is encoded with the FS20 protocol.
//    // Returns pointer to the terminating 0xff on exit.
//    static uint8_t *FHT8VCreate200usBitStreamBptr(uint8_t *bptr, const fht8v_msg_t *command);
//
//    // Approximate maximum transmission (TX) time for bare FHT8V command frame in ms; strictly positive.
//    // This ignores any prefix needed for particular radios such as the RFM23B.
//    // ~80ms upwards.
//    static const uint8_t FHT8V_APPROX_MAX_RAW_TX_MS = ((((MIN_FHT8V_200US_BIT_STREAM_BUF_SIZE-1)*8) + 4) / 5);
//
//    // Typical FHT8V 'open' percentage, though partly depends on valve tails, etc.
//    // This is set to err on the side of slightly open to allow
//    // the 'linger' feature to work to help boilers dump heat with pump over-run
//    // when the the boiler is turned off.
//    // Actual values observed by DHD range from 6% to 25%.
//    static const uint8_t TYPICAL_MIN_PERCENT_OPEN = 10;
//
//    // Returns true if radio or house codes not set.
//    // Remains false while syncing as that is only temporary unavailability.
//    virtual bool isUnavailable() const { return((NULL == radio) || !isValidHC()); }
//
//    // Get estimated minimum percentage open for significant flow for this device; strictly positive in range [1,99].
//    // Defaults to typical value from observation.
//    virtual uint8_t getMinPercentOpen() const { return(TYPICAL_MIN_PERCENT_OPEN); }
//
//    // Call to reset comms with FHT8V valve and force resync.
//    // Resets values to power-on state so need not be called in program preamble if variables not tinkered with.
//    // Requires globals defined that this maintains:
//    //   syncedWithFHT8V (bit, true once synced)
//    //   FHT8V_isValveOpen (bit, true if this node has last sent command to open valve)
//    //   syncStateFHT8V (byte, internal)
//    //   halfSecondsToNextFHT8VTX (byte).
//    // FIXME: fit into standard RadValve API.
//    void FHT8VSyncAndTXReset()
//      {
//      syncedWithFHT8V = false;
//      syncStateFHT8V = 0;
//      halfSecondsToNextFHT8VTX = 0;
//      FHT8V_isValveOpen = false;
//      }
//
//    //#ifndef IGNORE_FHT_SYNC
//    // True once/while this node is synced with and controlling the target FHT8V valve; initially false.
//    // FIXME: fit into standard RadValve API.
//    bool isSyncedWithFHT8V() { return(syncedWithFHT8V); }
//    //#else
//    //bool isSyncedWithFHT8V() { return(true); } // Lie and claim always synced.
//    //#endif
//
//    // True if FHT8V valve is believed to be open under instruction from this system; false if not in sync.
//    // FIXME: fit into standard RadValve API.
//    bool getFHT8V_isValveOpen() { return(syncedWithFHT8V && FHT8V_isValveOpen); }
//
//    // GLOBAL NOTION OF CONTROLLED FHT8V VALVE STATE PROVIDED HERE
//    // True iff the FHT8V valve(s) (if any) controlled by this unit are really open.
//    // This waits until at least the command to open the FHT8Vhas been sent.
//    // FIXME: fit into standard RadValve API.
//    bool FHT8VisControlledValveOpen() { return(getFHT8V_isValveOpen()); }
//
//
//    // A set of RFM22/RFM23 register settings for use with FHT8V, stored in (read-only) program/Flash memory.
//    // Consists of a sequence of (reg#,value) pairs terminated with a 0xff register.
//    // The (valid) reg#s are <128, ie top bit clear.
//    // Magic numbers c/o Mike Stirling!
//    // Should not be linked into code image unless explicitly referred to.
//    static const uint8_t FHT8V_RFM23_Reg_Values[][2] PROGMEM;
//
//    // Values designed to work with FHT8V_RFM23_Reg_Values register settings.
//    static const uint8_t RFM23_PREAMBLE_BYTE = 0xaa; // Preamble byte for RFM23 reception.
//    static const uint8_t RFM23_PREAMBLE_MIN_BYTES = 4; // Minimum number of preamble bytes for reception.
//    static const uint8_t RFM23_PREAMBLE_BYTES = 5; // Recommended number of preamble bytes for reliable reception.
//    static const uint8_t RFM23_SYNC_BYTE = 0xcc; // Sync-word trailing byte (with FHT8V primarily).
//    static const uint8_t RFM23_SYNC_MIN_BYTES = 3; // Minimum number of sync bytes.
//
//    // Does nothing for now; different timing/driver routines are used.
//    virtual uint8_t read() { return(value); }
//
//    // Call at start of minor cycle to manage initial sync and subsequent comms with FHT8V valve.
//    // Conveys this system's TRVPercentOpen value to the FHT8V value periodically,
//    // setting FHT8V_isValveOpen true when the valve will be open/opening provided it received the latest TX from this system.
//    //
//    //   * allowDoubleTX  if true then a double TX is allowed for better resilience, but at cost of extra time and energy
//    //
//    // Uses its static/internal transmission buffer, and always leaves it in valid date.
//    //
//    // Iff this returns true then call FHT8VPollSyncAndTX_Next() at or before each 0.5s from the cycle start
//    // to allow for possible transmissions.
//    //
//    // See https://sourceforge.net/p/opentrv/wiki/FHT%20Protocol/ for the underlying protocol.
//    //
//    // ALSO MANAGES RX FROM OTHER NODES WHEN ENABLED IN HUB MODE.
//    bool FHT8VPollSyncAndTX_First(bool allowDoubleTX = false);
//
//    // If FHT8VPollSyncAndTX_First() returned true then call this each 0.5s from the start of the cycle, as nearly as possible.
//    // This allows for possible transmission slots on each half second.
//    //
//    //   * allowDoubleTX  if true then a double TX is allowed for better resilience, but at cost of extra time and energy
//    //
//    // This will sleep (at reasonably low power) as necessary to the start of its TX slot,
//    // else will return immediately if no TX needed in this slot.
//    //
//    // Iff this returns false then no further TX slots will be needed
//    // (and thus this routine need not be called again) on this minor cycle
//    //
//    // ALSO MANAGES RX FROM OTHER NODES WHEN ENABLED IN HUB MODE.
//    bool FHT8VPollSyncAndTX_Next(bool allowDoubleTX = false);
//  };


    }

#endif