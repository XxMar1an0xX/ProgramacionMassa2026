
bool anterior_estado = false;
int cara = 0;
void setup() {
  pinMode(4, INPUT_PULLUP);
  Serial.begin(115200);
}

void loop() {
  if (!digitalRead(4) && anterior_estado != digitalRead(4)) {
    cara = (cara + 1) % 3;
    Serial.println(cara + 1);
  }

  anterior_estado = digitalRead(4);
  delay(15);
}
