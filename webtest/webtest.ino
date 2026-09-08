// #include <WiFi.h>
#include <WebGUI.h>

Toggle hola("LED", 50, 50);
void setup() {
  Serial.begin(115200);
  GUI.addElement(&hola);
  GUI.startAP("ESP-webGUI", "hola");
  GUI.setTitle("jkasdhkasj");
  GUI.begin();
  Serial.println(GUI.getIP());
}

void loop() {
  GUI.update();
  if (hola.isOn()) {
    Serial.println("boton aprtado");
  }
}
