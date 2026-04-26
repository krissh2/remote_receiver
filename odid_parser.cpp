// ============================================================================
// odid_parser.cpp — TĪRA RAŽOŠANAS VERSIJA
// ============================================================================
// OpenDroneID payload parsēšana. Atbalsta:
//   - Atsevišķus ziņojumu tipus (BasicID, Location, SelfID, System, OperatorID)
//   - Message Pack (msg_type=15) ar mainīgu ziņojumu skaitu
//
// Message Pack garums tiek validēts dinamiski (3B header + N × single_size),
// nevis salīdzinot ar fiksētu sizeof(struct), jo paketes var saturēt mazāk
// nekā ODID_PACK_MAX_MESSAGES ziņojumu.
// ============================================================================

#include "odid_parser.h"
#include "config.h"
#include <string.h>

void ODIDParser::begin() {
  reset();
}

void ODIDParser::reset() {
  memset(&_uas_data, 0, sizeof(_uas_data));
}

int ODIDParser::parsePayload(const uint8_t *payload, size_t len) {
  if (len < 1) return -1;

  // Pirmais baits: augstie 4 biti = msg_type, zemie 4 = protocol version
  uint8_t msg_type = (payload[0] >> 4) & 0x0F;

  if (msg_type == ODID_MESSAGETYPE_PACKED) {
    // Message Pack header: msg_type+ver(1) + size(1) + count(1) = 3 baiti
    if (len < 3) return -1;
    uint8_t single_size = payload[1];      // parasti 25
    uint8_t msg_count   = payload[2];      // ziņojumu skaits

    if (single_size != ODID_MESSAGE_SIZE) return -1;
    if (msg_count == 0 || msg_count > ODID_PACK_MAX_MESSAGES) return -1;
    if (len < (size_t)(3 + (size_t)single_size * msg_count)) return -1;

    ODID_MessagePack_encoded *pack = (ODID_MessagePack_encoded *)payload;
    if (decodeMessagePack(&_uas_data, pack) != ODID_SUCCESS) return -1;
    return msg_type;
  }

  // Atsevišķi ziņojumu tipi — katrs ir ODID_MESSAGE_SIZE = 25 baiti
  if (len < ODID_MESSAGE_SIZE) return -1;

  switch (msg_type) {
    case ODID_MESSAGETYPE_BASIC_ID: {
      ODID_BasicID_encoded *e = (ODID_BasicID_encoded *)payload;
      if (decodeBasicIDMessage(&_uas_data.BasicID[0], e) == ODID_SUCCESS) {
        _uas_data.BasicIDValid[0] = 1;
        return msg_type;
      }
      break;
    }
    case ODID_MESSAGETYPE_LOCATION: {
      ODID_Location_encoded *e = (ODID_Location_encoded *)payload;
      if (decodeLocationMessage(&_uas_data.Location, e) == ODID_SUCCESS) {
        _uas_data.LocationValid = 1;
        return msg_type;
      }
      break;
    }
    case ODID_MESSAGETYPE_SELF_ID: {
      ODID_SelfID_encoded *e = (ODID_SelfID_encoded *)payload;
      if (decodeSelfIDMessage(&_uas_data.SelfID, e) == ODID_SUCCESS) {
        _uas_data.SelfIDValid = 1;
        return msg_type;
      }
      break;
    }
    case ODID_MESSAGETYPE_SYSTEM: {
      ODID_System_encoded *e = (ODID_System_encoded *)payload;
      if (decodeSystemMessage(&_uas_data.System, e) == ODID_SUCCESS) {
        _uas_data.SystemValid = 1;
        return msg_type;
      }
      break;
    }
    case ODID_MESSAGETYPE_OPERATOR_ID: {
      ODID_OperatorID_encoded *e = (ODID_OperatorID_encoded *)payload;
      if (decodeOperatorIDMessage(&_uas_data.OperatorID, e) == ODID_SUCCESS) {
        _uas_data.OperatorIDValid = 1;
        return msg_type;
      }
      break;
    }
    default:
      break;
  }
  return -1;
}
