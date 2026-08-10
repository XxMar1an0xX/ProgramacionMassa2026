
#include <Wire.h>
const int a0 = 2, b0 = 3, c0 = 4, d0 = 5, e0 = 6, f0 = 7, g0 = 8;        //DIGITO UNIDAD
const int h1 = 9, b1 = 10, c1 = 11, d1 = 12, e1 = 13, f1 = A1, g1 = A0;  //DIGITO DECENA

#define dia 86400000

#define horainicio 10
unsigned long tiempoInicio;
unsigned long intervalo = dia;  //86 400 000 son los segundos que hay en el dia
String stringcuenta;
char charcuenta[3];
byte chardecena, charunidad;


int cuenta = 60;
unsigned long tiemporestante = 0;
void setup() {
  Serial.begin(9600);
  tiempoInicio = millis();
  pinMode(a0, OUTPUT);
  pinMode(b0, OUTPUT);
  pinMode(c0, OUTPUT);
  pinMode(d0, OUTPUT);
  pinMode(e0, OUTPUT);
  pinMode(f0, OUTPUT);
  pinMode(g0, OUTPUT);

  pinMode(h1, OUTPUT);
  pinMode(b1, OUTPUT);
  pinMode(c1, OUTPUT);
  pinMode(d1, OUTPUT);
  pinMode(e1, OUTPUT);
  pinMode(f1, OUTPUT);
  pinMode(g1, OUTPUT);
  pinMode(A2, INPUT_PULLUP);  //BOTON
  stringcuenta = String(cuenta);
  stringcuenta.toCharArray(charcuenta, 3);
  chardecena = charcuenta[0] - '0';
  charunidad = charcuenta[1] - '0';
  //
  decena8();  //para que muestre en el digito decena el numero 8, una vez activado
  unidad8();  //para que muestre en el digito unidad el numero 8, una vez activado
  delay(800);
  unidad6();
  decena6();
  Serial.println("termina setup");
  Serial.println(millis());
  // tiempoInicio += (86400000 / 24 * 5);
  Serial.println(tiempoInicio);
  intervalo = (dia / 24 * (24 - horainicio));
}

void loop() {
  decena();
  unidad();
  delay(100);
  Serial.println(millis());
  Serial.println(tiempoInicio + intervalo);
  if (millis() >= intervalo + tiempoInicio) {
    intervalo = dia;
    tiempoInicio = millis();
    cuenta = cuenta - 1;
    if (cuenta > 9) {
      stringcuenta = String(cuenta);
      stringcuenta.toCharArray(charcuenta, 3);
      chardecena = charcuenta[0] - '0';
      charunidad = charcuenta[1] - '0';
      decena();
      unidad();
    }
    if (cuenta <= 9 && cuenta > 0) {
      stringcuenta = String(cuenta);
      stringcuenta.toCharArray(charcuenta, 3);
      chardecena = 0;
      charunidad = charcuenta[0] - '0';
      unidad();
      decena();
    }
    while (cuenta <= 0) {
      chardecena = 0;
      charunidad = 0;
      decena();
      unidad();
      delay(500);
      decenaoff();
      unidadoff();
      delay(500);
    }
  }
  if (millis() <= 499) {
    tiempoInicio = millis();
    intervalo = tiemporestante;
    delay(500);
  } else {
    tiemporestante = (intervalo + tiempoInicio) - millis();
  }
}






void decena() {
  switch (chardecena) {
    case 0: decena0(); break;
    case 1: decena1(); break;
    case 2: decena2(); break;
    case 3: decena3(); break;
    case 4: decena4(); break;
    case 5: decena5(); break;
    case 6: decena6(); break;
    case 7: decena7(); break;
    case 8: decena8(); break;
    case 9: decena9(); break;
  }
}

void unidad() {
  switch (charunidad) {
    case 0: unidad0(); break;
    case 1: unidad1(); break;
    case 2: unidad2(); break;
    case 3: unidad3(); break;
    case 4: unidad4(); break;
    case 5: unidad5(); break;
    case 6: unidad6(); break;
    case 7: unidad7(); break;
    case 8: unidad8(); break;
    case 9: unidad9(); break;
  }
}


void decenaoff() {
  digitalWrite(h1, LOW);
  digitalWrite(b1, LOW);
  digitalWrite(c1, LOW);
  digitalWrite(d1, LOW);
  digitalWrite(e1, LOW);
  digitalWrite(f1, LOW);
  digitalWrite(g1, LOW);
  Serial.println("off");
}
void unidadoff() {
  digitalWrite(a0, LOW);
  digitalWrite(b0, LOW);
  digitalWrite(c0, LOW);
  digitalWrite(d0, LOW);
  digitalWrite(e0, LOW);
  digitalWrite(f0, LOW);
  digitalWrite(g0, LOW);
  Serial.println("off");
}


void decena0() {
  digitalWrite(h1, HIGH);
  digitalWrite(b1, HIGH);
  digitalWrite(c1, HIGH);
  digitalWrite(d1, HIGH);
  digitalWrite(e1, HIGH);
  digitalWrite(f1, HIGH);
  digitalWrite(g1, LOW);
}
void decena1() {
  digitalWrite(h1, LOW);
  digitalWrite(b1, HIGH);
  digitalWrite(c1, HIGH);
  digitalWrite(d1, LOW);
  digitalWrite(e1, LOW);
  digitalWrite(f1, LOW);
  digitalWrite(g1, LOW);
}
void decena2() {
  digitalWrite(h1, HIGH);
  digitalWrite(b1, HIGH);
  digitalWrite(c1, LOW);
  digitalWrite(d1, HIGH);
  digitalWrite(e1, HIGH);
  digitalWrite(f1, LOW);
  digitalWrite(g1, HIGH);
}
void decena3() {
  digitalWrite(h1, HIGH);
  digitalWrite(b1, HIGH);
  digitalWrite(c1, HIGH);
  digitalWrite(d1, HIGH);
  digitalWrite(e1, LOW);
  digitalWrite(f1, LOW);
  digitalWrite(g1, HIGH);
}
void decena4() {
  digitalWrite(h1, LOW);
  digitalWrite(b1, HIGH);
  digitalWrite(c1, HIGH);
  digitalWrite(d1, LOW);
  digitalWrite(e1, LOW);
  digitalWrite(f1, HIGH);
  digitalWrite(g1, HIGH);
}
void decena5() {
  digitalWrite(h1, HIGH);
  digitalWrite(b1, LOW);
  digitalWrite(c1, HIGH);
  digitalWrite(d1, HIGH);
  digitalWrite(e1, LOW);
  digitalWrite(f1, HIGH);
  digitalWrite(g1, HIGH);
}
void decena6() {
  digitalWrite(h1, HIGH);
  digitalWrite(b1, LOW);
  digitalWrite(c1, HIGH);
  digitalWrite(d1, HIGH);
  digitalWrite(e1, HIGH);
  digitalWrite(f1, HIGH);
  digitalWrite(g1, HIGH);
}
void decena7() {
  digitalWrite(h1, HIGH);
  digitalWrite(b1, HIGH);
  digitalWrite(c1, HIGH);
  digitalWrite(d1, LOW);
  digitalWrite(e1, LOW);
  digitalWrite(f1, LOW);
  digitalWrite(g1, LOW);
}
void decena8() {
  digitalWrite(h1, HIGH);
  digitalWrite(b1, HIGH);
  digitalWrite(c1, HIGH);
  digitalWrite(d1, HIGH);
  digitalWrite(e1, HIGH);
  digitalWrite(f1, HIGH);
  digitalWrite(g1, HIGH);
}
void decena9() {
  digitalWrite(h1, HIGH);
  digitalWrite(b1, HIGH);
  digitalWrite(c1, HIGH);
  digitalWrite(d1, LOW);
  digitalWrite(e1, LOW);
  digitalWrite(f1, HIGH);
  digitalWrite(g1, HIGH);
}


void unidad0() {
  digitalWrite(a0, HIGH);
  digitalWrite(b0, HIGH);
  digitalWrite(c0, HIGH);
  digitalWrite(d0, HIGH);
  digitalWrite(e0, HIGH);
  digitalWrite(f0, HIGH);
  digitalWrite(g0, LOW);
}
void unidad1() {
  digitalWrite(a0, LOW);
  digitalWrite(b0, HIGH);
  digitalWrite(c0, HIGH);
  digitalWrite(d0, LOW);
  digitalWrite(e0, LOW);
  digitalWrite(f0, LOW);
  digitalWrite(g0, LOW);
}
void unidad2() {
  digitalWrite(a0, HIGH);
  digitalWrite(b0, HIGH);
  digitalWrite(c0, LOW);
  digitalWrite(d0, HIGH);
  digitalWrite(e0, HIGH);
  digitalWrite(f0, LOW);
  digitalWrite(g0, HIGH);
}
void unidad3() {
  digitalWrite(a0, HIGH);
  digitalWrite(b0, HIGH);
  digitalWrite(c0, HIGH);
  digitalWrite(d0, HIGH);
  digitalWrite(e0, LOW);
  digitalWrite(f0, LOW);
  digitalWrite(g0, HIGH);
}
void unidad4() {
  digitalWrite(a0, LOW);
  digitalWrite(b0, HIGH);
  digitalWrite(c0, HIGH);
  digitalWrite(d0, LOW);
  digitalWrite(e0, LOW);
  digitalWrite(f0, HIGH);
  digitalWrite(g0, HIGH);
}
void unidad5() {
  digitalWrite(a0, HIGH);
  digitalWrite(b0, LOW);
  digitalWrite(c0, HIGH);
  digitalWrite(d0, HIGH);
  digitalWrite(e0, LOW);
  digitalWrite(f0, HIGH);
  digitalWrite(g0, HIGH);
}
void unidad6() {
  digitalWrite(a0, HIGH);
  digitalWrite(b0, LOW);
  digitalWrite(c0, HIGH);
  digitalWrite(d0, HIGH);
  digitalWrite(e0, HIGH);
  digitalWrite(f0, HIGH);
  digitalWrite(g0, HIGH);
}
void unidad7() {
  digitalWrite(a0, HIGH);
  digitalWrite(b0, HIGH);
  digitalWrite(c0, HIGH);
  digitalWrite(d0, LOW);
  digitalWrite(e0, LOW);
  digitalWrite(f0, LOW);
  digitalWrite(g0, LOW);
}
void unidad8() {
  digitalWrite(a0, HIGH);
  digitalWrite(b0, HIGH);
  digitalWrite(c0, HIGH);
  digitalWrite(d0, HIGH);
  digitalWrite(e0, HIGH);
  digitalWrite(f0, HIGH);
  digitalWrite(g0, HIGH);
}
void unidad9() {
  digitalWrite(a0, HIGH);
  digitalWrite(b0, HIGH);
  digitalWrite(c0, HIGH);
  digitalWrite(d0, LOW);
  digitalWrite(e0, LOW);
  digitalWrite(f0, HIGH);
  digitalWrite(g0, HIGH);
}
