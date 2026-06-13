extern "C" {
  #include "lcd.h"
}

const int debounceDelay = 50;
unsigned long lastDebounceTime = 0;
bool lastButtonState = HIGH;
bool buttonState = HIGH;

int diceResult = 1;

// --- BUFFERS PARA VISUALIZACIÓN LCD ---
char buffer_linea1[17]; 
char buffer_linea2[17];

void setup() {
  // Inicialización del LCD
  lcd_init();
  lcd_on();
  lcd_clear();

  // Enciende el Backlight del LCD (Pin PK3)
  DDRK = DDRK | (1 << PK3);
  PORTK |= (1 << PK3);

  Serial.begin(115200);
  Serial.println("--- Dado Virtual D20 Iniciado ---");
  Serial.println("Presiona el boton en PE4 para lanzar...");

  // Configuración de registros de entrada/salida (Mapeo de hardware)
  DDRE &= ~(1 << PE4);
  PORTE |= (1 << PE4);
  DDRH |= (1 << PH2) | (1 << PH3) | (1 << PH4) | (1 << PH5) | (1 << PH6);
  DDRB |= (1 << PB4) | (1 << PB6) | (1 << PB5);

  // Mostrar 00 inicialmente en los 7 segmentos
  updateDisplay2(0);
  updateDisplay1(0);

  // Mensaje inicial en la pantalla LCD
  lcd_gotoxy(0, 0);
  lcd_puts("IEEE CAS");
  lcd_gotoxy(0, 1);
  lcd_puts("STK 500 TESTING");

  // Esperar 3 segundos
  _delay_ms(3000);
  lcd_clear();
  
  // Mensaje de espera en LCD
  lcd_gotoxy(0, 0);
  lcd_puts("DADO VIRTUAL D20");
  lcd_gotoxy(0, 1);
  lcd_puts("Listo: Presione");
}

void loop() {
  bool reading = (PINE & (1 << PE4)) ? HIGH : LOW;

  // Mientras el botón esté presionado (LOW), simular el giro rápido del dado
  if (reading == LOW) {
    int efectoVisual = random(1, 21);
    
    // Actualizar los displays de 7 segmentos en tiempo real
    updateDisplay2(efectoVisual / 10);
    updateDisplay1(efectoVisual % 10);
    
    // Mostrar efecto de giro en el LCD
    lcd_gotoxy(0, 0);
    lcd_puts("Lanzando dado... ");
    snprintf(buffer_linea2, sizeof(buffer_linea2), "Valor:   %2d     ", efectoVisual);
    lcd_gotoxy(0, 1);
    lcd_puts(buffer_linea2);

    delay(80); // Velocidad del parpadeo de giro
  }

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      // Al soltar el botón (Transición de LOW a HIGH)
      if (buttonState == HIGH) {
        // Inicializar semilla con el ruido del temporizador
        randomSeed(micros());
        
        // Generar el número final entre 1 y 20
        diceResult = random(1, 21);

        // Puntero para almacenar la cadena del evento de rol
        const char* eventoRol;

        // --- SISTEMA DE LOGICA Y EVENTOS DE ROL ---
        if (diceResult == 20) {
          eventoRol = "CRITICO! ";
          Serial.println("Lanzamiento: [ 20 ] -> ¡CRÍTICO! CRITICAL HIT!");
        } 
        else if (diceResult == 1) {
          eventoRol = "PIFIA... ";
          Serial.println("Lanzamiento: [  1 ] -> ¡PIFIA! FALLO CRÍTICO...");
        } 
        else if (diceResult >= 2 && diceResult <= 5) {
          eventoRol = "Mal Intento";
          Serial.print("Lanzamiento: [ "); Serial.print(diceResult); Serial.println(" ] -> Fallo Deficiente");
        } 
        else if (diceResult >= 6 && diceResult <= 14) {
          eventoRol = "Exito Regular";
          Serial.print("Lanzamiento: [ "); Serial.print(diceResult); Serial.println(" ] -> Logro Normal");
        } 
        else { // Resultados de 15 a 19
          eventoRol = "Buena Tirada!";
          Serial.print("Lanzamiento: [ "); Serial.print(diceResult); Serial.println(" ] -> Gran Éxito");
        }

        // --- ACTUALIZAR PANTALLA LCD CON BUFFERS ---
        snprintf(buffer_linea1, sizeof(buffer_linea1), "Resultado: [%2d] ", diceResult);
        snprintf(buffer_linea2, sizeof(buffer_linea2), "%s", eventoRol);

        lcd_clear();
        lcd_gotoxy(0, 0);          
        lcd_puts(buffer_linea1);
        
        lcd_gotoxy(0, 1);          
        lcd_puts(buffer_linea2);

        // --- ACTUALIZAR DISPLAYS DE 7 SEGMENTOS ---
        updateDisplay2(diceResult / 10); // Decenas
        updateDisplay1(diceResult % 10); // Unidades
      }
    }
  }
  lastButtonState = reading;
}

// Funciones de actualización con el mapeo de registros
void updateDisplay1(uint8_t val) {
  PORTH &= ~((1 << PH2) | (1 << PH3) | (1 << PH4) | (1 << PH5));
  if (val & 0x01) PORTH |= (1 << PH2);
  if (val & 0x02) PORTH |= (1 << PH3);
  if (val & 0x04) PORTH |= (1 << PH4);
  if (val & 0x08) PORTH |= (1 << PH5);
}

void updateDisplay2(uint8_t val) {
  PORTH &= ~(1 << PH6);
  PORTB &= ~((1 << PB4) | (1 << PB5) | (1 << PB6));
  if (val & 0x01) PORTH |= (1 << PH6);
  if (val & 0x02) PORTB |= (1 << PB4);
  if (val & 0x04) PORTB |= (1 << PB6); // C -> Conectado a PB6
  if (val & 0x08) PORTB |= (1 << PB5); // D -> Conectado a PB5
}