extern "C" {
  #include "lcd.h"
}

// Variables para el control del antirrebote del botón
const int debounceDelay = 50;
unsigned long lastDebounceTime = 0;
bool lastButtonState = HIGH;
bool buttonState = HIGH;

// Variable del contador (0 - 99)
int counter = 0;

// --- BUFFERS PARA VISUALIZACIÓN LCD ---
char buffer_linea1[17]; 
char buffer_linea2[17];

// Función para generar la barra de progreso en texto
void generarBarraProgreso(char* buffer, int valor, int maxValor) {
  int bloques = (valor * 14) / maxValor; // 14 caracteres disponibles entre los corchetes []
  int i;
  buffer[0] = '[';
  for (i = 1; i <= 14; i++) {
    if (i <= bloques) {
      buffer[i] = '='; // Carácter de llenado
    } else {
      buffer[i] = ' '; // Espacio vacío
    }
  }
  buffer[15] = ']';
  buffer[16] = '\0'; // Fin de cadena
}

void setup() {
  // Inicialización del LCD
  lcd_init();
  lcd_on();
  lcd_clear();

  // Enciende el Backlight del LCD (Pin PK3)
  DDRK = DDRK | (1 << PK3);
  PORTK |= (1 << PK3);

  // Inicializar el puerto serial a 115200 bps
  Serial.begin(115200);
  Serial.println("--- Contador BCD Iniciado ---");
  Serial.print("Valor inicial: ");
  Serial.println(counter);

  // 1. Configurar PE4 como entrada con resistencia pull-up interna activada
  DDRE &= ~(1 << PE4);  // Dirección: Entrada (0)
  PORTE |= (1 << PE4);  // Estado: Pull-up activado (1)

  // 2. Configurar pines del Display 1 (PH2, PH3, PH4, PH5) como salidas (Nuevo Mapeo)
  DDRH |= (1 << PH2) | (1 << PH3) | (1 << PH4) | (1 << PH5);

  // 3. Configurar pines del Display 2 (PH6, PB4, PB6, PB5) como salidas (Nuevo Mapeo)
  DDRH |= (1 << PH6);
  DDRB |= (1 << PB4) | (1 << PB6) | (1 << PB5);

  // Inicializar ambos displays mostrando un '0'
  updateDisplay1(0);
  updateDisplay2(0);

  // Mensaje de bienvenida en el LCD
  lcd_gotoxy(0, 0);
  lcd_puts("IEEE CAS");
  lcd_gotoxy(0, 1);
  lcd_puts("STK 500 TESTING");
  _delay_ms(2000);
  lcd_clear();

  // Forzar la primera impresión del estado 0 en el LCD
  mostrarDatosEnLCD();
}

void loop() {
  // Leer el estado actual del botón en el pin PE4 (activo en bajo por el pull-up)
  bool reading = (PINE & (1 << PE4)) ? HIGH : LOW;

  // Comprobar si hubo un cambio de estado para reiniciar el temporizador
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Si el estado ha sido estable durante el tiempo de debounce
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      // Incrementar el contador solo si el botón fue presionado (transición a LOW)
      if (buttonState == LOW) {
        counter++;
        if (counter > 99) {
          counter = 0; // Reiniciar al llegar a 100
        }

        // Enviar el valor actual por el Monitor Serial
        Serial.print("Contador: ");
        Serial.println(counter);

        // Actualizar la pantalla LCD con los nuevos datos
        mostrarDatosEnLCD();

        // Extraer decenas y unidades
        int decenas = counter / 10;
        int unidades = counter % 10;

        // Actualizar los displays de 7 segmentos con el mapeo corregido
        updateDisplay2(decenas); 
        updateDisplay1(unidades);
      }
    }
  }
  
  // Guardar el estado actual para la siguiente iteración
  lastButtonState = reading;
}

// Función centralizada para formatear y enviar datos al LCD
void mostrarDatosEnLCD() {
  // Formatear Línea 1: Muestra el valor numérico alineado
  snprintf(buffer_linea1, sizeof(buffer_linea1), "CONTEO:    %2d   ", counter);
  
  // Formatear Línea 2: Genera la barra analógica [====          ]
  generarBarraProgreso(buffer_linea2, counter, 99);

  // Escribir de forma segura en el LCD
  lcd_gotoxy(0, 0);          
  lcd_puts(buffer_linea1);
  
  lcd_gotoxy(0, 1);          
  lcd_puts(buffer_linea2);
}

// Funciones de actualización con el mapeo físico corregido de tu PCB
void updateDisplay1(uint8_t val) {
  PORTH &= ~((1 << PH2) | (1 << PH3) | (1 << PH4) | (1 << PH5));
  if (val & 0x01) PORTH |= (1 << PH2); // A -> PH2
  if (val & 0x02) PORTH |= (1 << PH3); // B -> PH3
  if (val & 0x04) PORTH |= (1 << PH4); // C -> PH4
  if (val & 0x08) PORTH |= (1 << PH5); // D -> PH5
}

void updateDisplay2(uint8_t val) {
  PORTH &= ~(1 << PH6);
  PORTB &= ~((1 << PB4) | (1 << PB5) | (1 << PB6));
  if (val & 0x01) PORTH |= (1 << PH6); // A -> PH6
  if (val & 0x02) PORTB |= (1 << PB4); // B -> PB4
  if (val & 0x04) PORTB |= (1 << PB6); // C -> PB6
  if (val & 0x08) PORTB |= (1 << PB5); // D -> PB5
}