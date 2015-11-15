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

/*
 Basic security support.
 */

#ifndef OTV0P2BASE_SECURITY_H
#define OTV0P2BASE_SECURITY_H


namespace OTV0P2BASE
{


// Leaf node privacy level: how much to transmit about stats such as temperature and occupancy.
// The greater numerically the value, the less data is sent, especially over an insecure channel.
// Excess unencrypted stats may, for example, allow a clever burglar to work out when no one is home.
// Note that even in the 'always' setting,
// some TXes may be selectively skipped or censored for energy saving and security reasons
// eg an additional 'never transmit occupancy' flag may be set locally.
// The values correspond to levels and intermediate values not explicitly enumerated are allowed.
// Lower values mean less that security is required.
enum stats_TX_level
  {
  stTXalwaysAll = 0,    // Always be prepared to transmit all stats (zero privacy).
  stTXmostUnsec = 0x80, // Allow TX of all but most security-sensitive stats in plaintext, eg occupancy status.
  stTXsecOnly   = 0xfe, // Only transmit if the stats TX can be kept secure/encrypted.
  stTXnever     = 0xff, // Never transmit status info beyond the minimum necessary.
  };

// Get the current basic stats transmission level (for data outbound from this node).
// May not exactly match enumerated levels; use inequalities.
// Not thread-/ISR- safe.
stats_TX_level getStatsTXLevel();


//#if 0 // Pairing API outline.
//struct pairInfo { bool successfullyPaired; };
//bool startPairing(bool primary, &pairInfo);
//bool continuePairing(bool primary, &pairInfo); // Incremental work.
//void clearPairing(bool primary, &pairInfo);
//#endif


}
#endif