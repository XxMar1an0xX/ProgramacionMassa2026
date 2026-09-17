#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "hola";
const char* contraseña = "hola1234";

AsyncWebServer gui_esp32(80);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, contraseña);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.println(".");
  // }
  Serial.println("");
  Serial.print("Conectado a Wifi, puerto: ");
  Serial.println(WiFi.softAPIP());
  gui_esp32.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(
      200,
      "text/html",
      R"rawliteral(
      <html>
        <body>
          <h2>Enter a message</h2>

        <form action="/submit" method="POST">
          <input type="text" name="message">
          <input type="submit" value="Send">
        </form>
        </body>
      </html>
    )rawliteral");
  });
  gui_esp32.on("/submit", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("message", true)) {

      String message = request->getParam("message", true)->value();

      Serial.print("Received: ");
      Serial.println(message);
    }
    // request->send(200, "text/html",
    //               "<h1>Message received!</h1>");
    // delay(500);
    request->redirect("/");
  });
  gui_esp32.begin();
  Serial.println("web server started");
}

void loop() {
}
