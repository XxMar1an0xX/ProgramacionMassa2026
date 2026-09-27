#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define LED_PIN 2
#define LED_Ojos 5

#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define BUTTON_CHAR_UUID "12345678-1234-1234-1234-123456789abd"

BLECharacteristic *buttonCharacteristic;

void apagado_gradual() {
  for (int brillo = 255; brillo > 1; brillo--) {
    analogWrite(LED_Ojos, brillo);
  }
  digitalWrite(LED_Ojos, LOW);
}
void prendido_gradual() {
  for (int brillo = 0; brillo < 255; brillo++) {
    analogWrite(LED_Ojos, brillo);
  }
  digitalWrite(LED_Ojos, HIGH);
}

class ButtonCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String value = characteristic->getValue();

    if (value.length() > 0) {
      if (value[0] == 1) {
        digitalWrite(LED_PIN, HIGH);
        prendido_gradual();
        Serial.println("ON");
      } else {
        digitalWrite(LED_PIN, LOW);
        apagado_gradual();
        Serial.println("OFF");
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_Ojos, OUTPUT);

  BLEDevice::init("ESP32-Button");

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

void loop() {
}
