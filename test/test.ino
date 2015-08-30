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

Author(s) / Copyright (s): Damon Hart-Davis 2015
*/

/*Unit test routines for library code.
 */

// Arduino libraries imported here (even for use in other .cpp files).
//#include <SPI.h>

#define UNIT_TESTS

// Include the library under test.
#include <OTV0p2Base.h>
#include <OTRadioLink.h>
#include <OTRFM23BLink.h>


#if F_CPU == 1000000 // 1MHz CPU indicates V0p2 board.
#define ON_V0P2_BOARD
#endif


void setup()
  {
#ifdef ON_V0P2_BOARD
  // initialize serial communications at 4800 bps for typical use with V0p2 board.
  Serial.begin(4800);
#else
  // initialize serial communications at 9600 bps for typical use with (eg) Arduino UNO.
  Serial.begin(9600);
#endif
  }



// Error exit from failed unit test, one int parameter and the failing line number to print...
// Expects to terminate like panic() with flashing light can be detected by eye or in hardware if required.
static void error(int err, int line)
  {
  for( ; ; )
    {
    Serial.print(F("***Test FAILED*** val="));
    Serial.print(err, DEC);
    Serial.print(F(" =0x"));
    Serial.print(err, HEX);
    if(0 != line)
      {
      Serial.print(F(" at line "));
      Serial.print(line);
      }
    Serial.println();
//    LED_HEATCALL_ON();
//    tinyPause();
//    LED_HEATCALL_OFF();
//    sleepLowPowerMs(1000);
    delay(1000);
    }
  }

// Deal with common equality test.
static inline void errorIfNotEqual(int expected, int actual, int line) { if(expected != actual) { error(actual, line); } }
// Allowing a delta.
static inline void errorIfNotEqual(int expected, int actual, int delta, int line) { if(abs(expected - actual) > delta) { error(actual, line); } }

// Test expression and bucket out with error if false, else continue, including line number.
// Macros allow __LINE__ to work correctly.
#define AssertIsTrueWithErr(x, err) { if(!(x)) { error((err), __LINE__); } }
#define AssertIsTrue(x) AssertIsTrueWithErr((x), 0)
#define AssertIsEqual(expected, x) { errorIfNotEqual((expected), (x), __LINE__); }
#define AssertIsEqualWithDelta(expected, x, delta) { errorIfNotEqual((expected), (x), (delta), __LINE__); }


// Check that correct version of this library is under test.
static void testLibVersion()
  {
  Serial.println("LibVersion");
#if !(0 == ARDUINO_LIB_OTRADIOLINK_VERSION_MAJOR) || !(9 == ARDUINO_LIB_OTRADIOLINK_VERSION_MINOR)
#error Wrong library version!
#endif
//  AssertIsEqual(0, ARDUINO_LIB_OTRADIOLINK_VERSION_MAJOR);
//  AssertIsEqual(2, ARDUINO_LIB_OTRADIOLINK_VERSION_MINOR);
  }

// Check that correct versions of underlying libraries are in use.
static void testLibVersions()
  {
  Serial.println("LibVersions");
#if !(0 == ARDUINO_LIB_OTV0P2BASE_VERSION_MAJOR) || !(9 == ARDUINO_LIB_OTV0P2BASE_VERSION_MINOR)
#error Wrong library version!
#endif  
#if !(0 == ARDUINO_LIB_OTRFM23BLINK_VERSION_MAJOR) || !(9 == ARDUINO_LIB_OTRFM23BLINK_VERSION_MINOR)
#error Wrong library version!
#endif
  }

// Test class for printing stuff.
class PrintToBuf : public Print
  {
  private:
    int count;
  public:
    PrintToBuf(uint8_t *bufp, size_t bufl) : count(0), buf(bufp), buflen(bufl) { }
    virtual size_t write(const uint8_t b)
      {
      if(count < buflen)
        {
        buf[count] = b;
        // If there is space, null terminate.
        if(count+1 < buflen) { buf[count+1] = '\0'; }
        }
      ++count;
      }
    int getCount() { return(count); }
    uint8_t *const buf;
    const uint8_t buflen;
  };

// Test the frame-dump routine.
static void testFrameDump()
  {
  Serial.println("FrameDump");
  const char *s1 = "Hello, world!";
  // Eyeball output...
  OTRadioLink::dumpRXMsg((uint8_t *)s1, strlen(s1));
  // Real test...
  uint8_t buf[64];
  PrintToBuf ptb1(buf, sizeof(buf));
  OTRadioLink::printRXMsg(&ptb1, (uint8_t *)s1, strlen(s1));
  AssertIsEqual(32, ptb1.getCount());
  AssertIsEqual(0, strncmp("|13  H e l l o ,   w o r l d !\r\n", (const char *)ptb1.buf, sizeof(buf)));
  PrintToBuf ptb2(buf, sizeof(buf));
  uint8_t m2[] = { '{', '}'|0x80 };
  OTRadioLink::dumpRXMsg(m2, sizeof(m2));
  OTRadioLink::printRXMsg(&ptb2, m2, sizeof(m2));
  AssertIsEqual(0, strncmp("|2  {FD\r\n", (const char *)ptb2.buf, sizeof(buf)));
  PrintToBuf ptb3(buf, sizeof(buf));
  uint8_t m3[] = { 0, 1, 10, 15, 16, 31 };
  OTRadioLink::dumpRXMsg(m3, sizeof(m3));
  OTRadioLink::printRXMsg(&ptb3, m3, sizeof(m3));
  AssertIsEqual(0, strncmp("|6 00010A0F101F\r\n", (const char *)ptb3.buf, sizeof(buf)));
  }

// Do some basic testing of CRC 7/5B routine.
static void testCRC7_5B()
  {
  Serial.println("CRC7_5B");
  // Test the 7-bit CRC (0x5b) routine at a few points.
  const uint8_t crc0 = OTRadioLink::crc7_5B_update(0, 0); // Minimal stats payload with normal power and minimum temperature.
  AssertIsTrueWithErr((0 == crc0), crc0); 
  const uint8_t crc1 = OTRadioLink::crc7_5B_update(0x40, 0); // Minimal stats payload with normal power and minimum temperature.
  AssertIsTrueWithErr((0x1a == crc1), crc1); 
  const uint8_t crc2 = OTRadioLink::crc7_5B_update(0x50, 40); // Minimal stats payload with low power and 20C temperature.
  AssertIsTrueWithErr((0x7b == crc2), crc2); 
  AssertIsEqual(0x7b, OTRadioLink::crc7_5B_update_nz_final(0x50, 40)); 
  }

// Test the trim-trailing-zeros frame filter.
static void testFrameFilterTrailingZeros()
  {
  Serial.println("FrameFilterTrailingZeros");
  uint8_t buf[64];
  uint8_t len;
  len = sizeof(buf);
  AssertIsTrue(OTRadioLink::frameFilterTrailingZeros(buf, len)); // Should never reject frame.
  AssertIsTrue(len <= sizeof(buf)); // Should not make frame bigger!
  memset(buf, 0, sizeof(buf)); // Clear to all zeros.
  len = sizeof(buf);
  AssertIsTrue(OTRadioLink::frameFilterTrailingZeros(buf, len)); // Should never reject frame.
  AssertIsEqual(1, len); // Should work with shortest (all-zeros) frame retaining final zero.
  buf[0] = 69;
  len = sizeof(buf);
  AssertIsTrue(OTRadioLink::frameFilterTrailingZeros(buf, len)); // Should never reject frame.
  AssertIsEqual(2, len); // Should work with shortest non-zero frame retaining final zero.
  const uint8_t p = 23;
  buf[p] = 1;
  len = sizeof(buf);
  AssertIsTrue(OTRadioLink::frameFilterTrailingZeros(buf, len)); // Should never reject frame.
  AssertIsEqual(p+2, len); // Should work with intermediate frame retaining final zero.
  buf[sizeof(buf)-1] = 42;
  len = sizeof(buf);
  AssertIsTrue(OTRadioLink::frameFilterTrailingZeros(buf, len)); // Should never reject frame.
  AssertIsEqual(sizeof(buf), len); // Should work with max frame without trailing zeros.
  }

// Do some basic exercise of the RFM23B class, eg that it compiles.
static void testRFM23B()
  {
  Serial.println("RFM23B");
  OTRFM23BLink::OTRFM23BLink<OTV0P2BASE::V0p2_PIN_SPI_nSS> l0;
  OTRFM23BLink::OTRFM23BLink<OTV0P2BASE::V0p2_PIN_SPI_nSS, -1> l1;
  OTRFM23BLink::OTRFM23BLink<OTV0P2BASE::V0p2_PIN_SPI_nSS, 9> l2;
//#ifdef ON_V0P2_BOARD
//  // Can't do anything with this unless on V0p2 board.
//  l0.preinit(NULL); // Must not break anything nor stall!
//#endif
  }

// Pick test buffer size to match actual RFM23B buffer/FIFO size.
static const uint8_t TEST_MIN_Q_MSG_SIZE = 64;

// Tests for all ISRRXQueue implementations.
// Assumes being passed an empty freshly-created instance.
// Leaves the queue instance empty when done.
static void allISRRXQueue(OTRadioLink::ISRRXQueue &q)
  {
//  Serial.println("allISRRXQueue");
  for(uint8_t j = 2; j-- > 0; )
    {
    uint8_t queueRXMsgsMin;
    uint8_t maxRXMsgLen;
    q.getRXCapacity(queueRXMsgsMin, maxRXMsgLen);
    AssertIsTrue(q.isEmpty());
    AssertIsTrue(!q.isFull());
    uint8_t len;
    AssertIsTrue(NULL == q.peekRXMsg(len));
    AssertIsTrueWithErr((queueRXMsgsMin >= 1), queueRXMsgsMin); 
    AssertIsTrueWithErr((maxRXMsgLen >= TEST_MIN_Q_MSG_SIZE), maxRXMsgLen); 
    AssertIsEqual(0, q.getRXMsgsQueued());
    // Pretend to be an ISR and try to load up a message.
    volatile uint8_t *ib1 = q._getRXBufForInbound();
    AssertIsTrue(NULL != ib1); 
    const uint8_t r1 = OTV0P2BASE::randRNG8();
    *ib1 = r1;
    q._loadedBuf(1);
    // Try to retrieve the queued message.
    AssertIsTrue(!q.isEmpty());
    AssertIsTrue(NULL != q.peekRXMsg(len));
    const volatile uint8_t *pb = q.peekRXMsg(len);
    AssertIsTrue(NULL != pb);
    AssertIsEqual(1, len);
    AssertIsEqual(1, q.getRXMsgsQueued());
    AssertIsEqual(r1, pb[0]);
    q.removeRXMsg();
    // Check that the queue is empty again.
    AssertIsEqual(0, q.getRXMsgsQueued());
    AssertIsTrue(q.isEmpty());
    AssertIsTrue(!q.isFull());
    AssertIsTrue(NULL == q.peekRXMsg(len));
    q.removeRXMsg();
    AssertIsTrue(q.isEmpty());
    
    // Fill the queue up and empty it again, a few times!
    for(int8_t i = 1 + (3 & OTV0P2BASE::randRNG8() % TEST_MIN_Q_MSG_SIZE); i-- > 0; )
      {
      // (Maximum of 255 messages queued in fact.)
      uint8_t queued = 0;
      while((queued < 255) && !q.isFull())
        {
        AssertIsEqual(queued, q.getRXMsgsQueued());
        uint8_t bufFull[TEST_MIN_Q_MSG_SIZE];
        len = 1 + (OTV0P2BASE::randRNG8() % TEST_MIN_Q_MSG_SIZE);
        volatile uint8_t *ibF = q._getRXBufForInbound();
        ibF[0] = queued;
        ibF[len-1] = queued;
        AssertIsTrue(NULL != ibF); 
        q._loadedBuf(len);
        ++queued;
        AssertIsEqual(queued, q.getRXMsgsQueued());
        }
//      Serial.print("Queued: "); Serial.println(queued);
      for(int dq = 0; queued > 0; ++dq)
        {
        AssertIsEqual(queued, q.getRXMsgsQueued());
        AssertIsTrue(!q.isEmpty());
        uint8_t len;
        const volatile uint8_t *pb = q.peekRXMsg(len);
        AssertIsTrueWithErr(NULL != pb, queued);
        AssertIsTrue((len > 0) && (len <= TEST_MIN_Q_MSG_SIZE));
        AssertIsEqual(dq, pb[0]);
        AssertIsEqual(dq, pb[len - 1]);
        q.removeRXMsg();
        --queued;
        AssertIsEqual(queued, q.getRXMsgsQueued());
        AssertIsTrue((queued > 0) || !q.isFull()); // Fragmentation may prevent queue becoming 'not full' immediately.
        }
      }

    // Check that the queue is empty again.
    AssertIsEqual(0, q.getRXMsgsQueued());
    AssertIsTrue(q.isEmpty());
    AssertIsTrue(!q.isFull());
    AssertIsTrue(NULL == q.peekRXMsg(len));
    q.removeRXMsg();
    AssertIsTrue(q.isEmpty());
    }
  }

// Do some basic exercise of ISRRXQueue1Deep.
static void testISRRXQueue1Deep()
  {
  Serial.println("ISRRXQueue1Deep");
  OTRadioLink::ISRRXQueue1Deep<TEST_MIN_Q_MSG_SIZE> q;
  allISRRXQueue(q);
  // Specific tests for this queue type.
  // Check queue is empty again.
  AssertIsEqual(0, q.getRXMsgsQueued());
  // Pretend to be an ISR and try to load up a message.
  volatile uint8_t *ib2 = q._getRXBufForInbound();
  AssertIsTrue(NULL != ib2); 
  const uint8_t r1 = OTV0P2BASE::randRNG8();
  ib2[0] = r1; ib2[1] = 0;
  q._loadedBuf(2);
  // Check that the message was queued.
  AssertIsEqual(1, q.getRXMsgsQueued());
  // Verify that the queue is now full (no space for new RX).
  AssertIsTrue(!q.isEmpty());
  AssertIsTrue(q.isFull());
  AssertIsTrue(NULL == q._getRXBufForInbound());
  // Try to retrieve the queued message.
  AssertIsEqual(1, q.getRXMsgsQueued());
  uint8_t len;
  const volatile uint8_t *pb = q.peekRXMsg(len);
  AssertIsTrue(NULL != pb);
  AssertIsEqual(r1, pb[0]);
  q.removeRXMsg();
  // Check that the queue is empty again.
  AssertIsTrue(q.isEmpty());
  AssertIsEqual(0, q.getRXMsgsQueued());
  }


// Do some basic exercise of ISRRXQueue1Deep.
static void testISRRXQueueVarLenMsg()
  {
  Serial.println("ISRRXQueueVarLenMsg");
  OTRadioLink::ISRRXQueueVarLenMsg<TEST_MIN_Q_MSG_SIZE, 2> q;
  allISRRXQueue(q);
  // Some type/impl-specific whitebox tests.
#ifdef ISRRXQueueVarLenMsg_VALIDATE
  OTRadioLink::ISRRXQueueVarLenMsg<2, 2> q0;
  AssertIsTrue(q0.isEmpty());
  uint8_t n, o, c;
  const volatile uint8_t *bp;
  int s;
  q0.validate(&Serial, n, o, c, bp, s);
  AssertIsEqual(6, s); // Space for 2 x (1 + 2) entries.
  AssertIsEqual(0, c);
  AssertIsEqual(0, n); AssertIsEqual(0, o); // Contingent on impl.
  // Pretend to be an ISR and try to load up 1st (max-size) message.
  volatile uint8_t *ib = q0._getRXBufForInbound();
  AssertIsTrue(NULL != ib); 
  const uint8_t r1 = OTV0P2BASE::randRNG8();
  ib[0] = r1; ib[1] = ~r1;
  q0._loadedBuf(2);
  q0.validate(&Serial, n, o, c, bp, s);
  AssertIsEqual(1, c);
  AssertIsEqual(3, n); AssertIsEqual(0, o); // Contingent on impl.
  AssertIsEqual(2, bp[0]);
  AssertIsEqual(r1, bp[1]);
  AssertIsEqual((uint8_t)~r1, bp[2]);
  AssertIsTrue(!q0.isEmpty());
  AssertIsTrue(!q0.isFull());
  // Pretend to be an ISR and try to load up a 2nd (max-size) message.
  ib = q0._getRXBufForInbound();
  AssertIsTrue(NULL != ib); 
  const uint8_t r2 = OTV0P2BASE::randRNG8();
  ib[0] = r2; ib[1] = ~r2;
  q0._loadedBuf(2);
  AssertIsTrue(!q0.isEmpty());
  AssertIsTrue(q0.isFull());
  q0.validate(&Serial, n, o, c, bp, s);
  AssertIsEqual(2, c);
  AssertIsEqual(0, n); AssertIsEqual(0, o); // Contingent on impl; 'next' index wrapped around.
  AssertIsEqual(2, bp[0]);
  AssertIsEqual(r1, bp[1]);
  AssertIsEqual((uint8_t)~r1, bp[2]);
  AssertIsEqual(2, bp[3]);
  AssertIsEqual(r2, bp[4]);
  AssertIsEqual((uint8_t)~r2, bp[5]);
  // Attempt to unqueue the first message.
  uint8_t len;
  const volatile uint8_t *pb = q0.peekRXMsg(len);
  AssertIsTrue(NULL != pb);
  AssertIsEqual(2, len);
  AssertIsEqual(r1, pb[0]);
  AssertIsEqual((uint8_t)~r1, pb[1]);
  AssertIsTrue(!q0.isEmpty());
  AssertIsTrue(q0.isFull());
  AssertIsEqual(2, q0.getRXMsgsQueued());
  q0.removeRXMsg();
  AssertIsEqual(1, q0.getRXMsgsQueued());
  AssertIsTrue(!q0.isEmpty());
  AssertIsTrue(!q0.isFull());
  q0.validate(&Serial, n, o, c, bp, s);
  AssertIsEqual(1, c);
  AssertIsEqual(0, n); AssertIsEqual(3, o); // Contingent on impl.
  // Pretend to be an ISR and try to load up a 3rd (min-size) message.
  ib = q0._getRXBufForInbound();
  AssertIsTrue(NULL != ib); 
  const uint8_t r3 = OTV0P2BASE::randRNG8();
  ib[0] = r3;
  q0._loadedBuf(1);
  AssertIsTrue(!q0.isEmpty());
  AssertIsTrue(q0.isFull());
  q0.validate(&Serial, n, o, c, bp, s);
  AssertIsEqual(2, c);
  AssertIsEqual(2, n); AssertIsEqual(3, o); // Contingent on impl.
  AssertIsEqual(1, bp[0]);
  AssertIsEqual(r3, bp[1]);
  // Attempt to unqueue the 2nd message.
  pb = q0.peekRXMsg(len);
  AssertIsTrue(NULL != pb);
  AssertIsEqual(2, len);
  AssertIsEqual(r2, pb[0]);
  AssertIsEqual((uint8_t)~r2, pb[1]);
  AssertIsTrue(!q0.isEmpty());
  AssertIsTrue(q0.isFull());
  AssertIsEqual(2, q0.getRXMsgsQueued());
  q0.removeRXMsg();
  AssertIsEqual(1, q0.getRXMsgsQueued());
  AssertIsTrue(!q0.isEmpty());
  AssertIsTrue(!q0.isFull());
  q0.validate(&Serial, n, o, c, bp, s);
  AssertIsEqual(1, c);
  AssertIsEqual(2, n); AssertIsEqual(0, o); // Contingent on impl.
  // Attempt to unqueue the 3rd message.
  pb = q0.peekRXMsg(len);
  AssertIsTrue(NULL != pb);
  AssertIsEqual(1, len);
  AssertIsEqual(r3, pb[0]);
  AssertIsTrue(!q0.isEmpty());
  AssertIsTrue(!q0.isFull());
  AssertIsEqual(1, q0.getRXMsgsQueued());
  q0.removeRXMsg();
  AssertIsEqual(0, q0.getRXMsgsQueued());
  AssertIsTrue(q0.isEmpty());
  AssertIsTrue(!q0.isFull());
  q0.validate(&Serial, n, o, c, bp, s);
  AssertIsEqual(0, c);
  AssertIsEqual(2, n); AssertIsEqual(2, o); // Contingent on impl.
#endif
  }


// BASE

// Self-test of EEPROM functioning (and smart/split erase/write).
// Will not usually perform any wear-inducing activity (is idempotent).
static void testEEPROM()
  {
  Serial.println("EEPROM");
  if((uint8_t) 0xff != eeprom_read_byte((uint8_t*)V0P2BASE_EE_START_TEST_LOC))
    {
    AssertIsTrue(OTV0P2BASE::eeprom_smart_erase_byte((uint8_t*)V0P2BASE_EE_START_TEST_LOC)); // Should have attempted erase.
    AssertIsEqual((uint8_t) 0xff, eeprom_read_byte((uint8_t*)V0P2BASE_EE_START_TEST_LOC)); // Should have erased.
    }
  AssertIsTrue(!OTV0P2BASE::eeprom_smart_erase_byte((uint8_t*)V0P2BASE_EE_START_TEST_LOC)); // Should not need erase nor attempt one.

  const uint8_t eaTestPattern = 0xa5; // Test pattern for masking (selective bit clearing).
  if(0 != ((~eaTestPattern) & eeprom_read_byte((uint8_t*)V0P2BASE_EE_START_TEST_LOC2))) // Will need to clear some bits.
    {
    AssertIsTrue(OTV0P2BASE::eeprom_smart_clear_bits((uint8_t*)V0P2BASE_EE_START_TEST_LOC2, eaTestPattern)); // Should have attempted write.
    AssertIsEqual(0, ((~eaTestPattern) & eeprom_read_byte((uint8_t*)V0P2BASE_EE_START_TEST_LOC2))); // Should have written.
    }
  AssertIsTrue(!OTV0P2BASE::eeprom_smart_clear_bits((uint8_t*)V0P2BASE_EE_START_TEST_LOC2, eaTestPattern)); // Should not need write nor attempt one.
  }

// Check that the various forms of sleep don't break anything or hang.
// TODO: check that user of async timer 2 doesn't cause problems.
static void testSleep()
  {
  Serial.println("Sleep");
  // Flush serial output to avoid TXing while messing with (CPU and serial) clock.
  Serial.flush();
  for(uint8_t i = 0; i < 25; ++i)
    {
    const uint8_t to = OTV0P2BASE::randRNG8NextBoolean() ? WDTO_15MS : WDTO_60MS;
    OTV0P2BASE::nap(to);
    OTV0P2BASE::nap(to, OTV0P2BASE::randRNG8NextBoolean());
    OTV0P2BASE::idleCPU(to, OTV0P2BASE::randRNG8NextBoolean());
    OTV0P2BASE::idleCPU(to, OTV0P2BASE::randRNG8NextBoolean());
    }
  }

// Test for expected behaviour of RNG8 PRNG starting from a known state.
static void testRNG8()
  {
  Serial.println("RNG8");
  const uint8_t r = OTV0P2BASE::randRNG8(); // Capture a value and some noise before reset.
  // Reset to known state; API not normally exposed and only exists for unit tests.
  OTV0P2BASE::resetRNG8();
  // Extract and check a few initial values.
  const uint8_t v1 = OTV0P2BASE::randRNG8();
  const uint8_t v2 = OTV0P2BASE::randRNG8();
  const uint8_t v3 = OTV0P2BASE::randRNG8();
  const uint8_t v4 = OTV0P2BASE::randRNG8();
  AssertIsTrue(1 == v1);
  AssertIsTrue(0 == v2);
  AssertIsTrue(3 == v3);
  AssertIsTrue(14 == v4);
  // Be kind to other test routines that use RNG8 and put some noise back in!
  static uint8_t c1, c2;
  OTV0P2BASE::seedRNG8(r, ++c1, c2--);
  }

// Tests of entropy gathering routines.
//
// Maximum number of identical nominally random bits (or values with approx one bit of entropy) in a row tolerated.
// Set large enough that even soak testing for many hours should not trigger a failure if behaviour is plausibly correct.
#define MAX_IDENTICAL_BITS_SEQUENTIALLY 32
void testEntropyGathering()
  {
  Serial.println("EntropyGathering");
  Serial.println("  FIXME: tests need re-enabling here"); // When code moves over.

  // Test WDT jitter: assumed about 1 bit of entropy per call/result.
  //DEBUG_SERIAL_PRINT_FLASHSTRING("jWDT... ");
  const uint8_t jWDT = OTV0P2BASE::clockJitterWDT();
  for(int i = MAX_IDENTICAL_BITS_SEQUENTIALLY; --i >= 0; )
    {
    if(jWDT != OTV0P2BASE::clockJitterWDT()) { break; } // Stop as soon as a different value is obtained.
    AssertIsTrueWithErr(0 != i, i); // Generated too many identical values in a row. 
    }
  //DEBUG_SERIAL_PRINT_FLASHSTRING(" 1st=");
  //DEBUG_SERIAL_PRINTFMT(jWDT, BIN);
  //DEBUG_SERIAL_PRINTLN();

#ifndef NO_clockJitterRTC
  // Test RTC jitter: assumed about 1 bit of entropy per call/result.
  //DEBUG_SERIAL_PRINT_FLASHSTRING("jRTC... ");
  // FIXME // for(const uint8_t t0 = getSubCycleTime(); t0 == getSubCycleTime(); ) { } // Wait for sub-cycle time to roll to toughen test.
  const uint8_t jRTC = OTV0P2BASE::clockJitterRTC();
  for(int i = MAX_IDENTICAL_BITS_SEQUENTIALLY; --i >= 0; )
    {
    if(jRTC != OTV0P2BASE::clockJitterRTC()) { break; } // Stop as soon as a different value is obtained.
    AssertIsTrue(0 != i); // Generated too many identical values in a row. 
    }
  //DEBUG_SERIAL_PRINT_FLASHSTRING(" 1st=");
  //DEBUG_SERIAL_PRINTFMT(jRTC, BIN);
  //DEBUG_SERIAL_PRINTLN();
#endif

#ifndef NO_clockJitterEntropyByte
  // Test full-byte jitter: assumed about 8 bits of entropy per call/result.
  //DEBUG_SERIAL_PRINT_FLASHSTRING("jByte... ");
  //const uint8_t t0j = getSubCycleTime();
  // FIXME // while(t0j == getSubCycleTime()) { } // Wait for sub-cycle time to roll to toughen test.
  const uint8_t jByte = OTV0P2BASE::clockJitterEntropyByte();

  for(int i = MAX_IDENTICAL_BITS_SEQUENTIALLY/8; --i >= 0; )
    {
    if(jByte != OTV0P2BASE::clockJitterEntropyByte()) { break; } // Stop as soon as a different value is obtained.
    AssertIsTrue(0 != i); // Generated too many identical values in a row. 
    }
  //DEBUG_SERIAL_PRINT_FLASHSTRING(" 1st=");
  //DEBUG_SERIAL_PRINTFMT(jByte, BIN);
  //DEBUG_SERIAL_PRINT_FLASHSTRING(", ticks=");
  //DEBUG_SERIAL_PRINT((uint8_t)(t1j - t0j - 1));
  //DEBUG_SERIAL_PRINTLN();
#endif
  
//  // Test noisy ADC read: assumed at least one bit of noise per call/result.
//  const uint8_t nar1 = noisyADCRead(true);
//#if 0
//  DEBUG_SERIAL_PRINT_FLASHSTRING("nar1 ");
//  DEBUG_SERIAL_PRINTFMT(nar1, BIN);
//  DEBUG_SERIAL_PRINTLN();
//#endif
//  for(int i = MAX_IDENTICAL_BITS_SEQUENTIALLY; --i >= 0; )
//    {
//    const uint8_t nar = noisyADCRead(true);
//    if(nar1 != nar) { break; } // Stop as soon as a different value is obtained.
//#if 0
//    DEBUG_SERIAL_PRINT_FLASHSTRING("repeat nar ");
//    DEBUG_SERIAL_PRINTFMT(nar, BIN);
//    DEBUG_SERIAL_PRINTLN();
//#endif
//    AssertIsTrue(0 != i); // Generated too many identical values in a row. 
//    }

//  for(int w = 0; w < 2; ++w)
//    {
//    const bool whiten = (w != 0);
//    // Test secure random byte generation with and without whitening
//    // to try to ensure that the underlying generation is sound.
//    const uint8_t srb1 = getSecureRandomByte(whiten);
//#if 0
//    DEBUG_SERIAL_PRINT_FLASHSTRING("srb1 ");
//    DEBUG_SERIAL_PRINTFMT(srb1, BIN);
//    if(whiten) { DEBUG_SERIAL_PRINT_FLASHSTRING(" whitened"); } 
//    DEBUG_SERIAL_PRINTLN();
//#endif
//    for(int i = MAX_IDENTICAL_BITS_SEQUENTIALLY/8; --i >= 0; )
//      {
//      if(srb1 != getSecureRandomByte(whiten)) { break; } // Stop as soon as a different value is obtained.
//      AssertIsTrue(0 != i); // Generated too many identical values in a row. 
//      }
//    }
  }



// To be called from loop() instead of main code when running unit tests.
// Tests generally flag an error and stop the test cycle with a call to panic() or error().
void loop()
  {
  static int loopCount = 0;

  // Allow the terminal console to be brought up.
  for(int i = 3; i > 0; --i)
    {
    Serial.print(F("Tests starting... "));
    Serial.print(i);
    Serial.println();
    delay(1000);
    }
  Serial.println();


  // Run the tests, fastest / newest / most-fragile / most-interesting first...
  testLibVersion();
  testLibVersions();

  // OTRadioLink
  testFrameDump();
  testCRC7_5B();
  testFrameFilterTrailingZeros();
  testISRRXQueue1Deep();
  testISRRXQueueVarLenMsg();

  // OTRFM23BLink
  testRFM23B();

  // OTV0p2Base
  testEEPROM();
  testSleep();
  testRNG8();
  testEntropyGathering();


  // Announce successful loop completion and count.
  ++loopCount;
  Serial.println();
  Serial.print(F("%%% All tests completed OK, round "));
  Serial.print(loopCount);
  Serial.println();
  Serial.println();
  Serial.println();
  delay(2000);
  }
