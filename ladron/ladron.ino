#include <WebGUI.h>

void setup() {
  Serial.begin(115200);
  GUI.startAP("ESPwebgui", "hola1234");
  GUI.initSettings();
  GUI.setTitle("control");
  GUI.begin();
  Serial.println(GUI.getIP());
}
