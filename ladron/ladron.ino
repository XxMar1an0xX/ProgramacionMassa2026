#include <ESPUI.h>
#include <Preferences.h>
// #include <Adafruit_NeoPixel.h>
#include <FastLED.h>

#define LED_RESPIRACION 21
#define TIRA_CARA 19
#define NUMPIXELS 237
#define PIXELES_CARA 80
// #define NUMPIXELS 23

//TODO: cambiar estor pines a lo que son en realidad
#define ENTRADA_SENSOR_EFECTOHALL 2
#define SENSOR_HALL_ORIENTACION 4
#define MOTOR_SALIDA 14
#define FRENO_SALIDA 27


const char* ssid = "ladron";
String contraseña;

Preferences variables;

uint16_t estado_giro;
uint16_t contraseña_status;

CRGB tira_cara[PIXELES_CARA];
CRGB tira_efectos[NUMPIXELS];

String color_tira_hex;
uint16_t brillo_tira;
bool habilitar_respiracion = 0;
bool habilitar_efectodegiro = 0;
short estado_cara = 0;
short cara_seleccionada = 0;
bool estado_sensor_hall = 0;
bool girando = false;

void boton_empezar_giro(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("girando...");
      digitalWrite(FRENO_SALIDA, HIGH);
      digitalWrite(MOTOR_SALIDA, HIGH);
      ESPUI.print(estado_giro, "Girando...");
      girando = true;
      habilitar_efectodegiro = (habilitar_efectodegiro + 1) % 2;
      cara_seleccionada = 120;
      break;
    case B_UP:
      // Serial.println("boton apagado");
      break;
  }
}

void boton_cara_1(Control* sender, int type) {
  switch (type) {
    case B_DOWN:
      Serial.println("cara 1");
      ESPUI.print(estado_giro, "Detenido, cara 1");
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

void manejar_color(Control* sender, int type) {
  Serial.println(sender->value);
  variables.begin("valores", false);
  variables.putString("color hex", sender->value);
  color_tira_hex = variables.getString("color hex", sender->value);
  variables.end();
  fill_solid(tira_efectos, NUMPIXELS, hexStringToColor(color_tira_hex));
  FastLED.show();
  // ESPUI.updateSlider(brillo, int nValue)
}

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
  fill_solid(tira_efectos, NUMPIXELS, hexStringToColor(color_tira_hex));
  FastLED.setBrightness(brillo_tira);
  FastLED.show();
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


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);

  pinMode(MOTOR_SALIDA, OUTPUT);
  pinMode(ENTRADA_SENSOR_EFECTOHALL, INPUT_PULLDOWN);
  pinMode(SENSOR_HALL_ORIENTACION, INPUT_PULLDOWN);
  pinMode(FRENO_SALIDA, OUTPUT);

  variables.begin("valores", false);
  //NOTE: recomiendo cambiar la contraseña default puesta
  contraseña = variables.getString("contraseña", "contraseña");
  brillo_tira = variables.getInt("brillo tira", 0);
  color_tira_hex = variables.getString("color hex", "#FFFFFF");


  WiFi.softAP(ssid, contraseña.c_str());
  Serial.print("direccion ip: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("contraseña: ");
  Serial.println(contraseña);

  FastLED.addLeds<WS2811, LED_RESPIRACION, BRG>(tira_efectos, NUMPIXELS);
  FastLED.addLeds<WS2811, TIRA_CARA, BRG>(tira_cara, PIXELES_CARA);

  Serial.print("hex al inicio: ");
  Serial.println(color_tira_hex);

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

  auto color_selector = ESPUI.addControl(ControlType::Text, "Color y brillo", color_tira_hex.c_str(), ControlColor::None, pestaña_luces, manejar_color);
  ESPUI.setInputType(color_selector, "color");
  auto brillo_slider = ESPUI.addControl(Slider, "Brillo", String(variables.getInt("brillo tira", 0)), Dark, color_selector, manejar_brillo);
  ESPUI.addControl(Max, "", "255", None, brillo_slider);
  ESPUI.addControl(Min, "", "1", None, brillo_slider);

  ESPUI.addControl(Button, "Respiracion", "Iniciar", ControlColor::Dark, pestaña_luces, &boton_respiracion);

  ESPUI.addControl(Separator, "Led sin efecto", "", None, pestaña_luces);
  auto boton_led_noefecto = ESPUI.addControl(Button, "Leds sin efecto", "Prender/Apagar", ControlColor::Peterriver, pestaña_luces, &boton_led_sin_efecto);

  //NOTE: pestaña de contraseña
  auto pestaña_contraseña = ESPUI.addControl(Tab, "", "Contraseña");

  contraseña_status = ESPUI.addControl(Label, "Contraseña actual", contraseña.c_str(), Turquoise, pestaña_contraseña);

  auto panel_contraseña = ESPUI.addControl(Label, "Contraseña", "Cambiar contraseña de zona wifi", Alizarin, pestaña_contraseña);
  ESPUI.addControl(Text, "", "", Alizarin, panel_contraseña, manejar_texto);
  ESPUI.addControl(Button, "", "Actualizar", Dark, panel_contraseña, &boton_contraseña);

  variables.end();
  ESPUI.begin("Ladron Control");


  Serial.println("tira andadndo");
  fill_solid(tira_efectos, NUMPIXELS, hexStringToColor(color_tira_hex));
  FastLED.show();
}
void loop() {
  // digitalWrite(MOTOR_SALIDA, HIGH);
  // digitalWrite(FRENO_SALIDA, HIGH);

  if (habilitar_respiracion == 1) {
    for (short brillo_loop = brillo_tira; brillo_loop > 1 && habilitar_respiracion == 1; brillo_loop--) {
      delay(5);
      fill_solid(tira_efectos, NUMPIXELS, hexStringToColor(color_tira_hex));
      nscale8_video(tira_efectos, NUMPIXELS, brillo_loop);
      FastLED.show();
    }
    Serial.println("entre loops");
    for (short brillo_loop = 1; brillo_loop < brillo_tira && habilitar_respiracion == 1; brillo_loop++) {
      delay(5);
      fill_solid(tira_efectos, NUMPIXELS, hexStringToColor(color_tira_hex));
      nscale8_video(tira_efectos, NUMPIXELS, brillo_loop);
      FastLED.show();
    }
  }

  // if (habilitar_efectodegiro == 1) {
  //
  //   for (short delays = 200; delays > 20 && habilitar_efectodegiro == 1; delays = delays - random(1, 10)) {
  //     FastLED.delay(delays);
  //     fill_solid(tira_efectos, NUMPIXELS, CRGB(random(0, 255), random(0, 255), random(0, 255)));
  //     FastLED.delay(delays);
  //     fill_solid(tira_efectos, NUMPIXELS, CRGB(random(0, 255), random(0, 255), random(0, 255)));
  //     FastLED.delay(delays);
  //     fill_solid(tira_efectos, NUMPIXELS, CRGB(random(0, 255), random(0, 255), random(0, 255)));
  //   }
  // }

  if (digitalRead(ENTRADA_SENSOR_EFECTOHALL) == LOW && estado_sensor_hall == HIGH) {

    estado_cara = (estado_cara + 1) % 3;
    switch (estado_cara) {
      case 1:
        fill_solid(tira_cara, PIXELES_CARA, CRGB(0, 200, 0));
        break;
      case 2:
        fill_solid(tira_cara, PIXELES_CARA, CRGB(0, 0, 200));
        break;
      case 3:
        fill_solid(tira_cara, PIXELES_CARA, CRGB(200, 0, 0));
        break;
    }
    Serial.print("cara actual: ");
    Serial.println(estado_cara);
    delay(50);
  }
  EVERY_N_MILLISECONDS(500) {
    Serial.println(digitalRead(MOTOR_SALIDA));
    Serial.println(digitalRead(FRENO_SALIDA));
    Serial.print("condicional: ");
    ;
    Serial.println(girando);
  }
  estado_sensor_hall = digitalRead(ENTRADA_SENSOR_EFECTOHALL);
  // if (cara_seleccionada == estado_cara && girando == true) {
  //   Serial.println("frenando...");
  //   digitalWrite(FRENO_SALIDA, LOW);
  //   digitalWrite(MOTOR_SALIDA, LOW);
  //   girando = false;
  // }
  if (girando == true) {
    // Serial.println(girando);
    digitalWrite(MOTOR_SALIDA, HIGH);
    digitalWrite(FRENO_SALIDA, HIGH);
  }
}
