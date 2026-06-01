/* * ATmega2560 8x8 LED Matrix - 64-bit Hex Arrays
 * Control por registros y decodificación de fotogramas uint64_t
 */

extern "C" {
  #include "lcd.h"
}

struct PinMap {
  volatile uint8_t* ddrReg;
  volatile uint8_t* portReg;
  uint8_t bitMask;
};

// 8 FILAS
PinMap filas[8] = {
  {&DDRH, &PORTH, (1 << DDH3)}, // Fila 0
  {&DDRH, &PORTH, (1 << DDH4)}, // Fila 1
  {&DDRH, &PORTH, (1 << DDH5)}, // Fila 2
  {&DDRH, &PORTH, (1 << DDH6)}, // Fila 3
  {&DDRB, &PORTB, (1 << DDB4)}, // Fila 4
  {&DDRB, &PORTB, (1 << DDB5)}, // Fila 5
  {&DDRB, &PORTB, (1 << DDB6)}, // Fila 6
  {&DDRB, &PORTB, (1 << DDB7)}  // Fila 7
};

// 8 COLUMNAS
PinMap columnas[8] = {
  {&DDRE, &PORTE, (1 << DDE4)}, // Columna 0
  {&DDRE, &PORTE, (1 << DDE2)}, // Columna 1
  {&DDRE, &PORTE, (1 << DDE3)}, // Columna 2
  {&DDRE, &PORTE, (1 << DDE7)}, // Columna 3
  {&DDRE, &PORTE, (1 << DDE5)}, // Columna 4
  {&DDRH, &PORTH, (1 << DDH0)}, // Columna 5
  {&DDRH, &PORTH, (1 << DDH1)}, // Columna 6
  {&DDRH, &PORTH, (1 << DDH2)}  // Columna 7
};

struct Animation {
  const uint64_t* frames;     // Puntero al arreglo de fotogramas (en Flash/Memoria)
  uint8_t totalFrames;        // Cantidad de fotogramas en esta secuencia
  uint16_t tiempoPorFrame;    // Tiempo en milisegundos para cada fotograma
  char name[13];              // Etiqueta de texto de hasta 12 caracteres (+1 del terminador nulo)
};

const uint64_t GROW_SQUARE[] = {
  0x0000000000000000,
  0x0000001818000000,
  0x00003c24243c0000,
  0x007e424242427e00,
  0xff818181818181ff
};

const uint64_t EXP_START[] = {
  0x0000000000000000,
  0x0000001818000000,
  0x0000241818240000,
  0x0042241818244200,
  0x8142241818244281,
  0x995a3cffff3c5a99,
  0x66a5c30000c3a566,
  0x995a3cffff3c5a99,
  0x66a5c30000c3a566
};

const uint64_t SNAIL[] = {
  0x0000000000000000,
  0x0000000001010101,
  0x0000000101010100,
  0x0000010101010000,
  0x0001010101000000,
  0x0101010100000000,
  0x0301010000000000,
  0x0701000000000000,
  0x0f00000000000000,
  0x1e00000000000000,
  0x1c10000000000000,
  0x1810100000000000,
  0x1010101000000000,
  0x0010101010000000,
  0x0000101010100000,
  0x0000001010180000,
  0x0000000018180000,
  0x0000000808180000,
  0x0000080808080000,
  0x0008080808000000,
  0x0808080800000000,
  0x0c08080000000000,
  0x0e08000000000000,
  0x0f00000000000000,
  0x0701000000000000,
  0x0301010000000000,
  0x0101010100000000,
  0x0001010101000000,
  0x0000010101010000,
  0x0000000101010100,
  0x0000000001010101,
  0x0000000001010101
};

const uint64_t DIAGONAL[] = {
  0x0000000000000000,
  0x0100000000000000,
  0x0201000000000000,
  0x0402010000000000,
  0x0804020100000000,
  0x1008040201000000,
  0x2010080402010000,
  0x4020100804020100,
  0x8040201008040201,
  0x0080402010080402,
  0x0000804020100804,
  0x0000008040201008,
  0x0000000080402010,
  0x0000000000804020,
  0x0000000000008040,
  0x0000000000000080,
  0x0000000000008040,
  0x0000000000804020,
  0x0000000080402010,
  0x0000008040201008,
  0x0000804020100804,
  0x0080402010080402,
  0x8040201008040201,
  0x4020100804020100,
  0x2010080402010000,
  0x1008040201000000,
  0x0804020100000000,
  0x0402010000000000,
  0x0201000000000000,
  0x0100000000000000
};

const uint64_t FLASH[] = {
  0x0000000000000000,
  0xffffffffffffffff
};

const uint64_t MALLA[] = {
  0xaa55aa55aa55aa55,
  0x55aa55aa55aa55aa
};

const uint64_t CARITAS[] = {
  0x3C42A581A599423C, // Frame 0: Feliz :)
  0x3C42A581BD81423C, // Frame 1: Seria :|
  0x3C42A58199A5423C  // Frame 2: Triste :(
};

const uint64_t EXPAND[] = {
  0x0000000000000000,
  0x0000001818000000,
  0x00003c3c3c3c0000,
  0x007e7e7e7e7e7e00,
  0xffffffffffffffff,
  0xffffffe7e7ffffff,
  0xffffc3dbdbc3ffff,
  0xff81bda5a5bd81ff,
  0x007e425a5a427e00,
  0xff81bda5a5bd81ff,
  0x007e425a5a427e00,
  0xff81bda5a5bd81ff,
  0x007e425a5a427e00,
  0xff81bda5a5bd81ff,
  0x007e425a5a427e00,
  0x00003c24243c0000,
  0x0000001818000000
};



// --- VARIABLES PARA EL STROBE (PA7) ---
unsigned long ultimoCambioStrobe = 0;
uint8_t estadoStrobe = 0; // Controla en qué paso del destello doble estamos


// --- CREACIÓN DE LAS SECUENCIAS (Instanciación de Estructuras) ---
// { Arreglo, Cantidad de Frames, Tiempo por Frame(ms) }
Animation Caritas = { CARITAS, sizeof(CARITAS) / sizeof(CARITAS[0]), 800 ," Caritas    "};
Animation Snail  = { SNAIL,   sizeof(SNAIL) / sizeof(SNAIL[0]), 300 ," Gusano     "};
Animation Diagonal   = { DIAGONAL,   sizeof(DIAGONAL) / sizeof(DIAGONAL[0]), 50 ," Diagonal   "};
Animation Exp_Start  = { EXP_START,   sizeof(EXP_START) / sizeof(EXP_START[0]), 125 ," Estrella   "};
Animation Grow_Square  = { GROW_SQUARE,   sizeof(GROW_SQUARE) / sizeof(GROW_SQUARE[0]), 100 ," Cuadrado   "};
Animation Flash   = { FLASH,   sizeof(FLASH) / sizeof(FLASH[0]), 200  ," Flash      "}; 
Animation Malla   = { MALLA,   sizeof(MALLA) / sizeof(MALLA[0]), 100 ," Cascada    "}; 
Animation Expand   = { EXPAND,   sizeof(EXPAND) / sizeof(EXPAND[0]), 100 ," Callejon    "}; 

Animation* Animation_List[] = { &Expand, &Diagonal, &Exp_Start, &Grow_Square, &Flash, &Malla };
const int totalSecuencias = sizeof(Animation_List) / sizeof(Animation_List[0]);

int secuenciaIdx = 0;              // Índice de la secuencia activa
unsigned long ultimoCambioSecuencia = 0; // Temporizador para los 5 segundos
const unsigned long tiempoPorSecuencia = 5000; // 5000 ms = 5 segundos

// Puntero que controla qué animación se está reproduciendo actualmente
Animation* animacionActual = &Expand;

// Variables de control de tiempo
int frameIdx = 0;
unsigned long ultimoCambioFrame = 0;

void setup() {

  // Inicializa los puertos y la configuración de 4 bits de la LCD
  lcd_init();

  // Enciende la pantalla
  lcd_on();

  // Limpia cualquier residuo
  lcd_clear();

  //Coloca un cursor en la ubicación de escritura del LCD
  //lcd_enable_cursor();

  //Enciende el Backlight del LCD
  DDRK = DDRK | (1<< PK3);
	PORTK |= (1<< PK3);

  // Configurar todas las filas como salidas y APAGADAS (HIGH)
  for (int i = 0; i < 8; i++) {
    *(filas[i].ddrReg)  |= filas[i].bitMask;
    *(filas[i].portReg) |= filas[i].bitMask; 
  }

  // Configurar todas las columnas como salidas y APAGADAS (LOW)
  for (int i = 0; i < 8; i++) {
    *(columnas[i].ddrReg)  |= columnas[i].bitMask;
    *(columnas[i].portReg) &= ~columnas[i].bitMask; 
  }
  // --- CONFIGURAR PA7 (Indicador de Estado) ---
  DDRA |= (1 << DDA7);    // Registro de dirección: Salida
  PORTA &= ~(1 << PORTA7); // Estado inicial: Apagado

  // Escribe en la primera fila
  lcd_gotoxy(0, 0);
  lcd_puts("Matrix 8x8 LEDS");
  
  // Escribe en la segunda fila
  lcd_gotoxy(0, 1);
  lcd_puts("Sec: Callejon");
}

void loop() {
  unsigned long tiempoActual = millis();

  // -------------------------------------------------------------------------
  // TAREA 1: Cambiar de secuencia automática cada 5 segundos
  // -------------------------------------------------------------------------
  if (tiempoActual - ultimoCambioSecuencia >= tiempoPorSecuencia) {

    ultimoCambioSecuencia = millis();
    
    // Avanzar a la siguiente secuencia en el arreglo
    secuenciaIdx++;
    if (secuenciaIdx >= totalSecuencias) {
      secuenciaIdx = 0; // Volver a la primera secuencia
    }
    
    // Actualizar el puntero global y reiniciar el índice de fotogramas
    animacionActual = Animation_List[secuenciaIdx];
    frameIdx = 0; 
    ultimoCambioFrame = millis(); // Evita desfases en el primer frame de la nueva secuencia

    // Actualizar LCD
    lcd_gotoxy(4, 1);
    lcd_puts(animacionActual->name);

  }

  // -------------------------------------------------------------------------
  // TAREA 2: Control asíncrono del fotograma
  // -------------------------------------------------------------------------
  if (millis() - ultimoCambioFrame >= animacionActual->tiempoPorFrame) {
    ultimoCambioFrame = millis();
    frameIdx++;
    
    if (frameIdx >= animacionActual->totalFrames) {
      frameIdx = 0;
    }
  }
  
  // -------------------------------------------------------------------------
  // TAREA 3: Strobe (PA7)
  // -------------------------------------------------------------------------
  switch (estadoStrobe) {
    case 0: // --- Primer Destello (Encendido) ---
      PORTA |= (1 << PORTA7); // Encender PA7
      ultimoCambioStrobe = tiempoActual;
      estadoStrobe = 1;
      break;

    case 1: // Esperar a que termine el primer destello (50ms)
      if (tiempoActual - ultimoCambioStrobe >= 50) {
        PORTA &= ~(1 << PORTA7); // Apagar PA7
        ultimoCambioStrobe = tiempoActual;
        estadoStrobe = 2;
      }
      break;

    case 2: // Esperar la pausa intermedia corta (100ms)
      if (tiempoActual - ultimoCambioStrobe >= 100) {
        PORTA |= (1 << PORTA7); // Segundo Destello (Encendido)
        ultimoCambioStrobe = tiempoActual;
        estadoStrobe = 3;
      }
      break;

    case 3: // Esperar a que termine el segundo destello (50ms)
      if (tiempoActual - ultimoCambioStrobe >= 50) {
        PORTA &= ~(1 << PORTA7); // Apagar PA7 (Pausa larga de oscuridad)
        ultimoCambioStrobe = tiempoActual;
        estadoStrobe = 4;
      }
      break;

    case 4: // Esperar la pausa larga antes de reiniciar el ciclo (1200ms)
      if (tiempoActual - ultimoCambioStrobe >= 1200) {
        estadoStrobe = 0; // Reiniciar máquina de estados
      }
      break;
  }

  // -------------------------------------------------------------------------
  // TAREA 4: Refrescar la matriz constantemente
  // -------------------------------------------------------------------------
  actualizarMatriz(animacionActual->frames[frameIdx]);
}

void actualizarMatriz(uint64_t frameData) {
  for (int f = 0; f < 8; f++) {
    
    // 1. Configurar los estados de las columnas con la orientación corregida 180°
    for (int c = 0; c < 8; c++) {
      
      // CAMBIO: Ahora extraemos el byte usando 'c' directamente en lugar de '7 - c'
      uint8_t byteFila = (uint8_t)((frameData >> (c * 8)) & 0xFF);

      // CAMBIO: Evaluamos el bit simétrico (7 - f) en lugar de 'f' para invertir el eje vertical
      if ((byteFila & (1 << (7 - f))) != 0) {
        *(columnas[c].portReg) |= columnas[c].bitMask;  // Lógica Negada: HIGH apaga
      } else {
        *(columnas[c].portReg) &= ~columnas[c].bitMask; // Lógica Negada: LOW enciende
      }
    }

    // 2. Activar la fila actual (HIGH enciende)
    *(filas[f].portReg) |= filas[f].bitMask;

    // 3. Persistencia de la visión (POV)
    delayMicroseconds(2000); 

    // 4. Desactivar la fila actual para mitigar el ghosting (LOW apaga)
    *(filas[f].portReg) &= ~filas[f].bitMask;
  }
}