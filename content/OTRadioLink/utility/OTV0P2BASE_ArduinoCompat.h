/*
//The OpenTRV project licenses this file to you
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
 Basic compatibility support for Arduino and non-Arduino environments.
 */

#ifndef OTV0P2BASE_ARDUINOCOMPAT_H
#define OTV0P2BASE_ARDUINOCOMPAT_H


#ifndef ARDUINO

#include <stddef.h>
#include <stdint.h>
#include <string.h>


// Enable minimal elements to support cross-compilation.
// NOT in normal OpenTRV namespace(s).

// F() macro on Arduino for hosting string constant in Flash, eg print(F("no space taken in RAM")).
#ifndef F
class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(static_cast<const char *>(string_literal)))
#endif

// pgm_read_byte() macro for Arduino reads one byte from Flash.
#ifndef pgm_read_byte
#define pgm_read_byte(p) (*reinterpret_cast<const char *>(p))
#endif

// Minimal skeleton matching Print to permit at least compilation and test on non-Arduino platforms.
// Implementation is not necessarily efficient as assumed to be for (unit) test.
class Print
    {
    private:
        size_t printUnsigned(unsigned long ul, uint_fast8_t b)
            {
            if(b < 2) { b = 2; }
            else if(b > 36) { b = 36; }
            // Worst case space requirement is base 2, for 8-bit bytes.
            const size_t maxSpace = (8*sizeof(unsigned long));
            char buf[maxSpace];
            char *p = buf;
            // Compute least-significant digit first (and thus the result number backwards).
            // Digits are 0-9 and then A-Z.
            // Always generate lsd for 0.
            do {
               const uint_fast8_t digit = (uint_fast8_t)(ul % b);
               *p++ = char((digit <= 9) ? ('0' + digit) : ('A' + digit - 10));
               ul /= b;
               } while(ul > 0);
            // Spit digits out in correct order.
            size_t n = 0;
            while(--p >= buf) { n += write(uint8_t(*p)); }
            return(n);
            }
        size_t printSigned(const long l, const uint_fast8_t b)
            {
            // Special case for base 10, deal with -ve.
            if((l < 0) && (10 == b)) { const size_t n = write('-'); return(n + printUnsigned((unsigned long)-l, b)); }
            return(printUnsigned((unsigned long)l, b));
            }
    public:
        virtual size_t write(uint8_t) = 0;
        virtual size_t write(const uint8_t *buf, size_t size) { size_t n = 0; while((size-- > 0) && (0 != write(*buf++))) { ++n; } return(n); }
        size_t write(const char *buf, size_t size) { return write((const uint8_t *)buf, size); }
        size_t write(const char *s) { return((NULL == s) ? 0 : write((const uint8_t *)s, (size_t)strlen(s))); }
        size_t println() { return(write("\r\n", 2)); }
        size_t print(char c) { return(write((uint8_t)c)); }
        size_t println(char c) { const size_t n = print(c); return(n + println()); }
        size_t print(unsigned char uc, int b = 10 /* DEC */) { return(print((unsigned long)uc, b)); }
        size_t println(unsigned char uc, int b = 10 /* DEC */) { const size_t n = print(uc, b); return(n + println()); }
        size_t print(int i, int b = 10 /* DEC */) { return(print((long) i, b)); }
        size_t println(int i, int b = 10 /* DEC */) { const size_t n = print(i, b); return(n + println()); }
        size_t print(long l, int b = 10 /* DEC */) { return(printSigned(l, (uint_fast8_t)b)); }
        size_t println(long l, int b = 10 /* DEC */) { const size_t n = print(l, b); return(n + println()); }
        size_t print(unsigned long ul, int b = 10 /* DEC */) { return(printUnsigned(ul, (uint_fast8_t)b)); }
        size_t println(unsigned long ul, int b = 10 /* DEC */) { const size_t n = print(ul, b); return(n + println()); }
        size_t print(const char *s) { return(write(s, strlen(s))); }
        size_t println(const char *s) { const size_t n = print(s); return(n + println()); }
        size_t print(const __FlashStringHelper *f) { return(print(reinterpret_cast<const char *>(f))); }
        size_t println(const __FlashStringHelper *f) { const size_t n = print(f); return(n + println()); }
    };

// Minimal skeleton matching Stream to permit at least compilation and test on non-Arduino platforms.
class Stream : public Print
  {
  protected:
    unsigned long _timeout = 1000; // Timeout in milliseconds before aborting read.
  public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void flush() = 0;
  };

#endif // ARDUINO


#endif
