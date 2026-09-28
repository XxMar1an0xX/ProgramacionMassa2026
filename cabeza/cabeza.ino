#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <vector>


#define LED_PIN 5

#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define BUTTON_CHAR_UUID "12345678-1234-1234-1234-123456789abd"

BLECharacteristic *buttonCharacteristic;

String estado_boton;

struct Tiempos {
  int duracion;
  bool estado_led;
};

class ButtonCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    estado_boton = characteristic->getValue();

    if (estado_boton.length() > 0) {
      if (estado_boton[0] == 1) {
        Serial.println("ON");
      } else {
        digitalWrite(LED_PIN, LOW);
        Serial.println("OFF");
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  BLEDevice::init("Cabeza");

  BLEServer *server = BLEDevice::createServer();
  BLEService *service = server->createService(SERVICE_UUID);

  buttonCharacteristic = service->createCharacteristic(
    BUTTON_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  buttonCharacteristic->setValue((uint8_t)0);
  buttonCharacteristic->setCallbacks(new ButtonCallback());

  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->start();

  Serial.println("BLE ready");
}

std::vector<Tiempos> frase;
void loop() {
  for (int palabras = random(5, 13); palabras > 0 && estado_boton[0] == 1; palabras--) {
    frase.push_back({ random(250, 400), true });
    frase.push_back({ random(50, 150), false });
  }
  for (Tiempos seccion : frase) {
    if (estado_boton[0] != 1) {
      break;
    }
    digitalWrite(LED_PIN, seccion.estado_led);
    delay(seccion.duracion);
  }
  delay(random(400, 700));
  frase.clear();
}
