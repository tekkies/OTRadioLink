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

Author(s) / Copyright (s): Deniz Erbilgin 2016
                           Damon Hart-Davis 2016
*/

#include "OTV0P2BASE_CLI.h"

#include "OTV0P2BASE_EEPROM.h"
#include "OTV0P2BASE_RTC.h"
#include "OTV0P2BASE_Security.h"
#include "OTV0P2BASE_Sleep.h"
#include "OTV0P2BASE_Util.h"


namespace OTV0P2BASE {
namespace CLI {


// Prints warning to serial (that must be up and running) that invalid (CLI) input has been ignored.
// Probably should not be inlined, to avoid creating duplicate strings in Flash.
void InvalidIgnored() { Serial.println(F("Invalid, ignored.")); }


// Set / clear node association(s) (nodes to accept frames from) (eg "A hh hh hh hh hh hh hh hh").
//        To add a new node/association: "A hh hh hh hh hh hh hh hh"
//        - Reads first two bytes of each token in hex and ignores the rest.
//        - Prints number of nodes set to serial
//        - Stops adding nodes once EEPROM full
//        - No error checking yet.
//        - Only accepts upper case for hex values.
//        To clear all nodes: "A *".
//        To query current status "A ?".
bool SetNodeAssoc::doCommand(char *const buf, const uint8_t buflen)
    {
    char *last; // Used by strtok_r().
    char *tok1;
    // Minimum 3 character sequence makes sense and is safe to tokenise, eg "A *".
    if((buflen >= 3) && (NULL != (tok1 = strtok_r(buf + 2, " ", &last))))
        {
        // "A ?" or "A ? XX"
        if('?' == *tok1)
            {
            // Query current association status.
            Serial.println(F("Nodes:"));
            const uint8_t nn = countNodeAssociations();
//            Serial.println(nn);
            // Print first two bytes (and last) of each association's node ID.
            for(uint8_t i = 0; i < nn; ++i)
                {
                uint8_t nodeID[OpenTRV_Node_ID_Bytes];
                getNodeAssociation(i, nodeID);
                Serial.print(nodeID[0], HEX);
                Serial.print(' ');
                Serial.print(nodeID[1], HEX);
                Serial.print(F(" ... "));
                Serial.print(nodeID[OpenTRV_Node_ID_Bytes - 1], HEX);
                Serial.println();
                }
            // If byte is provided in token after '?' then show index from looking up that prefix from 0.
            char *tok2 = strtok_r(NULL, " ", &last);
            if(NULL != tok2)
                {
                const int p1 = OTV0P2BASE::parseHexByte(tok2);
                if(-1 != p1)
                    {
                    const uint8_t prefix1 = p1;
                    uint8_t nodeID[OpenTRV_Node_ID_Bytes];
                    const int8_t index = getNextMatchingNodeID(0, &prefix1, 1, nodeID);
                    Serial.println(index);
                    }
                }
            }
        else if('*' == *tok1)
            {
            // Clear node IDs.
            OTV0P2BASE::clearAllNodeAssociations();
            Serial.println(F("Cleared"));
            }
        else if(buflen >= (1 + 3*OpenTRV_Node_ID_Bytes)) // 25)
            {
            // corresponds to "A " followed by 8 space-separated hex-byte tokens.
            // Note: As there is no variable for the number of nodes stored, will pass pointer to
            //       addNodeAssociation which will return a value based on how many spaces there are left
            uint8_t nodeID[OpenTRV_Node_ID_Bytes]; // Buffer to store node ID // TODO replace with settable node size constant
            // Loop through tokens setting nodeID.
            // FIXME check for invalid ID bytes (i.e. containing 0xFF or non-HEX)
            char *thisTok = tok1;
            for(uint8_t i = 0; i < sizeof(nodeID); i++, thisTok = strtok_r(NULL, " ", &last))
                {
                // if token is valid, parse hex to binary.
                if(NULL != thisTok)
                    {
                    const int ib = OTV0P2BASE::parseHexByte(thisTok);
                    if(-1 == ib) { InvalidIgnored(); return(false); } // ERROR: abrupt exit.
                    nodeID[i] = (uint8_t) ib;
                    }
                }
            // Try to save this association to EEPROM, reporting result.
            const int8_t index = OTV0P2BASE::addNodeAssociation(nodeID);
            if(index >= 0)
                {
                Serial.print(F("Index "));
                Serial.println(index);
                }
            else
                { InvalidIgnored(); } // Full.
            }
        else { InvalidIgnored(); } // Indicate bad args.
        }
    else { InvalidIgnored(); } // Indicate bad args.
    return(false); // Don't print stats: may have done a lot of work...
    }

// Dump (human-friendly) stats (eg "D N").
// DEBUG only: "D?" to force partial stats sample and "D!" to force an immediate full stats sample; use with care.
// Avoid showing status afterwards as may already be rather a lot of output.
bool DumpStats::doCommand(char *const buf, const uint8_t buflen)
    {
#if 0 && defined(DEBUG)
    if(buflen == 2) // Sneaky way of forcing stats samples.
      {
      if('?' == buf[1]) { sampleStats(false); Serial.println(F("Part sample")); }
      else if('!' == buf[1]) { sampleStats(true); Serial.println(F("Full sample")); }
      break;
      }
#endif
    char *last; // Used by strtok_r().
    char *tok1;
    // Minimum 3 character sequence makes sense and is safe to tokenise, eg "D 0".
    if((buflen >= 3) && (NULL != (tok1 = strtok_r(buf+2, " ", &last))))
      {
      const uint8_t setN = (uint8_t) atoi(tok1);
      const uint8_t thisHH = OTV0P2BASE::getHoursLT();
//          const uint8_t lastHH = (thisHH > 0) ? (thisHH-1) : 23;
      // Print label.
      switch(setN)
        {
        default: { Serial.print('?'); break; }
        case V0P2BASE_EE_STATS_SET_TEMP_BY_HOUR:
        case V0P2BASE_EE_STATS_SET_TEMP_BY_HOUR_SMOOTHED:
            { Serial.print('C'); break; }
        case V0P2BASE_EE_STATS_SET_AMBLIGHT_BY_HOUR:
        case V0P2BASE_EE_STATS_SET_AMBLIGHT_BY_HOUR_SMOOTHED:
            { Serial.print(F("ambl")); break; }
        case V0P2BASE_EE_STATS_SET_OCCPC_BY_HOUR:
        case V0P2BASE_EE_STATS_SET_OCCPC_BY_HOUR_SMOOTHED:
            { Serial.print(F("occ%")); break; }
        case V0P2BASE_EE_STATS_SET_RHPC_BY_HOUR:
        case V0P2BASE_EE_STATS_SET_RHPC_BY_HOUR_SMOOTHED:
            { Serial.print(F("RH%")); break; }
        case V0P2BASE_EE_STATS_SET_USER1_BY_HOUR:
        case V0P2BASE_EE_STATS_SET_USER1_BY_HOUR_SMOOTHED:
            { Serial.print('u'); break; }
#if defined(V0P2BASE_EE_STATS_SET_WARMMODE_BY_HOUR_OF_WK)
        case V0P2BASE_EE_STATS_SET_WARMMODE_BY_HOUR_OF_WK:
            { Serial.print('W'); break; }
#endif
        }
      Serial.print(' ');
      if(setN & 1) { Serial.print(F("smoothed")); } else { Serial.print(F("last")); }
      Serial.print(' ');
      // Now print values.
      for(uint8_t hh = 0; hh < 24; ++hh)
        {
        const uint8_t statRaw = OTV0P2BASE::getByHourStat(setN, hh);
        // For unset stat show '-'...
        if(OTV0P2BASE::STATS_UNSET_BYTE == statRaw) { Serial.print('-'); }
        // ...else print more human-friendly version of stat.
        else switch(setN)
          {
          default: { Serial.print(statRaw); break; } // Generic decimal stats.

          // Special formatting cases.
          case V0P2BASE_EE_STATS_SET_TEMP_BY_HOUR:
          case V0P2BASE_EE_STATS_SET_TEMP_BY_HOUR_SMOOTHED:
            // Uncompanded temperature, rounded.
            { Serial.print((expandTempC16(statRaw)+8) >> 4); break; }
#if defined(V0P2BASE_EE_STATS_SET_WARMMODE_BY_HOUR_OF_WK)
          case V0P2BASE_EE_STATS_SET_WARMMODE_BY_HOUR_OF_WK:
            // Warm mode usage bitmap by hour over week.
            { Serial.print(statRaw, HEX); break; }
#endif
          }
#if 0 && defined(DEBUG)
        // Show how many values are lower than the current one.
        Serial.print('(');
        Serial.print(OTV0P2BASE::countStatSamplesBelow(setN, statRaw));
        Serial.print(')');
#endif
        if(hh == thisHH) { Serial.print('<'); } // Highlight current stat in this set.
#if 0 && defined(DEBUG)
        if(inOutlierQuartile(false, setN, hh)) { Serial.print('B'); } // In bottom quartile.
        if(inOutlierQuartile(true, setN, hh)) { Serial.print('T'); } // In top quartile.
#endif
        Serial.print(' ');
        }
      Serial.println();
      }
    return(false);
    }

// Show / reset node ID ('I').
bool NodeID::doCommand(char *const buf, const uint8_t buflen)
    {
    if((3 == buflen) && ('*' == buf[2]))
      { OTV0P2BASE::ensureIDCreated(true); } // Force ID change.
    Serial.print(F("ID:"));
    for(uint8_t i = 0; i < V0P2BASE_EE_LEN_ID; ++i)
      {
      Serial.print(' ');
      Serial.print(eeprom_read_byte((uint8_t *)(V0P2BASE_EE_START_ID + i)), HEX);
      }
    Serial.println();
    return(true);
    }

// Set secret key ("K ...").
// "K B XX .. XX"  sets the primary building key, "K B *" erases it.
bool SetSecretKey::doCommand(char *const buf, const uint8_t buflen)
    {
    char *last; // Used by strtok_r().
    char *tok1;
    // Minimum 5 character sequence makes sense and is safe to tokenise, eg "K B *".
    if((buflen >= 5) && (NULL != (tok1 = strtok_r(buf + 2, " ", &last))))
        {
        if ('B' == *tok1)
            { // Check first token is a 'B'
            char *tok2 = strtok_r(NULL, " ", &last);
            // if second token is '*' clears eeprom. Otherwise, test to see if
            // a valid key has been entered
            if (NULL != tok2)
                {
                if (*tok2 == '*')
                    {
                    OTV0P2BASE::setPrimaryBuilding16ByteSecretKey (NULL);
                    Serial.println(F("Building Key cleared"));
#if 0 && defined(DEBUG)
                    uint8_t keyTest[16];
                    OTV0P2BASE::getPrimaryBuilding16ByteSecretKey(keyTest);
                    for(uint8_t i = 0; i < sizeof(keyTest); i++)
                        {
                        Serial.print(keyTest[i], HEX);
                        Serial.print(" ");
                        }
                    Serial.println();
#endif
                    }
                else if (buflen >= 3 + 3*16)
                    { // "K B" + 16x " hh" tokens.
                    // 0 array to store new key
                    uint8_t newKey[OTV0P2BASE::VOP2BASE_EE_LEN_16BYTE_PRIMARY_BUILDING_KEY];
                    uint8_t *eepromPtr =
                            (uint8_t *) OTV0P2BASE::VOP2BASE_EE_START_16BYTE_PRIMARY_BUILDING_KEY;
                    // parse and set first token, which has already been recovered
                    newKey[0] = OTV0P2BASE::parseHexByte(tok2);
                    // loop through rest of secret key
                    for (uint8_t i = 1; i < OTV0P2BASE::VOP2BASE_EE_LEN_16BYTE_PRIMARY_BUILDING_KEY; i++)
                        {
                        char *thisTok = strtok_r(NULL, " ", &last);
                        const int ib = OTV0P2BASE::parseHexByte(thisTok);
                        if(-1 == ib) { InvalidIgnored(); return(false); } // ERROR: abrupt exit.
                        newKey[i] = (uint8_t)ib;
                        }
                    OTV0P2BASE::setPrimaryBuilding16ByteSecretKey(newKey);
#if 0 && defined(DEBUG)
                    Serial.println(F("Building Key set"));
#endif

#if 0 && defined(DEBUG)
                    uint8_t keyTest[16];
                    OTV0P2BASE::getPrimaryBuilding16ByteSecretKey(keyTest);
                    for(uint8_t i = 0; i < sizeof(keyTest); i++)
                        {
                        Serial.print(keyTest[i], HEX);
                        Serial.print(" ");
                        }
                    Serial.println();
#endif

                    }
                else { InvalidIgnored(); }
                }
            }
        }
    else { InvalidIgnored(); }
    return(false);
    }

// Set local time (eg "T HH MM").
bool SetTime::doCommand(char *const buf, const uint8_t buflen)
    {
    char *last; // Used by strtok_r().
    char *tok1;
    // Minimum 5 character sequence makes sense and is safe to tokenise, eg "T 1 2".
    if((buflen >= 5) && (NULL != (tok1 = strtok_r(buf+2, " ", &last))))
      {
      char *tok2 = strtok_r(NULL, " ", &last);
      if(NULL != tok2)
        {
        const int hh = atoi(tok1);
        const int mm = atoi(tok2);
        // TODO: zap collected stats if time change too large (eg >> 1h).
        if(!OTV0P2BASE::setHoursMinutesLT(hh, mm)) { OTV0P2BASE::CLI::InvalidIgnored(); }
        }
      }
    return(true);
    }

// Set TX privacy level ("X NN").
// Lower means less privacy: 0 means send everything, 255 means send as little as possible.
bool SetTXPrivacy::doCommand(char *const buf, const uint8_t buflen)
    {
    char *last; // Used by strtok_r().
    char *tok1;
    // Minimum 3 character sequence makes sense and is safe to tokenise, eg "X 0".
    if((buflen >= 3) && (NULL != (tok1 = strtok_r(buf+2, " ", &last))))
      {
      const uint8_t nn = (uint8_t) atoi(tok1);
      OTV0P2BASE::eeprom_smart_update_byte((uint8_t *)V0P2BASE_EE_START_STATS_TX_ENABLE, nn);
      }
    return(true);
    }

// Zap/erase learned statistics ('Z').
// Avoid showing status afterwards as may already be rather a lot of output.
bool ZapStats::doCommand(char *const buf, const uint8_t buflen)
    {
    // Try to avoid causing an overrun if near the end of the minor cycle (even allowing for the warning message if unfinished!).
    if(OTV0P2BASE::zapStats((uint16_t) OTV0P2BASE::fnmax(1, ((int)OTV0P2BASE::msRemainingThisBasicCycle()/2) - 20)))
      { Serial.println(F("Zapped.")); }
    else
      { Serial.println(F("Not finished.")); }
    return(false); // May be slow; avoid showing stats line which will in any case be unchanged.
    }


} }