#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "BOX5";
const char* contraseña = "pintacasa(tuco)";

AsyncWebServer gui_esp32(80);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, contraseña);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  Serial.println("");
  Serial.print("Conectado a Wifi, puerto: ");
  Serial.println(WiFi.localIP());
  gui_esp32.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate("hola", "hola1")) {
      return request->requestAuthentication();
    }
    request->send(
      200,
      "text/html",
      "<h1>hola mundo</h1>");
  });
  gui_esp32.begin();
  Serial.println("web server started");
}

void loop() {
}
