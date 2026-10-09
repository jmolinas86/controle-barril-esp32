#include <cassert>
#include <cstdio>
#include <cstring>

#include "scale/ScaleHttpJson.h"

int main() {
  keezer::models::ScalePacket packet{};
  const char json[] =
      "{\"protocol_version\":1,\"sequence\":1842,"
      "\"scale_id\":\"SCALE_MAIN\",\"weight_grams\":18540,"
      "\"stable\":true,\"nfc_uid\":\"04A23F891C\","
      "\"rssi_dbm\":-58,\"uptime_ms\":928441}";
  assert(keezer::scale::ScaleHttpJson::parseReading(json, packet));
  assert(packet.protocolVersion == 1U);
  assert(packet.sequence == 1842U);
  assert(packet.weightGrams == 18'540);
  assert(packet.stable);
  assert(packet.hasNfcUid);
  assert(std::strcmp(packet.nfcUid.data(), "04A23F891C") == 0);
  assert(packet.rssiDbm == -58);
  assert(packet.senderUptimeMs == 928'441U);

  const char withoutNfc[] =
      "{\"protocol_version\":1,\"sequence\":2,"
      "\"scale_id\":\"SCALE_MAIN\",\"weight_grams\":0,"
      "\"stable\":false,\"nfc_uid\":null,\"rssi_dbm\":-70,"
      "\"uptime_ms\":200}";
  assert(keezer::scale::ScaleHttpJson::parseReading(withoutNfc, packet));
  assert(!packet.hasNfcUid);
  assert(!packet.stable);
  assert(!keezer::scale::ScaleHttpJson::parseReading("{}", packet));
  assert(!keezer::scale::ScaleHttpJson::parseReading(
      "{\"protocol_version\":1,\"sequence\":2,"
      "\"scale_id\":\"SCALE_MAIN\",\"weight_grams\":0,"
      "\"stable\":false,\"nfc_uid\":null,\"rssi_dbm\":999,"
      "\"uptime_ms\":200}",
      packet));
  assert(!keezer::scale::ScaleHttpJson::parseReading(
      "{\"protocol_version\":1,\"sequence\":2,"
      "\"scale_id\":\"SCALE_MAIN\",\"weight_grams\":0,"
      "\"stable\":true_invalid,\"nfc_uid\":null,\"rssi_dbm\":-70,"
      "\"uptime_ms\":200}",
      packet));
  assert(!keezer::scale::ScaleHttpJson::parseReading(
      "{\"protocol_version\":1,\"sequence\":2,"
      "\"scale_id\":\"SCALE_MAIN\",\"weight_grams\":0,"
      "\"stable\":false,\"nfc_uid\":\"UID-MUITO-LONGO-123456789\","
      "\"rssi_dbm\":-70,\"uptime_ms\":200}",
      packet));
  std::puts("ScaleHttpJson smoke test: PASS");
  return 0;
}
