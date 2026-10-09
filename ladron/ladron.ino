#include <ESPUI.h>
#include <Preferences.h>
// #include <Adafruit_NeoPixel.h>
#include <FastLED.h>

#define PIN_TIRA 21
// #define TIRA_CARA 19
#define PIXELES_TOTAL 257
#define PIXELES_CARA 80
// #define PIXELES_TOTAL 23

//TODO: cambiar estor pines a lo que son en realidad
#define ENTRADA_SENSOR_EFECTOHALL 14
// #define SENSOR_HALL_ORIENTACION 27
#define MOTOR_SALIDA 26
#define FRENO_SALIDA 25


const char* ssid = "ladron";
String contraseña;

uint16_t selector_cara;

Preferences variables;

uint16_t estado_giro;
uint16_t contraseña_status;

// CRGB tira_cara[PIXELES_CARA];
CRGB tira_total[PIXELES_TOTAL];

String color_tira_hex;
String color_cara_1_hex;
String color_cara_2_hex;
String color_cara_3_hex;
uint16_t brillo_tira;
bool habilitar_respiracion = 0;
bool habilitar_efectodegiro = 0;
short estado_cara = 0;
short cara_seleccionada = 0;
bool estado_sensor_hall = 0;
bool orientacion_bool = 0;
bool girando = false;
bool hablitiar_cuerpo = false;

void boton_empezar_giro(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("girando...");
      digitalWrite(FRENO_SALIDA, HIGH);
      digitalWrite(MOTOR_SALIDA, HIGH);
      girando = true;
      habilitar_efectodegiro = (habilitar_efectodegiro + 1) % 2;
      cara_seleccionada = 120;
      break;
    case B_UP:
      break;
  }
}

void boton_cara_1(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("cara 1");
      // ESPUI.print(estado_giro, "Detenido, cara 1");
      cara_seleccionada = 0;
      if (girando == false) {
        digitalWrite(MOTOR_SALIDA, HIGH);
        digitalWrite(FRENO_SALIDA, HIGH);
        girando = true;
      }
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
      cara_seleccionada = 1;
      if (girando == false) {
        digitalWrite(MOTOR_SALIDA, HIGH);
        digitalWrite(FRENO_SALIDA, HIGH);
        girando = true;
      }
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
      cara_seleccionada = 2;
      if (girando == false) {
        digitalWrite(MOTOR_SALIDA, HIGH);
        digitalWrite(FRENO_SALIDA, HIGH);
        girando = true;
      }
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

// void manejar_color(Control* sender, int type) {
//   Serial.println(sender->value);
//   variables.begin("valores", false);
//   variables.putString("color hex", sender->value);
//   color_tira_hex = variables.getString("color hex", sender->value);
//   variables.end();
//   fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_tira_hex));
//   FastLED.show();
//   // ESPUI.updateSlider(brillo, int nValue)
// }

CRGB hexStringToColor(String hex) {
  if (hex.startsWith("#")) {
    hex.remove(0, 1);
  }

  //NOTE: nose que hace esta funcion
  uint32_t value = strtoul(hex.c_str(), NULL, 16);

  uint8_t r = (value >> 16) & 0xFF;
  uint8_t g = (value >> 8) & 0xFF;
  uint8_t b = value & 0xFF;

  return CRGB(r, g, b);
}


void manejar_brillo(Control* sender, int type) {
  Serial.println(sender->value);

  variables.begin("valores", false);
  variables.putInt("brillo tira", sender->value.toInt());
  // variables.putString("color hex", sender->value);
  // color_tira_hex = variables.getString("color hex", "#FFFFFF");
  brillo_tira = variables.getInt("brillo tira", 0);
  variables.end();
  // fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_tira_hex));
  FastLED.setBrightness(brillo_tira);
  FastLED.show();
}

// void boton_respiracion(Control* sender, int type) {
//   switch (type) {
//     case B_DOWN:
//       Serial.println("respiracionaskdaskdjskdsaldas");
//       habilitar_respiracion = (habilitar_respiracion + 1) % 2;
//       break;
//     case B_UP:
//       break;
//   }
// }

void boton_detener(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("frenando.......");
      delay(50);
      digitalWrite(MOTOR_SALIDA, LOW);
      digitalWrite(FRENO_SALIDA, LOW);
      girando = false;
      // habilitar_respiracion = (habilitar_respiracion + 1) % 2;
      break;
    case B_UP:
      break;
  }
}
void manejar_color_cara(Control* sender, int type) {
  Serial.println(sender->id);
  switch (sender->id) {
    case 14:
      Serial.print("cara 1: ");
      variables.begin("valores", false);
      variables.putString("cara 1 hex", sender->value);
      color_cara_1_hex = variables.getString("cara 1 hex", sender->value);
      Serial.println(color_cara_1_hex);
      variables.end();
      fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_cara_1_hex));
      FastLED.show();
      break;
    case 15:
      Serial.print("cara 2: ");
      variables.begin("valores", false);
      variables.putString("cara 2 hex", sender->value);
      color_cara_2_hex = variables.getString("cara 2 hex", sender->value);
      Serial.println(color_cara_2_hex);
      variables.end();
      fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_cara_2_hex));
      FastLED.show();
      break;
    case 16:
      Serial.print("cara 3: ");
      variables.begin("valores", false);
      variables.putString("cara 3 hex", sender->value);
      color_cara_3_hex = variables.getString("cara 3 hex", sender->value);
      Serial.println(color_cara_3_hex);
      variables.end();
      fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_cara_3_hex));
      FastLED.show();
      break;
  }
}
void seleccion_cara(Control* sender, int type) {
  Serial.println(sender->value);
  estado_cara = sender->value.toInt() - 1;
}
void habilitar_cuerpo(Control* sender, int type) {
  Serial.println(sender->value);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);

  pinMode(MOTOR_SALIDA, OUTPUT);
  pinMode(ENTRADA_SENSOR_EFECTOHALL, INPUT_PULLUP);
  // pinMode(SENSOR_HALL_ORIENTACION, INPUT_PULLUP);
  pinMode(FRENO_SALIDA, OUTPUT);

  variables.begin("valores", false);
  //NOTE: recomiendo cambiar la contraseña default puesta
  contraseña = variables.getString("contraseña", "contraseña");
  brillo_tira = variables.getInt("brillo tira", 0);
  color_tira_hex = variables.getString("color hex", "#FFFFFF");
  color_cara_1_hex = variables.getString("cara 1 hex", "#FFFFFF");
  color_cara_2_hex = variables.getString("cara 2 hex", "#FFFFFF");
  color_cara_3_hex = variables.getString("cara 3 hex", "#FFFFFF");

  WiFi.softAP(ssid, contraseña.c_str());
  Serial.print("direccion ip: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("contraseña: ");
  Serial.println(contraseña);

  FastLED.addLeds<WS2811, PIN_TIRA, BRG>(tira_total, PIXELES_TOTAL);

  Serial.print("hex al inicio: ");
  Serial.println(color_tira_hex);
  Serial.print("color cara 1: ");
  Serial.println(color_cara_1_hex);
  Serial.print("color cara 2: ");
  Serial.println(color_cara_2_hex);
  Serial.print("color cara 3: ");
  Serial.println(color_cara_3_hex);

  //NOTE: pestaña control principal
  auto pestaña_control = ESPUI.addControl(Tab, "", "Controles");

  // estado_giro = ESPUI.label("Estado de Giro", ControlColor::None, "Detenido");
  estado_giro = ESPUI.addControl(ControlType::Label, "Estado de Giro", "Detenido", ControlColor::Wetasphalt, pestaña_control);

  auto girar = ESPUI.addControl(Button, "Empezar Giro", "Girar", Dark, pestaña_control, &boton_empezar_giro);
  ESPUI.addControl(Button, "", "Iniciar", ControlColor::Dark, girar, &boton_detener);
  ESPUI.addControl(Separator, "Led sin efecto", "", None, girar, &boton_detener);

  auto panel_caras = ESPUI.addControl(ControlType::Button, "Seleccion Cara", "Cara 1", ControlColor::Dark, pestaña_control, &boton_cara_1);
  ESPUI.addControl(Button, "", "Cara 2", None, panel_caras, &boton_cara_2);
  ESPUI.addControl(Button, "", "Cara 3", None, panel_caras, &boton_cara_3);

  selector_cara = ESPUI.addControl(Slider, "Cara actual", "1", Carrot, panel_caras, seleccion_cara);
  ESPUI.addControl(Max, "", "3", None, selector_cara);
  ESPUI.addControl(Min, "", "1", None, selector_cara);

  //NOTE: pestaña de luces
  auto pestaña_luces = ESPUI.addControl(Tab, "", "Luces");

  ESPUI.addControl(Separator, "Leds Cuerpo/Cara", "", None, pestaña_luces);

  auto color_cara_1 = ESPUI.addControl(ControlType::Text, "Color Caras", color_cara_1_hex.c_str(), ControlColor::None, pestaña_luces, manejar_color_cara);
  ESPUI.setInputType(color_cara_1, "color");
  auto color_cara_2 = ESPUI.addControl(ControlType::Text, "Cara 2", color_cara_2_hex.c_str(), ControlColor::None, color_cara_1, manejar_color_cara);
  ESPUI.setInputType(color_cara_2, "color");
  auto color_cara_3 = ESPUI.addControl(ControlType::Text, "Cara 3", color_cara_3_hex.c_str(), ControlColor::None, color_cara_1, manejar_color_cara);
  ESPUI.setInputType(color_cara_3, "color");
  // auto color_selector = ESPUI.addControl(ControlType::Text, "Color y brillo", color_tira_hex.c_str(), ControlColor::None, pestaña_luces, manejar_color);
  // ESPUI.setInputType(color_selector, "color");
  auto brillo_slider = ESPUI.addControl(Slider, "Brillo", String(variables.getInt("brillo tira", 0)), Dark, color_cara_1, manejar_brillo);
  ESPUI.addControl(Max, "", "255", None, brillo_slider);
  ESPUI.addControl(Min, "", "1", None, brillo_slider);
  auto interruptor_cuerpo = ESPUI.addControl(Switcher, "habilitar Cuerpo", "0", Dark, pestaña_luces);

  //NOTE: pestaña de contraseña
  auto pestaña_contraseña = ESPUI.addControl(Tab, "", "Contraseña");

  contraseña_status = ESPUI.addControl(Label, "Contraseña actual", contraseña.c_str(), Turquoise, pestaña_contraseña);

  auto panel_contraseña = ESPUI.addControl(Label, "Contraseña", "Cambiar contraseña de zona wifi", Alizarin, pestaña_contraseña);
  ESPUI.addControl(Text, "", "", Alizarin, panel_contraseña, manejar_texto);
  ESPUI.addControl(Button, "", "Actualizar", Dark, panel_contraseña, &boton_contraseña);

  variables.end();
  ESPUI.begin("Ladron Control");


  Serial.println("tira andadndo");
  fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_tira_hex));
  FastLED.show();
}
void loop() {
  // digitalWrite(MOTOR_SALIDA, HIGH);
  // digitalWrite(FRENO_SALIDA, HIGH);

  if (habilitar_respiracion == 1) {
    for (short brillo_loop = brillo_tira; brillo_loop > 1 && habilitar_respiracion == 1; brillo_loop--) {
      delay(5);
      fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_tira_hex));
      nscale8_video(tira_total, PIXELES_TOTAL, brillo_loop);
      FastLED.show();
    }
    Serial.println("entre loops");
    for (short brillo_loop = 1; brillo_loop < brillo_tira && habilitar_respiracion == 1; brillo_loop++) {
      delay(5);
      fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_tira_hex));
      nscale8_video(tira_total, PIXELES_TOTAL, brillo_loop);
      FastLED.show();
    }
  }


  if (!digitalRead(ENTRADA_SENSOR_EFECTOHALL) == LOW && estado_sensor_hall == HIGH) {

    estado_cara = (estado_cara + 1) % 3;
    Serial.print("cara actual: ");
    Serial.println(estado_cara);
    delay(50);
  }
  EVERY_N_MILLISECONDS(50) {
    String inicio_estado;
    if (girando == true) {
      inicio_estado = "Girando, ";
    } else {
      inicio_estado = "Detenido, ";
    }
    switch (estado_cara) {
      case 0:
        fill_solid(tira_total, PIXELES_CARA, hexStringToColor(color_cara_1_hex));
        ESPUI.print(estado_giro, (inicio_estado + "En Cara 1: (Mascara azul)"));
        break;
      case 1:
        fill_solid(tira_total, PIXELES_CARA, hexStringToColor(color_cara_2_hex));
        ESPUI.print(estado_giro, (inicio_estado + "En Cara 2: (Mateo)"));
        break;
      case 2:
        fill_solid(tira_total, PIXELES_CARA, hexStringToColor(color_cara_3_hex));
        ESPUI.print(estado_giro, (inicio_estado + "En Cara 3: (Cara Blanca?)"));
        break;
    }
    FastLED.setBrightness(brillo_tira);
    FastLED.show();
    ESPUI.updateSlider(selector_cara, (estado_cara + 1));
  }
  estado_sensor_hall = !digitalRead(ENTRADA_SENSOR_EFECTOHALL);
  if (cara_seleccionada == estado_cara && girando == true) {
    Serial.println("frenando...");
    digitalWrite(FRENO_SALIDA, LOW);
    digitalWrite(MOTOR_SALIDA, LOW);
    girando = false;
    delay(60);
    digitalWrite(MOTOR_SALIDA, HIGH);
    delay(100);
    digitalWrite(MOTOR_SALIDA, LOW);
  }
  if (girando == true) {
    digitalWrite(MOTOR_SALIDA, HIGH);
    digitalWrite(FRENO_SALIDA, HIGH);
  }
}
