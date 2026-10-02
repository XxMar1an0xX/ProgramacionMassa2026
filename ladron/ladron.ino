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

uint32_t color_tira;
String color_tira_hex;
uint16_t brillo_tira;
bool habilitar_respiracion = 0;

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
  variables.begin("valores", false);
  variables.putInt("color tira", hexStringToColor(sender->value));
  color_tira = variables.getInt("color tira", tira_respiracion.Color(0, 0, 0));
  variables.end();
  // for (int led = 0; led < NUMPIXELS; led++) {
  //   tira_respiracion.setPixelColor(led, color_tira);
  // }
  tira_respiracion.fill(color_tira);
  tira_respiracion.show();
  // ESPUI.updateSlider(brillo, int nValue)

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

  variables.begin("valores", false);
  variables.putInt("brillo tira", sender->value.toInt());
  variables.putString("color hex", sender->value);
  color_tira_hex = variables.getString("color hex", "#FFFFFF");
  brillo_tira = variables.getInt("brillo tira", 0);
  variables.end();
  tira_respiracion.fill(color_tira);
  tira_respiracion.setBrightness(brillo_tira);
  tira_respiracion.show();
}

void boton_respiracion(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("respiracionaskdaskdjskdsaldas");
      habilitar_respiracion = (habilitar_respiracion + 1) % 2;
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
  brillo_tira = variables.getInt("brillo tira", 0);
  color_tira = variables.getInt("color tira", tira_respiracion.Color(0, 0, 0));
  color_tira_hex = variables.getString("color hex", "#FFFFFF");
  // variables.end();


  WiFi.softAP(ssid, contraseña.c_str());
  Serial.print("direccion ip: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("contraseña: ");
  Serial.println(contraseña);

  tira_respiracion.begin();
  tira_respiracion.fill(color_tira);
  tira_respiracion.setBrightness(brillo_tira);
  tira_respiracion.show();

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

  auto color_selector = ESPUI.addControl(ControlType::Text, "Color y brillo", variables.getString("color hex", "#FFFFFF"), ControlColor::None, pestaña_luces, manejar_color);
  ESPUI.setInputType(color_selector, "color");
  auto brillo_slider = ESPUI.addControl(Slider, "Brillo", String(variables.getInt("brillo tira", 0)), Dark, color_selector, manejar_brillo);
  ESPUI.addControl(Max, "", "255", None, brillo_slider);
  ESPUI.addControl(Min, "", "1", None, brillo_slider);

  ESPUI.addControl(Button, "Respiracion", "Iniciar", ControlColor::Dark, pestaña_luces, &boton_respiracion);

  ESPUI.addControl(Separator, "Led sin efecto", "", None, pestaña_luces);
  auto boton_led_noefecto = ESPUI.addControl(Button, "Leds sin efecto", "Prender/Apagar", ControlColor::Peterriver, pestaña_luces);

  //NOTE: pestaña de contraseña
  auto pestaña_contraseña = ESPUI.addControl(Tab, "", "Contraseña");

  contraseña_status = ESPUI.addControl(Label, "Contraseña actual", contraseña.c_str(), Turquoise, pestaña_contraseña);

  auto panel_contraseña = ESPUI.addControl(Label, "Contraseña", "Cambiar contraseña de zona wifi", Alizarin, pestaña_contraseña);
  ESPUI.addControl(Text, "", "", Alizarin, panel_contraseña, manejar_texto);
  ESPUI.addControl(Button, "", "Actualizar", Dark, panel_contraseña, &boton_contraseña);

  variables.end();
  ESPUI.begin("Ladron Control");


  Serial.println("tira andadndo");
  // delay(100);
  ESPUI.updateText(color_selector, color_tira_hex);
  // ESPUI.updateSlider(brillo_slider, brillo_tira);
}
void loop() {
  if (habilitar_respiracion == 1) {
    for (int brillo_loop = brillo_tira; brillo_loop > 2 && habilitar_respiracion == 1; brillo_loop--) {
      Serial.println((255 / brillo_tira) * (255 / brillo_tira));
      delay((255 / brillo_tira) * (255 / brillo_tira));
      tira_respiracion.setBrightness(brillo_loop);
      tira_respiracion.show();
    }
    Serial.println("entre loops");
    // delay(10);
    for (int brillo_loop = 2; brillo_loop < brillo_tira && habilitar_respiracion == 1; brillo_loop++) {
      delay((255 / brillo_tira) * (255 / brillo_tira));
      // Serial.println(brillo_loop);
      tira_respiracion.setBrightness(brillo_loop);
      tira_respiracion.show();
    }
    tira_respiracion.setBrightness(brillo_tira);
    tira_respiracion.show();
  }
  // tira_respiracion.show();
  // tira_respiracion.setPixelColor(NUMPIXELS, color_tira);
}
