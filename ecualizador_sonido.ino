/*
 * Ecualizador / VU-metro de sonido — LCD1602 I2C + sensor de sonido
 * Arduino UNO R4 WiFi
 * ------------------------------------------------------------
 * Barra horizontal en la fila de abajo que crece con el volumen
 * (resolución de hasta 80 pasos usando caracteres personalizados),
 * y el nivel en % en la esquina de la fila de arriba.
 *
 * CABLEADO:
 *   LCD1602 (módulo I2C)     Sensor de sonido
 *   VCC -> 5V                VCC -> 5V
 *   GND -> GND                GND -> GND
 *   SDA -> A4                 AO  -> A0
 *   SCL -> A5                 (DO sin conectar)
 *
 * Librería necesaria: "LiquidCrystal_I2C" (instalar desde el
 * Gestor de Librerías del Arduino IDE).
 *
 * Si al subir el código no ves nada en la pantalla (ni cursor,
 * ni nada), prueba a cambiar 0x27 por 0x3F en la línea de abajo
 * — son las dos direcciones I2C más comunes de estos módulos.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define PIN_SONIDO A0

// Caracteres personalizados: barra parcial de 1 a 4 columnas rellenas
// (de 5 posibles por celda). El relleno completo (5/5) usa el
// carácter 255, que ya viene integrado en el LCD (bloque sólido).
byte barra1[8] = {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
byte barra2[8] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18};
byte barra3[8] = {0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C};
byte barra4[8] = {0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E};

float nivelActual = 0;  // 0.0 a 1.0, suavizado

void setup() {
  lcd.init();
  lcd.backlight();

  lcd.createChar(1, barra1);
  lcd.createChar(2, barra2);
  lcd.createChar(3, barra3);
  lcd.createChar(4, barra4);

  lcd.setCursor(0, 0);
  lcd.print("ECUALIZADOR");
}

// Pico a pico durante ~15ms: refleja mejor el volumen real que
// una sola lectura suelta, que salta demasiado
int leerPicoSonido() {
  int minV = 1023, maxV = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < 15) {
    int v = analogRead(PIN_SONIDO);
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
  }
  return maxV - minV;
}

void dibujarBarra(float nivel) {
  int ticks = (int)(nivel * 80.0); // 16 columnas x 5 sub-niveles = 80
  if (ticks > 80) ticks = 80;

  lcd.setCursor(0, 1);
  for (int col = 0; col < 16; col++) {
    int ticksCol = ticks - col * 5;
    if (ticksCol >= 5) {
      lcd.write((byte)255);   // columna llena
    } else if (ticksCol <= 0) {
      lcd.print(" ");         // columna vacía
    } else {
      lcd.write((byte)ticksCol); // columna parcial (1 a 4)
    }
  }
}

void loop() {
  int picoBruto = leerPicoSonido();

  // *** CALIBRACIÓN: 10 y 300 son un punto de partida, no un valor
  // medido para tu sensor/habitación. Descomenta el Serial.println
  // de abajo, mira los valores reales en silencio y hablando/con
  // música, y ajusta estos dos números. ***
  // Serial.println(picoBruto);
  float nuevoNivel = constrain((picoBruto - 10) / 300.0, 0.0, 1.0);

  // Sube rápido, baja despacio (como un VU meter real)
  if (nuevoNivel > nivelActual) nivelActual = nuevoNivel;
  else nivelActual = nivelActual * 0.85 + nuevoNivel * 0.15;

  dibujarBarra(nivelActual);

  lcd.setCursor(12, 0);
  lcd.print("    "); // limpia el hueco antes de escribir el %
  lcd.setCursor(12, 0);
  lcd.print((int)(nivelActual * 100));
  lcd.print("%");
}
