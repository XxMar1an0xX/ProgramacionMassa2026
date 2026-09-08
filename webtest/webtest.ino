// #include <WiFi.h>
#include <WebGUI.h>


Slider angulo_servo("servo", 20, 20, 0, 270, 0, 350);
Button salvar("guardar", 20, 50);

void setup() {
  Serial.begin(115200);
  GUI.startAP("ESP-webGUI", "hola");
  GUI.initSettings();
  GUI.addElement(&angulo_servo);
  GUI.addElement(&salvar);
  GUI.setTitle("ServoControl");
  GUI.begin();

  angulo_servo.setValue(GUI.loadIntSetting("Angulo"));

  Serial.print("http://");
  Serial.println(GUI.getIP());
}

void loop() {
  GUI.update();

  if (salvar.wasPressed()) {
    GUI.saveSetting("Angulo", angulo_servo.getIntValue());
    Serial.println("guardado");
  }
}
