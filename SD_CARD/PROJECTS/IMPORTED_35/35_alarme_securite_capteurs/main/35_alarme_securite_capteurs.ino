/*
 * Projet 35 - Systeme d'alarme de securite
 * Combine : HC-SR04 (detection de presence par distance) + WS2812 (indicateur visuel)
 *           + buzzer actif (alerte sonore) + WiFi AP (notification via dashboard web)
 * Cible : ESP32
 *
 * Logique : si un objet/une personne s'approche a moins du seuil configure,
 * le systeme passe en etat ALARME (LED rouge clignotante + buzzer + log
 * horodate consultable via le dashboard web), avec una temporisation
 * d'armement pour eviter les fausses alertes au demarrage.
 *
 * Architecture 100% non bloquante (millis()) : mesure ultrason, animation LED,
 * bip du buzzer et serveur web tournent en parallele.
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WebServer.h>

// ---------- Capteur ultrason ----------
const uint8_t TRIG_PIN = 5;
const uint8_t ECHO_PIN = 18; // via diviseur de tension, voir wiring.md
const unsigned long ECHO_TIMEOUT_US = 30000UL;
const float ALARM_THRESHOLD_CM = 30.0;
const unsigned long MEASURE_INTERVAL_MS = 200;
unsigned long lastMeasureTime = 0;
float lastDistance = -1;

// ---------- LED WS2812 (indicateur d'etat) ----------
#define LED_PIN 15
#define LED_COUNT 1
Adafruit_NeoPixel statusLed(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ---------- Buzzer actif ----------
const uint8_t BUZZER_PIN = 26;

// ---------- Machine a etats de l'alarme ----------
enum AlarmState { ARMING, ARMED_IDLE, ALARM_TRIGGERED };
AlarmState alarmState = ARMING;
unsigned long stateEnterTime = 0;
const unsigned long ARMING_DELAY_MS = 5000; // delai avant armement (le temps de s'eloigner du capteur)

// Clignotement non bloquant pendant l'alarme
unsigned long lastBlinkTime = 0;
const unsigned long BLINK_INTERVAL_MS = 300;
bool blinkOn = false;

// ---------- WiFi + dashboard ----------
const char* AP_SSID = "ESP32_ALARME";
const char* AP_PASSWORD = "alarme1234";
WebServer server(80);
String lastEvent = "Systeme en cours d'armement...";
uint32_t triggerCount = 0;

float measureDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1.0;
  return (duration * 0.0343) / 2.0;
}

void enterState(AlarmState newState, const String& eventMessage) {
  alarmState = newState;
  stateEnterTime = millis();
  lastEvent = eventMessage;
  Serial.println("[35_alarme] " + eventMessage);
}

void updateAlarmLogic() {
  unsigned long now = millis();

  switch (alarmState) {
    case ARMING:
      statusLed.setPixelColor(0, statusLed.Color(40, 40, 0)); // jaune = armement en cours
      statusLed.show();
      if (now - stateEnterTime >= ARMING_DELAY_MS) {
        enterState(ARMED_IDLE, "Systeme arme - surveillance active.");
      }
      break;

    case ARMED_IDLE:
      statusLed.setPixelColor(0, statusLed.Color(0, 40, 0)); // vert = arme, tout va bien
      statusLed.show();
      digitalWrite(BUZZER_PIN, LOW);
      if (lastDistance > 0 && lastDistance < ALARM_THRESHOLD_CM) {
        triggerCount++;
        enterState(ALARM_TRIGGERED, "ALERTE ! Intrusion detectee a " + String(lastDistance, 1) + " cm");
      }
      break;

    case ALARM_TRIGGERED:
      // Clignotement rouge non bloquant + buzzer actif
      if (now - lastBlinkTime >= BLINK_INTERVAL_MS) {
        lastBlinkTime = now;
        blinkOn = !blinkOn;
        statusLed.setPixelColor(0, blinkOn ? statusLed.Color(255, 0, 0) : statusLed.Color(0, 0, 0));
        statusLed.show();
        digitalWrite(BUZZER_PIN, blinkOn ? HIGH : LOW);
      }
      // Reste en alarme tant que l'obstacle est proche ; sinon revient a la surveillance
      if (lastDistance < 0 || lastDistance >= ALARM_THRESHOLD_CM) {
        digitalWrite(BUZZER_PIN, LOW);
        enterState(ARMED_IDLE, "Fin d'alerte - zone degagee, reprise de la surveillance.");
      }
      break;
  }
}

// ---------- Serveur web ----------
String buildDashboard() {
  String etatTxt;
  switch (alarmState) {
    case ARMING: etatTxt = "ARMEMENT EN COURS"; break;
    case ARMED_IDLE: etatTxt = "ARME - SURVEILLANCE"; break;
    case ALARM_TRIGGERED: etatTxt = "ALARME DECLENCHEE"; break;
  }

  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='2'>"; // auto-refresh dashboard toutes les 2s
  html += "<title>Alarme de securite</title></head><body>";
  html += "<h1>Systeme d'alarme</h1>";
  html += "<p>Etat : <strong>" + etatTxt + "</strong></p>";
  html += "<p>Distance mesuree : " + (lastDistance >= 0 ? String(lastDistance, 1) + " cm" : String("hors de portee")) + "</p>";
  html += "<p>Dernier evenement : " + lastEvent + "</p>";
  html += "<p>Nombre de declenchements depuis demarrage : " + String(triggerCount) + "</p>";
  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", buildDashboard());
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  statusLed.begin();
  statusLed.setBrightness(80);
  statusLed.show();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("[35_alarme] Dashboard disponible sur : http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();

  enterState(ARMING, "Demarrage - armement dans 5 secondes, eloignez-vous du capteur.");
}

void loop() {
  unsigned long now = millis();

  if (now - lastMeasureTime >= MEASURE_INTERVAL_MS) {
    lastMeasureTime = now;
    lastDistance = measureDistanceCm();
  }

  updateAlarmLogic();
  server.handleClient();
}
