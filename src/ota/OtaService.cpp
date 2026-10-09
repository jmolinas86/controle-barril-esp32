#include "ota/OtaService.h"

#include <Arduino.h>
#include <Update.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "diagnostics/Logger.h"

namespace keezer::ota {
namespace {

constexpr char kLogTag[] = "OTA";
constexpr std::uint32_t kRestartDelayMs = 1'500U;
constexpr std::uint32_t kUploadTimeoutMs = 15'000U;

const char kUpdatePage[] PROGMEM = R"HTML(
<!doctype html><html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Keezer OTA</title><style>
*{box-sizing:border-box}body{margin:0;background:#020913;color:#e9f4ff;
font:16px Arial,sans-serif;display:grid;min-height:100vh;place-items:center}
main{width:min(92vw,420px);padding:24px;border:1px solid #087fc0;
border-radius:12px;background:#061526}h1{margin:0 0 8px;color:#18b8ff;font-size:24px}
p{color:#a9bfd0}input,button{width:100%;padding:13px;margin-top:12px;border-radius:7px}
input{border:1px solid #36556c;background:#020913;color:#fff}
button{border:1px solid #18b8ff;background:#064b8d;color:#fff;font-weight:bold}
button:disabled{opacity:.5}progress{width:100%;height:18px;margin-top:18px}
#status{min-height:22px;color:#ffb020}.ok{color:#7bdc00!important}.err{color:#ff5252!important}
</style></head><body><main><h1>KEEZER CONTROLLER</h1>
<p>Atualização local de firmware — Fase 19.1</p>
<form id="f"><input id="bin" name="firmware" type="file" accept=".bin" required>
<button id="send" type="submit">ATUALIZAR FIRMWARE</button></form>
<progress id="bar" value="0" max="100"></progress><p id="status">Pronto.</p>
<script>
const f=document.getElementById('f'),b=document.getElementById('bin'),
s=document.getElementById('status'),bar=document.getElementById('bar'),
btn=document.getElementById('send');
f.onsubmit=e=>{e.preventDefault();if(!b.files.length)return;
const file=b.files[0];if(!file.name.toLowerCase().endsWith('.bin')){
s.className='err';s.textContent='Selecione um arquivo .bin válido.';return}
const data=new FormData();data.append('firmware',file);const x=new XMLHttpRequest();
x.open('POST','/update');x.setRequestHeader('X-Firmware-Size',file.size);
btn.disabled=true;s.className='';s.textContent='Enviando...';
x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(e.loaded*100/e.total);
bar.value=p;s.textContent='Enviando: '+p+'%'}};
x.onload=()=>{if(x.status===200){bar.value=100;s.className='ok';
s.textContent='Atualização concluída. Reiniciando...'}else{s.className='err';
s.textContent=x.responseText||'Falha na atualização.';btn.disabled=false}};
x.onerror=()=>{s.className='err';s.textContent='Conexão interrompida.';btn.disabled=false};
x.send(data)};
</script></main></body></html>
)HTML";

bool hasBinExtension(const String& filename) {
  if (filename.length() < 5U) return false;
  String normalized = filename;
  normalized.toLowerCase();
  return normalized.endsWith(".bin");
}

}  // namespace

bool OtaService::begin(const char* const username, const char* const password,
                       const MaintenanceHandler maintenanceHandler,
                       void* const maintenanceContext) {
  if (username == nullptr || password == nullptr || username[0] == '\0' ||
      password[0] == '\0' || std::strlen(username) >= username_.size() ||
      std::strlen(password) >= password_.size()) {
    KEEZER_LOG_ERROR(kLogTag, "OTA disabled: invalid credentials");
    return false;
  }

  std::snprintf(username_.data(), username_.size(), "%s", username);
  std::snprintf(password_.data(), password_.size(), "%s", password);
  maintenanceHandler_ = maintenanceHandler;
  maintenanceContext_ = maintenanceContext;
  std::snprintf(result_.data(), result_.size(), "%s", "READY");

  const char* collectedHeaders[] = {"X-Firmware-Size"};
  server_.collectHeaders(collectedHeaders, 1U);

  server_.on("/update", HTTP_GET, [this]() { handlePage(); });
  server_.on(
      "/update", HTTP_POST, [this]() { handleUploadFinished(); },
      [this]() { handleUpload(); });
  server_.onNotFound([this]() {
    if (!authenticate(true)) return;
    server_.send(404, "text/plain", "Use /update para atualizar o firmware.");
  });
  server_.begin();
  started_ = true;
  KEEZER_LOG_INFO(kLogTag,
                  "Web OTA ready: port=80 path=/update auth=ENABLED");
  return true;
}

void OtaService::update(const bool networkConnected,
                        const std::uint32_t nowMs) {
  if (!started_) return;
  if (uploadInProgress_ &&
      (!networkConnected || nowMs - lastUploadActivityAtMs_ >=
                                kUploadTimeoutMs)) {
    failUpload(networkConnected ? "UPLOAD_TIMEOUT" : "NETWORK_DISCONNECTED");
  }
  if (networkConnected) {
    server_.handleClient();
    if (!networkWasConnected_) {
      KEEZER_LOG_INFO(kLogTag, "OTA endpoint available at /update");
    }
  }
  networkWasConnected_ = networkConnected;

  if (restartPending_ &&
      static_cast<std::int32_t>(nowMs - restartAtMs_) >= 0) {
    KEEZER_LOG_INFO(kLogTag, "OTA_RESTART");
    delay(50U);
    ESP.restart();
  }
}

bool OtaService::uploadInProgress() const { return uploadInProgress_; }

bool OtaService::restartPending() const { return restartPending_; }

bool OtaService::authenticate(const bool requestChallenge) {
  if (server_.authenticate(username_.data(), password_.data())) return true;
  if (requestChallenge) {
    server_.requestAuthentication(BASIC_AUTH, "Keezer OTA",
                                  "Autenticacao necessaria");
  }
  return false;
}

void OtaService::handlePage() {
  if (!authenticate(true)) return;
  server_.sendHeader("Cache-Control", "no-store");
  server_.send_P(200, "text/html; charset=utf-8", kUpdatePage);
}

void OtaService::handleUpload() {
  HTTPUpload& upload = server_.upload();
  if (upload.status == UPLOAD_FILE_START) {
    uploadAuthenticated_ = authenticate(false);
    uploadSucceeded_ = false;
    restartPending_ = false;
    if (!uploadAuthenticated_) return;
    if (!hasBinExtension(upload.filename)) {
      failUpload("INVALID_FILE_EXTENSION");
      return;
    }
    const String sizeHeader = server_.header("X-Firmware-Size");
    char* end = nullptr;
    const unsigned long parsedSize =
        std::strtoul(sizeHeader.c_str(), &end, 10);
    if (sizeHeader.isEmpty() || end == sizeHeader.c_str() || *end != '\0' ||
        parsedSize == 0UL || parsedSize > ESP.getFreeSketchSpace()) {
      failUpload("INVALID_FIRMWARE_SIZE");
      return;
    }
    expectedFirmwareSize_ = static_cast<std::size_t>(parsedSize);
    if (maintenanceHandler_ != nullptr) {
      maintenanceHandler_(maintenanceContext_);
    }
    uploadInProgress_ = true;
    lastUploadActivityAtMs_ = millis();
    std::snprintf(result_.data(), result_.size(), "%s", "UPLOADING");
    if (!Update.begin(expectedFirmwareSize_, U_FLASH)) {
      failUpload("UPDATE_BEGIN_FAILED");
      return;
    }
    KEEZER_LOG_INFO(kLogTag, "OTA_STARTED file=%s",
                    upload.filename.c_str());
    return;
  }

  if (!uploadAuthenticated_ || !uploadInProgress_) return;
  if (upload.status == UPLOAD_FILE_WRITE) {
    lastUploadActivityAtMs_ = millis();
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      failUpload("UPDATE_WRITE_FAILED");
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (upload.totalSize != expectedFirmwareSize_ || !Update.end(false) ||
        !Update.isFinished()) {
      failUpload("UPDATE_FINALIZE_FAILED");
      return;
    }
    uploadInProgress_ = false;
    uploadSucceeded_ = true;
    std::snprintf(result_.data(), result_.size(), "%s", "SUCCESS");
    KEEZER_LOG_INFO(kLogTag, "OTA_SUCCESS bytes=%lu",
                    static_cast<unsigned long>(upload.totalSize));
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    failUpload("UPLOAD_ABORTED");
  }
}

void OtaService::handleUploadFinished() {
  if (!authenticate(true)) {
    uploadAuthenticated_ = false;
    return;
  }
  server_.sendHeader("Connection", "close");
  if (!uploadSucceeded_) {
    char response[128]{};
    std::snprintf(response, sizeof(response), "OTA falhou: %s (codigo %u)",
                  result_.data(), static_cast<unsigned int>(Update.getError()));
    server_.send(400, "text/plain; charset=utf-8", response);
    uploadAuthenticated_ = false;
    return;
  }
  server_.send(200, "text/plain; charset=utf-8",
               "Firmware atualizado. Reiniciando...");
  uploadAuthenticated_ = false;
  scheduleRestart(millis());
}

void OtaService::failUpload(const char* const reason) {
  if (Update.isRunning()) Update.abort();
  uploadInProgress_ = false;
  uploadSucceeded_ = false;
  expectedFirmwareSize_ = 0U;
  std::snprintf(result_.data(), result_.size(), "%s",
                reason == nullptr ? "UNKNOWN_ERROR" : reason);
  KEEZER_LOG_ERROR(kLogTag, "OTA_FAILED reason=%s code=%u", result_.data(),
                   static_cast<unsigned int>(Update.getError()));
}

void OtaService::scheduleRestart(const std::uint32_t nowMs) {
  restartAtMs_ = nowMs + kRestartDelayMs;
  restartPending_ = true;
}

}  // namespace keezer::ota
