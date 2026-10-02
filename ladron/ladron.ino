#include <ESPUI.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

#define LED_RESPIRACION 21  //NOTE: cambiar
#define NUMPIXELS 137


const char* ssid = "hola";
String contraseña;

Preferences variables;

uint16_t estado_giro;
uint16_t contraseña_status;

Adafruit_NeoPixel tira_respiracion(NUMPIXELS, LED_RESPIRACION, NEO_BRG + NEO_KHZ800);

uint32_t color_tira = tira_respiracion.Color(0, 0, 0);
uint16_t brillo_tira = 1;

void boton_empezar_giro(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("boton encendido");
      ESPUI.print(estado_giro, "Girando...");
      break;
    case B_UP:
      Serial.println("boton apagado");
      break;
  }
}

void boton_cara_1(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("cara 1");
      ESPUI.print(estado_giro, "Detenido, cara 1");
      break;
    case B_UP:
      Serial.println(".");
      break;
  }
}
void boton_cara_2(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("Cara 2");
      ESPUI.print(estado_giro, "Detenido, cara 2");
      break;
    case B_UP:
      Serial.println(".");
      break;
  }
}
void boton_cara_3(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("Cara 3");
      ESPUI.print(estado_giro, "Detenido, cara 3");
      break;
    case B_UP:
      Serial.println(".");
      break;
  }
}

void boton_contraseña(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      if (contraseña.length() >= 8) {
        Serial.print("Actualizando a contraseña: ");
        Serial.print(contraseña.c_str());
        ESPUI.updateLabel(contraseña_status, contraseña.c_str());

        WiFi.softAP(ssid, contraseña);
      } else {
        Serial.println("Contraseña no es suficiente");
      }
      break;
    case B_UP:
      Serial.println(".");
      break;
  }
}

void manejar_texto(Control* sender, int type) {
  Serial.println(sender->value);
  if (sender->value.length() >= 8) {
    variables.begin("valores", false);
    variables.putString("contraseña", sender->value);
    contraseña = variables.getString("contraseña", "contraseña");
    variables.end();
  }
}

void manejar_color(Control* sender, int type) {
  Serial.println(sender->value);
  // Serial.print(hexStringToColor(sender->value));
  color_tira = hexStringToColor(sender->value);
  for (short led; led < NUMPIXELS; led++) {
    tira_respiracion.setPixelColor(led, color_tira);
    tira_respiracion.show();
  }
  // tira_respiracion.fill(color_tira, 0, NUMPIXELS);
}

uint32_t hexStringToColor(String hex) {
  if (hex.startsWith("#")) {
    hex.remove(0, 1);
  }

  //NOTE: nose que hace esta funcion
  uint32_t value = strtoul(hex.c_str(), NULL, 16);

  uint8_t r = (value >> 16) & 0xFF;
  uint8_t g = (value >> 8) & 0xFF;
  uint8_t b = value & 0xFF;

  return tira_respiracion.Color(r, g, b);
}


void manejar_brillo(Control* sender, int type) {
  Serial.println(sender->value);
  brillo_tira = sender->value.toInt();
  tira_respiracion.fill(color_tira, 0, NUMPIXELS);
  tira_respiracion.setBrightness(brillo_tira);
  tira_respiracion.show();
}

void boton_respiracion(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("respiracionaskdaskdjskdsaldas");
      // for (int brillo = 0; brillo < 255; brillo++) {
      //   delay(1);
      //   tira_respiracion.setBrightness(brillo);
      //   tira_respiracion.show();
      // }
      // delay(50);
      for (int brillo = brillo_tira; brillo > 1; brillo--) {
        delay(5);
        Serial.print(brillo);
        tira_respiracion.setBrightness(brillo);
        tira_respiracion.show();
      }
      Serial.println(tira_respiracion.getBrightness());
      // delay(10);
      for (int brillo = 1; brillo < brillo_tira; brillo++) {
        delay(5);
        Serial.print(brillo);
        tira_respiracion.setBrightness(brillo);
        tira_respiracion.show();
      }
      Serial.println(tira_respiracion.getBrightness());
      tira_respiracion.show();
      break;
    case B_UP:
      break;
  }
}

void boton_led_sin_efecto(Control* sender, int type) {
  Serial.println("holakldasjdasjdasjldasjkljas");
}


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);

  variables.begin("valores", false);
  //NOTE: recomiendo cambiar la contraseña default puesta
  contraseña = variables.getString("contraseña", "contraseña");
  variables.end();


  WiFi.softAP(ssid, contraseña.c_str());
  Serial.print("direccion ip: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("contraseña: ");
  Serial.println(contraseña);

  //NOTE: pestaña control principal
  auto pestaña_control = ESPUI.addControl(Tab, "", "Controles");

  // estado_giro = ESPUI.label("Estado de Giro", ControlColor::None, "Detenido");
  estado_giro = ESPUI.addControl(ControlType::Label, "Estado de Giro", "Detenido", ControlColor::Wetasphalt, pestaña_control);

  // ESPUI.button("Empezar Giro", &boton_empezar_giro, ControlColor::Dark, "Press");

  ESPUI.addControl(Button, "Empezar Giro", "Girar", Dark, pestaña_control, &boton_empezar_giro);

  auto panel_caras = ESPUI.addControl(ControlType::Button, "Seleccion Cara", "Cara 1", ControlColor::Dark, pestaña_control, &boton_cara_1);
  ESPUI.addControl(Button, "", "Cara 2", None, panel_caras, &boton_cara_2);
  ESPUI.addControl(Button, "", "Cara 3", None, panel_caras, &boton_cara_3);

  //NOTE: pestaña de luces
  auto pestaña_luces = ESPUI.addControl(Tab, "", "Luces");

  ESPUI.addControl(Separator, "Leds respiracion", "", None, pestaña_luces);

  auto color = ESPUI.addControl(ControlType::Text, "Color y brillo", "#000000", ControlColor::None, pestaña_luces, manejar_color);
  ESPUI.setInputType(color, "color");
  auto brillo = ESPUI.addControl(Slider, "Brillo", "50", Dark, color, manejar_brillo);
  ESPUI.addControl(Max, "", "255", None, brillo);
  ESPUI.addControl(Min, "", "1", None, brillo);

  ESPUI.addControl(Button, "Respiracion", "Iniciar", ControlColor::Dark, pestaña_luces, &boton_respiracion);

  ESPUI.addControl(Separator, "Led sin efecto", "", None, pestaña_luces);
  auto boton_led_noefecto = ESPUI.addControl(Button, "Leds sin efecto", "Prender/Apagar", ControlColor::Peterriver, pestaña_luces);

  //NOTE: pestaña de contraseña
  auto pestaña_contraseña = ESPUI.addControl(Tab, "", "Contraseña");

  contraseña_status = ESPUI.addControl(Label, "Contraseña actual", contraseña.c_str(), Turquoise, pestaña_contraseña);

  auto panel_contraseña = ESPUI.addControl(Label, "Contraseña", "Cambiar contraseña de zona wifi", Alizarin, pestaña_contraseña);
  ESPUI.addControl(Text, "", "", Alizarin, panel_contraseña, manejar_texto);
  ESPUI.addControl(Button, "", "Actualizar", Dark, panel_contraseña, &boton_contraseña);

  ESPUI.begin("Ladron Control");

  tira_respiracion.fill(color_tira, 0, NUMPIXELS);

  Serial.println("tira andadndo");
  // delay(100);
}
void loop() {
  // tira_respiracion.show();
  // tira_respiracion.setPixelColor(NUMPIXELS, color_tira);
}
