extern "C" {
  #include "lcd.h"
}

// --- VARIABLES PARA EL KEYPAD ---
int ultimoBotonPresionado = 0; // Almacena el número del botón (1 al 12)
bool teclaPresionada = false;   // Bandera para evitar lecturas repetitivas continuas

// --- Tiempo y registro de secuecias --
unsigned long ultimoCambioSecuencia = 0; //Orden de secuencia
const unsigned long tiempoPorSecuencia = 500; // Muestreo y actualización cada 500 ms

char ultimoCaracter = ' ';  // Guarda el carácter visible
int codigoAscii;        // Guarda el código numérico ASCII


// --- VARIABLES PARA EL LED (PA7) ---
unsigned long ultimoCambioStrobe = 0;
uint8_t estadoStrobe = 0; 

// --- CONTROL DE MODOS ---
#define MODO_STROBE 0
#define MODO_BLINK  1
uint8_t modoActual = MODO_STROBE; // Iniciamos en modo Strobe por defecto


//Inicialización del ADC
void ADC_init() {
  DIDR0 = 0x00;
  DIDR2 = 0x00;
  ADMUX = 0x40;
  ADCSRA = 0x84;
  ADCSRB = 0x00;
}


//Lectura del ADC
unsigned int ADC_read(unsigned char adc_input) {
  ADMUX = (adc_input & 0x1F) | 0X40;
  if (adc_input & 0x20) ADCSRB |= (1 << MUX5);
  else ADCSRB &= ~(1 << MUX5);
  _delay_us(1);
  ADCSRA |= (1 << ADSC);
  while ((ADCSRA & (1 << ADIF)) == 0);
  ADCSRA |= (1 << ADIF);
  return ADCW;
}


// Función de lectura del ADC optimizada con Sobremuestreo (Oversampling)
// Para 12 bits: se toman 16 muestras y el resultado final se divide por 4 (desplazamiento >> 2)
// Para 13 bits: se toman 64 muestras y el resultado final se divide por 8 (desplazamiento >> 3)
unsigned int ADC_read_oversampled(unsigned char adc_input, uint8_t bits_deseados) 
{
    // 1. Configurar multiplexor (Igual que tu código original)
    ADMUX = (adc_input & 0x1F) | 0X40;
    if (adc_input & 0x20) ADCSRB |= (1 << MUX5);
    else ADCSRB &= ~(1 << MUX5);
    _delay_us(1);

    uint32_t acumulador = 0; // Usamos 32 bits para evitar desbordamientos al sumar tantas muestras
    uint16_t numero_muestras = 0;
    uint8_t desplazamiento_bits = 0;

    // 2. Determinar configuración según la resolución buscada
    if (bits_deseados == 12) {
        numero_muestras = 16;  // 4^2
        desplazamiento_bits = 2; // Al final escalamos hacia abajo: (10 bits + 4 bits de ruido) >> 2 = 12 bits
    } else if (bits_deseados == 13) {
        numero_muestras = 64;  // 4^3
        desplazamiento_bits = 3; // (10 bits + 6 bits de ruido) >> 3 = 13 bits
    } else {
        numero_muestras = 1;   // Volver a 10 bits estándar si el parámetro es incorrecto
        desplazamiento_bits = 0;
    }

    // 3. Bucle de muestreo rápido
    for (uint16_t i = 0; i < numero_muestras; i++) {
        ADCSRA |= (1 << ADSC);                 // Iniciar conversión
        while ((ADCSRA & (1 << ADIF)) == 0);   // Esperar conversión
        ADCSRA |= (1 << ADIF);                 // Limpiar bandera de interrupción
        acumulador += ADCW;                    // Acumular lectura
    }

    // 4. Decimación (Desplazamiento a la derecha para obtener los bits requeridos)
    return (unsigned int)(acumulador >> desplazamiento_bits);
}

void setup() {

  //Inicialización del puerto Serial
  Serial.begin(115200);

  // --- CONFIGURAR KEYPAD Y LED EN PORTA ---
  // PA2, PA1, PA0: Salidas (Columnas 1, 2, 3)
  // PA6, PA5, PA4, PA3: Entradas (Filas 1, 2, 3, 4)
  
  DDRA = (DDRA & 0x80) | (1 << DDA2) | (1 << DDA1) | (1 << DDA0); // Salidas
  DDRA = (DDRA & ~( (1<<DDA6)|(1<<DDA5)|(1<<DDA4)|(1<<DDA3)));    // Entradas

  // Activar resistencias Pull-up en las filas y poner las columnas en ALTO (1) por defecto
  PORTA |= (1 << PORTA6) | (1 << PORTA5) | (1 << PORTA4) | (1 << PORTA3) | (1 << PORTA2) | (1 << PORTA1) | (1 << PORTA0);

  // Inicialización del LCD
  lcd_init();
  //Encender el LCD
  lcd_on();
  //Limpiar memoria temporal del LCD
  lcd_clear();

  // Enciende el Backlight del LCD
  DDRK = DDRK | (1 << PK3);
  PORTK |= (1 << PK3);

  // --- CONFIGURAR PA7 (Indicador de Estado) ---
  DDRA |= (1 << DDA7);    
  PORTA &= ~(1 << PORTA7); 
  
  // Mensaje inicial en la pantalla
  lcd_gotoxy(0, 0);
  lcd_puts("IEEE CAS");
  lcd_gotoxy(0, 1);
  lcd_puts("STK 500 TESTING");

  //Esperar 3 segundos
  _delay_ms(3000);
  lcd_clear();
}

void loop() {
  unsigned long tiempoActual = millis();

  // ==========================================
  // 1. CONTROL DE COMANDOS POR PUERTO SERIAL
  // ==========================================
  if (Serial.available() > 0) {
    ultimoCaracter = Serial.read(); // Leemos el carácter (ej: 'A')
    codigoAscii = (int)ultimoCaracter; // Obtenemos su valor numérico ASCII (ej: 65)
  
    // Forzamos una actualización inmediata de la pantalla al presionar la tecla
    ultimoCambioSecuencia = tiempoActual - tiempoPorSecuencia; 
  
    // Alternamos el modo según el carácter recibido
    if (modoActual == MODO_STROBE) {
      modoActual = MODO_BLINK;
      estadoStrobe = 0; 
      PORTA &= ~(1 << PORTA7); 
    } else {
      modoActual = MODO_STROBE;
      estadoStrobe = 0;
      PORTA &= ~(1 << PORTA7);
    }
  }

  // ==========================================
  // 2. MUESTREO DE SENSORES Y REFRESCO DE LCD
  // ==========================================
  if (tiempoActual - ultimoCambioSecuencia >= tiempoPorSecuencia) {
    ultimoCambioSecuencia = tiempoActual;

    // Lecturas de potenciometros y sensor de temperatura (Sobremuestreo a 13 bits para obtener una medición más precisa)
    int PT1 = ADC_read(0x00);   
    int PT2 = ADC_read(0x01);   
    int LM35 = ADC_read_oversampled(0x24, 13);;  

    // Conversión de voltaje y datos
    float voltage_LM35 = (LM35 / 8191.0) * 5.0;
    float temperature = voltage_LM35 * 100.0;
    float Pot1 = (PT1 / 1023.0) * 100.0;
    float Pot2 = (PT2 / 1023.0) * 100.0;
      
    // --- REPORTE POR CONSOLA ---
    Serial.print("T:"); Serial.print(temperature, 1);
    Serial.print(" P1:"); Serial.print(Pot1, 0);
    Serial.print(" P2:"); Serial.print(Pot2, 0);
    Serial.print(" | Rx: '"); Serial.print(ultimoCaracter); 
    Serial.print("' [ASCII: "); Serial.print(codigoAscii);
    Serial.print("] | Modo: "); Serial.print(modoActual == MODO_STROBE ? "STROBE" : "BLINK");
    Serial.print(" | Boton: "); Serial.println(ultimoBotonPresionado);

    // --- BUFFERS PARA VISUALIZACIÓN LCD ---
    char buffer_linea1[17]; 
    char buffer_linea2[17];
    char str_temp[6];
    
    // CONVERSION DE FLOAT e INT
    dtostrf(temperature, 4, 1, str_temp);
    int int_pot1 = (int)Pot1;
    int int_pot2 = (int)Pot2;

    // Formatear Línea 1
    // El formato "%c" pinta el carácter, "%2d" pinta el código ASCII alineado
    snprintf(buffer_linea1, sizeof(buffer_linea1), "T:%sC Rx:%c=%3d", str_temp, ultimoCaracter, codigoAscii);
    
    // Formatear Línea 2
    snprintf(buffer_linea2, sizeof(buffer_linea2), "P1:%2d%%P2:%2d%%B:%2d", int_pot1, int_pot2, ultimoBotonPresionado);

    // --- ESCRIBIR EN EL LCD ---
    lcd_gotoxy(0, 0);          
    lcd_puts(buffer_linea1);
    
    lcd_gotoxy(0, 1);          
    lcd_puts(buffer_linea2);
  }

  // ==========================================
  // 3. MÁQUINA DE ESTADOS DINÁMICA PARA EL LED (PA7)
  // ==========================================
  if (modoActual == MODO_STROBE) {
    // --- COMPORTAMIENTO ORIGINAL (DESTELLO DOBLE STROBE) ---
    switch (estadoStrobe) {
      case 0: 
        PORTA |= (1 << PORTA7); 
        ultimoCambioStrobe = tiempoActual;
        estadoStrobe = 1;
        break;

      case 1: 
        if (tiempoActual - ultimoCambioStrobe >= 50) {
          PORTA &= ~(1 << PORTA7); 
          ultimoCambioStrobe = tiempoActual;
          estadoStrobe = 2;
        }
        break;

      case 2: 
        if (tiempoActual - ultimoCambioStrobe >= 100) {
          PORTA |= (1 << PORTA7); 
          ultimoCambioStrobe = tiempoActual;
          estadoStrobe = 3;
        }
        break;

      case 3: 
        if (tiempoActual - ultimoCambioStrobe >= 50) {
          PORTA &= ~(1 << PORTA7); 
          ultimoCambioStrobe = tiempoActual;
          estadoStrobe = 4;
        }
        break;

      case 4: 
        if (tiempoActual - ultimoCambioStrobe >= 1200) {
          estadoStrobe = 0; 
        }
        break;
    }
  } 
  else if (modoActual == MODO_BLINK) {
    // --- NUEVO COMPORTAMIENTO (BLINK ESTÁNDAR 500ms ON / 500ms OFF) ---
    switch (estadoStrobe) {
      case 0: // Encendido
        PORTA |= (1 << PORTA7);
        ultimoCambioStrobe = tiempoActual;
        estadoStrobe = 1;
        break;

      case 1: // Esperar 500ms encendido
        if (tiempoActual - ultimoCambioStrobe >= 500) {
          PORTA &= ~(1 << PORTA7); // Apagar
          ultimoCambioStrobe = tiempoActual;
          estadoStrobe = 2;
        }
        break;

      case 2: // Esperar 500ms apagado
        if (tiempoActual - ultimoCambioStrobe >= 500) {
          estadoStrobe = 0; // Volver a encender
        }
        break;
    }
  }
  // ==========================================
  // 4. Testeo de Botones (Keypad 4x3)
  // ==========================================

  int botonDetectado = 0;

  // Escaneo columna por columna
  for (uint8_t c = 0; c < 3; c++) {
      // 1. Poner todas las columnas en ALTO
      PORTA |= (1 << PORTA2) | (1 << PORTA1) | (1 << PORTA0);
      
      // 2. Bajar a BAJO (0) únicamente la columna actual en evaluación
      PORTA &= ~(1 << (PORTA2 - c)); 
      
      // Pequeño retardo de estabilización para el cambio de estado eléctrico en los pines
      _delay_us(5); 
      
      // 3. Leer las filas (PINA). Si una fila lee '0', es porque se presionó un botón.
      // Mapeo físico inverso: si el pin está en 0, la fila está activa.
      if (!(PINA & (1 << PINA6))) { botonDetectado = 1 + c;  break; } // Fila 1 (Botones 1, 2, 3)
      if (!(PINA & (1 << PINA5))) { botonDetectado = 4 + c;  break; } // Fila 2 (Botones 4, 5, 6)
      if (!(PINA & (1 << PINA4))) { botonDetectado = 7 + c;  break; } // Fila 3 (Botones 7, 8, 9)
      if (!(PINA & (1 << PINA3))) { botonDetectado = 10 + c; break; } // Fila 4 (Botones 10, 11, 12)
  }

  // Volver a dejar las columnas en ALTO al terminar el escaneo
  PORTA |= (1 << PORTA2) | (1 << PORTA1) | (1 << PORTA0);

  // --- CONTROL DE FLANCO (Detectar pulsación única) ---
  if (botonDetectado != 0) {
      if (!teclaPresionada) {
        
          // Almacenar el botón activo
          ultimoBotonPresionado = botonDetectado;
          teclaPresionada = true; // Bloquear hasta que se suelte la tecla
          
          // Forzar actualización inmediata del LCD
          ultimoCambioSecuencia = tiempoActual - tiempoPorSecuencia;
      }
  } else {
      teclaPresionada = false; // Se soltó la tecla, liberar el bloqueo
  }
}