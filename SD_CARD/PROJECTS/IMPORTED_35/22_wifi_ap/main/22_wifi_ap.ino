/*
 * Projet 22 - Point d'acces WiFi (Access Point) avec serveur web embarque
 * Cible : ESP32
 * But : creer un reseau WiFi autonome et servir une page web de controle
 * (ex: allumer/eteindre une LED) sans dependre d'un routeur externe.
 */

#include <WiFi.h>
#include <WebServer.h>

const char* AP_SSID = "ESP32_LAB_AP";
const char* AP_PASSWORD = "esp32lab123"; // >= 8 caracteres obligatoire en WPA2

const uint8_t LED_PIN = LED_BUILTIN;
WebServer server(80);
bool ledState = false;

String buildPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 LAB - Point d'acces</title></head><body>";
  html += "<h1>ESP32 Access Point</h1>";
  html += "<p>Etat LED : <strong>" + String(ledState ? "ALLUMEE" : "ETEINTE") + "</strong></p>";
  html += "<p><a href='/on'><button>Allumer</button></a> ";
  html += "<a href='/off'><button>Eteindre</button></a></p>";
  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", buildPage());
}

void handleOn() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleOff() {
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleNotFound() {
  server.send(404, "text/plain", "Page non trouvee");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_AP);
  bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
  if (apStarted) {
    Serial.printf("[22_wifi_ap] Point d'acces demarre : SSID=%s\n", AP_SSID);
    Serial.print("[22_wifi_ap] Adresse IP de l'AP : ");
    Serial.println(WiFi.softAPIP()); // par defaut 192.168.4.1
  } else {
    Serial.println("[22_wifi_ap] ECHEC du demarrage du point d'acces !");
  }

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[22_wifi_ap] Serveur web demarre sur le port 80");
}

void loop() {
  server.handleClient(); // non bloquant, traite une requete a la fois
}
