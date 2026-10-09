#include <FastLED.h>
#include <NimBLEDevice.h>

// ================== CONFIGURACIÓN BLE ==================
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// ================== ISLAS ==================
#define NUM_ISLAS 7
const uint8_t PIN_ISLA[NUM_ISLAS] = { 13, 12, 14, 25, 26, 33, 32 };

// ================== SECUENCIALES (1 pin, 3 tiras encadenadas) ==================
#define TIPO_LED WS2812B
#define ORDEN_COLOR GRB  // si los colores salen cambiados, probá RGB o BRG
#define PIN_SEC 4
#define LEDS_POR_TRAMO 12
#define NUM_TRAMOS_SEC 3
#define LEDS_TOTALES_SEC (LEDS_POR_TRAMO * NUM_TRAMOS_SEC)

CRGB ledsSec[LEDS_TOTALES_SEC];

// ================== TRAMOS DE RESINA (3 tramos x 2 pines) ==================
#define PINES_POR_TRAMO_RES 2
const uint8_t PIN_RES[3 * PINES_POR_TRAMO_RES] = { 16, 17, 19, 21, 22, 23 };

// ================== FADE DE COLOR ==================
#define FADE_K 0.2f      // fracción que se acerca al objetivo en cada paso
#define FADE_STEP_MS 30  // ms entre pasos del fade

#define BLINK_MS 1000

// ================== ESTADO ==================
uint8_t pasoActual = 0;

bool estadoBlink = false;
unsigned long ultimoCambioBlink = 0;

volatile char comandoPendiente = 0;

CRGB colorActualSec = CRGB(255, 255, 255);
CRGB colorObjetivoSec = CRGB(255, 255, 255);
bool animandoColor = false;
unsigned long ultimoPasoFade = 0;

// Color objetivo de la tira secuencial para cada paso (isla 1..7).
// Los valores RGB dependen de ORDEN_COLOR, por eso no siempre coinciden con el color "real".
const CRGB COLOR_PASO[NUM_ISLAS] = {
  CRGB(200, 200, 200),  // isla 1: blanco
  CRGB(200, 200, 200),  // isla 2: blanco
  CRGB(200, 0, 100),    // isla 3
  CRGB(200, 0, 50),     // isla 4
  CRGB(200, 0, 0),      // isla 5: rojo puro
  CRGB(200, 0, 0),      // isla 6: rojo puro
  CRGB(200, 0, 0)       // isla 7: rojo puro
};

// ================== PROTOTIPOS ==================
void avanzar();
void retroceder();
void llegarAPaso(const char* msg);
void actualizarBlink();
void actualizarFadeColor();
void animarTramo(uint8_t idx);
void apagarTramo(uint8_t idx);
int pinResina(uint8_t idx, int p);
uint8_t acercarCanal(uint8_t actual, uint8_t objetivo);

// ================== CALLBACKS BLE (NimBLE 2.x) ==================
class MyCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    auto rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      char c = tolower(rxValue.data()[0]);
      if (c == 'a' || c == 'r') comandoPendiente = c;
    }
  }
};

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_ISLAS; i++) {
    pinMode(PIN_ISLA[i], OUTPUT);
    digitalWrite(PIN_ISLA[i], LOW);
  }
  for (int i = 0; i < 3 * PINES_POR_TRAMO_RES; i++) {
    pinMode(PIN_RES[i], OUTPUT);
    digitalWrite(PIN_RES[i], LOW);
  }

  FastLED.addLeds<TIPO_LED, PIN_SEC, ORDEN_COLOR>(ledsSec, LEDS_TOTALES_SEC);
  FastLED.setBrightness(180);
  FastLED.clear();
  FastLED.show();

  // ================== BLE ==================
  NimBLEDevice::init("Mapa_7_Islas");
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->advertiseOnDisconnect(true);  // reanuda el advertising solo al desconectarse

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  // NimBLE agrega solo el descriptor 2902 a las características con NOTIFY
  pService->createCharacteristic(CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY);

  // WRITE_NR permite que apps que escriben "sin respuesta" también funcionen
  pService->createCharacteristic(
            CHARACTERISTIC_UUID_RX,
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR)
    ->setCallbacks(new MyCallbacks());

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->enableScanResponse(true);
  NimBLEDevice::startAdvertising();

  Serial.println("BLE listo. Buscá 'Mapa_7_Islas' desde la app.");

  // Isla 1 parpadeando
  llegarAPaso("Mapa 7 islas listo → Isla ");
}

// ================== LOOP ==================
void loop() {
  actualizarBlink();
  actualizarFadeColor();

  char c = comandoPendiente;
  if (c != 0) {
    comandoPendiente = 0;
    if (c == 'a') {
      Serial.println("BLE: AVANZAR");
      avanzar();
    } else {
      Serial.println("BLE: RETROCEDER");
      retroceder();
    }
  }

  delay(10);
}

// ================== BLINK ==================
void actualizarBlink() {
  unsigned long ahora = millis();
  if (ahora - ultimoCambioBlink >= BLINK_MS) {
    estadoBlink = !estadoBlink;
    ultimoCambioBlink = ahora;
    digitalWrite(PIN_ISLA[pasoActual], estadoBlink ? HIGH : LOW);
  }
}

// ================== AVANZAR / RETROCEDER ==================
void avanzar() {
  if (pasoActual >= NUM_ISLAS - 1) {
    Serial.println("Ya está en isla 7. Usá 'r' para retroceder.");
    return;
  }
  digitalWrite(PIN_ISLA[pasoActual], HIGH);
  animarTramo(pasoActual);
  pasoActual++;
  llegarAPaso("Avanzar → Isla ");
}

void retroceder() {
  if (pasoActual == 0) {
    Serial.println("Ya está en el inicio (isla 1)");
    return;
  }
  digitalWrite(PIN_ISLA[pasoActual], LOW);
  apagarTramo(pasoActual - 1);
  pasoActual--;
  llegarAPaso("Retroceder → Isla ");
}

// Cierre común: isla nueva parpadeando + fade hacia el color del paso
void llegarAPaso(const char* msg) {
  digitalWrite(PIN_ISLA[pasoActual], HIGH);
  estadoBlink = true;
  ultimoCambioBlink = millis();

  colorObjetivoSec = COLOR_PASO[pasoActual];
  animandoColor = true;
  ultimoPasoFade = millis();

  Serial.print(msg);
  Serial.println(pasoActual + 1);
}

// ================== EFECTOS ==================
// Tramos 0..2 = secuenciales; 3..5 = resina
int pinResina(uint8_t idx, int p) {
  return PIN_RES[(idx - NUM_TRAMOS_SEC) * PINES_POR_TRAMO_RES + p];
}

void apagarTramo(uint8_t idx) {
  if (idx < NUM_TRAMOS_SEC) {
    fill_solid(&ledsSec[idx * LEDS_POR_TRAMO], LEDS_POR_TRAMO, CRGB::Black);
    FastLED.show();
  } else {
    for (int p = 0; p < PINES_POR_TRAMO_RES; p++)
      digitalWrite(pinResina(idx, p), LOW);
  }
}

void animarTramo(uint8_t idx) {
  apagarTramo(idx);
  delay(120);

  if (idx < NUM_TRAMOS_SEC) {
    for (int i = 0; i < LEDS_POR_TRAMO; i += 2) {
      fill_solid(&ledsSec[idx * LEDS_POR_TRAMO + i], min(2, LEDS_POR_TRAMO - i), colorActualSec);
      FastLED.show();
      delay(180);
    }
  } else {
    for (int p = 0; p < PINES_POR_TRAMO_RES; p++) {
      digitalWrite(pinResina(idx, p), HIGH);
      delay(280);
    }
  }
  delay(200);
}

// ================== FADE DE COLOR ==================
// Mueve un canal un FADE_K hacia el objetivo, pero siempre al menos 1 unidad,
// así llega exactamente al valor final.
uint8_t acercarCanal(uint8_t actual, uint8_t objetivo) {
  int diff = (int)objetivo - (int)actual;
  if (diff == 0) return actual;
  int paso = (int)(diff * FADE_K);  // se trunca hacia 0
  if (paso == 0) paso = (diff > 0) ? 1 : -1;
  return (uint8_t)((int)actual + paso);
}

void actualizarFadeColor() {
  if (!animandoColor || millis() - ultimoPasoFade < FADE_STEP_MS) return;
  ultimoPasoFade = millis();

  for (uint8_t i = 0; i < 3; i++)
    colorActualSec.raw[i] = acercarCanal(colorActualSec.raw[i], colorObjetivoSec.raw[i]);

  if (colorActualSec == colorObjetivoSec) animandoColor = false;

  // Tramos activos = 0 .. min(pasoActual, NUM_TRAMOS_SEC)-1 (bloque contiguo desde el inicio)
  uint8_t activos = (pasoActual < NUM_TRAMOS_SEC) ? pasoActual : NUM_TRAMOS_SEC;
  fill_solid(ledsSec, activos * LEDS_POR_TRAMO, colorActualSec);
  FastLED.show();
}

// FIN DEL CÓDIGO
