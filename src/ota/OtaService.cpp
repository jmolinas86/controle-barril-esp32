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

const char kLogsPage[] PROGMEM = R"HTML(
<!doctype html><html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Keezer Logs</title><style>
*{box-sizing:border-box}body{margin:0;background:#020913;color:#e9f4ff;
font:14px Arial,sans-serif;padding:12px}main{max-width:1100px;margin:auto}
header{display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-bottom:10px}
h1{margin:0 auto 0 0;color:#18b8ff;font-size:21px}button,a{border:1px solid
#18b8ff;background:#064b8d;color:#fff;padding:9px 12px;border-radius:6px;
font-weight:bold;text-decoration:none}label{color:#a9bfd0}pre{margin:0;height:72vh;
overflow:auto;white-space:pre-wrap;overflow-wrap:anywhere;border:1px solid #087fc0;
border-radius:8px;background:#00070d;color:#bde9ff;padding:12px;
font:12px/1.45 Consolas,monospace}#state{margin:8px 0;color:#7bdc00}
.paused{color:#ffb020!important}.error{color:#ff5252!important}
</style></head><body><main><header><h1>MONITOR DE LOGS</h1>
<button id="pause">PAUSAR</button><button id="clear">LIMPAR</button>
<label><input id="follow" type="checkbox" checked> acompanhar</label>
<a href="/update">OTA</a></header><div id="state">Conectando...</div>
<pre id="output">Carregando logs...</pre><script>
const out=document.getElementById('output'),state=document.getElementById('state'),
pause=document.getElementById('pause'),clear=document.getElementById('clear'),
follow=document.getElementById('follow');let stopped=false,busy=false,revision='';
async function load(){if(stopped||busy)return;busy=true;try{
const r=await fetch('/api/logs?revision='+revision,{cache:'no-store'});
if(r.status===204){state.className='';state.textContent='Online - sem novos logs';return}
if(!r.ok)throw Error(r.status);revision=r.headers.get('X-Log-Revision')||'';
const text=await r.text();out.textContent=text||'(nenhum log no buffer)';
if(follow.checked)out.scrollTop=out.scrollHeight;state.className='';
state.textContent='Online - atualizado '+new Date().toLocaleTimeString();
}catch(e){state.className='error';state.textContent='Falha ao consultar logs: '+e.message}
finally{busy=false}}
pause.onclick=()=>{stopped=!stopped;pause.textContent=stopped?'CONTINUAR':'PAUSAR';
state.className=stopped?'paused':'';if(stopped)state.textContent='Atualização pausada';
else load()};clear.onclick=async()=>{if(!confirm('Limpar os logs em memória?'))return;
await fetch('/api/logs/clear',{method:'POST'});await load()};
setInterval(load,1000);load();
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
  server_.on("/logs", HTTP_GET, [this]() { handleLogsPage(); });
  server_.on("/api/logs", HTTP_GET, [this]() { handleLogsData(); });
  server_.on("/api/logs/clear", HTTP_POST,
             [this]() { handleLogsClear(); });
  server_.onNotFound([this]() {
    if (!authenticate(true)) return;
    server_.send(404, "text/plain",
                 "Use /update para OTA ou /logs para diagnosticos.");
  });
  server_.begin();
  started_ = true;
  KEEZER_LOG_INFO(kLogTag,
                  "Web services ready: port=80 ota=/update logs=/logs auth=ENABLED");
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

void OtaService::handleLogsPage() {
  if (!authenticate(true)) return;
  server_.sendHeader("Cache-Control", "no-store");
  server_.send_P(200, "text/html; charset=utf-8", kLogsPage);
}

void OtaService::handleLogsData() {
  if (!authenticate(true)) return;
  const std::uint32_t revision = diagnostics::Logger::bufferRevision();
  if (server_.hasArg("revision") &&
      server_.arg("revision") == String(revision)) {
    server_.send(204);
    return;
  }
  server_.sendHeader("Cache-Control", "no-store");
  server_.sendHeader("X-Log-Revision", String(revision));
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/plain; charset=utf-8", "");

  const std::size_t lineCount =
      diagnostics::Logger::bufferedLineCount();
  std::array<char, 1'024U> chunk{};
  std::size_t used = 0U;
  for (std::size_t index = 0U; index < lineCount; ++index) {
    if (chunk.size() - used < 193U) {
      server_.sendContent(chunk.data(), used);
      used = 0U;
    }
    const std::size_t length = diagnostics::Logger::copyBufferedLine(
        index, chunk.data() + used, chunk.size() - used);
    if (length == 0U) continue;
    used += length;
    chunk[used++] = '\n';
  }
  if (used > 0U) {
    server_.sendContent(chunk.data(), used);
  }
  server_.sendContent("");
}

void OtaService::handleLogsClear() {
  if (!authenticate(true)) return;
  diagnostics::Logger::clearBuffer();
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "text/plain; charset=utf-8", "Logs limpos.");
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
