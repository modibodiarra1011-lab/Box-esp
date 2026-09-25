#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUDP.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <mbedtls/sha256.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config.h"

Preferences prefs;
WebServer server(80);
WiFiUDP udp;
WiFiUDP logUdp;
uint8_t workerId = DEFAULT_WORKER_ID;
IPAddress masterIP(192, 168, 4, 1);

String state = "BOOT";
String currentJob = "";
String lastResult = "";
String bootId = "";
uint32_t progress = 0;
uint32_t lastHeartbeat = 0;
uint32_t lastDiscovery = 0;
uint32_t lastJob = 0;
uint32_t lastReconnect = 0;
uint32_t bootCount = 0;
int32_t lastRssi = 0;

volatile uint32_t benchmarkSink = 0;

enum ServiceJob : uint8_t {
  SERVICE_NONE = 0,
  SERVICE_SYSTEM_TEST,
  SERVICE_BENCHMARK,
  SERVICE_FS_TEST
};

ServiceJob serviceJobKind = SERVICE_NONE;
bool serviceTurbo = false;
bool cancelRequested = false;
uint8_t servicePhase = 0;
uint32_t servicePhaseAt = 0;
bool cpuOk = false;
bool flashOk = false;
bool wifiOk = false;
bool fsOk = false;
uint32_t benchmarkElapsedUs = 0;
uint32_t benchmarkOpsPerSec = 0;
uint32_t benchmarkDone=0; String resumeType=""; uint8_t resumePhase=0; uint32_t resumeProgress=0; uint32_t resumeBenchmarkDone=0; bool resumePending=false; uint32_t lastCheckpointMs=0; uint32_t lastCheckpointProgress=0;
static void logLine(const String &s);

static String urlDecode(const String &in) {
  String out;
  out.reserve(in.length());
  auto hexValue = [](char x) -> int {
    if (x >= '0' && x <= '9') return x - '0';
    if (x >= 'A' && x <= 'F') return x - 'A' + 10;
    if (x >= 'a' && x <= 'f') return x - 'a' + 10;
    return -1;
  };
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    if (c == '%' && i + 2 < in.length()) {
      int a = hexValue(in[i + 1]);
      int b = hexValue(in[i + 2]);
      if (a >= 0 && b >= 0) {
        out += static_cast<char>((a << 4) | b);
        i += 2;
        continue;
      }
    }
    if (c == '+') c = ' ';
    out += c;
  }
  return out;
}

static void saveApCredentials(const String &ssid, const String &pass) {
  prefs.begin("ap", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}

static void loadApCredentials(String &ssid, String &pass) {
  prefs.begin("ap", true);
  ssid = prefs.getString("ssid", LAB_AP_SSID);
  pass = prefs.getString("pass", LAB_AP_PASSWORD);
  prefs.end();
  if (ssid.isEmpty()) ssid = LAB_AP_SSID;
  if (pass.length() < 8) pass = LAB_AP_PASSWORD;
}

static const char *resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_UNKNOWN: return "Unknown";
    case ESP_RST_POWERON: return "PowerOn";
    case ESP_RST_EXT: return "External";
    case ESP_RST_SW: return "Software";
    case ESP_RST_PANIC: return "Crash";
    case ESP_RST_INT_WDT: return "WDT_Int";
    case ESP_RST_TASK_WDT: return "WDT_Task";
    case ESP_RST_WDT: return "WDT";
    case ESP_RST_DEEPSLEEP: return "DeepSleep";
    case ESP_RST_BROWNOUT: return "Brownout";
    case ESP_RST_SDIO: return "SDIO";
    default: return "Other";
  }
}

static String macString() {
  uint8_t m[6];
  WiFi.macAddress(m);
  char b[18];
  snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  return String(b);
}

static void remember(const String &kind, const String &note) {
  if (!LittleFS.begin(false)) return;
  File f = LittleFS.open("/worker_memory.log", "a");
  if (!f) return;
  f.print(millis());
  f.print("|");
  f.print(kind);
  f.print("|");
  f.println(note);
  f.close();
}

static void clearCheckpoint(){if(LittleFS.begin(false))LittleFS.remove("/resume.cfg");resumeType="";resumePending=false;}
static void saveCheckpoint(){if(!LittleFS.begin(false)||currentJob.isEmpty())return;if(millis()-lastCheckpointMs<CHECKPOINT_INTERVAL_MS&&progress<lastCheckpointProgress+CHECKPOINT_PROGRESS_STEP)return;File f=LittleFS.open("/resume.cfg","w");if(!f)return;f.println(String("type=")+currentJob);f.println(String("phase=")+servicePhase);f.println(String("progress=")+progress);f.println(String("bench=")+benchmarkDone);f.close();lastCheckpointMs=millis();lastCheckpointProgress=progress;}
static bool loadCheckpoint(){if(!LittleFS.begin(false))return false;File f=LittleFS.open("/resume.cfg","r");if(!f)return false;String type="";uint8_t phase=0;uint32_t pct=0,bench=0;while(f.available()){String l=f.readStringUntil('\n');l.trim();int q=l.indexOf('=');if(q<1)continue;String k=l.substring(0,q),v=l.substring(q+1);if(k=="type")type=v;else if(k=="phase")phase=(uint8_t)v.toInt();else if(k=="progress")pct=(uint32_t)v.toInt();else if(k=="bench")bench=(uint32_t)v.toInt();}f.close();if(type!="SYSTEM_TEST"&&type!="BENCHMARK"&&type!="FS_TEST"){LittleFS.remove("/resume.cfg");return false;}resumeType=type;resumePhase=phase;resumeProgress=min(pct,100UL);resumeBenchmarkDone=min(bench,(uint32_t)BENCHMARK_OPERATIONS);resumePending=true;return true;}
static void startResumeIfNeeded(){if(!resumePending||WiFi.status()!=WL_CONNECTED||state=="FLASHING"||serviceJobKind!=SERVICE_NONE)return;String t=resumeType;resumePending=false;servicePhase=resumePhase;servicePhaseAt=millis();progress=resumeProgress;currentJob=t;lastResult="RESUMING";state="TESTING";lastJob=millis();serviceJobKind=t=="BENCHMARK"?SERVICE_BENCHMARK:(t=="FS_TEST"?SERVICE_FS_TEST:SERVICE_SYSTEM_TEST);benchmarkDone=resumeBenchmarkDone;benchmarkElapsedUs=micros();logLine(String("RESUME ")+t);}
}

static void logLine(const String &s) {
  Serial.println(s);
  if (masterIP != IPAddress(0, 0, 0, 0)) {
    String packet = "LOG|W" + String(workerId) + "|" + String(millis()) + "|" + s;
    logUdp.beginPacket(masterIP, LOG_UDP_PORT);
    logUdp.write(reinterpret_cast<const uint8_t *>(packet.c_str()), packet.length());
    logUdp.endPacket();
  }
}

static void connectLabWifi() {
  String ssid, pass;
  loadApCredentials(ssid, pass);
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname((String(WORKER_NAME) + "-" + String(workerId)).c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000UL) delay(100);

  if (WiFi.status() != WL_CONNECTED && (ssid != LAB_AP_SSID || pass != LAB_AP_PASSWORD)) {
    WiFi.disconnect(true);
    delay(100);
    WiFi.begin(LAB_AP_SSID, LAB_AP_PASSWORD);
    start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 6000UL) delay(100);
  }
  lastRssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -127;
}

static void startNetworkSockets() {
  if (WiFi.status() != WL_CONNECTED) return;
  udp.stop();
  logUdp.stop();
  udp.begin(DISCOVERY_PORT);
  logUdp.begin(LOG_UDP_PORT);
  if (MDNS.begin((String("esp32-lab-w") + String(workerId)).c_str())) {
    MDNS.addService("http", "tcp", 80);
  }
}

static String infoJson() {
  lastRssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -127;
  String json = "{";
  json += "\"id\":" + String(workerId);
  json += ",\"name\":\"W" + String(workerId) + "\"";
  json += ",\"hostname\":\"esp32-lab-w" + String(workerId) + ".local\"";
  json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += ",\"mac\":\"" + macString() + "\"";
  json += ",\"version\":\"" + String(LAB_VERSION) + "\"";
  json += ",\"protocol\":" + String(WORKER_PROTOCOL_VERSION);
  json += ",\"state\":\"" + state + "\"";
  json += ",\"job\":\"" + currentJob + "\"";
  json += ",\"progress\":" + String(progress);
  json += ",\"uptime_ms\":" + String(millis());
  json += ",\"free_heap\":" + String(ESP.getFreeHeap());
  json += ",\"heap_total\":" + String(ESP.getHeapSize());
  json += ",\"heap_min\":" + String(ESP.getMinFreeHeap());
  json += ",\"max_alloc\":" + String(ESP.getMaxAllocHeap());
  json += ",\"psram\":" + String(ESP.getPsramSize());
  json += ",\"psram_free\":" + String(ESP.getFreePsram());
  json += ",\"cpu_mhz\":" + String(ESP.getCpuFreqMHz());
  json += ",\"cores\":" + String(ESP.getChipCores());
  json += ",\"chip\":\"" + String(ESP.getChipModel()) + "\"";
  json += ",\"chip_revision\":" + String(ESP.getChipRevision());
  json += ",\"flash_size\":" + String(ESP.getFlashChipSize());
  json += ",\"flash_speed\":" + String(ESP.getFlashChipSpeed());
  json += ",\"sketch_size\":" + String(ESP.getSketchSize());
  json += ",\"sketch_free\":" + String(ESP.getFreeSketchSpace());
  json += ",\"rssi\":" + String(lastRssi);
  json += ",\"boot_count\":" + String(bootCount);
  json += ",\"reset_reason\":\"" + String(resetReasonName(esp_reset_reason())) + "\"";
  json += ",\"boot_id\":\"" + bootId + "\"";
  json += ",\"last_result\":\"" + lastResult + "\"";
  json += ",\"stack_free\":" + String(uxTaskGetStackHighWaterMark(NULL));
  json += ",\"sdk\":\"" + String(ESP.getSdkVersion()) + "\"";
  json += ",\"core_version\":\"" + String(ESP.getCoreVersion()) + "\"";
  json += ",\"fs_total\":" + String(LittleFS.totalBytes());
  json += ",\"fs_used\":" + String(LittleFS.usedBytes());
  json += "}";
  return json;
}

static void sendDiscovery() {
  String packet = "HELLO|" + macString() + "|" + String(LAB_VERSION) + "|" + WiFi.localIP().toString();
  udp.beginPacket(IPAddress(255, 255, 255, 255), DISCOVERY_PORT);
  udp.write(reinterpret_cast<const uint8_t *>(packet.c_str()), packet.length());
  udp.endPacket();
}

static void sendHeartbeat() {
  lastRssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -127;
  String packet = "HB|" + String(workerId) + "|" + macString() + "|" + state + "|" + String(progress) + "|" +
                  String(millis()) + "|" + String(ESP.getFreeHeap()) + "|" + WiFi.localIP().toString() + "|" + currentJob +
                  "|" + String(lastRssi) + "|" + String(ESP.getCpuFreqMHz()) + "|" + String(ESP.getMinFreeHeap()) +
                  "|" + String(ESP.getFlashChipSize()) + "|" + String(ESP.getChipCores()) + "|" + String(ESP.getPsramSize());
  logUdp.beginPacket(masterIP, DISCOVERY_PORT);
  logUdp.write(reinterpret_cast<const uint8_t *>(packet.c_str()), packet.length());
  logUdp.endPacket();
}

static void cancelServiceJob() {
  cancelRequested = false;
  serviceJobKind = SERVICE_NONE;
  servicePhase = 0;
  progress = 0;
  clearCheckpoint();
  lastResult = "CANCELLED";
  currentJob = "";
  state = "READY";
  remember("JOB", "cancelled");
  logLine("JOB CANCELLED");
}

static void finishServiceJob(bool ok, const String &result) {
  serviceJobKind = SERVICE_NONE;
  servicePhase = 0;
  progress = ok ? 100 : 0;
  clearCheckpoint();
  lastResult = result;
  currentJob = "";
  state = ok ? "READY" : "ERROR";
  remember(ok ? "TEST" : "ERROR", result);
  logLine(result);
}

static bool runFsSelfTest(String &detail) {
  if (!LittleFS.begin(false)) {
    detail = "FS=FAIL_MOUNT";
    return false;
  }
  const char *path = "/selftest.bin";
  File f = LittleFS.open(path, "w");
  if (!f) {
    detail = "FS=FAIL_WRITE_OPEN";
    return false;
  }
  uint8_t block[128];
  for (size_t i = 0; i < sizeof(block); ++i) block[i] = static_cast<uint8_t>(i ^ 0xA5U);
  size_t written = 0;
  while (written < FS_TEST_BYTES) {
    size_t n = min(sizeof(block), static_cast<size_t>(FS_TEST_BYTES - written));
    if (f.write(block, n) != n) {
      f.close();
      LittleFS.remove(path);
      detail = "FS=FAIL_WRITE";
      return false;
    }
    written += n;
  }
  f.close();

  f = LittleFS.open(path, "r");
  if (!f) {
    LittleFS.remove(path);
    detail = "FS=FAIL_READ_OPEN";
    return false;
  }
  uint8_t readback[128];
  size_t readTotal = 0;
  bool valid = true;
  while (readTotal < FS_TEST_BYTES) {
    size_t n = min(sizeof(readback), static_cast<size_t>(FS_TEST_BYTES - readTotal));
    size_t got = f.read(readback, n);
    if (got != n) {
      valid = false;
      break;
    }
    for (size_t i = 0; i < got; ++i) {
      if (readback[i] != static_cast<uint8_t>(i ^ 0xA5U)) {
        valid = false;
        break;
      }
    }
    if (!valid) break;
    readTotal += got;
  }
  f.close();
  LittleFS.remove(path);
  detail = valid ? "FS=PASS" : "FS=FAIL_VERIFY";
  return valid;
}

static void startServiceJob(const String &type, bool turbo) {
  serviceTurbo = turbo;
  cancelRequested = false;
  clearCheckpoint(); benchmarkDone=0; lastCheckpointMs=0; lastCheckpointProgress=0;
  servicePhase = 0;
  servicePhaseAt = millis();
  progress = 0;
  lastResult = "RUNNING";
  state = "TESTING";
  currentJob = type;
  lastJob = millis();

  if (type == "BENCHMARK") serviceJobKind = SERVICE_BENCHMARK;
  else if (type == "FS_TEST") serviceJobKind = SERVICE_FS_TEST;
  else serviceJobKind = SERVICE_SYSTEM_TEST;
}

static void serviceSystemTest() {
  uint32_t delayMs = serviceTurbo ? 25UL : 120UL;
  if (cancelRequested) {
    cancelServiceJob();
    return;
  }
  if (millis() - servicePhaseAt < delayMs) return;
  servicePhaseAt = millis();

  switch (servicePhase) {
    case 0:
      cpuOk = ESP.getCpuFreqMHz() > 0 && ESP.getChipCores() >= 1;
      progress = 20;
      servicePhase = 1;
      saveCheckpoint();
      break;
    case 1:
      flashOk = ESP.getFlashChipSize() >= (1024UL * 1024UL);
      progress = 40;
      servicePhase = 2;
      saveCheckpoint();
      break;
    case 2:
      wifiOk = WiFi.status() == WL_CONNECTED;
      progress = 60;
      servicePhase = 3;
      saveCheckpoint();
      break;
    case 3:
      fsOk = LittleFS.begin(false);
      progress = 80;
      servicePhase = 4;
      saveCheckpoint();
      break;
    default: {
      bool ok = cpuOk && flashOk && wifiOk && fsOk;
      String result = String("CPU=") + (cpuOk ? "PASS" : "FAIL") +
                      " FLASH=" + (flashOk ? "PASS" : "FAIL") +
                      " WIFI=" + (wifiOk ? "PASS" : "FAIL") +
                      " FS=" + (fsOk ? "PASS" : "FAIL") +
                      " CPU_MHZ=" + String(ESP.getCpuFreqMHz()) +
                      " HEAP=" + String(ESP.getFreeHeap()) +
                      " RSSI=" + String(lastRssi);
      finishServiceJob(ok, result);
      break;
    }
  }
}

static void serviceBenchmark(){if(cancelRequested){cancelServiceJob();return;}if(servicePhase==0){benchmarkElapsedUs=micros();benchmarkSink=0x12345678UL;servicePhase=1;}uint32_t x=benchmarkSink,end=min(benchmarkDone+6000UL,(uint32_t)BENCHMARK_OPERATIONS);for(uint32_t i=benchmarkDone;i<end;++i){x=x*1664525UL+1013904223UL;x^=x>>13;}benchmarkSink=x;benchmarkDone=end;progress=(uint32_t)((uint64_t)benchmarkDone*100ULL/BENCHMARK_OPERATIONS);saveCheckpoint();if(benchmarkDone>=BENCHMARK_OPERATIONS){benchmarkElapsedUs=micros()-benchmarkElapsedUs;benchmarkOpsPerSec=benchmarkElapsedUs?(uint32_t)((uint64_t)BENCHMARK_OPERATIONS*1000000ULL/benchmarkElapsedUs):0;finishServiceJob(benchmarkElapsedUs>0,"BENCHMARK OPS="+String(BENCHMARK_OPERATIONS)+" US="+String(benchmarkElapsedUs)+" OPS_S="+String(benchmarkOpsPerSec));}}
static void serviceFsTest() {
  if (cancelRequested) {
    cancelServiceJob();
    return;
  }
  if (servicePhase == 0) {
    String detail;
    bool ok = runFsSelfTest(detail);
    progress = ok ? 100 : 0;
    finishServiceJob(ok, detail + " FREE_HEAP=" + String(ESP.getFreeHeap()));
  }
}

static void serviceJobs() {
  if (serviceJobKind == SERVICE_NONE) return;
  if (millis() - lastJob > JOB_TIMEOUT_MS) {
    cancelRequested = false;
    serviceJobKind = SERVICE_NONE;
    currentJob = "";
    progress = 0;
    state = "ERROR";
    lastResult = "JOB_TIMEOUT";
    remember("ERROR", "JOB_TIMEOUT");
    logLine("JOB TIMEOUT");
    return;
  }
  if (serviceJobKind == SERVICE_SYSTEM_TEST) serviceSystemTest();
  else if (serviceJobKind == SERVICE_BENCHMARK) serviceBenchmark();
  else if (serviceJobKind == SERVICE_FS_TEST) serviceFsTest();
}

static bool validSha256(const String &value) {
  if (value.length() != 64) return false;
  for (size_t i = 0; i < value.length(); ++i) {
    char c = value[i];
    bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    if (!ok) return false;
  }
  return true;
}

static void flashFromMaster(const String &url, const String &expected) {
  if (!validSha256(expected) || url.length() < 8 || !url.startsWith("http://") && !url.startsWith("https://")) {
    state = "ERROR";
    currentJob = "";
    lastResult = "OTA_INVALID_REQUEST";
    logLine(lastResult);
    return;
  }

  state = "FLASHING";
  currentJob = "OTA";
  progress = 0;
  lastJob = millis();
  logLine("OTA start");
  remember("OTA", "start");

  HTTPClient http;
  if (!http.begin(url)) {
    lastResult = "OTA_BEGIN_FAILED";
    state = "ERROR";
    currentJob = "";
    logLine(lastResult);
    return;
  }
  http.setConnectTimeout(10000);
  http.setTimeout(OTA_HTTP_TIMEOUT_MS);

  int code = http.GET();
  if (code != 200) {
    lastResult = "OTA_HTTP_" + String(code);
    http.end();
    state = "ERROR";
    currentJob = "";
    logLine(lastResult);
    return;
  }

  int total = http.getSize();
  if (total <= 0 || static_cast<uint32_t>(total) > MAX_UPLOAD_BYTES) {
    lastResult = "OTA_BAD_SIZE";
    http.end();
    state = "ERROR";
    currentJob = "";
    logLine(lastResult);
    return;
  }

  if (!Update.begin(static_cast<size_t>(total))) {
    lastResult = "OTA_UPDATE_BEGIN_FAILED";
    http.end();
    state = "ERROR";
    currentJob = "";
    logLine(lastResult);
    return;
  }

  WiFiClient *stream = http.getStreamPtr();
  stream->setTimeout(1000);
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);

  uint8_t buffer[4096];
  size_t written = 0;
  uint32_t lastData = millis();

  while (written < static_cast<size_t>(total)) {
    if (millis() - lastData > OTA_IDLE_TIMEOUT_MS || millis() - lastJob > (OTA_IDLE_TIMEOUT_MS + OTA_HTTP_TIMEOUT_MS)) {
      Update.abort();
      mbedtls_sha256_free(&ctx);
      http.end();
      lastResult = "OTA_TIMEOUT";
      state = "ERROR";
      currentJob = "";
      progress = 0;
      logLine(lastResult);
      return;
    }

    size_t available = stream->available();
    if (available == 0) {
      delay(2);
      continue;
    }

    size_t request = min(available, sizeof(buffer));
    size_t readBytes = stream->readBytes(buffer, request);
    if (readBytes == 0) continue;

    if (Update.write(buffer, readBytes) != readBytes) {
      Update.abort();
      mbedtls_sha256_free(&ctx);
      http.end();
      lastResult = "OTA_WRITE_FAILED";
      state = "ERROR";
      currentJob = "";
      progress = 0;
      logLine(lastResult);
      return;
    }

    mbedtls_sha256_update(&ctx, buffer, readBytes);
    written += readBytes;
    progress = static_cast<uint32_t>((written * 100UL) / static_cast<size_t>(total));
    lastData = millis();
  }

  http.end();

  uint8_t digest[32];
  mbedtls_sha256_finish(&ctx, digest);
  mbedtls_sha256_free(&ctx);
  String got;
  const char *hex = "0123456789abcdef";
  for (size_t i = 0; i < sizeof(digest); ++i) {
    got += hex[digest[i] >> 4];
    got += hex[digest[i] & 0x0F];
  }

  if (!got.equalsIgnoreCase(expected)) {
    Update.abort();
    lastResult = "OTA_SHA_MISMATCH";
    remember("ERROR", lastResult);
    state = "ERROR";
    currentJob = "";
    progress = 0;
    logLine(lastResult);
    return;
  }

  if (!Update.end(true)) {
    lastResult = "OTA_FINALIZE_FAILED";
    state = "ERROR";
    currentJob = "";
    progress = 0;
    logLine(lastResult);
    return;
  }

  lastResult = "OTA_VERIFIED";
  logLine("OTA verified; reboot");
  delay(300);
  ESP.restart();
}

static void handleInfo() {
  server.send(200, "application/json", infoJson());
}

static const char *wifiAuthName(wifi_auth_mode_t t) {
  switch (t) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_EAP";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3";
    default: return "OTHER";
  }
}

static void handleScan() {
  int16_t count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    server.send(200, "application/json", "{\"state\":\"RUNNING\"}");
    return;
  }
  if (server.hasArg("start")) {
    String v = server.arg("start");
    if (v == "1" || v.equalsIgnoreCase("true")) {
      WiFi.scanDelete();
      int16_t started = WiFi.scanNetworks(true, true, false, 250);
      if (started == WIFI_SCAN_RUNNING) {
        server.send(202, "application/json", "{\"state\":\"RUNNING\"}");
        return;
      }
      count = WiFi.scanComplete();
    }
  }
  if (count < 0) {
    WiFi.scanDelete();
    WiFi.scanNetworks(true, true, false, 250);
    server.send(202, "application/json", "{\"state\":\"STARTED\"}");
    return;
  }

  String json = String("{\"state\":\"DONE\",\"count\":") + count + ",\"networks\":[";
  for (int16_t i = 0; i < count; ++i) {
    if (i) json += ",";
    String ssid = WiFi.SSID(i);
    ssid.replace("\\", "\\\\");
    ssid.replace("\"", "\\\"");
    json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
            ",\"channel\":" + String(WiFi.channel(i)) + ",\"auth\":\"" + String(wifiAuthName(WiFi.encryptionType(i))) + "\"}";
  }
  json += "]}";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

static void handleCapabilities() {
  const String caps = String("{\"protocol\":") + WORKER_PROTOCOL_VERSION +
    ",\"version\":\"" + LAB_VERSION +
    "\",\"features\":[\"PING\",\"SYSTEM_TEST\",\"CHECKUP\",\"BENCHMARK\",\"FS_TEST\",\"OTA\",\"HEARTBEAT\",\"MEMORY\",\"RESET_REASON\",\"MDNS\",\"CANCEL\",\"TELEMETRY\",\"WIFI_SCAN\",\"STACK_HEALTH\",\"LITTLEFS_STATS\"]}";
  server.send(200, "application/json", caps);
}

static void handleRoot() {
  static const char page[] PROGMEM = R"HTML(<!doctype html><html lang="fr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#071426"><title>ESP32 LAB Worker</title><style>:root{--bg:#06111e;--p:#0b2034;--l:#19405d;--t:#e9f7ff;--m:#87a7c0;--c:#39ddff;--g:#25e39b;--r:20px}*{box-sizing:border-box}body{margin:0;background:radial-gradient(900px 500px at 50% -200px,#16466d55,transparent),var(--bg);color:var(--t);font:15px system-ui;padding:14px}.wrap{max-width:720px;margin:auto}.hero{padding:18px;border:1px solid var(--l);background:#091b2ce8;backdrop-filter:blur(15px);border-radius:24px;box-shadow:0 15px 55px #0005}.row{display:flex;gap:12px;align-items:center;justify-content:space-between}.mut{color:var(--m);font-size:12px}.g{width:150px;height:150px;border-radius:50%;display:grid;place-items:center;background:conic-gradient(var(--c) calc(var(--p)*1%),#14324c 0);margin:18px auto;box-shadow:0 0 40px #39ddff20}.g:after{content:"";position:absolute;width:120px;height:120px;border-radius:50%;background:#071827;border:1px solid #1f4766}.g b{position:relative;z-index:2;font-size:30px}.cards{display:grid;grid-template-columns:1fr 1fr;gap:10px}.card{background:var(--p);border:1px solid var(--l);border-radius:var(--r);padding:13px}.v{font-size:21px;font-weight:900}.btn{border:1px solid #235578;background:#10304d;color:var(--t);border-radius:14px;padding:11px 13px;font-weight:800;margin:4px 3px 0 0}.ok{color:var(--g)}pre{white-space:pre-wrap;background:#04101b;border:1px solid #15344e;border-radius:14px;padding:12px;overflow:auto}.bar{height:9px;background:#173149;border-radius:999px;overflow:hidden}.bar i{display:block;height:100%;background:linear-gradient(90deg,#4e8dff,var(--c));transition:width .25s}@media(max-width:520px){.cards{grid-template-columns:1fr}}</style></head><body><div class="wrap"><div class="hero"><div class="row"><div><div style="font-size:23px;font-weight:900">ESP32 LAB • WORKER</div><div id="sub" class="mut">connexion...</div></div><div id="st" class="mut">BOOT</div></div><div class="g" id="g" style="--p:0"><b id="gp">0%</b></div><div class="bar"><i id="bi" style="width:0%"></i></div><div style="margin-top:12px"><button class="btn" onclick="cmd('PING',5)">PING</button><button class="btn" onclick="cmd('SYSTEM_TEST',60)">CHECK-UP</button><button class="btn" onclick="cmd('BENCHMARK',70)">BENCHMARK</button><button class="btn" onclick="cmd('FS_TEST',60)">FS TEST</button><button class="btn" onclick="cancelJob()">ANNULER</button><button class="btn" onclick="scan()">RADAR WI-FI</button><button class="btn" onclick="reboot()">REBOOT</button><button class="btn" onclick="location.reload()">REFRESH</button></div></div><div class="cards" style="margin-top:10px"><div class="card"><div class="mut">CPU</div><div id="cpu" class="v">—</div></div><div class="card"><div class="mut">RAM LIBRE</div><div id="heap" class="v">—</div></div><div class="card"><div class="mut">Wi-Fi</div><div id="wifi" class="v">—</div></div><div class="card"><div class="mut">FLASH</div><div id="flash" class="v">—</div></div><div class="card"><div class="mut">UPTIME</div><div id="up" class="v">—</div></div><div class="card"><div class="mut">RESET</div><div id="rr" class="v">—</div></div><div class="card"><div class="mut">STACK LIBRE</div><div id="stack" class="v">—</div></div><div class="card"><div class="mut">LITTLEFS</div><div id="fs" class="v">—</div></div></div><div class="card" style="margin-top:10px"><div class="mut">DERNIER RÉSULTAT</div><pre id="res">—</pre></div><div class="card" style="margin-top:10px"><div class="mut">RADAR WI-FI</div><div id="radar" class="mut">appuie sur RADAR WI-FI</div></div><div class="card" style="margin-top:10px"><div class="mut">TÉLÉMÉTRIE</div><pre id="json">—</pre></div></div><script>const $=i=>document.getElementById(i);async function j(u,o){let r=await fetch(u,o);let t=await r.text();try{return JSON.parse(t)}catch{return{raw:t,status:r.status}}}function draw(x){let p=x.progress||0;$('g').style.setProperty('--p',p);$('gp').textContent=p+'%';$('bi').style.width=p+'%';$('st').textContent=x.state;$('sub').textContent='W'+x.id+' • '+x.ip+' • '+x.version+' • '+(x.hostname||'');$('cpu').textContent=x.cpu_mhz+' MHz × '+x.cores;$('heap').textContent=Math.round((x.free_heap||0)/1024)+' KB';$('wifi').textContent=x.rssi+' dBm';$('flash').textContent=Math.round((x.flash_size||0)/1048576)+' MB';$('up').textContent=Math.round((x.uptime_ms||0)/1000)+' s';$('rr').textContent=x.reset_reason||'—';$('stack').textContent=(x.stack_free??'—')+' words';$('fs').textContent=Math.round((x.fs_used||0)/1024)+' / '+Math.round((x.fs_total||0)/1024)+' KB';$('res').textContent=x.last_result||'—';$('json').textContent=JSON.stringify(x,null,2)}async function refresh(){let x=await j('/api/info');if(x.id!==undefined)draw(x)}async function cmd(t,p){await j('/api/job',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'type='+encodeURIComponent(t)+'&priority='+p});refresh()}async function cancelJob(){await j('/api/cancel',{method:'POST'});refresh()}async function scan(){let x=await j('/api/scan?start=1');if(x.state==='RUNNING'||x.state==='STARTED'){$('radar').textContent='scan en cours…';setTimeout(scan,1100);return}if(x.networks){const esc=s=>String(s??'').replace(/[&<>\"']/g,m=>({'&':'&amp;','<':'&lt;','>':'&gt;','\"':'&quot;',"'":'&#39;'}[m]));$('radar').innerHTML=x.networks.map(n=>`<div style='padding:7px 0;border-bottom:1px solid #17344e'><b>${esc(n.ssid)||'—'}</b> • ${Number(n.rssi)||0} dBm • CH ${Number(n.channel)||0} • ${esc(n.auth)}</div>`).join('')||'aucun réseau';}else{$('radar').textContent=JSON.stringify(x)}}async function reboot(){await j('/api/reboot',{method:'POST'});$('res').textContent='reboot demandé';}refresh();setInterval(refresh,900)</script></body></html>)HTML";
  server.send(200, "text/html; charset=utf-8", page);
}

static void handleJob() {
  if (!server.hasArg("type")) {
    server.send(400, "text/plain", "missing type");
    return;
  }
  if (state == "FLASHING" || state == "TESTING") {
    server.send(409, "text/plain", "busy");
    return;
  }

  String type = server.arg("type");
  type.toUpperCase();
  int priority = server.hasArg("priority") ? server.arg("priority").toInt() : 5;

  if (type == "PING") {
    lastJob = millis();
    state = "READY";
    currentJob = "";
    progress = 100;
    lastResult = "PONG";
    logLine("PONG");
    server.send(200, "text/plain", "pong");
    return;
  }

  if (type != "SYSTEM_TEST" && type != "CHECKUP" && type != "BENCHMARK" && type != "FS_TEST") {
    server.send(400, "text/plain", "unknown job");
    return;
  }

  startServiceJob(type, priority >= 70);
  server.send(202, "text/plain", "accepted");
}

static void handleCancel() {
  if (state == "FLASHING") {
    server.send(409, "text/plain", "OTA cannot be safely interrupted here");
    return;
  }
  if (serviceJobKind == SERVICE_NONE) {
    state = "READY";
    currentJob = "";
    progress = 0;
    lastResult = "NOTHING_TO_CANCEL";
    server.send(200, "text/plain", "idle");
    return;
  }
  cancelRequested = true;
  server.send(202, "text/plain", "cancelling");
}

static void handleFlash() {
  if (!server.hasArg("url") || !server.hasArg("sha256")) {
    server.send(400, "text/plain", "missing url/sha256");
    return;
  }
  if (state != "READY") {
    server.send(409, "text/plain", "busy");
    return;
  }
  server.send(202, "text/plain", "accepted");
  flashFromMaster(server.arg("url"), server.arg("sha256"));
}

static void handleReboot() {
  server.send(202, "text/plain", "rebooting");
  delay(100);
  ESP.restart();
}

static void receiveReply() {
  int n = udp.parsePacket();
  if (n <= 0) return;
  char buffer[400];
  int readLen = udp.read(buffer, sizeof(buffer) - 1);
  if (readLen <= 0) return;
  buffer[readLen] = 0;
  String packet(buffer);

  if (packet.startsWith("ASSIGN|")) {
    int p1 = packet.indexOf('|', 7);
    int p2 = packet.indexOf('|', p1 + 1);
    if (p1 < 0) return;
    workerId = static_cast<uint8_t>(packet.substring(7, p1).toInt());
    if (p2 > 0) masterIP.fromString(packet.substring(p1 + 1, p2));
    else masterIP.fromString(packet.substring(p1 + 1));
    prefs.begin("worker", false);
    prefs.putUChar("id", workerId);
    prefs.end();
    state = "READY";
    startNetworkSockets();
    logLine("Assigned W" + String(workerId) + " master=" + masterIP.toString());
  } else if (packet.startsWith("APCFG|")) {
    int p1 = packet.indexOf('|', 6);
    if (p1 > 6) {
      String ssid = urlDecode(packet.substring(6, p1));
      String pass = urlDecode(packet.substring(p1 + 1));
      if (!ssid.isEmpty() && pass.length() >= 8) {
        saveApCredentials(ssid, pass);
        state = "RECONNECTING";
        logLine("AP credentials saved; reconnecting to MASTER AP");
        WiFi.disconnect(true);
        lastReconnect = 0;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  bootId = String(static_cast<unsigned long>(esp_random()), HEX);
  prefs.begin("worker", false);
  workerId = prefs.getUChar("id", DEFAULT_WORKER_ID);
  bootCount = prefs.getULong("boots", 0) + 1;
  prefs.putULong("boots", bootCount);
  prefs.end();

  LittleFS.begin(true);
  loadCheckpoint();
  remember("BOOT", String("worker online reset=") + resetReasonName(esp_reset_reason()));
  logLine(String("BOOT W") + workerId + " core=" + ESP.getCoreVersion());

  connectLabWifi();
  if (WiFi.status() == WL_CONNECTED) {
    startNetworkSockets();
    startResumeIfNeeded();
    if(serviceJobKind==SERVICE_NONE) state = "DISCOVERING";
    logLine("WiFi " + WiFi.localIP().toString());
    sendDiscovery();
  } else {
    state = "OFFLINE";
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/info", HTTP_GET, handleInfo);
  server.on("/api/capabilities", HTTP_GET, handleCapabilities);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/job", HTTP_POST, handleJob);
  server.on("/api/cancel", HTTP_POST, handleCancel);
  server.on("/api/flash", HTTP_POST, handleFlash);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.begin();
}

void loop() {
  server.handleClient();
  receiveReply();
  startResumeIfNeeded();
  serviceJobs();

  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnect > 15000UL) {
      lastReconnect = millis();
      state = "RECONNECTING";
      connectLabWifi();
      if (WiFi.status() == WL_CONNECTED) {
        startNetworkSockets();
        state = "DISCOVERING";
        sendDiscovery();
      } else {
        state = "OFFLINE";
      }
    }
  } else {
    if (millis() - lastDiscovery > DISCOVERY_INTERVAL_MS) {
      sendDiscovery();
      lastDiscovery = millis();
    }
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS) {
      sendHeartbeat();
      lastHeartbeat = millis();
    }
  }

  delay(2);
}
