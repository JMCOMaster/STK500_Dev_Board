#include "lcd.h"

//#include <stdarg.h>
//#include <stdio.h>
#include <util/delay.h>

void lcd_send(uint8_t value, uint8_t mode);
void lcd_write_nibble(uint8_t nibble);

static uint8_t lcd_displayparams;


void lcd_command(uint8_t command) {
  lcd_send(command, 0);
}

void lcd_putc(uint8_t value) {
  lcd_send(value, 1);
}

void lcd_send(uint8_t value, uint8_t mode)
{
	if(mode)
	LCD_CTRL_PORT |= (1 << LCD_RS);
	else
	LCD_CTRL_PORT &= ~(1 << LCD_RS);

	LCD_CTRL_PORT &= ~(1 << LCD_RW);

	lcd_write_nibble(value >> 4);
	lcd_write_nibble(value);
}

void lcd_write_nibble(uint8_t nibble)
{
	// Limpiar bits D4-D7
	LCD_DATA_PORT &= ~((1 << LCD_D4) |
	(1 << LCD_D5) |
	(1 << LCD_D6) |
	(1 << LCD_D7));

	// Escribir nibble
	if(nibble & 0x01) LCD_DATA_PORT |= (1 << LCD_D4);
	if(nibble & 0x02) LCD_DATA_PORT |= (1 << LCD_D5);
	if(nibble & 0x04) LCD_DATA_PORT |= (1 << LCD_D6);
	if(nibble & 0x08) LCD_DATA_PORT |= (1 << LCD_D7);

	// Pulso Enable
	LCD_CTRL_PORT &= ~(1 << LCD_EN);
	LCD_CTRL_PORT |=  (1 << LCD_EN);
	LCD_CTRL_PORT &= ~(1 << LCD_EN);

	_delay_ms(0.3);
}

void lcd_init(void)
{
	// Control como salida
	LCD_CTRL_DDR |= (1 << LCD_RS) |
	(1 << LCD_RW) |
	(1 << LCD_EN);

	// Datos como salida
	LCD_DATA_DDR |= (1 << LCD_D4) |
	(1 << LCD_D5) |
	(1 << LCD_D6) |
	(1 << LCD_D7);

	_delay_ms(15);

	LCD_CTRL_PORT &= ~(1 << LCD_EN);
	LCD_CTRL_PORT &= ~(1 << LCD_RS);
	LCD_CTRL_PORT &= ~(1 << LCD_RW);

	_delay_ms(4.1);

	lcd_write_nibble(0x03);
	_delay_ms(4.1);

	lcd_write_nibble(0x03);
	_delay_ms(4.1);

	lcd_write_nibble(0x03);
	_delay_ms(4.1);

	lcd_write_nibble(0x02);

	lcd_command(LCD_FUNCTIONSET |
	LCD_4BITMODE |
	LCD_2LINE |
	LCD_5x8DOTS);

	lcd_displayparams = LCD_CURSOROFF | LCD_BLINKOFF;

	lcd_command(LCD_DISPLAYCONTROL | lcd_displayparams);

	lcd_clear();
}

void lcd_on(void) {
  lcd_displayparams |= LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | lcd_displayparams);
}

void lcd_off(void) {
  lcd_displayparams &= ~LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | lcd_displayparams);
}

void lcd_clear(void) {
  lcd_command(LCD_CLEARDISPLAY);
  _delay_ms(2);
}

void lcd_return_home(void) {
  lcd_command(LCD_RETURNHOME);
  _delay_ms(2);
}

void lcd_enable_blinking(void) {
  lcd_displayparams |= LCD_BLINKON;
  lcd_command(LCD_DISPLAYCONTROL | lcd_displayparams);
}

void lcd_disable_blinking(void) {
  lcd_displayparams &= ~LCD_BLINKON;
  lcd_command(LCD_DISPLAYCONTROL | lcd_displayparams);
}

void lcd_enable_cursor(void) {
  lcd_displayparams |= LCD_CURSORON;
  lcd_command(LCD_DISPLAYCONTROL | lcd_displayparams);
}

void lcd_disable_cursor(void) {
  lcd_displayparams &= ~LCD_CURSORON;
  lcd_command(LCD_DISPLAYCONTROL | lcd_displayparams);
}

void lcd_scroll_left(void) {
  lcd_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void lcd_scroll_right(void) {
  lcd_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void lcd_set_left_to_right(void) {
  lcd_displayparams |= LCD_ENTRYLEFT;
  lcd_command(LCD_ENTRYMODESET | lcd_displayparams);
}

void lcd_set_right_to_left(void) {
  lcd_displayparams &= ~LCD_ENTRYLEFT;
  lcd_command(LCD_ENTRYMODESET | lcd_displayparams);
}

void lcd_enable_autoscroll(void) {
  lcd_displayparams |= LCD_ENTRYSHIFTINCREMENT;
  lcd_command(LCD_ENTRYMODESET | lcd_displayparams);
}

void lcd_disable_autoscroll(void) {
  lcd_displayparams &= ~LCD_ENTRYSHIFTINCREMENT;
  lcd_command(LCD_ENTRYMODESET | lcd_displayparams);
}

void lcd_create_char(uint8_t location, uint8_t *charmap) {
  lcd_command(LCD_SETCGRAMADDR | ((location & 0x7) << 3));
  for (int i = 0; i < 8; i++) {
    lcd_putc(charmap[i]);
  }
  lcd_command(LCD_SETDDRAMADDR);
}

void lcd_gotoxy(uint8_t col, uint8_t row) {
  static uint8_t offsets[] = { 0x00, 0x40, 0x14, 0x54 };

  lcd_command(LCD_SETDDRAMADDR | (col + offsets[row]));
}

void lcd_puts(const char *string) {
    const char *it;
    for (it = string; *it; it++) {
        lcd_putc(*it);
    }
}

void lcd_putsf(const char *s) {
    register char c;
    while ((c = pgm_read_byte(s++))) { // Corregido para leer correctamente de la Flash (PROGMEM)
        lcd_putc(c);
    }
}