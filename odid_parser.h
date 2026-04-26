// ============================================================================
// odid_parser.h — opendroneid-core-c bibliotēkas wrapper
// ============================================================================
// Pieņem raw ODID payload un atjaunina iekšējo UAS_Data struktūru.
// Izmanto esošo opendroneid-core-c bibliotēku (jāinstalē Arduino IDE).
// ============================================================================

#ifndef ODID_PARSER_H
#define ODID_PARSER_H

#include <Arduino.h>
#include "opendroneid.h"

class ODIDParser {
public:
  void begin();

  // Parse raw ODID payload. Atgriež ziņojuma tipu, kas tika atjaunināts.
  // Ja tas ir Message Pack (tipi=15), iekšēji atjaunina vairākus laukus.
  // Atgriež -1, ja parsēšana neizdevās.
  int parsePayload(const uint8_t *payload, size_t len);

  // Piekļuve iekšējiem datiem
  const ODID_UAS_Data& getData() const { return _uas_data; }
  bool hasBasicID() const  { return _uas_data.BasicIDValid[0]; }
  bool hasLocation() const { return _uas_data.LocationValid; }
  bool hasSystem() const   { return _uas_data.SystemValid; }
  bool hasOperatorID() const { return _uas_data.OperatorIDValid; }

  // Notīrīt datus (starp dažādiem droniem pēc MAC maiņas)
  void reset();

private:
  ODID_UAS_Data _uas_data = {};
};

#endif // ODID_PARSER_H
