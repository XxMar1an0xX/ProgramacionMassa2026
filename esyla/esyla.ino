/*
  =========================================================
  OJOS + BARRIDO (pin 19) + LATIDO/CARGA/EXPLOSION/26 (pin 21)
  ESP32 + WS2811
  =========================================================
  OJOS: pines 2, 4, 16, 17, 5, 18  (6 cabezas).
    MUERTE: el ojo parpadea rapido, se va poniendo rojo y despues de
    unos segundos se apaga. Velocidad del parpadeo, tiempo en volverse
    rojo y duracion total se controlan desde la web.

  PIN 19 (cuadrantes + 2 tiras de 24 en serie):
    - Modo CARGA: el barrido recorre los 27 pasos de los cuadrantes y
      SIGUE en cascada por las 2 tiras nuevas: se prenden juntos el
      segmento 1 y el 25, luego el 2 y el 26, luego el 3 y el 27... hasta
      el 24 y el 48. La descarga vuelve en sentido inverso.
    - Modo DOBLE y NORMAL tambien recorren cuadrantes + tiras.
    - Color de los cuadrantes y color de las tiras editables por separado.

  PIN 21 (ciclo completo):
    FASE 1 - LATIDO CON CARGA (30 segmentos), SIMETRICA: desde el centro
             hacia los extremos (15->1 y 16->30 a la vez).
    FASE 2 - CARGA + EXPLOSION (29 segmentos), con los 30 latiendo.
    FASE 3 - DESCARGA de los 29 en dos frentes sincronizados:
             del segmento 15 al 1 y del 16 al 29.
    FASE 4 - DESCARGA CON LATIDO en los 30, SIMETRICA: desde los extremos
             hacia el centro (1->15 y 30->16 a la vez).
    Luego se repite el ciclo. El 26 respira constantemente.

  GUARDADO: desde la web se pueden guardar los parametros de cada
  seccion en la flash (Preferences) y quedan al reiniciar.

  DIAGNOSTICO: al arrancar imprime por serie el motivo del ultimo
  reinicio y sanea los valores leidos de la flash.

  WiFi: "sahur" / 12345678  ->  http://192.168.4.1
  =========================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <Preferences.h>
#include <esp_system.h>   // para esp_reset_reason

Preferences prefs;

const char* AP_SSID     = "sahur";
const char* AP_PASSWORD = "12345678";

WebServer server(80);

// =========================================================
// ================= SECCION OJOS =========================
// =========================================================

#define LED_TYPE_OJOS        WS2811
#define COLOR_ORDER_OJOS     BRG

#define SECCIONES_POR_OJO    4
#define OJOS_POR_PIN         2
#define LEDS_POR_PIN_OJOS    (SECCIONES_POR_OJO * OJOS_POR_PIN)   // 8
#define NUM_PINES_OJOS       6    // pines 2, 4, 16, 17, 5, 18

CRGB ledsOjos[NUM_PINES_OJOS][LEDS_POR_PIN_OJOS];

CRGB COLOR_BLANCO  = CRGB::White;
CRGB COLOR_CENTRO  = CRGB(255, 170, 0);

bool PATRON_OJO[SECCIONES_POR_OJO] = { true, false, false, true };
bool INVERTIR_SEGUNDO_OJO = false;

uint8_t  brilloGlobalOjos  = 180;
bool     animacionActiva   = true;
float    velocidadPctOjos  = 100.0;
bool     forzarBlink       = false;

float tiempoEntreParpadeosSeg = 4.5;

const float VELOCIDAD_MUERTE = 20.0;

bool quedarApagadoPin[NUM_PINES_OJOS]    = { false, false, false, false, false, false };
bool usarVelocidadMuerte[NUM_PINES_OJOS] = { false, false, false, false, false, false };

// --- Efecto MUERTE (editables desde la web) ---
float muerteDuracionSeg   = 3.0;   // tiempo total hasta apagarse
float muerteTiempoRojoSeg = 1.5;   // tiempo en pasar a rojo
float muerteParpadeoMs    = 80;    // ms por cambio on/off (menor = mas rapido)

bool muriendo[NUM_PINES_OJOS]               = { false, false, false, false, false, false };
unsigned long tInicioMuerte[NUM_PINES_OJOS] = { 0, 0, 0, 0, 0, 0 };

enum EstadoPin {
  ABIERTO,
  CERRANDO_BLANCO,
  CERRANDO_AMARILLO,
  CERRADO,
  ABRIENDO_AMARILLO,
  ABRIENDO_BLANCO
};

struct GrupoPin {
  EstadoPin estado;
  unsigned long tCambio;
  unsigned long esperaAbierto;
  unsigned long durFadeBlanco;
  unsigned long durFadeAmarillo;
  unsigned long durCerrado;
  uint8_t nivelBlanco;
  uint8_t nivelAmarillo;
};

GrupoPin pinesOjos[NUM_PINES_OJOS];

const unsigned long BASE_FADE_MIN     = 250;
const unsigned long BASE_FADE_MAX     = 450;
const unsigned long BASE_CERRADO_MIN  = 80;
const unsigned long BASE_CERRADO_MAX  = 220;

// =========================================================
// ================ SECCION BARRIDO (pin 19) ================
// =========================================================

#define LED_TYPE_BARRIDO      WS2811
#define COLOR_ORDER_BARRIDO   BRG
#define PIN_BARRIDO             19

#define DIRECCIONES_POR_TIRA        27
#define TIRAS_POR_CUADRANTE         2
#define DIRECCIONES_POR_CUADRANTE   (DIRECCIONES_POR_TIRA * TIRAS_POR_CUADRANTE)  // 54
#define NUM_CUADRANTES              2
#define FILAS_TOTALES                DIRECCIONES_POR_TIRA   // 27

#define SEGMENTOS_TIPO_A            6
#define SEGMENTOS_TIPO_B            6
#define DIRECCIONES_POR_BLOQUE_A    3
#define DIRECCIONES_TIRA_NUEVA      (SEGMENTOS_TIPO_A * DIRECCIONES_POR_BLOQUE_A + SEGMENTOS_TIPO_B * 1) // 24
#define NUM_TIRAS_NUEVAS            2

#define OFFSET_CUAD0        0
#define OFFSET_CUAD1        (DIRECCIONES_POR_CUADRANTE)
#define OFFSET_TIRA_NUEVA0  (DIRECCIONES_POR_CUADRANTE * NUM_CUADRANTES)
#define OFFSET_TIRA_NUEVA1  (OFFSET_TIRA_NUEVA0 + DIRECCIONES_TIRA_NUEVA)
#define TOTAL_DIRECCIONES_BARRIDO (OFFSET_TIRA_NUEVA1 + DIRECCIONES_TIRA_NUEVA)

CRGB ledsBarrido[TOTAL_DIRECCIONES_BARRIDO];
int filaDeDireccion[DIRECCIONES_POR_CUADRANTE];

CRGB COLOR_BARRIDO        = CRGB(0, 255, 0);   // color de los cuadrantes
CRGB COLOR_BARRIDO_TIRAS  = CRGB(0, 255, 0);   // color de las 2 tiras de 24

const int LARGO_COLA = 8;

float velocidadBarridoPct = 100.0;
const float BASE_VELOCIDAD_FILAS_SEG = 12.0;

float barridoNivelActual   = 255.0;
float barridoNivelObjetivo = 255.0;
const float BARRIDO_FADE_DURACION_MS = 1500.0;

unsigned long tUltimoFrameBarrido = 0;

enum ModoBarrido { BARRIDO_NORMAL, BARRIDO_DOBLE, BARRIDO_CARGA };
ModoBarrido modoBarrido = BARRIDO_NORMAL;

// Posiciones combinadas: 1..27 = cuadrantes (filas), 28..51 = tiras nuevas
// (la posicion 28 prende a la vez el segmento 1 y el 25, la 29 el 2 y el 26, etc.)
const int LARGO_COMBINADO = FILAS_TOTALES + DIRECCIONES_TIRA_NUEVA;   // 51
float posNormal = -LARGO_COLA;
int   direccionNormal = 1;

float posBarridoDoble = -LARGO_COLA;

float posCarga = 0.0;
enum EstadoCarga { CARGANDO, COMPLETO, DESCARGANDO, VACIO };
EstadoCarga estadoCarga = CARGANDO;
unsigned long tCambioEstadoCarga = 0;
float tiempoPausaCargaSeg = 0.8;
const uint8_t BRILLO_MIN_CARGA = 50;
bool cargaDesdeAbajo = false;   // se mantiene en false: siempre cuadrantes -> tiras

// =========================================================
// == SECCION LATIDO + CARGA + EXPLOSION + 26 (pin 21) =====
// =========================================================

#define LED_TYPE_P21        WS2811
#define COLOR_ORDER_P21      BRG
#define PIN_21                 21

// Orden de los tramos en el pin 21 (en serie):
//   [ 30 segmentos del LATIDO ] -> [ 29 segmentos de CARGA+EXPLOSION ] -> [ 15 del 26 ]
#define SEG_LATIDO              30
#define SEG29                   29
#define NUM_FRONT1              15   // segmentos 1 a 15
#define NUM_FRONT2              14   // segmentos 16 a 29
#define NUM_NUMERO              15
#define OFFSET_LATIDO           0
#define OFFSET_SEG29            (SEG_LATIDO)
#define OFFSET_NUMERO           (SEG_LATIDO + SEG29)
#define TOTAL_PIN21             (SEG_LATIDO + SEG29 + NUM_NUMERO)   // 74

CRGB ledsPin21[TOTAL_PIN21];

// Colores (editables desde la web)
CRGB COLOR_LATIDO_PIN21    = CRGB(0, 255, 0);     // color del PULSO del latido
CRGB COLOR_BASE_LATIDO21   = CRGB(0, 255, 0);     // color BASE de los 30 segmentos
CRGB COLOR_CARGA_PIN21     = CRGB(0, 200, 255);   // color base del barrido de 29
CRGB COLOR_EXPLOSION_PIN21 = CRGB::White;
CRGB COLOR_NUMERO_PIN21    = CRGB(255, 60, 0);

// Velocidad y tiempo (editables desde la web)
float velocidadCarga21Pct  = 100.0;
float tiempoExplosion21Seg = 3.0;
float velocidadDescarga21Pct = 100.0;

// --- Latido (editables desde la web) ---
float velocidadLatido21Pct = 100.0;
float nivelLatido21        = 0.80;
float nivelSegLatido21     = 0.60;
float nivelFinalLatido21   = 1.00;
float escalaLatido21       = 1.00;

// --- Latido (fijos) ---
const int   GRUPO_SEG_LATIDO21           = 5;
const int   PARES_LATIDO21               = SEG_LATIDO / 2;   // 15 pares (15->1 y 16->30)
const int   GRUPO_PARES21                = 3;                // pares que se prenden/apagan por paso
const unsigned long DUR_LATIDO21_MS      = 800;
const unsigned long DUR_CARGA_GRUPO21_MS = 500;
const unsigned long PAUSA_FINAL21_MS     = 800;
const unsigned long PERIODO_LATIDO_CONTINUO_MS = 1200;  // un latido cada 1.2 s a 100%

// Parametros fijos de la carga + explosion
const float BASE_DURACION_CARGA21_SEG   = 1.5;
const float BASE_DURACION_DESCARGA21_SEG = 1.5;
const float NIVEL_MAX_CARGA21           = 0.80;
const float RAMPA_CARGA21               = 3.0;
const unsigned long PERIODO_PARPADEO_EXPLOSION_MS = 70;
const float PERIODO_RESPIRACION_NUMERO_SEG = 3.0;
const uint8_t BRILLO_MIN_RESPIRACION    = 25;

float posNorm21 = 0.0;
float posDescarga21 = 0.0;
enum EstadoPin21 {
  P21_PRE_LATIDO, P21_PRE_CARGA, P21_PRE_HOLD,
  P21_CARGANDO, P21_EXPLOSION, P21_DESCARGANDO,
  P21_DESC_LATIDO, P21_DESC_GRUPO
};
EstadoPin21 estadoPin21 = P21_PRE_LATIDO;
unsigned long tCambioEstadoPin21 = 0;
unsigned long tUltimoFramePin21 = 0;

int preSegEncendidos21 = 0;   // ahora cuenta PARES encendidos (0..15)
int preBase21 = 0;
int descSeg21 = 0;            // ahora cuenta PARES encendidos (0..15)
int descBase21 = 0;

// =========================================================
// ---------------- DECLARACIONES ADELANTADAS ----------------
// =========================================================
void pintarPinOjos(int p);
void pintarPinOjosColor(int p, CRGB cBlanco, CRGB cCentro);
void actualizarPinOjos(int p, unsigned long ahora);
unsigned long tiempoConVelocidad(unsigned long minBase, unsigned long maxBase, float velocidad);
unsigned long tiempoEsperaOjos(float velocidad);
float velocidadDelPinOjos(int p);
CRGB hexAColor(String hex);
void actualizarBarrido(unsigned long ahora);
void escribirPosicionCombinada(int lado, int posicion, CRGB color);
CRGB colorBarridoPos(int posicion);
void actualizarPin21(unsigned long ahora);
float formaLatido21(float t);
uint8_t aU8(float x);
CRGB colorLatido21(float nivelBase, float nivelPulso);
int parSegmento21(int i);
void sanearValores();

void manejarRaiz();
void manejarOn();
void manejarOff();
void manejarApagadoGradual();
void manejarEncendidoGradual();
void manejarMuerte();
void manejarMuerteDuracion();
void manejarMuerteTiempoRojo();
void manejarMuerteParpadeo();
void manejarBrillo();
void manejarVelocidad();
void manejarTiempoOjos();
void manejarBlink();
void manejarColor();
void manejarColorBlanco();

void manejarBarridoOn();
void manejarBarridoOff();
void manejarBarridoApagadoGradual();
void manejarBarridoEncendidoGradual();
void manejarBarridoColor();
void manejarBarridoColorTiras();
void manejarBarridoVelocidad();
void manejarBarridoModo();
void manejarTiempoBarrido();

void manejarCarga21Color();
void manejarCarga21ColorExplosion();
void manejarCarga21ColorNumero();
void manejarCarga21Velocidad();
void manejarCarga21TiempoExplosion();
void manejarCarga21VelocidadDescarga();

void manejarLatido21Color();
void manejarLatido21ColorBase();
void manejarLatido21Velocidad();
void manejarLatido21NivelLatido();
void manejarLatido21NivelSegmentos();
void manejarLatido21NivelFinal();
void manejarLatido21Escala();
void manejarLatido21Reiniciar();

void manejarGuardar();
void manejarRestaurar();
void manejarEstado();

// =========================================================
unsigned long tiempoConVelocidad(unsigned long minBase, unsigned long maxBase, float velocidad) {
  float factor = 100.0 / velocidad;
  unsigned long minEsc = (unsigned long)(minBase * factor);
  unsigned long maxEsc = (unsigned long)(maxBase * factor);
  if (maxEsc <= minEsc) maxEsc = minEsc + 1;
  return random(minEsc, maxEsc);
}

unsigned long tiempoEsperaOjos(float velocidad) {
  float centroMs = tiempoEntreParpadeosSeg * 1000.0;
  unsigned long minMs = (unsigned long)(centroMs * 0.6);
  unsigned long maxMs = (unsigned long)(centroMs * 1.4);
  return tiempoConVelocidad(minMs, maxMs, velocidad);
}

float velocidadDelPinOjos(int p) {
  return usarVelocidadMuerte[p] ? VELOCIDAD_MUERTE : velocidadPctOjos;
}

CRGB hexAColor(String hex) {
  hex.trim();
  if (hex.startsWith("#")) hex = hex.substring(1);
  if (hex.length() != 6) return CRGB::Black;

  long r = strtol(hex.substring(0, 2).c_str(), NULL, 16);
  long g = strtol(hex.substring(2, 4).c_str(), NULL, 16);
  long b = strtol(hex.substring(4, 6).c_str(), NULL, 16);

  return CRGB((uint8_t)r, (uint8_t)g, (uint8_t)b);
}

// =========================================================
// GUARDADO EN FLASH (Preferences / NVS)
// =========================================================
uint32_t colorAU32(CRGB c) { return ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b; }
CRGB u32AColor(uint32_t v) { return CRGB((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF); }
String colorAHex(CRGB c) {
  char b[8];
  snprintf(b, sizeof(b), "#%02x%02x%02x", c.r, c.g, c.b);
  return String(b);
}

// ---------- OJOS ----------
void guardarOjos() {
  prefs.begin("ojos", false);
  prefs.putUChar("brillo", brilloGlobalOjos);
  prefs.putFloat("vel", velocidadPctOjos);
  prefs.putFloat("tpar", tiempoEntreParpadeosSeg);
  prefs.putUInt("cCentro", colorAU32(COLOR_CENTRO));
  prefs.putUInt("cBlanco", colorAU32(COLOR_BLANCO));
  prefs.putFloat("mDur", muerteDuracionSeg);
  prefs.putFloat("mRojo", muerteTiempoRojoSeg);
  prefs.putFloat("mBlink", muerteParpadeoMs);
  prefs.end();
}
void cargarOjos() {
  prefs.begin("ojos", false);
  brilloGlobalOjos        = prefs.getUChar("brillo", brilloGlobalOjos);
  velocidadPctOjos        = prefs.getFloat("vel", velocidadPctOjos);
  tiempoEntreParpadeosSeg = prefs.getFloat("tpar", tiempoEntreParpadeosSeg);
  COLOR_CENTRO = u32AColor(prefs.getUInt("cCentro", colorAU32(COLOR_CENTRO)));
  COLOR_BLANCO = u32AColor(prefs.getUInt("cBlanco", colorAU32(COLOR_BLANCO)));
  muerteDuracionSeg   = prefs.getFloat("mDur", muerteDuracionSeg);
  muerteTiempoRojoSeg = prefs.getFloat("mRojo", muerteTiempoRojoSeg);
  muerteParpadeoMs    = prefs.getFloat("mBlink", muerteParpadeoMs);
  prefs.end();
}

// ---------- BARRIDO (pin 19) ----------
void guardarBarrido() {
  prefs.begin("barrido", false);
  prefs.putUInt("color", colorAU32(COLOR_BARRIDO));
  prefs.putUInt("cTiras", colorAU32(COLOR_BARRIDO_TIRAS));
  prefs.putFloat("vel", velocidadBarridoPct);
  prefs.putFloat("pausa", tiempoPausaCargaSeg);
  prefs.putUChar("modo", (uint8_t)modoBarrido);
  prefs.end();
}
void cargarBarrido() {
  prefs.begin("barrido", false);
  COLOR_BARRIDO       = u32AColor(prefs.getUInt("color", colorAU32(COLOR_BARRIDO)));
  COLOR_BARRIDO_TIRAS = u32AColor(prefs.getUInt("cTiras", colorAU32(COLOR_BARRIDO_TIRAS)));
  velocidadBarridoPct = prefs.getFloat("vel", velocidadBarridoPct);
  tiempoPausaCargaSeg = prefs.getFloat("pausa", tiempoPausaCargaSeg);
  modoBarrido = (ModoBarrido)constrain((int)prefs.getUChar("modo", (uint8_t)modoBarrido), 0, 2);
  prefs.end();
}

// ---------- LATIDO (pin 21, 30 segmentos) ----------
void guardarLatido() {
  prefs.begin("latido", false);
  prefs.putUInt("cLatido", colorAU32(COLOR_LATIDO_PIN21));
  prefs.putUInt("cBase", colorAU32(COLOR_BASE_LATIDO21));
  prefs.putFloat("vel", velocidadLatido21Pct);
  prefs.putFloat("nLat", nivelLatido21);
  prefs.putFloat("nSeg", nivelSegLatido21);
  prefs.putFloat("nFin", nivelFinalLatido21);
  prefs.putFloat("esc", escalaLatido21);
  prefs.end();
}
void cargarLatido() {
  prefs.begin("latido", false);
  COLOR_LATIDO_PIN21  = u32AColor(prefs.getUInt("cLatido", colorAU32(COLOR_LATIDO_PIN21)));
  COLOR_BASE_LATIDO21 = u32AColor(prefs.getUInt("cBase", colorAU32(COLOR_BASE_LATIDO21)));
  velocidadLatido21Pct = prefs.getFloat("vel", velocidadLatido21Pct);
  nivelLatido21        = prefs.getFloat("nLat", nivelLatido21);
  nivelSegLatido21     = prefs.getFloat("nSeg", nivelSegLatido21);
  nivelFinalLatido21   = prefs.getFloat("nFin", nivelFinalLatido21);
  escalaLatido21       = prefs.getFloat("esc", escalaLatido21);
  prefs.end();
}

// ---------- CARGA + EXPLOSION + 26 (pin 21) ----------
void guardarCarga() {
  prefs.begin("carga", false);
  prefs.putUInt("cCarga", colorAU32(COLOR_CARGA_PIN21));
  prefs.putUInt("cExp", colorAU32(COLOR_EXPLOSION_PIN21));
  prefs.putUInt("cNum", colorAU32(COLOR_NUMERO_PIN21));
  prefs.putFloat("vel", velocidadCarga21Pct);
  prefs.putFloat("tExp", tiempoExplosion21Seg);
  prefs.putFloat("velDesc", velocidadDescarga21Pct);
  prefs.end();
}
void cargarCarga() {
  prefs.begin("carga", false);
  COLOR_CARGA_PIN21     = u32AColor(prefs.getUInt("cCarga", colorAU32(COLOR_CARGA_PIN21)));
  COLOR_EXPLOSION_PIN21 = u32AColor(prefs.getUInt("cExp", colorAU32(COLOR_EXPLOSION_PIN21)));
  COLOR_NUMERO_PIN21    = u32AColor(prefs.getUInt("cNum", colorAU32(COLOR_NUMERO_PIN21)));
  velocidadCarga21Pct    = prefs.getFloat("vel", velocidadCarga21Pct);
  tiempoExplosion21Seg   = prefs.getFloat("tExp", tiempoExplosion21Seg);
  velocidadDescarga21Pct = prefs.getFloat("velDesc", velocidadDescarga21Pct);
  prefs.end();
}

void cargarTodo() {
  cargarOjos();
  cargarBarrido();
  cargarLatido();
  cargarCarga();
}

// descarta valores corruptos leidos de la flash (NaN o fuera de rango)
void sanearValores() {
  auto ok = [](float v, float lo, float hi, float def) {
    return (isnan(v) || v < lo || v > hi) ? def : v;
  };
  velocidadPctOjos        = ok(velocidadPctOjos, 10, 500, 100);
  tiempoEntreParpadeosSeg = ok(tiempoEntreParpadeosSeg, 0.5, 15, 4.5);
  muerteDuracionSeg       = ok(muerteDuracionSeg, 0.5, 15, 3.0);
  muerteTiempoRojoSeg     = ok(muerteTiempoRojoSeg, 0, 15, 1.5);
  muerteParpadeoMs        = ok(muerteParpadeoMs, 30, 500, 80);
  velocidadBarridoPct     = ok(velocidadBarridoPct, 10, 500, 100);
  tiempoPausaCargaSeg     = ok(tiempoPausaCargaSeg, 0, 5, 0.8);
  velocidadLatido21Pct    = ok(velocidadLatido21Pct, 10, 500, 100);
  nivelLatido21           = ok(nivelLatido21, 0.05, 1, 0.8);
  nivelSegLatido21        = ok(nivelSegLatido21, 0.05, 1, 0.6);
  nivelFinalLatido21      = ok(nivelFinalLatido21, 0.05, 1, 1.0);
  escalaLatido21          = ok(escalaLatido21, 0.05, 1, 1.0);
  velocidadCarga21Pct     = ok(velocidadCarga21Pct, 10, 500, 100);
  tiempoExplosion21Seg    = ok(tiempoExplosion21Seg, 0.5, 10, 3.0);
  velocidadDescarga21Pct  = ok(velocidadDescarga21Pct, 10, 500, 100);
}

void borrarNamespace(const char* ns) {
  prefs.begin(ns, false);
  prefs.clear();
  prefs.end();
}

// =========================================================
// PAGINA WEB
// =========================================================
const char PAGINA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Control Ojos LED</title>
<style>
  body {
    font-family: -apple-system, Arial, sans-serif;
    background: #111;
    color: #eee;
    text-align: center;
    padding: 20px;
    margin: 0;
  }
  h1 { font-size: 1.5em; margin-bottom: 10px; }
  h2 { font-size: 1.1em; margin: 35px 0 10px; opacity: 0.85; border-top: 1px solid #333; padding-top: 20px; }
  .boton {
    display: inline-block;
    width: 45%;
    padding: 18px 0;
    margin: 8px;
    font-size: 1.05em;
    border: none;
    border-radius: 12px;
    color: white;
    cursor: pointer;
  }
  .on       { background: #2e7d32; }
  .off      { background: #c62828; }
  .off-grad { background: #ef6c00; width: 94%; }
  .on-grad  { background: #0277bd; width: 94%; }
  .blink    { background: #6a1b9a; width: 94%; }
  .muerte   { background: #4a0000; width: 45%; }
  .barr-on  { background: #1b5e20; }
  .barr-off { background: #263238; }
  .reinicio { background: #00695c; width: 94%; }
  .guardar   { background: #1565c0; width: 94%; }
  .restaurar { background: #455a64; width: 94%; font-size: 0.9em; padding: 12px 0; }
  .modo     { background: #37474f; width: 29%; padding: 14px 0; font-size: 0.95em; margin: 4px 1%; }
  .modo-activo { background: #00695c; }
  .bloque { margin: 22px auto; max-width: 400px; text-align: left; }
  label { font-size: 1em; display: block; margin-bottom: 6px; }
  input[type=range] { width: 100%; }
  input[type=color] {
    width: 100%;
    height: 50px;
    border: none;
    border-radius: 10px;
    background: none;
  }
  .valor { float: right; opacity: 0.7; }
  .estado { margin-top: 20px; font-size: 0.9em; opacity: 0.6; }
</style>
</head>
<body>
  <h1>👁 Control de Ojos LED</h1>

  <div class="bloque">
    <button class="boton guardar" onclick="guardar('todo')">💾 GUARDAR TODO</button>
  </div>

  <div class="bloque">
    <button class="boton on" onclick="mandar('/on')">ENCENDER</button>
    <button class="boton off" onclick="mandar('/off')">APAGAR YA</button>
  </div>

  <div class="bloque">
    <button class="boton off-grad" onclick="mandar('/apagar_gradual')">APAGADO GRADUAL</button>
  </div>

  <div class="bloque">
    <button class="boton on-grad" onclick="mandar('/encendido_gradual')">ENCENDIDO GRADUAL</button>
  </div>

  <h2>💀 Muerte por cabeza</h2>
  <div class="bloque">
    <button class="boton muerte" onclick="mandar('/muerte?cabeza=1')">MUERTE CABEZA 1</button>
    <button class="boton muerte" onclick="mandar('/muerte?cabeza=2')">MUERTE CABEZA 2</button>
    <button class="boton muerte" onclick="mandar('/muerte?cabeza=3')">MUERTE CABEZA 3</button>
    <button class="boton muerte" onclick="mandar('/muerte?cabeza=4')">MUERTE CABEZA 4</button>
    <button class="boton muerte" onclick="mandar('/muerte?cabeza=5')">MUERTE CABEZA 5</button>
    <button class="boton muerte" onclick="mandar('/muerte?cabeza=6')">MUERTE CABEZA 6</button>
  </div>

  <div class="bloque">
    <label>Velocidad del parpadeo en muerte (ms) <span class="valor" id="valMuerteBlink">80</span></label>
    <input type="range" min="30" max="500" step="10" value="80" id="sliderMuerteBlink"
      oninput="document.getElementById('valMuerteBlink').innerText=this.value"
      onchange="mandar('/muerte_parpadeo?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Tiempo en volverse rojo (segundos) <span class="valor" id="valMuerteRojo">1.5</span></label>
    <input type="range" min="0" max="15" step="0.1" value="1.5" id="sliderMuerteRojo"
      oninput="document.getElementById('valMuerteRojo').innerText=this.value"
      onchange="mandar('/muerte_tiempo_rojo?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Duración total hasta apagarse (segundos) <span class="valor" id="valMuerteDur">3.0</span></label>
    <input type="range" min="0.5" max="15" step="0.1" value="3.0" id="sliderMuerteDur"
      oninput="document.getElementById('valMuerteDur').innerText=this.value"
      onchange="mandar('/muerte_duracion?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Brillo ojos <span class="valor" id="valBrillo">180</span></label>
    <input type="range" min="0" max="255" value="180" id="sliderBrillo"
      oninput="document.getElementById('valBrillo').innerText=this.value"
      onchange="mandar('/brillo?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Velocidad ojos % <span class="valor" id="valVelocidad">100</span></label>
    <input type="range" min="10" max="500" value="100" id="sliderVelocidad"
      oninput="document.getElementById('valVelocidad').innerText=this.value"
      onchange="mandar('/velocidad?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Tiempo entre parpadeos (segundos) <span class="valor" id="valTiempoOjos">4.5</span></label>
    <input type="range" min="0.5" max="15" step="0.1" value="4.5" id="sliderTiempoOjos"
      oninput="document.getElementById('valTiempoOjos').innerText=this.value"
      onchange="mandar('/tiempo_ojos?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Color de la zona central</label>
    <input type="color" id="colorPickerCentro" value="#ffaa00"
      onchange="mandar('/color?valor=' + this.value.substring(1))">
  </div>

  <div class="bloque">
    <label>Color de la zona lateral (antes blanca)</label>
    <input type="color" id="colorPickerBlanco" value="#ffffff"
      onchange="mandar('/colorBlanco?valor=' + this.value.substring(1))">
  </div>

  <div class="bloque">
    <button class="boton blink" onclick="mandar('/blink')">PARPADEO FORZADO</button>
  </div>

  <div class="bloque">
    <button class="boton guardar" onclick="guardar('ojos')">💾 GUARDAR OJOS</button>
    <button class="boton restaurar" onclick="restaurar('ojos')">VOLVER A VALORES DE FÁBRICA (ojos)</button>
  </div>

  <h2>🟩 Barrido (cuadrantes + tiras de 24)</h2>

  <div class="bloque">
    <button class="boton barr-on" onclick="mandar('/barrido_on')">BARRIDO ON</button>
    <button class="boton barr-off" onclick="mandar('/barrido_off')">BARRIDO OFF</button>
  </div>

  <div class="bloque">
    <button class="boton off-grad" onclick="mandar('/barrido_apagar_gradual')">APAGADO GRADUAL BARRIDO</button>
  </div>

  <div class="bloque">
    <button class="boton on-grad" onclick="mandar('/barrido_encendido_gradual')">ENCENDIDO GRADUAL BARRIDO</button>
  </div>

  <div class="bloque">
    <label>Modo de barrido</label>
    <button class="boton modo" id="modoNormal" onclick="elegirModo('normal')">NORMAL (con rebote)</button>
    <button class="boton modo" id="modoDoble" onclick="elegirModo('doble')">DOBLE</button>
    <button class="boton modo" id="modoCarga" onclick="elegirModo('carga')">CARGA</button>
  </div>

  <div class="bloque">
    <label>Velocidad barrido % <span class="valor" id="valVelocidadBarrido">100</span></label>
    <input type="range" min="10" max="500" value="100" id="sliderVelocidadBarrido"
      oninput="document.getElementById('valVelocidadBarrido').innerText=this.value"
      onchange="mandar('/barrido_velocidad?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Tiempo de pausa - modo Carga (segundos) <span class="valor" id="valTiempoBarrido">0.8</span></label>
    <input type="range" min="0" max="5" step="0.1" value="0.8" id="sliderTiempoBarrido"
      oninput="document.getElementById('valTiempoBarrido').innerText=this.value"
      onchange="mandar('/tiempo_barrido?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Color de los cuadrantes</label>
    <input type="color" id="colorPickerBarrido" value="#00ff00"
      onchange="mandar('/barrido_color?valor=' + this.value.substring(1))">
  </div>

  <div class="bloque">
    <label>Color de las tiras de 24 (segmentos 1-48)</label>
    <input type="color" id="colorPickerBarridoTiras" value="#00ff00"
      onchange="mandar('/barrido_color_tiras?valor=' + this.value.substring(1))">
  </div>

  <div class="bloque">
    <button class="boton guardar" onclick="guardar('barrido')">💾 GUARDAR BARRIDO (pin 19)</button>
    <button class="boton restaurar" onclick="restaurar('barrido')">VOLVER A VALORES DE FÁBRICA (barrido)</button>
  </div>

  <h2>💚 Latido con carga (30 segmentos)</h2>

  <div class="bloque">
    <button class="boton reinicio" onclick="mandar('/latido21_reiniciar')">REINICIAR CICLO (desde el latido)</button>
  </div>

  <div class="bloque">
    <label>Velocidad del latido y la carga % <span class="valor" id="valVelocidadLatido21">100</span></label>
    <input type="range" min="10" max="500" value="100" id="sliderVelocidadLatido21"
      oninput="document.getElementById('valVelocidadLatido21').innerText=this.value"
      onchange="mandar('/latido21_velocidad?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Brillo del latido % <span class="valor" id="valNivelLatido21">80</span></label>
    <input type="range" min="5" max="100" value="80" id="sliderNivelLatido21"
      oninput="document.getElementById('valNivelLatido21').innerText=this.value"
      onchange="mandar('/latido21_nivel_latido?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Brillo de los segmentos cargados % <span class="valor" id="valNivelSegLatido21">60</span></label>
    <input type="range" min="5" max="100" value="60" id="sliderNivelSegLatido21"
      oninput="document.getElementById('valNivelSegLatido21').innerText=this.value"
      onchange="mandar('/latido21_nivel_segmentos?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Brillo del latido final % <span class="valor" id="valNivelFinalLatido21">100</span></label>
    <input type="range" min="5" max="100" value="100" id="sliderNivelFinalLatido21"
      oninput="document.getElementById('valNivelFinalLatido21').innerText=this.value"
      onchange="mandar('/latido21_nivel_final?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Brillo general de toda esta fase % <span class="valor" id="valEscalaLatido21">100</span></label>
    <input type="range" min="5" max="100" value="100" id="sliderEscalaLatido21"
      oninput="document.getElementById('valEscalaLatido21').innerText=this.value"
      onchange="mandar('/latido21_escala?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Color del latido (pulso)</label>
    <input type="color" id="colorPickerLatido21" value="#00ff00"
      onchange="mandar('/latido21_color?valor=' + this.value.substring(1))">
  </div>

  <div class="bloque">
    <label>Color base de los 30 segmentos</label>
    <input type="color" id="colorPickerBaseLatido21" value="#00ff00"
      onchange="mandar('/latido21_color_base?valor=' + this.value.substring(1))">
  </div>

  <div class="bloque">
    <button class="boton guardar" onclick="guardar('latido')">💾 GUARDAR LATIDO (30 segmentos)</button>
    <button class="boton restaurar" onclick="restaurar('latido')">VOLVER A VALORES DE FÁBRICA (latido)</button>
  </div>

  <h2>⚡ Carga + Explosión + Descarga (29 segmentos) y 26</h2>

  <div class="bloque">
    <label>Velocidad del barrido % <span class="valor" id="valVelocidadCarga21">100</span></label>
    <input type="range" min="10" max="500" value="100" id="sliderVelocidadCarga21"
      oninput="document.getElementById('valVelocidadCarga21').innerText=this.value"
      onchange="mandar('/carga21_velocidad?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Tiempo de explosión (segundos) <span class="valor" id="valTiempoExplosion21">3.0</span></label>
    <input type="range" min="0.5" max="10" step="0.1" value="3.0" id="sliderTiempoExplosion21"
      oninput="document.getElementById('valTiempoExplosion21').innerText=this.value"
      onchange="mandar('/carga21_tiempo_explosion?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Velocidad de la descarga % <span class="valor" id="valVelocidadDescarga21">100</span></label>
    <input type="range" min="10" max="500" value="100" id="sliderVelocidadDescarga21"
      oninput="document.getElementById('valVelocidadDescarga21').innerText=this.value"
      onchange="mandar('/carga21_descarga_velocidad?valor=' + this.value)">
  </div>

  <div class="bloque">
    <label>Color base del barrido</label>
    <input type="color" id="colorPickerCarga21" value="#00c8ff"
      onchange="mandar('/carga21_color?valor=' + this.value.substring(1))">
  </div>

  <div class="bloque">
    <label>Color de la explosión</label>
    <input type="color" id="colorPickerExplosion21" value="#ffffff"
      onchange="mandar('/carga21_color_explosion?valor=' + this.value.substring(1))">
  </div>

  <div class="bloque">
    <label>Color del 26</label>
    <input type="color" id="colorPickerNumero21" value="#ff3c00"
      onchange="mandar('/carga21_color_numero?valor=' + this.value.substring(1))">
  </div>

  <div class="bloque">
    <button class="boton guardar" onclick="guardar('carga')">💾 GUARDAR CARGA + EXPLOSIÓN + 26</button>
    <button class="boton restaurar" onclick="restaurar('carga')">VOLVER A VALORES DE FÁBRICA (carga)</button>
  </div>

  <div class="estado" id="estadoTexto">Conectado</div>

<script>
function mandar(ruta) {
  fetch(ruta).catch(e => console.log(e));
}

function elegirModo(modo) {
  mandar('/barrido_modo?valor=' + modo);
  document.querySelectorAll('.modo').forEach(b => b.classList.remove('modo-activo'));
  document.getElementById('modo' + modo.charAt(0).toUpperCase() + modo.slice(1)).classList.add('modo-activo');
}

function guardar(seccion) {
  fetch('/guardar?s=' + seccion)
    .then(r => r.text())
    .then(t => { document.getElementById('estadoTexto').innerText = t + ' (' + seccion + ')'; })
    .catch(e => { document.getElementById('estadoTexto').innerText = 'Error al guardar'; });
}

function restaurar(seccion) {
  if (!confirm('Se borran los valores guardados de "' + seccion + '" y la placa se reinicia. ¿Seguro?')) return;
  fetch('/restaurar?s=' + seccion).catch(e => console.log(e));
  document.getElementById('estadoTexto').innerText = 'Reiniciando...';
  setTimeout(function () { location.reload(); }, 4000);
}

function ponerSlider(id, valId, v) {
  var e = document.getElementById(id);
  if (e) e.value = v;
  var s = document.getElementById(valId);
  if (s) s.innerText = v;
}
function ponerColor(id, v) {
  var e = document.getElementById(id);
  if (e) e.value = v;
}

function cargarEstado() {
  fetch('/estado').then(r => r.json()).then(d => {
    ponerSlider('sliderBrillo', 'valBrillo', d.brillo);
    ponerSlider('sliderVelocidad', 'valVelocidad', d.velOjos);
    ponerSlider('sliderTiempoOjos', 'valTiempoOjos', d.tOjos);
    ponerColor('colorPickerCentro', d.cCentro);
    ponerColor('colorPickerBlanco', d.cBlanco);
    ponerSlider('sliderMuerteBlink', 'valMuerteBlink', d.mBlink);
    ponerSlider('sliderMuerteRojo', 'valMuerteRojo', d.mRojo);
    ponerSlider('sliderMuerteDur', 'valMuerteDur', d.mDur);

    ponerSlider('sliderVelocidadBarrido', 'valVelocidadBarrido', d.velBarr);
    ponerSlider('sliderTiempoBarrido', 'valTiempoBarrido', d.tBarr);
    ponerColor('colorPickerBarrido', d.cBarr);
    ponerColor('colorPickerBarridoTiras', d.cTiras);
    document.querySelectorAll('.modo').forEach(b => b.classList.remove('modo-activo'));
    var m = document.getElementById('modo' + d.modo.charAt(0).toUpperCase() + d.modo.slice(1));
    if (m) m.classList.add('modo-activo');

    ponerSlider('sliderVelocidadLatido21', 'valVelocidadLatido21', d.velLat);
    ponerSlider('sliderNivelLatido21', 'valNivelLatido21', d.nivLat);
    ponerSlider('sliderNivelSegLatido21', 'valNivelSegLatido21', d.nivSeg);
    ponerSlider('sliderNivelFinalLatido21', 'valNivelFinalLatido21', d.nivFin);
    ponerSlider('sliderEscalaLatido21', 'valEscalaLatido21', d.escLat);
    ponerColor('colorPickerLatido21', d.cLat);
    ponerColor('colorPickerBaseLatido21', d.cBase);

    ponerSlider('sliderVelocidadCarga21', 'valVelocidadCarga21', d.velCar);
    ponerSlider('sliderTiempoExplosion21', 'valTiempoExplosion21', d.tExp);
    ponerSlider('sliderVelocidadDescarga21', 'valVelocidadDescarga21', d.velDesc);
    ponerColor('colorPickerCarga21', d.cCar);
    ponerColor('colorPickerExplosion21', d.cExp);
    ponerColor('colorPickerNumero21', d.cNum);
  }).catch(e => console.log(e));
}

cargarEstado();
</script>
</body>
</html>
)rawliteral";

// =========================================================
void manejarRaiz() {
  server.send(200, "text/html", PAGINA_HTML);
}

// ---------------- GUARDAR / RESTAURAR / ESTADO ----------------
void manejarGuardar() {
  String s = server.arg("s");
  bool todo = (s == "todo");
  if (todo || s == "ojos")    guardarOjos();
  if (todo || s == "barrido") guardarBarrido();
  if (todo || s == "latido")  guardarLatido();
  if (todo || s == "carga")   guardarCarga();
  server.send(200, "text/plain", "Guardado OK");
}

void manejarRestaurar() {
  String s = server.arg("s");
  bool todo = (s == "todo");
  if (todo || s == "ojos")    borrarNamespace("ojos");
  if (todo || s == "barrido") borrarNamespace("barrido");
  if (todo || s == "latido")  borrarNamespace("latido");
  if (todo || s == "carga")   borrarNamespace("carga");
  server.send(200, "text/plain", "Restaurado, reiniciando...");
  delay(300);
  ESP.restart();   // arranca con los valores por defecto del codigo
}

void manejarEstado() {
  const char* m = (modoBarrido == BARRIDO_NORMAL) ? "normal" :
                  (modoBarrido == BARRIDO_DOBLE)  ? "doble"  : "carga";
  String j = "{";
  j += "\"brillo\":"  + String(brilloGlobalOjos);
  j += ",\"velOjos\":" + String(velocidadPctOjos, 0);
  j += ",\"tOjos\":"   + String(tiempoEntreParpadeosSeg, 1);
  j += ",\"cCentro\":\"" + colorAHex(COLOR_CENTRO) + "\"";
  j += ",\"cBlanco\":\"" + colorAHex(COLOR_BLANCO) + "\"";
  j += ",\"mBlink\":" + String(muerteParpadeoMs, 0);
  j += ",\"mRojo\":"  + String(muerteTiempoRojoSeg, 1);
  j += ",\"mDur\":"   + String(muerteDuracionSeg, 1);
  j += ",\"velBarr\":" + String(velocidadBarridoPct, 0);
  j += ",\"tBarr\":"   + String(tiempoPausaCargaSeg, 1);
  j += ",\"cBarr\":\"" + colorAHex(COLOR_BARRIDO) + "\"";
  j += ",\"cTiras\":\"" + colorAHex(COLOR_BARRIDO_TIRAS) + "\"";
  j += ",\"modo\":\""  + String(m) + "\"";
  j += ",\"velLat\":"  + String(velocidadLatido21Pct, 0);
  j += ",\"nivLat\":"  + String(nivelLatido21 * 100.0, 0);
  j += ",\"nivSeg\":"  + String(nivelSegLatido21 * 100.0, 0);
  j += ",\"nivFin\":"  + String(nivelFinalLatido21 * 100.0, 0);
  j += ",\"escLat\":"  + String(escalaLatido21 * 100.0, 0);
  j += ",\"cLat\":\""  + colorAHex(COLOR_LATIDO_PIN21) + "\"";
  j += ",\"cBase\":\"" + colorAHex(COLOR_BASE_LATIDO21) + "\"";
  j += ",\"velCar\":"  + String(velocidadCarga21Pct, 0);
  j += ",\"tExp\":"    + String(tiempoExplosion21Seg, 1);
  j += ",\"velDesc\":" + String(velocidadDescarga21Pct, 0);
  j += ",\"cCar\":\""  + colorAHex(COLOR_CARGA_PIN21) + "\"";
  j += ",\"cExp\":\""  + colorAHex(COLOR_EXPLOSION_PIN21) + "\"";
  j += ",\"cNum\":\""  + colorAHex(COLOR_NUMERO_PIN21) + "\"";
  j += "}";
  server.send(200, "application/json", j);
}

// ---------------- OJOS ----------------
void manejarOn() {
  animacionActiva = true;
  unsigned long ahora = millis();
  for (int p = 0; p < NUM_PINES_OJOS; p++) {
    muriendo[p] = false;
    quedarApagadoPin[p] = false;
    usarVelocidadMuerte[p] = false;
    pinesOjos[p].estado = ABIERTO;
    pinesOjos[p].tCambio = ahora;
    pinesOjos[p].esperaAbierto = tiempoEsperaOjos(velocidadDelPinOjos(p));
    pinesOjos[p].nivelBlanco = 255;
    pinesOjos[p].nivelAmarillo = 255;
  }
  server.send(200, "text/plain", "OK");
}

void manejarOff() {
  animacionActiva = false;
  server.send(200, "text/plain", "OK");
}

void manejarApagadoGradual() {
  unsigned long ahora = millis();
  for (int p = 0; p < NUM_PINES_OJOS; p++) {
    quedarApagadoPin[p] = true;
    usarVelocidadMuerte[p] = false;
    if (pinesOjos[p].estado == ABIERTO) {
      pinesOjos[p].estado = CERRANDO_BLANCO;
      pinesOjos[p].tCambio = ahora;
      pinesOjos[p].durFadeBlanco = tiempoConVelocidad(BASE_FADE_MIN, BASE_FADE_MAX, velocidadDelPinOjos(p));
    }
  }
  server.send(200, "text/plain", "OK");
}

void manejarEncendidoGradual() {
  unsigned long ahora = millis();
  for (int p = 0; p < NUM_PINES_OJOS; p++) {
    muriendo[p] = false;
    quedarApagadoPin[p] = false;
    usarVelocidadMuerte[p] = false;
    pinesOjos[p].nivelBlanco = 0;
    pinesOjos[p].nivelAmarillo = 0;
    pinesOjos[p].estado = ABRIENDO_AMARILLO;
    pinesOjos[p].tCambio = ahora;
    pinesOjos[p].durFadeAmarillo = tiempoConVelocidad(BASE_FADE_MIN, BASE_FADE_MAX, velocidadDelPinOjos(p));
  }
  server.send(200, "text/plain", "OK");
}

void manejarMuerte() {
  if (server.hasArg("cabeza")) {
    int p = server.arg("cabeza").toInt() - 1;
    if (p >= 0 && p < NUM_PINES_OJOS) {
      muriendo[p] = true;
      tInicioMuerte[p] = millis();
      quedarApagadoPin[p] = true;      // al terminar se queda apagado
      usarVelocidadMuerte[p] = false;
    }
  }
  server.send(200, "text/plain", "OK");
}

void manejarMuerteDuracion() {
  if (server.hasArg("valor")) muerteDuracionSeg = constrain(server.arg("valor").toFloat(), 0.5, 15.0);
  server.send(200, "text/plain", "OK");
}

void manejarMuerteTiempoRojo() {
  if (server.hasArg("valor")) muerteTiempoRojoSeg = constrain(server.arg("valor").toFloat(), 0.0, 15.0);
  server.send(200, "text/plain", "OK");
}

void manejarMuerteParpadeo() {
  if (server.hasArg("valor")) muerteParpadeoMs = constrain(server.arg("valor").toFloat(), 30.0, 500.0);
  server.send(200, "text/plain", "OK");
}

void manejarBrillo() {
  if (server.hasArg("valor")) {
    int v = server.arg("valor").toInt();
    v = constrain(v, 0, 255);
    brilloGlobalOjos = v;
  }
  server.send(200, "text/plain", "OK");
}

void manejarVelocidad() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 10.0, 500.0);
    velocidadPctOjos = v;
  }
  server.send(200, "text/plain", "OK");
}

void manejarTiempoOjos() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 0.5, 15.0);
    tiempoEntreParpadeosSeg = v;
  }
  server.send(200, "text/plain", "OK");
}

void manejarBlink() {
  forzarBlink = true;
  server.send(200, "text/plain", "OK");
}

void manejarColor() {
  if (server.hasArg("valor")) {
    COLOR_CENTRO = hexAColor(server.arg("valor"));
  }
  server.send(200, "text/plain", "OK");
}

void manejarColorBlanco() {
  if (server.hasArg("valor")) {
    COLOR_BLANCO = hexAColor(server.arg("valor"));
  }
  server.send(200, "text/plain", "OK");
}

// ---------------- BARRIDO ----------------
void manejarBarridoOn() {
  barridoNivelActual = 255.0;
  barridoNivelObjetivo = 255.0;
  server.send(200, "text/plain", "OK");
}

void manejarBarridoOff() {
  barridoNivelActual = 0.0;
  barridoNivelObjetivo = 0.0;
  server.send(200, "text/plain", "OK");
}

void manejarBarridoApagadoGradual() {
  barridoNivelObjetivo = 0.0;
  server.send(200, "text/plain", "OK");
}

void manejarBarridoEncendidoGradual() {
  barridoNivelObjetivo = 255.0;
  server.send(200, "text/plain", "OK");
}

void manejarBarridoColor() {
  if (server.hasArg("valor")) {
    COLOR_BARRIDO = hexAColor(server.arg("valor"));
  }
  server.send(200, "text/plain", "OK");
}

void manejarBarridoColorTiras() {
  if (server.hasArg("valor")) {
    COLOR_BARRIDO_TIRAS = hexAColor(server.arg("valor"));
  }
  server.send(200, "text/plain", "OK");
}

void manejarBarridoVelocidad() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 10.0, 500.0);
    velocidadBarridoPct = v;
  }
  server.send(200, "text/plain", "OK");
}

void manejarTiempoBarrido() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 0.0, 5.0);
    tiempoPausaCargaSeg = v;
  }
  server.send(200, "text/plain", "OK");
}

void manejarBarridoModo() {
  if (server.hasArg("valor")) {
    String v = server.arg("valor");
    if (v == "normal")      modoBarrido = BARRIDO_NORMAL;
    else if (v == "doble")  modoBarrido = BARRIDO_DOBLE;
    else if (v == "carga")  modoBarrido = BARRIDO_CARGA;

    posNormal = -LARGO_COLA;
    direccionNormal = 1;
    posBarridoDoble = -LARGO_COLA;
    posCarga = 0.0;
    estadoCarga = CARGANDO;
    cargaDesdeAbajo = false;
  }
  server.send(200, "text/plain", "OK");
}

// ---------------- PIN 21: carga + explosion + 26 ----------------
void manejarCarga21Color() {
  if (server.hasArg("valor")) {
    COLOR_CARGA_PIN21 = hexAColor(server.arg("valor"));
  }
  server.send(200, "text/plain", "OK");
}

void manejarCarga21ColorExplosion() {
  if (server.hasArg("valor")) {
    COLOR_EXPLOSION_PIN21 = hexAColor(server.arg("valor"));
  }
  server.send(200, "text/plain", "OK");
}

void manejarCarga21ColorNumero() {
  if (server.hasArg("valor")) {
    COLOR_NUMERO_PIN21 = hexAColor(server.arg("valor"));
  }
  server.send(200, "text/plain", "OK");
}

void manejarCarga21Velocidad() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 10.0, 500.0);
    velocidadCarga21Pct = v;
  }
  server.send(200, "text/plain", "OK");
}

void manejarCarga21TiempoExplosion() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 0.5, 10.0);
    tiempoExplosion21Seg = v;
  }
  server.send(200, "text/plain", "OK");
}

void manejarCarga21VelocidadDescarga() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 10.0, 500.0);
    velocidadDescarga21Pct = v;
  }
  server.send(200, "text/plain", "OK");
}

// ---------------- PIN 21: latido ----------------
void manejarLatido21Color() {
  if (server.hasArg("valor")) {
    COLOR_LATIDO_PIN21 = hexAColor(server.arg("valor"));
  }
  server.send(200, "text/plain", "OK");
}

void manejarLatido21ColorBase() {
  if (server.hasArg("valor")) {
    COLOR_BASE_LATIDO21 = hexAColor(server.arg("valor"));
  }
  server.send(200, "text/plain", "OK");
}

void manejarLatido21Velocidad() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 10.0, 500.0);
    velocidadLatido21Pct = v;
  }
  server.send(200, "text/plain", "OK");
}

void manejarLatido21NivelLatido() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 5.0, 100.0);
    nivelLatido21 = v / 100.0;
  }
  server.send(200, "text/plain", "OK");
}

void manejarLatido21NivelSegmentos() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 5.0, 100.0);
    nivelSegLatido21 = v / 100.0;
  }
  server.send(200, "text/plain", "OK");
}

void manejarLatido21NivelFinal() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 5.0, 100.0);
    nivelFinalLatido21 = v / 100.0;
  }
  server.send(200, "text/plain", "OK");
}

void manejarLatido21Escala() {
  if (server.hasArg("valor")) {
    float v = server.arg("valor").toFloat();
    v = constrain(v, 5.0, 100.0);
    escalaLatido21 = v / 100.0;
  }
  server.send(200, "text/plain", "OK");
}

void manejarLatido21Reiniciar() {
  estadoPin21 = P21_PRE_LATIDO;
  preSegEncendidos21 = 0;
  preBase21 = 0;
  descSeg21 = 0;
  descBase21 = 0;
  posNorm21 = 0.0;
  posDescarga21 = 0.0;
  tCambioEstadoPin21 = millis();
  server.send(200, "text/plain", "OK");
}

// =========================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("Motivo de reinicio: %d (1=power 4=panic 5/6/7=watchdog 9=brownout)\n",
                (int)esp_reset_reason());

  cargarTodo();      // lee los parametros guardados en flash (si no hay, quedan los del codigo)
  sanearValores();   // descarta valores corruptos

  FastLED.addLeds<LED_TYPE_OJOS, 2,  COLOR_ORDER_OJOS>(ledsOjos[0], LEDS_POR_PIN_OJOS);
  FastLED.addLeds<LED_TYPE_OJOS, 4,  COLOR_ORDER_OJOS>(ledsOjos[1], LEDS_POR_PIN_OJOS);
  FastLED.addLeds<LED_TYPE_OJOS, 16, COLOR_ORDER_OJOS>(ledsOjos[2], LEDS_POR_PIN_OJOS);
  FastLED.addLeds<LED_TYPE_OJOS, 17, COLOR_ORDER_OJOS>(ledsOjos[3], LEDS_POR_PIN_OJOS);
  FastLED.addLeds<LED_TYPE_OJOS, 5,  COLOR_ORDER_OJOS>(ledsOjos[4], LEDS_POR_PIN_OJOS);
  FastLED.addLeds<LED_TYPE_OJOS, 18, COLOR_ORDER_OJOS>(ledsOjos[5], LEDS_POR_PIN_OJOS);

  FastLED.addLeds<LED_TYPE_BARRIDO, PIN_BARRIDO, COLOR_ORDER_BARRIDO>(ledsBarrido, TOTAL_DIRECCIONES_BARRIDO);

  FastLED.addLeds<LED_TYPE_P21, PIN_21, COLOR_ORDER_P21>(ledsPin21, TOTAL_PIN21);

  FastLED.setBrightness(brilloGlobalOjos);
  randomSeed(analogRead(0));

  for (int p = 0; p < NUM_PINES_OJOS; p++) {
    pinesOjos[p].estado = ABIERTO;
    pinesOjos[p].tCambio = millis();
    pinesOjos[p].esperaAbierto = tiempoEsperaOjos(velocidadDelPinOjos(p));
    pinesOjos[p].nivelBlanco = 255;
    pinesOjos[p].nivelAmarillo = 255;
    pintarPinOjos(p);
  }

  for (int a = 0; a < DIRECCIONES_POR_CUADRANTE; a++) {
    int a1 = a + 1;
    if (a1 <= DIRECCIONES_POR_TIRA) {
      filaDeDireccion[a] = (DIRECCIONES_POR_TIRA + 1) - a1;
    } else {
      int local = a1 - DIRECCIONES_POR_TIRA;
      filaDeDireccion[a] = local;
    }
  }
  tUltimoFrameBarrido = millis();
  tUltimoFramePin21 = millis();
  tCambioEstadoPin21 = millis();

  FastLED.show();

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress ip = WiFi.softAPIP();

  Serial.println("Red WiFi creada.");
  Serial.print("Nombre de red: "); Serial.println(AP_SSID);
  Serial.print("Contrasena: "); Serial.println(AP_PASSWORD);
  Serial.print("Entra a: http://"); Serial.println(ip);

  server.on("/", manejarRaiz);
  server.on("/guardar", manejarGuardar);
  server.on("/restaurar", manejarRestaurar);
  server.on("/estado", manejarEstado);

  server.on("/on", manejarOn);
  server.on("/off", manejarOff);
  server.on("/apagar_gradual", manejarApagadoGradual);
  server.on("/encendido_gradual", manejarEncendidoGradual);
  server.on("/muerte", manejarMuerte);
  server.on("/muerte_duracion", manejarMuerteDuracion);
  server.on("/muerte_tiempo_rojo", manejarMuerteTiempoRojo);
  server.on("/muerte_parpadeo", manejarMuerteParpadeo);
  server.on("/brillo", manejarBrillo);
  server.on("/velocidad", manejarVelocidad);
  server.on("/tiempo_ojos", manejarTiempoOjos);
  server.on("/blink", manejarBlink);
  server.on("/color", manejarColor);
  server.on("/colorBlanco", manejarColorBlanco);

  server.on("/barrido_on", manejarBarridoOn);
  server.on("/barrido_off", manejarBarridoOff);
  server.on("/barrido_apagar_gradual", manejarBarridoApagadoGradual);
  server.on("/barrido_encendido_gradual", manejarBarridoEncendidoGradual);
  server.on("/barrido_color", manejarBarridoColor);
  server.on("/barrido_color_tiras", manejarBarridoColorTiras);
  server.on("/barrido_velocidad", manejarBarridoVelocidad);
  server.on("/tiempo_barrido", manejarTiempoBarrido);
  server.on("/barrido_modo", manejarBarridoModo);

  server.on("/carga21_color", manejarCarga21Color);
  server.on("/carga21_color_explosion", manejarCarga21ColorExplosion);
  server.on("/carga21_color_numero", manejarCarga21ColorNumero);
  server.on("/carga21_velocidad", manejarCarga21Velocidad);
  server.on("/carga21_tiempo_explosion", manejarCarga21TiempoExplosion);
  server.on("/carga21_descarga_velocidad", manejarCarga21VelocidadDescarga);

  server.on("/latido21_color", manejarLatido21Color);
  server.on("/latido21_color_base", manejarLatido21ColorBase);
  server.on("/latido21_velocidad", manejarLatido21Velocidad);
  server.on("/latido21_nivel_latido", manejarLatido21NivelLatido);
  server.on("/latido21_nivel_segmentos", manejarLatido21NivelSegmentos);
  server.on("/latido21_nivel_final", manejarLatido21NivelFinal);
  server.on("/latido21_escala", manejarLatido21Escala);
  server.on("/latido21_reiniciar", manejarLatido21Reiniciar);

  server.begin();
  Serial.println("Servidor web iniciado.");
}

// =========================================================
void loop() {
  server.handleClient();
  unsigned long ahora = millis();

  FastLED.setBrightness(brilloGlobalOjos);

  if (animacionActiva) {
    for (int p = 0; p < NUM_PINES_OJOS; p++) {
      if (forzarBlink && !quedarApagadoPin[p]) {
        pinesOjos[p].estado = CERRANDO_BLANCO;
        pinesOjos[p].tCambio = ahora;
        pinesOjos[p].durFadeBlanco = tiempoConVelocidad(BASE_FADE_MIN, BASE_FADE_MAX, velocidadDelPinOjos(p));
      }
      actualizarPinOjos(p, ahora);
    }
    forzarBlink = false;
  } else {
    for (int p = 0; p < NUM_PINES_OJOS; p++) {
      fill_solid(ledsOjos[p], LEDS_POR_PIN_OJOS, CRGB::Black);
    }
  }

  actualizarBarrido(ahora);
  actualizarPin21(ahora);

  FastLED.show();
}

// =========================================================
void actualizarPinOjos(int p, unsigned long ahora) {
  GrupoPin &g = pinesOjos[p];
  unsigned long transcurrido = ahora - g.tCambio;
  float vel = velocidadDelPinOjos(p);

  // ---------- EFECTO MUERTE: parpadeo rapido + se pone rojo + se apaga ----------
  if (muriendo[p]) {
    unsigned long durMs = (unsigned long)(muerteDuracionSeg * 1000.0);
    unsigned long t = ahora - tInicioMuerte[p];

    if (t >= durMs) {
      // fin de la muerte: queda apagado
      muriendo[p] = false;
      g.estado = CERRADO;
      g.tCambio = ahora;
      g.durCerrado = 0;
      g.nivelBlanco = 0;
      g.nivelAmarillo = 0;
      transcurrido = 0;
    } else {
      float prog = (muerteTiempoRojoSeg <= 0.0) ? 1.0
                   : fminf(1.0f, (t / 1000.0f) / muerteTiempoRojoSeg);
      unsigned long semi = (unsigned long)muerteParpadeoMs;
      if (semi < 1) semi = 1;
      bool encendido = ((t / semi) % 2) == 0;

      if (encendido) {
        CRGB cB = blend(COLOR_BLANCO, CRGB::Red, aU8(prog));
        CRGB cC = blend(COLOR_CENTRO, CRGB::Red, aU8(prog));
        pintarPinOjosColor(p, cB, cC);
      } else {
        pintarPinOjosColor(p, CRGB::Black, CRGB::Black);
      }
      return;
    }
  }

  switch (g.estado) {

    case ABIERTO:
      g.nivelBlanco = 255;
      g.nivelAmarillo = 255;
      if (quedarApagadoPin[p] || transcurrido >= g.esperaAbierto) {
        g.estado = CERRANDO_BLANCO;
        g.tCambio = ahora;
        g.durFadeBlanco = tiempoConVelocidad(BASE_FADE_MIN, BASE_FADE_MAX, vel);
      }
      break;

    case CERRANDO_BLANCO: {
      float prog = (float)transcurrido / (float)g.durFadeBlanco;
      if (prog >= 1.0) prog = 1.0;
      g.nivelBlanco = 255 - (uint8_t)(prog * 255);
      g.nivelAmarillo = 255;
      if (prog >= 1.0) {
        g.estado = CERRANDO_AMARILLO;
        g.tCambio = ahora;
        g.durFadeAmarillo = tiempoConVelocidad(BASE_FADE_MIN, BASE_FADE_MAX, vel);
      }
      break;
    }

    case CERRANDO_AMARILLO: {
      float prog = (float)transcurrido / (float)g.durFadeAmarillo;
      if (prog >= 1.0) prog = 1.0;
      g.nivelBlanco = 0;
      g.nivelAmarillo = 255 - (uint8_t)(prog * 255);
      if (prog >= 1.0) {
        g.estado = CERRADO;
        g.tCambio = ahora;
        g.durCerrado = tiempoConVelocidad(BASE_CERRADO_MIN, BASE_CERRADO_MAX, vel);
      }
      break;
    }

    case CERRADO:
      g.nivelBlanco = 0;
      g.nivelAmarillo = 0;
      if (!quedarApagadoPin[p] && transcurrido >= g.durCerrado) {
        g.estado = ABRIENDO_AMARILLO;
        g.tCambio = ahora;
        g.durFadeAmarillo = tiempoConVelocidad(BASE_FADE_MIN, BASE_FADE_MAX, vel);
      }
      break;

    case ABRIENDO_AMARILLO: {
      float prog = (float)transcurrido / (float)g.durFadeAmarillo;
      if (prog >= 1.0) prog = 1.0;
      g.nivelBlanco = 0;
      g.nivelAmarillo = (uint8_t)(prog * 255);
      if (prog >= 1.0) {
        g.estado = ABRIENDO_BLANCO;
        g.tCambio = ahora;
        g.durFadeBlanco = tiempoConVelocidad(BASE_FADE_MIN, BASE_FADE_MAX, vel);
      }
      break;
    }

    case ABRIENDO_BLANCO: {
      float prog = (float)transcurrido / (float)g.durFadeBlanco;
      if (prog >= 1.0) prog = 1.0;
      g.nivelAmarillo = 255;
      g.nivelBlanco = (uint8_t)(prog * 255);
      if (prog >= 1.0) {
        g.estado = ABIERTO;
        g.tCambio = ahora;
        g.esperaAbierto = tiempoEsperaOjos(vel);
      }
      break;
    }
  }

  pintarPinOjos(p);
}

// =========================================================
void pintarPinOjos(int p) {
  GrupoPin &g = pinesOjos[p];

  CRGB colorBlancoActual = COLOR_BLANCO;
  CRGB colorCentroActual = COLOR_CENTRO;
  colorBlancoActual.nscale8_video(g.nivelBlanco);
  colorCentroActual.nscale8_video(g.nivelAmarillo);

  pintarPinOjosColor(p, colorBlancoActual, colorCentroActual);
}

void pintarPinOjosColor(int p, CRGB cBlanco, CRGB cCentro) {
  for (int ojo = 0; ojo < OJOS_POR_PIN; ojo++) {
    int base = ojo * SECCIONES_POR_OJO;
    for (int s = 0; s < SECCIONES_POR_OJO; s++) {
      int s_real = (ojo == 1 && INVERTIR_SEGUNDO_OJO) ? (SECCIONES_POR_OJO - 1 - s) : s;
      bool esBlanco = PATRON_OJO[s_real];
      ledsOjos[p][base + s] = esBlanco ? cBlanco : cCentro;
    }
  }
}

// =========================================================
// Color base segun la posicion: cuadrantes (1..27) o tiras (28..51)
CRGB colorBarridoPos(int posicion) {
  return (posicion <= FILAS_TOTALES) ? COLOR_BARRIDO : COLOR_BARRIDO_TIRAS;
}

// posicion 1..27  -> fila de los cuadrantes
// posicion 28..51 -> tiras nuevas: lado 0 = segmento (pos-27), lado 1 = segmento (pos-27)+24
//                    (o sea 1 con 25, 2 con 26, 3 con 27, ...)
void escribirPosicionCombinada(int lado, int posicion, CRGB color) {
  if (posicion < 1 || posicion > LARGO_COMBINADO) return;

  if (posicion <= FILAS_TOTALES) {
    int baseCuadrante = (lado == 0) ? OFFSET_CUAD0 : OFFSET_CUAD1;
    for (int a = 0; a < DIRECCIONES_POR_CUADRANTE; a++) {
      if (filaDeDireccion[a] == posicion) {
        ledsBarrido[baseCuadrante + a] = color;
      }
    }
  } else {
    int localIndex = posicion - FILAS_TOTALES - 1;
    int baseTiraNueva = (lado == 0) ? OFFSET_TIRA_NUEVA0 : OFFSET_TIRA_NUEVA1;
    ledsBarrido[baseTiraNueva + localIndex] = color;
  }
}

// =========================================================
void actualizarBarrido(unsigned long ahora) {
  float deltaSeg = (ahora - tUltimoFrameBarrido) / 1000.0;
  tUltimoFrameBarrido = ahora;
  if (deltaSeg < 0 || deltaSeg > 1.0) deltaSeg = 0;

  float velocidadFilasPorSeg = BASE_VELOCIDAD_FILAS_SEG * (velocidadBarridoPct / 100.0);
  unsigned long duracionPausaMs = (unsigned long)(tiempoPausaCargaSeg * 1000.0);

  float velCambioNivel = 255.0 / (BARRIDO_FADE_DURACION_MS / 1000.0);
  if (barridoNivelActual < barridoNivelObjetivo) {
    barridoNivelActual += velCambioNivel * deltaSeg;
    if (barridoNivelActual > barridoNivelObjetivo) barridoNivelActual = barridoNivelObjetivo;
  } else if (barridoNivelActual > barridoNivelObjetivo) {
    barridoNivelActual -= velCambioNivel * deltaSeg;
    if (barridoNivelActual < barridoNivelObjetivo) barridoNivelActual = barridoNivelObjetivo;
  }
  uint8_t nivelGlobal = (uint8_t)barridoNivelActual;

  fill_solid(ledsBarrido, TOTAL_DIRECCIONES_BARRIDO, CRGB::Black);

  if (modoBarrido == BARRIDO_NORMAL) {
    posNormal += velocidadFilasPorSeg * deltaSeg * direccionNormal;

    if (posNormal > LARGO_COMBINADO + LARGO_COLA) {
      posNormal = LARGO_COMBINADO + LARGO_COLA;
      direccionNormal = -1;
    } else if (posNormal < -LARGO_COLA) {
      posNormal = -LARGO_COLA;
      direccionNormal = 1;
    }

    for (int lado = 0; lado < 2; lado++) {
      for (int pos = 1; pos <= LARGO_COMBINADO; pos++) {
        float distancia = fabs(posNormal - pos);
        if (distancia <= LARGO_COLA) {
          float factor = 1.0 - (distancia / (float)LARGO_COLA);
          uint8_t brillo = (uint8_t)(factor * 255);
          CRGB color = colorBarridoPos(pos);
          color.nscale8_video(brillo);
          color.nscale8_video(nivelGlobal);
          escribirPosicionCombinada(lado, pos, color);
        }
      }
    }
    return;
  }

  if (modoBarrido == BARRIDO_DOBLE) {
    posBarridoDoble += velocidadFilasPorSeg * deltaSeg;
    if (posBarridoDoble > LARGO_COMBINADO + LARGO_COLA) {
      posBarridoDoble = -LARGO_COLA;
    }

    float cicloLongitud = LARGO_COMBINADO + 2.0 * LARGO_COLA;
    float posBarrido2 = posBarridoDoble + cicloLongitud / 2.0;
    if (posBarrido2 > LARGO_COMBINADO + LARGO_COLA) posBarrido2 -= cicloLongitud;

    for (int lado = 0; lado < 2; lado++) {
      for (int pos = 1; pos <= LARGO_COMBINADO; pos++) {
        uint8_t brilloLocal = 0;

        float distancia1 = posBarridoDoble - pos;
        if (distancia1 >= 0 && distancia1 <= LARGO_COLA) {
          float factor = 1.0 - (distancia1 / (float)LARGO_COLA);
          uint8_t b = (uint8_t)(factor * 255);
          if (b > brilloLocal) brilloLocal = b;
        }

        float distancia2 = posBarrido2 - pos;
        if (distancia2 >= 0 && distancia2 <= LARGO_COLA) {
          float factor = 1.0 - (distancia2 / (float)LARGO_COLA);
          uint8_t b = (uint8_t)(factor * 255);
          if (b > brilloLocal) brilloLocal = b;
        }

        CRGB color = colorBarridoPos(pos);
        color.nscale8_video(brilloLocal);
        color.nscale8_video(nivelGlobal);
        escribirPosicionCombinada(lado, pos, color);
      }
    }
    return;
  }

  if (modoBarrido == BARRIDO_CARGA) {
    // La carga recorre los 27 pasos de los cuadrantes y sigue en cascada por
    // las tiras (1+25, 2+26, ... 24+48). La descarga vuelve en sentido inverso.
    switch (estadoCarga) {
      case CARGANDO:
        posCarga += velocidadFilasPorSeg * deltaSeg;
        if (posCarga >= LARGO_COMBINADO) {
          posCarga = LARGO_COMBINADO;
          estadoCarga = COMPLETO;
          tCambioEstadoCarga = ahora;
        }
        break;
      case COMPLETO:
        if (ahora - tCambioEstadoCarga >= duracionPausaMs) estadoCarga = DESCARGANDO;
        break;
      case DESCARGANDO:
        posCarga -= velocidadFilasPorSeg * deltaSeg;
        if (posCarga <= 0.0) {
          posCarga = 0.0;
          estadoCarga = VACIO;
          tCambioEstadoCarga = ahora;
        }
        break;
      case VACIO:
        if (ahora - tCambioEstadoCarga >= duracionPausaMs) {
          estadoCarga = CARGANDO;   // siempre arranca de nuevo por los cuadrantes
        }
        break;
    }

    float progreso = posCarga / (float)LARGO_COMBINADO;
    uint8_t brilloCarga;
    if (estadoCarga == COMPLETO) brilloCarga = 255;
    else if (estadoCarga == VACIO) brilloCarga = 0;
    else brilloCarga = BRILLO_MIN_CARGA + (uint8_t)(progreso * (255 - BRILLO_MIN_CARGA));

    for (int lado = 0; lado < 2; lado++) {
      for (int pos = 1; pos <= LARGO_COMBINADO; pos++) {
        float distanciaDesdeOrigen = cargaDesdeAbajo ? (LARGO_COMBINADO + 1 - pos) : pos;
        if (distanciaDesdeOrigen <= posCarga) {
          CRGB color = colorBarridoPos(pos);
          color.nscale8_video(brilloCarga);
          color.nscale8_video(nivelGlobal);
          escribirPosicionCombinada(lado, pos, color);
        }
      }
    }
    return;
  }
}

// =========================================================
// Forma del latido: dos golpes (lub-dub) a velocidad 100%.
// t en segundos desde el inicio del latido; devuelve 0..1
// =========================================================
float formaLatido21(float t) {
  float a = expf(-powf((t - 0.10) / 0.06, 2.0));
  float b = expf(-powf((t - 0.30) / 0.07, 2.0));
  return a > b ? a : b;
}

uint8_t aU8(float x) {
  return (uint8_t)(constrain(x, 0.0, 1.0) * 255.0);
}

// Color de un segmento del latido: mezcla (maximo por canal) del color BASE
// al nivel "nivelBase" y del color del PULSO al nivel "nivelPulso".
CRGB colorLatido21(float nivelBase, float nivelPulso) {
  CRGB b = COLOR_BASE_LATIDO21;
  CRGB p = COLOR_LATIDO_PIN21;
  b.nscale8_video(aU8(nivelBase * escalaLatido21));
  p.nscale8_video(aU8(nivelPulso * escalaLatido21));
  return CRGB(b.r > p.r ? b.r : p.r,
              b.g > p.g ? b.g : p.g,
              b.b > p.b ? b.b : p.b);
}

// Devuelve el "par" de un segmento (0 = centro, 14 = extremo).
// Segmentos 15 y 16 -> par 0, 14 y 17 -> par 1, ... 1 y 30 -> par 14
int parSegmento21(int i) {
  int n = i + 1;                       // numero de segmento 1..30
  return (n <= 15) ? (15 - n) : (n - 16);
}

// =========================================================
// PIN 21: ciclo completo
//   1) latido con carga (30 seg), simetrica: centro -> extremos
//   2) carga (2 frentes) + explosion (29 seg), con latido continuo en los 30
//   3) descarga de los 29: 15->1 y 16->29 sincronizadas, con latido continuo en los 30
//   4) descarga con latido en los 30, simetrica: extremos -> centro
//   y se repite.  El 26 respira todo el tiempo.
// =========================================================
void actualizarPin21(unsigned long ahora) {
  float deltaSeg = (ahora - tUltimoFramePin21) / 1000.0;
  tUltimoFramePin21 = ahora;
  if (deltaSeg < 0 || deltaSeg > 1.0) deltaSeg = 0;

  unsigned long dt = ahora - tCambioEstadoPin21;

  float factorLat = 100.0 / velocidadLatido21Pct;
  unsigned long durLatido      = (unsigned long)(DUR_LATIDO21_MS * factorLat);
  unsigned long durCargaGrupo  = (unsigned long)(DUR_CARGA_GRUPO21_MS * factorLat);
  unsigned long pausaFinal     = (unsigned long)(PAUSA_FINAL21_MS * factorLat);
  if (durCargaGrupo < 1) durCargaGrupo = 1;

  // ---------------- Maquina de estados ----------------
  if (estadoPin21 == P21_PRE_LATIDO) {
    if (dt >= durLatido) {
      tCambioEstadoPin21 = ahora;
      if (preSegEncendidos21 >= PARES_LATIDO21) {     // era el latido final
        estadoPin21 = P21_PRE_HOLD;
      } else {
        preBase21 = preSegEncendidos21;
        estadoPin21 = P21_PRE_CARGA;
      }
    }
  } else if (estadoPin21 == P21_PRE_CARGA) {
    if (dt >= durCargaGrupo) {
      preSegEncendidos21 = min(preBase21 + GRUPO_PARES21, (int)PARES_LATIDO21);
      estadoPin21 = P21_PRE_LATIDO;
      tCambioEstadoPin21 = ahora;
    }
  } else if (estadoPin21 == P21_PRE_HOLD) {
    if (dt >= pausaFinal) {
      estadoPin21 = P21_CARGANDO;
      posNorm21 = 0.0;
      tCambioEstadoPin21 = ahora;
    }
  } else if (estadoPin21 == P21_CARGANDO) {
    float duracionCarga = BASE_DURACION_CARGA21_SEG / (velocidadCarga21Pct / 100.0);
    posNorm21 += deltaSeg / duracionCarga;
    if (posNorm21 >= 1.0) {
      posNorm21 = 1.0;
      estadoPin21 = P21_EXPLOSION;
      tCambioEstadoPin21 = ahora;
    }
  } else if (estadoPin21 == P21_EXPLOSION) {
    if (dt >= (unsigned long)(tiempoExplosion21Seg * 1000.0)) {
      estadoPin21 = P21_DESCARGANDO;
      posDescarga21 = 0.0;
      tCambioEstadoPin21 = ahora;
    }
  } else if (estadoPin21 == P21_DESCARGANDO) {
    float duracionDescarga = BASE_DURACION_DESCARGA21_SEG / (velocidadDescarga21Pct / 100.0);
    posDescarga21 += deltaSeg / duracionDescarga;
    if (posDescarga21 >= 1.0) {
      posDescarga21 = 0.0;
      estadoPin21 = P21_DESC_LATIDO;              // sigue la descarga de los 30 con latido
      descSeg21 = PARES_LATIDO21;
      descBase21 = PARES_LATIDO21;
      tCambioEstadoPin21 = ahora;
    }
  } else if (estadoPin21 == P21_DESC_LATIDO) {
    if (dt >= durLatido) {
      descBase21 = descSeg21;
      estadoPin21 = P21_DESC_GRUPO;
      tCambioEstadoPin21 = ahora;
    }
  } else { // P21_DESC_GRUPO
    if (dt >= durCargaGrupo) {
      descSeg21 = max(descBase21 - GRUPO_PARES21, 0);
      tCambioEstadoPin21 = ahora;
      if (descSeg21 <= 0) {
        // Todo apagado: se repite el ciclo desde el latido con carga
        estadoPin21 = P21_PRE_LATIDO;
        preSegEncendidos21 = 0;
        preBase21 = 0;
        posNorm21 = 0.0;
        posDescarga21 = 0.0;
      } else {
        estadoPin21 = P21_DESC_LATIDO;
      }
    }
  }

  // Recalcular dt por si hubo cambio de estado en este frame
  dt = ahora - tCambioEstadoPin21;

  // Limpiar los dos tramos; cada fase pinta solo lo suyo
  fill_solid(ledsPin21, SEG_LATIDO + SEG29, CRGB::Black);

  // ---------------- 30 segmentos del latido ----------------
  float tSeg = (dt / 1000.0) * (velocidadLatido21Pct / 100.0);

  if (estadoPin21 == P21_PRE_LATIDO || estadoPin21 == P21_PRE_CARGA || estadoPin21 == P21_PRE_HOLD) {
    bool esFinal = (estadoPin21 == P21_PRE_HOLD) ||
                   (estadoPin21 == P21_PRE_LATIDO && preSegEncendidos21 >= PARES_LATIDO21);

    for (int i = 0; i < SEG_LATIDO; i++) {
      CRGB c;
      int par = parSegmento21(i);     // 0 = centro, 14 = extremo

      if (estadoPin21 == P21_PRE_HOLD) {
        c = colorLatido21(nivelFinalLatido21, 0.0);
      } else if (estadoPin21 == P21_PRE_LATIDO) {
        float base  = (par < preSegEncendidos21) ? nivelSegLatido21 : 0.0;
        float pico  = esFinal ? nivelFinalLatido21 : nivelLatido21;
        float pulso = formaLatido21(tSeg) * pico;
        if (esFinal && tSeg >= 0.30) c = colorLatido21(nivelFinalLatido21, 0.0);   // queda prendido
        else                         c = colorLatido21(base, pulso);
      } else { // P21_PRE_CARGA: los nuevos pares se prenden de a uno desde el centro
        float nivel = 0.0;
        if (par < preBase21) {
          nivel = nivelSegLatido21;
        } else if (par < preBase21 + GRUPO_PARES21) {
          float p = ((float)dt / (float)durCargaGrupo) * GRUPO_PARES21 - (par - preBase21);
          nivel = constrain(p, 0.0, 1.0) * nivelSegLatido21;
        }
        c = colorLatido21(nivel, 0.0);
      }
      ledsPin21[OFFSET_LATIDO + i] = c;
    }

  } else if (estadoPin21 == P21_DESC_LATIDO || estadoPin21 == P21_DESC_GRUPO) {
    // Descarga simetrica: se apagan los pares desde los extremos hacia el centro
    // (1->15 y 30->16 a la vez). Inverso de la carga.
    for (int i = 0; i < SEG_LATIDO; i++) {
      CRGB c;
      int par = parSegmento21(i);     // 0 = centro, 14 = extremo

      if (estadoPin21 == P21_DESC_LATIDO) {
        float base  = (par < descSeg21) ? nivelSegLatido21 : 0.0;
        float pico  = (descSeg21 >= PARES_LATIDO21) ? nivelFinalLatido21 : nivelLatido21;
        float pulso = formaLatido21(tSeg) * pico;
        c = colorLatido21(base, pulso);
      } else { // DESC_GRUPO: los pares de afuera se apagan de a uno (del extremo hacia el centro)
        float nivel = 0.0;
        if (par < descBase21 - GRUPO_PARES21) {
          nivel = nivelSegLatido21;
        } else if (par < descBase21) {
          float p = ((float)dt / (float)durCargaGrupo) * GRUPO_PARES21 - (descBase21 - 1 - par);
          nivel = (1.0 - constrain(p, 0.0, 1.0)) * nivelSegLatido21;
        }
        c = colorLatido21(nivel, 0.0);
      }
      ledsPin21[OFFSET_LATIDO + i] = c;
    }

  } else {
    // CARGANDO / EXPLOSION / DESCARGANDO (29): los 30 segmentos siguen LATIENDO.
    unsigned long periodoLat = (unsigned long)(PERIODO_LATIDO_CONTINUO_MS * factorLat);
    if (periodoLat < 50) periodoLat = 50;
    float tCont = (float)(ahora % periodoLat) / 1000.0 * (velocidadLatido21Pct / 100.0);
    float pulso = formaLatido21(tCont) * nivelFinalLatido21;
    CRGB c = colorLatido21(nivelSegLatido21, pulso);
    for (int i = 0; i < SEG_LATIDO; i++) ledsPin21[OFFSET_LATIDO + i] = c;
  }

  // ---------------- 29 segmentos: carga + explosion + descarga ----------------
  if (estadoPin21 == P21_EXPLOSION) {
    bool encendido = ((dt / PERIODO_PARPADEO_EXPLOSION_MS) % 2) == 0;
    CRGB c = encendido ? COLOR_EXPLOSION_PIN21 : CRGB::Black;
    for (int i = 0; i < SEG29; i++) ledsPin21[OFFSET_SEG29 + i] = c;

  } else if (estadoPin21 == P21_DESCARGANDO) {
    // Dos frentes sincronizados que se apagan desde el centro hacia los extremos:
    //   frente 1: segmento 15 -> 1      frente 2: segmento 16 -> 29
    float front1Pos = posDescarga21 * (NUM_FRONT1 + RAMPA_CARGA21);
    float front2Pos = posDescarga21 * (NUM_FRONT2 + RAMPA_CARGA21);

    for (int j = 0; j < NUM_FRONT1; j++) {            // j = 0 es el segmento 15
      float f = 1.0 - constrain((front1Pos - j) / RAMPA_CARGA21, 0.0, 1.0);
      CRGB c = COLOR_CARGA_PIN21;
      c.nscale8_video((uint8_t)(f * NIVEL_MAX_CARGA21 * 255.0));
      ledsPin21[OFFSET_SEG29 + (NUM_FRONT1 - 1 - j)] = c;
    }
    for (int j = 0; j < NUM_FRONT2; j++) {            // j = 0 es el segmento 16
      float f = 1.0 - constrain((front2Pos - j) / RAMPA_CARGA21, 0.0, 1.0);
      CRGB c = COLOR_CARGA_PIN21;
      c.nscale8_video((uint8_t)(f * NIVEL_MAX_CARGA21 * 255.0));
      ledsPin21[OFFSET_SEG29 + NUM_FRONT1 + j] = c;
    }

  } else if (estadoPin21 == P21_CARGANDO) {
    // Frentes desde los extremos hacia el centro (1->15 y 29->16)
    float front1Pos = posNorm21 * (NUM_FRONT1 + RAMPA_CARGA21);
    float front2Pos = posNorm21 * (NUM_FRONT2 + RAMPA_CARGA21);

    for (int i = 0; i < NUM_FRONT1; i++) {
      float f = constrain((front1Pos - i) / RAMPA_CARGA21, 0.0, 1.0);
      CRGB c = COLOR_CARGA_PIN21;
      c.nscale8_video((uint8_t)(f * NIVEL_MAX_CARGA21 * 255.0));
      ledsPin21[OFFSET_SEG29 + i] = c;
    }
    for (int j = 0; j < NUM_FRONT2; j++) {
      float f = constrain((front2Pos - j) / RAMPA_CARGA21, 0.0, 1.0);
      CRGB c = COLOR_CARGA_PIN21;
      c.nscale8_video((uint8_t)(f * NIVEL_MAX_CARGA21 * 255.0));
      ledsPin21[OFFSET_SEG29 + SEG29 - 1 - j] = c;
    }
  }

  // ---------------- El 26: respiracion constante ----------------
  unsigned long periodoMs = (unsigned long)(PERIODO_RESPIRACION_NUMERO_SEG * 1000.0);
  float fase = (float)(ahora % periodoMs) / (float)periodoMs;
  float onda = (1.0 - cos(fase * 2.0 * PI)) / 2.0;
  uint8_t nivelRespiracion = BRILLO_MIN_RESPIRACION + (uint8_t)(onda * (255 - BRILLO_MIN_RESPIRACION));

  CRGB cNumero = COLOR_NUMERO_PIN21;
  cNumero.nscale8_video(nivelRespiracion);
  for (int i = 0; i < NUM_NUMERO; i++) ledsPin21[OFFSET_NUMERO + i] = cNumero;
}
