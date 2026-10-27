#include <TinyGPSPlus.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char DEVICE_LOGIN_NAME[] = "44805e5e-0849-4cb8-a3dd-413a9df57c15";

const char SSID[] = "Armor 24";                         // Network SSID (name)
const char PASS[] = "hola1111";                         // Network password (use for WPA, or use as key for WEP)
const char DEVICE_KEY[] = "RvY729qzdIG1wlMv1OGvyfEbx";  // Secret device password

// bool estado_led = true;

void initProperties() {

  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

TinyGPSPlus gps;

#define GPSRX 16
#define GPSTX 17
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, GPSRX, GPSTX);
  Serial.println("ya pasamos el inico");
  pinMode(2, OUTPUT);
  initProperties();
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(2);
  ArduinoCloud.addProperty(estado_led, READWRITE, ON_CHANGE, onEstadoLedChange);
  bugInfo();
}

void loop() {
  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
    // Serial.write(Serial2.read());
  }



  // delay(500);
  // Serial.println(gps.read)
  // Serial.print("Latitud: ");
  // Serial.println(gps.location.lat(), 6);
  // Serial.print("Longitud: ");
  // Serial.println(gps.location.lng(), 6);
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

    // delay(500);
    // Serial.println(gps.satellites.value());
    //
  }
  ArduinoCloud.update();
}

void onLedChange() {
  digitalWrite(2, led ? HIGH : LOW);
}
