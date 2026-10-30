#include <TinyGPSPlus.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Preferences.h>


// void setup() {
//   Serial.begin(115200);
//   Serial2.begin(9600, SERIAL_8N1, GPSRX, GPSTX);
//   Serial.println("ya pasamos el inico");
//   pinMode(2, OUTPUT);
//   ArduinoInit();
//   // Connect to Arduino IoT Cloud
// }
//
// void loop() {
//
//   //NOTE: Sensor GPS
//   while (Serial2.available() > 0) {
//     gps.encode(Serial2.read());
//   }
//
//   if (gps.location.isUpdated() && gps.location.isValid()) {
//
//     Serial.print("Latitude: ");
//     Serial.println(gps.location.lat(), 6);
//
//     Serial.print("Longitude: ");
//     Serial.println(gps.location.lng(), 6);
//
//     Serial.print("Altitude: ");
//     if (gps.altitude.isValid()) {
//       Serial.print(gps.altitude.meters());
//       Serial.println(" m");
//     } else {
//       Serial.println("Waiting");
//     }
//     Serial.print("asdasdasd: ");
//   }
//
//
//   //NOTE: Arduino
//   ArduinoCloud.update();
// }
// void onEstadoLedChange() {
//   digitalWrite(2, estado_led ? HIGH : LOW);
// }
// void actualizar_int() {
//   Serial.println(valor_variable);
// }




/*
  MONITOR AMBIENTAL GENERAL - Arduino UNO R3
  Une 4 sensores y muestra todo junto en el Monitor Serie (9600 baudios):

    - UV ............ ML8511 (GY-B511 / MH-B511)
    - Temp/Hum/Pres . BME280 (I2C)
    - CO2 ........... MH-Z19C (UART por software)
    - CO ............ MQ-7 / MH-MQ (analogico)

  CONEXIONES
    UV:     OUT -> A0 | 3V3 -> A1 | VIN -> 5V | GND -> GND | EN -> 3.3V
    MQ-7:   AO  -> A2 (OJO: antes iba en A0, ahora se mueve a A2)
            VCC -> 5V | GND -> GND | DO sin conectar
    BME280: SDA -> A4 | SCL -> A5 | VIN y GND segun tu modulo
    CO2:    TX del sensor -> pin 2 | RX del sensor -> pin 3 (con divisor a 3.3 V)
            Vin -> 5V | GND -> GND

  COMANDOS POR EL MONITOR SERIE
    c = calibrar el MQ-7 (solo en aire limpio, con el sensor ya estable)
    z = calibrar cero del CO2 (solo al aire libre, 400 ppm, tras 20 min)
    a = CO2: ABC apagado | b = CO2: ABC encendido

  LIBRERIAS: Adafruit BME280 y Adafruit Unified Sensor
*/

#include <Wire.h>
// #include <SoftwareSerial.h>
// #include <EEPROM.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME280.h>

// ============================================================
//  PINES
// ============================================================
const byte PIN_UV = A0;
const byte PIN_UV_REF = 39;
const byte PIN_CO = 34;

SoftwareSerial co2(2, 3);  // RX = 2, TX = 3

// ============================================================
//  UV (ML8511) - misma calibracion de tu sketch
// ============================================================
const float V_CERO = 1.0000;      // indice 0
const float V_INDICE_8 = 1.0600;  // indice 8
const float V_POR_INDICE = (V_INDICE_8 - V_CERO) / 8.0;

const float V_MODERADO = 1.0225;  // indice 3
const float V_ALTO = 1.0450;      // indice 6
const float V_MUY_ALTO = 1.0600;  // indice 8
const float V_EXTREMO = 1.0825;   // indice 11

const int CANTIDAD_LECTURAS = 256;
const float FILTRO = 0.30;
const float VCC_SENSOR = 3.3;
const float V_SATURACION = 2.8;

// ============================================================
//  MQ-7 (CO)
// ============================================================
const float TENSION_REFERENCIA = 5.0;
const int ADC_MAXIMO = 1023;
const float RL_KOHM = 10.0;               // se cancela en los ppm
const float RELACION_AIRE_LIMPIO = 27.5;  // Rs/R0 en aire limpio
const int MUESTRAS_CALIBRACION = 50;

const float CURVA_A = 99.042;
const float CURVA_B = -1.518;
const float FACTOR_AJUSTE = 1.0;  // ajustar con medidor de referencia

// R0 usado si no hay uno guardado en la EEPROM (reemplazar por el tuyo)
const float R0_POR_DEFECTO = 12.9;  // kOhm
const int DIR_EEPROM = 100;         // lejos de la 0 para no pisarlo
const byte MARCA = 0xA7;

// ============================================================
//  CO2 (MH-Z19C)
// ============================================================
const byte cmdLeer[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };
const byte cmdCero[9] = { 0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78 };
const byte cmdAbcOff[9] = { 0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86 };
const byte cmdAbcOn[9] = { 0xFF, 0x01, 0x79, 0xA0, 0x00, 0x00, 0x00, 0x00, 0xE6 };


//NOTE: PMS5003
SoftwareSerial pms(5, 6);  // RX, TX

const unsigned long CALENTAMIENTO = 30000;  // el sensor necesita ~30 s para estabilizar
const unsigned long PERIODO_PRINT = 1000;   // imprimir una vez por segundo

// Direcciones en EEPROM (cada float ocupa 4 bytes)
const int EE_OFFSET = 0;
const int EE_GANANCIA = 4;

struct Datos {
  uint16_t pm1_cf1, pm25_cf1, pm10_cf1;    // "standard particle" (ug/m3)
  uint16_t pm1, pm25, pm10;                // "atmospheric environment" (ug/m3)
  uint16_t n03, n05, n10, n25, n50, n100;  // partículas mayores a X um por 0,1 L de aire
};

float offsetPM25 = 0;
float gananciaPM25 = 1;
unsigned long ultimoPrint = 0;

uint16_t palabra(const byte *b, byte i) {
  return ((uint16_t)b[i] << 8) | b[i + 1];
}

// Lee una trama de 32 bytes, valida cabecera y checksum. Devuelve true si es válida.
bool leerTrama(Datos &d) {
  static byte buf[32];
  while (pms.available() >= 32) {
    if (pms.read() != 0x42) continue;  // buscar inicio de trama (descarta de a 1 byte)
    if (pms.peek() != 0x4D) continue;  // segundo byte de cabecera
    buf[0] = 0x42;
    pms.readBytes(buf + 1, 31);

    uint16_t suma = 0;
    for (byte i = 0; i < 30; i++) suma += buf[i];
    if (suma != palabra(buf, 30)) continue;  // trama corrupta

    d.pm1_cf1 = palabra(buf, 4);
    d.pm25_cf1 = palabra(buf, 6);
    d.pm10_cf1 = palabra(buf, 8);
    d.pm1 = palabra(buf, 10);
    d.pm25 = palabra(buf, 12);
    d.pm10 = palabra(buf, 14);
    d.n03 = palabra(buf, 16);
    d.n05 = palabra(buf, 18);
    d.n10 = palabra(buf, 20);
    d.n25 = palabra(buf, 22);
    d.n50 = palabra(buf, 24);
    d.n100 = palabra(buf, 26);
    return true;
  }
  return false;
}

float aplicarCalibracion(uint16_t crudo) {
  float v = ((float)crudo - offsetPM25) * gananciaPM25;
  return v < 0 ? 0 : v;
}

void calibrarCero() {
  if (millis() < CALENTAMIENTO) {
    Serial.println(F("Esperar el calentamiento del sensor (30 s)."));
    return;
  }
  Serial.println(F("Calibrando cero (aire limpio / filtrado)... 60 muestras"));
  float suma = 0;
  byte n = 0;
  Datos d;
  unsigned long inicio = millis();
  while (n < 60) {
    if (millis() - inicio > 120000UL) {
      Serial.println(F("\nTimeout: no llegan datos del sensor."));
      return;
    }
    if (leerTrama(d)) {
      suma += d.pm25;
      n++;
      Serial.print('.');
    }
  }
  offsetPM25 = suma / n;
  EEPROM.put(EE_OFFSET, offsetPM25);
  Serial.print(F("\nOffset PM2.5 = "));
  Serial.println(offsetPM25);
}

void calibrarGanancia(float ref) {
  if (millis() < CALENTAMIENTO) {
    Serial.println(F("Esperar el calentamiento del sensor (30 s)."));
    return;
  }
  Serial.println(F("Calibrando ganancia... 30 muestras"));
  float suma = 0;
  byte n = 0;
  Datos d;
  unsigned long inicio = millis();
  while (n < 30) {
    if (millis() - inicio > 60000UL) {
      Serial.println(F("Timeout: no llegan datos del sensor."));
      return;
    }
    if (leerTrama(d)) {
      suma += d.pm25 - offsetPM25;
      n++;
    }
  }
  float prom = suma / n;
  if (prom < 1) {
    Serial.println(F("Lectura muy baja: no se puede calcular la ganancia."));
    return;
  }
  gananciaPM25 = ref / prom;
  EEPROM.put(EE_GANANCIA, gananciaPM25);
  Serial.print(F("Ganancia PM2.5 = "));
  Serial.println(gananciaPM25, 3);
}

void procesarComando() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == 'z') {
    calibrarCero();
  } else if (c == 'g') {
    float ref = Serial.parseFloat();
    if (ref > 0) calibrarGanancia(ref);
    else Serial.println(F("Uso: g <valor de referencia en ug/m3>"));
  } else if (c == 'r') {
    offsetPM25 = 0;
    gananciaPM25 = 1;
    EEPROM.put(EE_OFFSET, offsetPM25);
    EEPROM.put(EE_GANANCIA, gananciaPM25);
    Serial.println(F("Calibración reseteada."));
  }
}


// ============================================================
//  UMBRALES (ajustables)
// ============================================================

// --- Riesgo de incendio: se evalua CO, CO2 y temperatura ---
const float FUEGO_CO_PPM = 35.0;  // nivel "preocupante" de tu tabla
const int FUEGO_CO2_PPM = 1500;   // muy por encima de un ambiente normal
const float FUEGO_TEMP_C = 40.0;

// --- Exposicion ambiental: puntos por variable (0, 1 o 2) ---
const float EXP_TEMP_MEDIA = 28.0;  // C
const float EXP_TEMP_ALTA = 35.0;
const float EXP_HUM_MEDIA = 60.0;  // %RH
const float EXP_HUM_ALTA = 80.0;

// ============================================================
//  VARIABLES GLOBALES
// ============================================================
Adafruit_BME280 bme;
bool bmeOK = false;

float temperatura = 0, humedad = 0, presion = 0;
int co2ppm = -1;  // -1 = sin lectura valida
float uvV = 0, uvIndice = 0;
bool uvPrimera = true;
float coRs = 0, coPPM = 0;
float R0 = R0_POR_DEFECTO;

unsigned long tiempoAnterior = 0;
const unsigned long INTERVALO = 2000;

// ============================================================
//  BME280
// ============================================================
byte leerID(byte dir) {
  Wire.beginTransmission(dir);
  Wire.write(0xD0);
  if (Wire.endTransmission() != 0) return 0;
  Wire.requestFrom(dir, (byte)1);
  return Wire.available() ? Wire.read() : 0;
}

void iniciarBME() {
  byte dir = 0;
  byte id = leerID(0x76);
  if (id != 0) dir = 0x76;
  else {
    id = leerID(0x77);
    if (id != 0) dir = 0x77;
  }

  if (dir == 0) {
    Serial.println(F("BME280: sin respuesta. Revisar VIN, GND, SDA y SCL."));
    return;
  }
  if (id == 0x58) {
    Serial.println(F("BME280: el chip es un BMP280 (no mide humedad)."));
    return;
  }
  if (id != 0x60) {
    Serial.println(F("BME280: chip desconocido."));
    return;
  }
  if (!bme.begin(dir)) {
    Serial.println(F("BME280: error al iniciar la libreria."));
    return;
  }

  bmeOK = true;
  Serial.print(F("BME280 listo en 0x"));
  Serial.println(dir, HEX);
}

// ====================== =====================================
//  CO2
// ============================================================
int leerCO2() {
  while (co2.available()) co2.read();
  co2.write(cmdLeer, 9);

  unsigned long inicio = millis();
  while (co2.available() < 9 && millis() - inicio < 200) {}
  if (co2.available() < 9) return -1;

  byte r[9];
  for (int i = 0; i < 9; i++) r[i] = co2.read();
  if (r[0] != 0xFF || r[1] != 0x86) return -1;

  byte suma = 0;
  for (int i = 1; i < 8; i++) suma += r[i];
  suma = (byte)(0xFF - suma + 1);
  if (suma != r[8]) return -1;  // lectura corrupta

  return r[2] * 256 + r[3];
}

// ============================================================
//  UV
// ============================================================
float leerTensionUV() {
  unsigned long sumaUV = 0, sumaRef = 0;
  for (int i = 0; i < CANTIDAD_LECTURAS; i++) {
    sumaUV += analogRead(PIN_UV);
    sumaRef += analogRead(PIN_UV_REF);
  }
  if (sumaRef == 0) return 0.0;
  return VCC_SENSOR * (float)sumaUV / (float)sumaRef;
}

void actualizarUV() {
  float v = leerTensionUV();
  if (uvPrimera) {
    uvV = v;
    uvPrimera = false;
  } else {
    uvV = uvV + FILTRO * (v - uvV);
  }

  uvIndice = (uvV - V_CERO) / V_POR_INDICE;
  if (uvIndice < 0) uvIndice = 0;
}

const __FlashStringHelper *nivelUV(float v) {
  if (v < V_MODERADO) return F("BAJO");
  if (v < V_ALTO) return F("MODERADO");
  if (v < V_MUY_ALTO) return F("ALTO");
  if (v < V_EXTREMO) return F("MUY ALTO");
  return F("EXTREMO");
}

// ============================================================
//  MQ-7 (CO)
// ============================================================
float leerTensionCO() {
  long suma = 0;
  for (int i = 0; i < 10; i++) {
    suma += analogRead(PIN_CO);
    delay(5);
  }
  return (suma / 10.0) * (TENSION_REFERENCIA / ADC_MAXIMO);
}

float calcularRs(float tension) {
  if (tension < 0.01) tension = 0.01;
  return RL_KOHM * (TENSION_REFERENCIA - tension) / tension;
}

float calcularPPM(float rs) {
  float ppm = CURVA_A * pow(rs / R0, CURVA_B) * FACTOR_AJUSTE;
  if (ppm < 0) ppm = 0;
  if (ppm > 2000) ppm = 2000;
  return ppm;
}

void calibrarCO() {
  Serial.println(F(">> Calibrando MQ-7 en aire limpio... no acerques gases ni humo."));
  float suma = 0;
  for (int i = 0; i < MUESTRAS_CALIBRACION; i++) {
    suma += calcularRs(leerTensionCO());
    delay(100);
  }
  R0 = (suma / MUESTRAS_CALIBRACION) / RELACION_AIRE_LIMPIO;

  //TODO: reemplazar eeprom con preferences
  // EEPROM.update(DIR_EEPROM, MARCA);
  EEPROM.put(DIR_EEPROM + 1, R0);

  Serial.print(F(">> Listo. R0 = "));
  Serial.print(R0, 2);
  Serial.println(F(" kOhm (guardado en EEPROM). Anotalo y ponlo en R0_POR_DEFECTO."));
}

void cargarR0() {
  float guardado = 0;
  if (EEPROM.read(DIR_EEPROM) == MARCA) {
    EEPROM.get(DIR_EEPROM + 1, guardado);
    if (guardado > 0.01 && guardado < 1000) {
      R0 = guardado;
      Serial.print(F("MQ-7: R0 cargado de EEPROM = "));
      Serial.print(R0, 2);
      Serial.println(F(" kOhm"));
      return;
    }
  }
  R0 = R0_POR_DEFECTO;
  Serial.print(F("MQ-7: usando R0 por defecto = "));
  Serial.print(R0, 2);
  Serial.println(F(" kOhm (envia 'c' en aire limpio para recalibrar)"));
}

const __FlashStringHelper *nivelCO(float ppm) {
  if (ppm < 1) return F("   se detecta CO");
  if (ppm < 10) return F("Baja - vigilar si se mantiene o aumenta");
  if (ppm < 35) return F("Requiere atencion (exposicion prolongada)");
  if (ppm < 100) return F("PREOCUPANTE - actuar y comprobar la fuente");
  return F("PELIGROSO - abandonar el ambiente y buscar ayuda");
}

// ============================================================
//  EVALUACIONES
// ============================================================

// Cuantas de las 3 variables de incendio estan elevadas (0 a 3)
int variablesDeIncendio(bool &coAlto, bool &co2Alto, bool &tempAlta) {
  coAlto = (coPPM >= FUEGO_CO_PPM);
  co2Alto = (co2ppm >= 0 && co2ppm >= FUEGO_CO2_PPM);
  tempAlta = (bmeOK && temperatura >= FUEGO_TEMP_C);
  return (int)coAlto + (int)co2Alto + (int)tempAlta;
}

// Puntos de exposicion ambiental (0 a 6)
int puntosExposicion() {
  int p = 0;

  if (bmeOK) {
    if (temperatura >= EXP_TEMP_ALTA) p += 2;
    else if (temperatura >= EXP_TEMP_MEDIA) p += 1;

    if (humedad >= EXP_HUM_ALTA) p += 2;
    else if (humedad >= EXP_HUM_MEDIA) p += 1;
  }

  if (uvV >= V_MUY_ALTO) p += 2;       // indice 8 o mas
  else if (uvV >= V_MODERADO) p += 1;  // indice 3 o mas

  return p;
}

// ============================================================
//  PRESENTACION
// ============================================================
void mostrarTodo() {
  Serial.println();
  Serial.println(F("================= LECTURAS ================="));

  // UV
  Serial.print(F("UV:           Vout "));
  Serial.print(uvV, 4);
  Serial.print(F(" V | Indice "));
  Serial.print(uvIndice, 1);
  Serial.print(F(" -> "));
  Serial.print(nivelUV(uvV));
  if (uvV > V_SATURACION) Serial.print(F(" | SATURADO"));
  Serial.println();

  // BME280
  if (bmeOK) {
    Serial.print(F("Temperatura:  "));
    Serial.print(temperatura, 2);
    Serial.println(F(" °C"));

    Serial.print(F("Humedad:      "));
    Serial.print(humedad, 2);
    Serial.println(F(" %RH"));
    Serial.print(F("Presion:      "));
    Serial.print(presion, 2);
    Serial.println(F(" hPa"));
  } else {
    Serial.println(F("Temp/Hum/Pres: sin datos (BME280 no disponible)"));
  }

  // CO2
  Serial.print(F("CO2:          "));
  if (co2ppm >= 0) {
    Serial.print(co2ppm);
    Serial.println(F(" ppm"));
  } else {
    Serial.println(F("sin lectura valida"));
  }

  // CO
  Serial.print(F("CO:           "));
  Serial.print(coPPM, 1);
  Serial.print(F(" ppm -> "));
  Serial.println(nivelCO(coPPM));

  Serial.println(F("--------------- EVALUACION ---------------"));

  // Incendio
  bool coAlto, co2Alto, tempAlta;
  int n = variablesDeIncendio(coAlto, co2Alto, tempAlta);

  Serial.print(F("INCENDIO:     "));
  if (n == 3) Serial.print(F("!!! INCENDIO / RIESGO DE INCENDIO ALTO !!!"));
  else if (n == 2) Serial.print(F("Riesgo de incendio MEDIO - vigilar"));
  else Serial.print(F("Sin riesgo de incendio"));
  Serial.print(F("  [CO:"));
  Serial.print(coAlto ? F("SI") : F("no"));
  Serial.print(F(" CO2:"));
  Serial.print(co2Alto ? F("SI") : F("no"));
  Serial.print(F(" Temp:"));
  Serial.print(tempAlta ? F("SI") : F("no"));
  Serial.println(F("]"));

  // Exposicion ambiental
  int p = puntosExposicion();
  Serial.print(F("EXPOSICION AMBIENTAL: "));
  if (p >= 4) Serial.println(F("ALTA"));
  else if (p >= 2) Serial.println(F("MEDIA"));
  else Serial.println(F("BAJA"));

  // Recomendacion UV
  if (uvV >= V_MODERADO) Serial.println(F("UV:           Indice UV elevado - se recomienda usar protector solar"));
  else Serial.println(F("UV:           Indice bajo - sin necesidad de protector solar"));

  Serial.println(F("============================================"));
}

//NOTE: arduino cloud
const char DEVICE_LOGIN_NAME[] = "44805e5e-0849-4cb8-a3dd-413a9df57c15";
//NOTE: aqui cambiar ssid por el nombre de la red wifi
//y pass es la contraseña
const char SSID[] = "Armor 24";                         // Network SSID (name)
const char PASS[] = "hola1111";                         // Network password (use for WPA, or use as key for WEP)
const char DEVICE_KEY[] = "RvY729qzdIG1wlMv1OGvyfEbx";  // Secret device password
//
bool estado_led;
int valor_variable;

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
void onEstadoLedChange();
void actualizar_int();
void ArduinoInit() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  //NOTE: aqui añadir en addproperty cada variable resultante de un sensor
  ArduinoCloud.addProperty(estado_led, READWRITE, ON_CHANGE, onEstadoLedChange);
  ArduinoCloud.addProperty(valor_variable, READWRITE, ON_CHANGE, actualizar_int);
}

//NOTE: Sensor GPS localizacion
TinyGPSPlus gps;
#define GPSRX 16
#define GPSTX 17
void onEstadoLedChange() {
  digitalWrite(2, estado_led ? HIGH : LOW);
}
void actualizar_int() {
  Serial.println(valor_variable);
}


// ============================================================
//  PROGRAMA
// ============================================================
void setup() {
  Serial.begin(9600);
  co2.begin(9600);

  pinMode(PIN_UV, INPUT);
  pinMode(PIN_UV_REF, INPUT);
  pinMode(PIN_CO, INPUT);

  Serial.println(F("MONITOR AMBIENTAL - UV | BME280 | CO2 | CO"));
  Serial.println(F("Comandos: c = calibrar CO | z = cero CO2 | a/b = ABC CO2 apagado/encendido"));
  Serial.println();

  Wire.begin();
  iniciarBME();
  cargarR0();
  Serial.println();
  pms.begin(9600);

  // Recuperar calibración guardada
  EEPROM.get(EE_OFFSET, offsetPM25);
  EEPROM.get(EE_GANANCIA, gananciaPM25);
  if (isnan(offsetPM25) || isnan(gananciaPM25) || gananciaPM25 <= 0) {
    offsetPM25 = 0;  // EEPROM virgen o datos inválidos: valores por defecto
    gananciaPM25 = 1;
  }

  Serial.println(F("PMS5003 listo. Calentando 30 s..."));
  Serial.print(F("Calibración cargada: offset = "));
  Serial.print(offsetPM25);
  Serial.print(F(" | ganancia = "));
  Serial.println(gananciaPM25, 3);

  Serial2.begin(9600, SERIAL_8N1, GPSRX, GPSTX);
  Serial.println("ya pasamos el inico");
  pinMode(2, OUTPUT);
  ArduinoInit();
  // Connect to Arduino IoT Cloud
}

void loop() {
  // Comandos desde el Monitor Serie
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'c' || c == 'C') calibrarCO();
    if (c == 'z') {
      co2.write(cmdCero, 9);
      Serial.println(F(">> CO2: cero calibrado (solo valido al aire libre, 400 ppm)"));
    }
    if (c == 'a') {
      co2.write(cmdAbcOff, 9);
      Serial.println(F(">> CO2: ABC apagado"));
    }
    if (c == 'b') {
      co2.write(cmdAbcOn, 9);
      Serial.println(F(">> CO2: ABC encendido"));
    }
  }

  if (millis() - tiempoAnterior >= INTERVALO) {
    tiempoAnterior = millis();

    // Leer todos los sensores
    actualizarUV();

    if (bmeOK) {
      temperatura = bme.readTemperature();
      humedad = bme.readHumidity();
      presion = bme.readPressure() / 100.0F;
    }

    co2ppm = leerCO2();

    coRs = calcularRs(leerTensionCO());
    coPPM = calcularPPM(coRs);

    mostrarTodo();
  }
  procesarComando();

  Datos d;
  if (!leerTrama(d)) return;
  if (millis() < CALENTAMIENTO) return;
  if (millis() - ultimoPrint < PERIODO_PRINT) return;
  ultimoPrint = millis();

  Serial.print(F("PM1.0: "));
  Serial.print(d.pm1);
  Serial.print(F(" | PM2.5 crudo: "));
  Serial.print(d.pm25);
  Serial.print(F(" | PM2.5 calibrado: "));
  Serial.print(aplicarCalibracion(d.pm25), 1);
  Serial.print(F(" | PM10: "));
  Serial.print(d.pm10);
  Serial.println(F(" ug/m3"));

  Serial.print(F("Particulas/0,1L  >0.3um: "));
  Serial.print(d.n03);
  Serial.print(F("  >0.5: "));
  Serial.print(d.n05);
  Serial.print(F("  >1.0: "));
  Serial.print(d.n10);
  Serial.print(F("  >2.5: "));
  Serial.print(d.n25);
  Serial.print(F("  >5.0: "));
  Serial.print(d.n50);
  Serial.print(F("  >10: "));
  Serial.println(d.n100);
  Serial.println();

  //NOTE: Sensor GPS
  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }

  if (gps.location.isUpdated() && gps.location.isValid()) {

    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);

    Serial.print("Altitude: ");
    if (gps.altitude.isValid()) {
      Serial.print(gps.altitude.meters());
      Serial.println(" m");
    } else {
      Serial.println("Waiting");
    }
    Serial.print("asdasdasd: ");
  }


  //NOTE: Arduino
  ArduinoCloud.update();
}
