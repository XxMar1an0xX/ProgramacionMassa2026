#include <ESPUI.h>
#include <Preferences.h>
// #include <Adafruit_NeoPixel.h>
#include <FastLED.h>

#define PIN_TIRA 21
// #define TIRA_CARA 19
#define PIXELES_TOTAL 257
// #define PIXELES_CARA 80
// #define PIXELES_TOTAL 23

//TODO: cambiar estor pines a lo que son en realidad
#define ENTRADA_SENSOR_EFECTOHALL 25
// #define SENSOR_HALL_ORIENTACION 27
#define MOTOR_SALIDA 2  //NOTE: cambiado de pin 3.3 a pin de 5v
#define FRENO_SALIDA 4
#define PIN_RED 14
#define PIN_BLUE 27
#define PIN_GREEN 26


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
bool habilitar_cuerpo = false;
int delay_freno = 200;
int pixeles_cara = 80;
bool ultima_accion_cara = false;
bool habilitar_reed_switch = true;
unsigned long millis_global = 0;

bool inicia_muerte = false;


void boton_empezar_giro(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("girando...");
      // digitalWrite(FRENO_SALIDA, HIGH);
      // digitalWrite(MOTOR_SALIDA, HIGH);
      girando = true;
      // habilitar_efectodegiro = (habilitar_efectodegiro + 1) % 2;
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
      if (girando == false && cara_seleccionada != estado_cara) {
        // digitalWrite(MOTOR_SALIDA, HIGH);
        // digitalWrite(FRENO_SALIDA, HIGH);
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
      if (girando == false && cara_seleccionada != estado_cara) {
        // digitalWrite(MOTOR_SALIDA, HIGH);
        // digitalWrite(FRENO_SALIDA, HIGH);
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
      if (girando == false && cara_seleccionada != estado_cara) {
        // digitalWrite(MOTOR_SALIDA, HIGH);
        // digitalWrite(FRENO_SALIDA, HIGH);
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
      // digitalWrite(MOTOR_SALIDA, LOW);
      // delay(delay_freno);
      // Serial.println("electroiman activado");
      // digitalWrite(FRENO_SALIDA, LOW);
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
    case 16:
      Serial.print("cara 1: ");
      variables.begin("valores", false);
      variables.putString("cara 1 hex", sender->value);
      color_cara_1_hex = variables.getString("cara 1 hex", sender->value);
      Serial.println(color_cara_1_hex);
      variables.end();
      fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_cara_1_hex));
      FastLED.show();
      break;
    case 17:
      Serial.print("cara 2: ");
      variables.begin("valores", false);
      variables.putString("cara 2 hex", sender->value);
      color_cara_2_hex = variables.getString("cara 2 hex", sender->value);
      Serial.println(color_cara_2_hex);
      variables.end();
      fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_cara_2_hex));
      FastLED.show();
      break;
    case 18:
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
void switch_habilitar_cuerpo(Control* sender, int type) {
  Serial.println(sender->value);
  habilitar_cuerpo = sender->value.toInt();
}
void manejar_delay_freno(Control* sender, int type) {
  Serial.println(sender->value);
  variables.begin("valores", false);
  variables.putInt("delay freno", sender->value.toInt());
  delay_freno = variables.getInt("delay freno", 500);
  variables.end();
}
void manejar_leds_cara(Control* sender, int type) {
  Serial.println(sender->value);
  variables.begin("valores", false);
  variables.putInt("pixeles cara", sender->value.toInt());
  pixeles_cara = variables.getInt("pixeles cara", 500);
  variables.end();
}
void manejo_boton_muerte(Control* sender, int type) {
  Serial.println(sender->value);
  switch (type) {
    case B_DOWN:
      inicia_muerte = true;
      break;
    case B_UP:
      break;
  }
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
  pixeles_cara = variables.getInt("pixeles cara", 80);
  delay_freno = variables.getInt("delay freno", 500);

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

  auto girar = ESPUI.addControl(Button, "Control Giro Manual", "Girar", Dark, pestaña_control, &boton_empezar_giro);
  ESPUI.addControl(Button, "", "Detener", ControlColor::Dark, girar, &boton_detener);
  ESPUI.addControl(Separator, "Led sin efecto", "", None, girar, &boton_detener);

  auto panel_caras = ESPUI.addControl(ControlType::Button, "Seleccion Cara", "Cara 1", ControlColor::Dark, pestaña_control, &boton_cara_1);
  ESPUI.addControl(Button, "", "Cara 2", None, panel_caras, &boton_cara_2);
  ESPUI.addControl(Button, "", "Cara 3", None, panel_caras, &boton_cara_3);

  selector_cara = ESPUI.addControl(Slider, "Cara actual", "1", Carrot, panel_caras, seleccion_cara);
  ESPUI.addControl(Max, "", "3", None, selector_cara);
  ESPUI.addControl(Min, "", "1", None, selector_cara);
  auto configuracion_delay_freno = ESPUI.addControl(Number, "Delay freno", String(delay_freno), Dark, pestaña_control, manejar_delay_freno);

  auto boton_muerte = ESPUI.addControl(Button, "Muerte", "iniciar", Dark, pestaña_control, &manejo_boton_muerte);

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
  auto interruptor_cuerpo = ESPUI.addControl(Switcher, "habilitar Cuerpo", "0", Dark, pestaña_luces, switch_habilitar_cuerpo);
  auto configuracion_leds_cara = ESPUI.addControl(Number, "leds en cara", String(pixeles_cara), Dark, pestaña_luces, manejar_leds_cara);

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

  // if (habilitar_respiracion == 1) {
  //   for (short brillo_loop = brillo_tira; brillo_loop > 1 && habilitar_respiracion == 1; brillo_loop--) {
  //     delay(5);
  //     fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_tira_hex));
  //     nscale8_video(tira_total, PIXELES_TOTAL, brillo_loop);
  //     FastLED.show();
  //   }
  //   Serial.println("entre loops");
  //   for (short brillo_loop = 1; brillo_loop < brillo_tira && habilitar_respiracion == 1; brillo_loop++) {
  //     delay(5);
  //     fill_solid(tira_total, PIXELES_TOTAL, hexStringToColor(color_tira_hex));
  //     nscale8_video(tira_total, PIXELES_TOTAL, brillo_loop);
  //     FastLED.show();
  //   }
  // }

  if (!digitalRead(ENTRADA_SENSOR_EFECTOHALL) == LOW && estado_sensor_hall == HIGH) {
    estado_cara = (estado_cara + 1) % 3;
    Serial.print("cara actual: ");
    Serial.println(estado_cara);
  }
  delay(15);
  estado_sensor_hall = !digitalRead(ENTRADA_SENSOR_EFECTOHALL);

  EVERY_N_MILLISECONDS(10) {
    String inicio_estado;
    if (girando == true) {
      inicio_estado = "Girando, ";
    } else {
      inicio_estado = "Detenido, ";
    }
    switch (estado_cara) {
      case 0:
        fill_solid(tira_total, pixeles_cara + ((PIXELES_TOTAL - pixeles_cara) * habilitar_cuerpo), hexStringToColor(color_cara_1_hex));
        ESPUI.print(estado_giro, (inicio_estado + "En Cara 1: (Mascara azul)"));
        analogWrite(PIN_RED, scale8(hexStringToColor(color_cara_1_hex).r, brillo_tira));
        analogWrite(PIN_BLUE, scale8(hexStringToColor(color_cara_1_hex).b, brillo_tira));
        analogWrite(PIN_GREEN, scale8(hexStringToColor(color_cara_1_hex).g, brillo_tira));
        break;
      case 1:
        fill_solid(tira_total, pixeles_cara + ((PIXELES_TOTAL - pixeles_cara) * habilitar_cuerpo), hexStringToColor(color_cara_2_hex));
        ESPUI.print(estado_giro, (inicio_estado + "En Cara 2: (Mateo)"));
        analogWrite(PIN_RED, scale8(hexStringToColor(color_cara_2_hex).r, brillo_tira));
        analogWrite(PIN_BLUE, scale8(hexStringToColor(color_cara_2_hex).b, brillo_tira));
        analogWrite(PIN_GREEN, scale8(hexStringToColor(color_cara_2_hex).g, brillo_tira));
        break;
      case 2:
        fill_solid(tira_total, pixeles_cara + ((PIXELES_TOTAL - pixeles_cara) * habilitar_cuerpo), hexStringToColor(color_cara_3_hex));
        ESPUI.print(estado_giro, (inicio_estado + "En Cara 3: (Cara Blanca?)"));
        analogWrite(PIN_RED, scale8(hexStringToColor(color_cara_3_hex).r, brillo_tira));
        analogWrite(PIN_BLUE, scale8(hexStringToColor(color_cara_3_hex).b, brillo_tira));
        analogWrite(PIN_GREEN, scale8(hexStringToColor(color_cara_3_hex).g, brillo_tira));
        break;
    }
    if (!habilitar_cuerpo) {
      fill_solid(&tira_total[pixeles_cara + 1], PIXELES_TOTAL - pixeles_cara, CRGB::Black);
    }
    // Serial.println(pixeles_cara + ((PIXELES_TOTAL - pixeles_cara) * habilitar_cuerpo));
    FastLED.setBrightness(brillo_tira);
    FastLED.show();
    ESPUI.updateSlider(selector_cara, (estado_cara + 1));
  }
  if (cara_seleccionada == (estado_cara + 1) % 3 && girando == true) {
    Serial.println("frenando...");
    digitalWrite(MOTOR_SALIDA, LOW);
    delay(delay_freno);
    Serial.println("electroiman activado");
    digitalWrite(FRENO_SALIDA, LOW);
    girando = false;
    delay(60);
    digitalWrite(MOTOR_SALIDA, HIGH);
    delay(100);
    digitalWrite(MOTOR_SALIDA, LOW);
  }
  if (girando == true && ultima_accion_cara != girando) {
    Serial.println("motor activado");
    digitalWrite(FRENO_SALIDA, HIGH);
    delay(250);
    Serial.println("electroiman desactivado");
    digitalWrite(MOTOR_SALIDA, HIGH);
    ultima_accion_cara = girando;
    habilitar_reed_switch = false;
    millis_global = millis();
  } else if (girando == false && ultima_accion_cara != girando) {
    Serial.println("frenando...");
    digitalWrite(MOTOR_SALIDA, LOW);
    delay(delay_freno);
    Serial.println("electroiman activado");
    digitalWrite(FRENO_SALIDA, LOW);
    ultima_accion_cara = girando;
  }
  if (millis() - millis_global > 300 && habilitar_reed_switch == false) {
    habilitar_reed_switch = true;
  }
  if (inicia_muerte) {
    Serial.println("motor activado");
    digitalWrite(FRENO_SALIDA, HIGH);
    delay(250);
    Serial.println("electroiman desactivado");
    digitalWrite(MOTOR_SALIDA, HIGH);
    while (inicia_muerte) {
      CRGB hola = CRGB(random(50, 255), random(50, 255), random(50, 255));
      fill_solid(tira_total, PIXELES_TOTAL, hola);
      analogWrite(PIN_RED, hola.r);
      analogWrite(PIN_BLUE, hola.b);
      analogWrite(PIN_GREEN, hola.g);
      delay(100);
    }
  }
}
