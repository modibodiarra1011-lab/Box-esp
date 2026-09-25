/*
 * Projet 32 - Domotique : controle d'acces RFID + Servo (serrure) + WiFi (dashboard)
 * Combine : RC522 (SPI) + SG90 (PWM) + WiFi AP + serveur web
 * Cible : ESP32
 *
 * Bus SPI partage (VSPI : SCK=18, MISO=19, MOSI=23) - Chip Select dedie :
 *   RC522 : CS=GPIO5, RST=GPIO27
 * Le servo utilise un GPIO PWM independant (GPIO16), sans conflit avec le bus SPI.
 *
 * Architecture non bloquante : scan RFID, mouvement du servo (non bloquant via
 * machine a etats + millis) et serveur web tournent en parallele dans loop().
 */

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ---------- RFID (SPI) ----------
#define RFID_CS_PIN  5
#define RFID_RST_PIN 27
MFRC522 mfrc522(RFID_CS_PIN, RFID_RST_PIN);

// UID(s) autorisee(s) - a adapter avec les UID reels de vos badges
// (utilisez d'abord le mode "log" ci-dessous pour identifier vos badges)
byte authorizedUid1[4] = {0xDE, 0xAD, 0xBE, 0xEF}; // exemple, a remplacer

// ---------- Servo (verrou) ----------
Servo lockServo;
const uint8_t SERVO_PIN = 16;
const int SERVO_LOCKED_ANGLE = 0;
const int SERVO_UNLOCKED_ANGLE = 90;

enum LockState { LOCKED, UNLOCKING, UNLOCKED, LOCKING };
LockState lockState = LOCKED;
unsigned long lockStateChangeTime = 0;
const unsigned long UNLOCK_DURATION_MS = 4000; // temps pendant lequel la porte reste ouverte

// ---------- WiFi + dashboard ----------
const char* AP_SSID = "ESP32_DOMOTIQUE";
const char* AP_PASSWORD = "domotique123";
WebServer server(80);
String lastEventLog = "Aucun evenement pour le moment.";

// ---------- Cadencement ----------
const unsigned long RFID_POLL_INTERVAL_MS = 200;
unsigned long lastRfidPoll = 0;

bool uidMatches(MFRC522::Uid uid, byte* authorized, byte len) {
  if (uid.size != len) return false;
  for (byte i = 0; i < len; i++) {
    if (uid.uidByte[i] != authorized[i]) return false;
  }
  return true;
}

String uidToString(MFRC522::Uid uid) {
  String s = "";
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) s += "0";
    s += String(uid.uidByte[i], HEX);
    if (i < uid.size - 1) s += ":";
  }
  s.toUpperCase();
  return s;
}

void requestUnlock(const String& reason) {
  if (lockState == LOCKED) {
    lockState = UNLOCKING;
    lockStateChangeTime = millis();
    lastEventLog = reason + " -> Deverrouillage.";
    Serial.println("[32_domotique] " + lastEventLog);
  }
}

void updateLockStateMachine() {
  unsigned long now = millis();
  switch (lockState) {
    case UNLOCKING:
      lockServo.write(SERVO_UNLOCKED_ANGLE);
      lockState = UNLOCKED;
      lockStateChangeTime = now;
      break;
    case UNLOCKED:
      if (now - lockStateChangeTime >= UNLOCK_DURATION_MS) {
        lockState = LOCKING;
        lockStateChangeTime = now;
      }
      break;
    case LOCKING:
      lockServo.write(SERVO_LOCKED_ANGLE);
      lockState = LOCKED;
      lastEventLog = "Reverrouillage automatique effectue.";
      Serial.println("[32_domotique] " + lastEventLog);
      break;
    case LOCKED:
    default:
      break; // rien a faire, en attente d'un badge valide
  }
}

void pollRfid() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String uidStr = uidToString(mfrc522.uid);
  Serial.println("[32_domotique] Badge detecte : " + uidStr);

  if (uidMatches(mfrc522.uid, authorizedUid1, 4)) {
    requestUnlock("Badge autorise (" + uidStr + ")");
  } else {
    lastEventLog = "Badge REFUSE (" + uidStr + ")";
    Serial.println("[32_domotique] " + lastEventLog);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// ---------- Serveur web ----------
String buildDashboard() {
  String etat = (lockState == LOCKED) ? "VERROUILLE" : "DEVERROUILLE";
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Domotique - Controle d'acces</title></head><body>";
  html += "<h1>Controle d'acces RFID</h1>";
  html += "<p>Etat de la serrure : <strong>" + etat + "</strong></p>";
  html += "<p>Dernier evenement : " + lastEventLog + "</p>";
  html += "<p><a href='/unlock'><button>Deverrouiller manuellement</button></a></p>";
  html += "<p><a href='/'><button>Rafraichir</button></a></p>";
  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", buildDashboard());
}

void handleManualUnlock() {
  requestUnlock("Deverrouillage manuel via dashboard");
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // --- SPI + RFID ---
  SPI.begin(18, 19, 23, RFID_CS_PIN);
  mfrc522.PCD_Init();
  Serial.println("[32_domotique] Lecteur RFID initialise.");

  // --- Servo ---
  ESP32PWM::allocateTimer(0);
  lockServo.setPeriodHertz(50);
  lockServo.attach(SERVO_PIN, 500, 2400);
  lockServo.write(SERVO_LOCKED_ANGLE);

  // --- WiFi AP + serveur web ---
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("[32_domotique] Dashboard disponible sur : http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/unlock", handleManualUnlock);
  server.begin();

  Serial.println("[32_domotique] Systeme pret - approchez un badge RFID.");
}

void loop() {
  unsigned long now = millis();

  if (now - lastRfidPoll >= RFID_POLL_INTERVAL_MS) {
    lastRfidPoll = now;
    pollRfid();
  }

  updateLockStateMachine(); // machine a etats non bloquante pour le servo
  server.handleClient();    // serveur web non bloquant
}
