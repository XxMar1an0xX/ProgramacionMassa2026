#include <TinyGPSPlus.h>
TinyGPSPlus gps;

#define GPSRX 16
#define GPSTX 17
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, GPSRX, GPSTX);
  Serial.println("ya pasamos el inico");
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
    Serial.println(gps.time.value());
  }
  // delay(500);
  // Serial.println(gps.satellites.value());
}
