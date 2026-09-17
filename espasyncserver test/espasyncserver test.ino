#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

//NOTE: nombre de la red wifi del esp
const char* ssid = "ladron";


// struct Pagina_config {
//   String color_fondo;
//   String color_boton;
//   int texto_boton;
// };
AsyncWebServer pagina(80);
Preferences variables;

String html = R"rawliteral(
      <!DOCTYPE html>
<html>
<head>
    <title>ESP32 Control</title>

    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f2f2f2;
            padding-top: 50px;
        }

        h1 {
            color: #333;
        }

        .button {
            width: 180px;
            padding: 15px;
            margin: 10px;
            font-size: 18px;
            color: white;
            background-color: #007bff;
            border: none;
            border-radius: 8px;
            cursor: pointer;
        }
          .boton_contraseña {
            width: 380px;
            padding: 15px;
            margin: 30px;
            font-size: 28px;
            color: white;
            background-color: #4D4D4D;
            border: none;
            border-radius: 10px;
            cursor: pointer;
        }


        .button:hover {
            background-color: #0056b3;
        }
    </style>
</head>

<body>

    <h1>ESP32 Control Panel</h1>

    <button class="button" onclick="location.href='/button1'">
        Button 1
    </button>

    <button class="button" onclick="location.href='/button2'">
        Button 2
    </button>

    <button class="button" onclick="location.href='/button3'">
        Button 3
    </button>

    <button class="button" onclick="location.href='/button4'">
        Button 4
    </button>
    <button class="boton_contraseña" onclick="location.href='/actualizar_contraseña'">
        Cambiar contraseña
    </button>

</body>
</html>
    )rawliteral";



void setup() {
  Serial.begin(115200);
  //NOTE: recomiendo desactivar los datos moviles al conectarse al esp
  //NOTE: para entrar es escribir en el buscador:
  // 192.168.1.4
  WiFi.mode(WIFI_AP);
  variables.begin("valores", false);
  // if (!variables.isKey("contraseña")) {
  //   variables.putString("contraseña", "contraseña");
  // }
  //NOTE: recomiendo cambiar la contraseña default puesta
  String contraseña = variables.getString("contraseña", "contraseña");
  variables.end();
  //NOTE: la contraseña tiene que ser minimo de 8 caracteres de largo, sino no anda
  WiFi.softAP(ssid, contraseña.c_str());
  Serial.println(WiFi.softAPIP());

  pagina.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", html);
  });
  pagina.on("/button1", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "boton1 prendido");
  });
  pagina.on("/button2", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "boton2 prendido");
  });
  pagina.on("/button3", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "boton3 prendido");
  });
  pagina.on("/button4", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "boton4 prendido");
  });
  pagina.on("/actualizar_contraseña", HTTP_GET, [](AsyncWebServerRequest* request) {
    // variables.begin("valores", false);
    request->send(200, "text/plain", "contraseña actualizada");
    delay(500);
    // contraseña = variables.getString("contraseña", "contraseña");
    // variables.end();
    // WiFi.softAP(ssid, contraseña.c_str());
  });
  pagina.begin();
}

void loop() {
}
