#include <LittleFS.h>


String texto;
String asd;
void setup() {
  Serial.begin(115200);
  LittleFS.begin(true);
  if (!LittleFS.exists("/hola.txt")) {
    File hola = LittleFS.open("/hola.txt", "w");
    Serial.println("archivo no existe");
    hola.println("xd");
    hola.close();
  } else {
    // File hola = LittleFS.open("/hola.txt", "a");
    // hola.println("linea asdasd");
    // Serial.println("Archivo existe");
    // hola.close();
  }
  File hola = LittleFS.open("/hola.txt", "r");
  texto = hola.readString();
  hola.close();
}
void loop() {
  asd = Serial.readStringUntil('\n');
  asd.trim();
  if (asd == "1") {
    Serial.println("deberia de forzar algo...");
    File hola = LittleFS.open("/hola.txt", "w");
    hola.println("archivo escrito");
    hola.close();
  }
  if (asd == "0") {
    Serial.println("deberia de borrar algo...");
    File hola = LittleFS.open("/hola.txt", "w");
    hola.println("");
    hola.close();
  }



  delay(500);
  File hola = LittleFS.open("/hola.txt", "r");
  texto = hola.readString();
  Serial.println(texto);
  hola.close();
  delay(500);
}
