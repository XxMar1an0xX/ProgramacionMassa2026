// Efecto "1, 2, 3" con 3 tiras LED azules
// ESP32 + IRF540 (uno por tira, con resistencia en el gate)
// Esta versión imprime por el Monitor Serie el motivo del último reinicio.

const int TIRA1 = 25;  // GPIO25 -> gate IRF540 tira 1
const int TIRA2 = 26;  // GPIO26 -> gate IRF540 tira 2
const int TIRA3 = 27;  // GPIO27 -> gate IRF540 tira 3

const int tiras[] = {TIRA1, TIRA2, TIRA3};
const int CANTIDAD = 3;

const unsigned long TIEMPO_ENCENDIDA = 500;  // ms que dura prendida cada tira

const char* motivoReinicio() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return "Encendido normal (power on)";
    case ESP_RST_SW:       return "Reinicio por software";
    case ESP_RST_PANIC:    return "Error del programa (panic / excepción)";
    case ESP_RST_INT_WDT:  return "Watchdog de interrupciones";
    case ESP_RST_TASK_WDT: return "Watchdog de tarea (el loop se colgó)";
    case ESP_RST_WDT:      return "Watchdog";
    case ESP_RST_BROWNOUT: return "BROWNOUT: cae la tensión de alimentación";
    default:               return "Otro motivo";
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.print("Motivo del ultimo reinicio: ");
  Serial.println(motivoReinicio());

  for (int i = 0; i < CANTIDAD; i++) {
    pinMode(tiras[i], OUTPUT);
    digitalWrite(tiras[i], LOW);
  }
}

void loop() {
  for (int i = 0; i < CANTIDAD; i++) {
    digitalWrite(tiras[i], HIGH);   // prende la tira
    delay(TIEMPO_ENCENDIDA);
    digitalWrite(tiras[i], LOW);    // apaga la tira
  }
}
