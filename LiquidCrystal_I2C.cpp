#include "LiquidCrystal_I2C.h"
#include <inttypes.h>
#include <Arduino.h>
#include <Wire.h>

// ============================================================
// Инициализация после включения питания (по datasheet HD44780):
//   1. Display clear
//   2. Function set: DL=1 (8-bit), N=0 (1-line), F=0 (5x8)
//   3. Display on/off: D=0, C=0, B=0
//   4. Entry mode: I/D=1 (increment), S=0 (no shift)
//
// Сброс Arduino НЕ сбрасывает LCD — нельзя полагаться на
// начальное состояние после reset.
// ============================================================

LiquidCrystal_I2C::LiquidCrystal_I2C(uint8_t lcd_addr, uint8_t lcd_cols,
                                     uint8_t lcd_rows, uint8_t charsize)
{
    _addr = lcd_addr;
    _cols = lcd_cols;
    _rows = lcd_rows;
    _charsize = charsize;
    _backlightval = LCD_BACKLIGHT;
    _lastError = 0;
    _initialized = false;
}

void LiquidCrystal_I2C::begin() {
    Wire.begin();
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

    if (_rows > 1) {
        _displayfunction |= LCD_2LINE;
    }

    // Для 1-строчных дисплеев можно выбрать шрифт 5x10
    if ((_charsize != 0) && (_rows == 1)) {
        _displayfunction |= LCD_5x10DOTS;
    }

    // По datasheet: минимум 40 мс после подъёма питания выше 2.7 В
    delay(50);

    // Сброс экспандера, RS и R/W в 0
    expanderWrite(_backlightval);
    delay(5);   // 5 мс достаточно для стабилизации PCF8574

    // === Перевод в 4-битный режим (datasheet HD44780, рис. 24, стр. 46) ===
    // Начинаем в 8-битном режиме, пытаемся переключить в 4-битный

    // Первая попытка
    write4bits(0x03 << 4);
    delayMicroseconds(4500);  // минимум 4.1 мс

    // Вторая попытка
    write4bits(0x03 << 4);
    delayMicroseconds(4500);  // минимум 4.1 мс

    // Третья попытка
    write4bits(0x03 << 4);
    delayMicroseconds(150);

    // Финально — 4-битный интерфейс
    write4bits(0x02 << 4);

    // Установка: кол-во строк, размер шрифта и т.д.
    command(LCD_FUNCTIONSET | _displayfunction);

    // Включаем дисплей (без курсора и мигания)
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();

    // Очистка
    clear();

    // Направление текста по умолчанию (для европейских языков)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    command(LCD_ENTRYMODESET | _displaymode);

    home();

    _initialized = true;
}

// ============================================================
// Команды высокого уровня
// ============================================================

void LiquidCrystal_I2C::clear() {
    command(LCD_CLEARDISPLAY);
    delayMicroseconds(2000);  // команда выполняется ~1.52 мс
}

void LiquidCrystal_I2C::home() {
    command(LCD_RETURNHOME);
    delayMicroseconds(2000);  // команда выполняется ~1.52 мс
}

void LiquidCrystal_I2C::setCursor(uint8_t col, uint8_t row) {
    // Защита от неинициализированного дисплея
    if (!_initialized || _rows == 0 || _cols == 0) return;

    // Статическая таблица — размещается в .rodata, не тратит стек
    static const uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };

    if (row >= _rows) row = _rows - 1;
    if (col >= _cols) col = _cols - 1;

    command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Включение/выключение дисплея (быстро, без очистки)
void LiquidCrystal_I2C::noDisplay() {
    _displaycontrol &= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::display() {
    _displaycontrol |= LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Курсор-подчёркивание
void LiquidCrystal_I2C::noCursor() {
    _displaycontrol &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::cursor() {
    _displaycontrol |= LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Мигание курсора
void LiquidCrystal_I2C::noBlink() {
    _displaycontrol &= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::blink() {
    _displaycontrol |= LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Прокрутка без изменения DDRAM
void LiquidCrystal_I2C::scrollDisplayLeft() {
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void LiquidCrystal_I2C::scrollDisplayRight() {
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// Направление текста
void LiquidCrystal_I2C::leftToRight() {
    _displaymode |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::rightToLeft() {
    _displaymode &= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// Автопрокрутка
void LiquidCrystal_I2C::autoscroll() {
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::noAutoscroll() {
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// Пользовательские символы (CGRAM, 8 слотов по 8 байт)
void LiquidCrystal_I2C::createChar(uint8_t location, const uint8_t charmap[]) {
    location &= 0x7;  // только 8 слотов: 0-7
    command(LCD_SETCGRAMADDR | (location << 3));
    for (uint8_t i = 0; i < 8; i++) {
        write(charmap[i]);
    }
    // Примечание: после записи в CGRAM адресный счётчик указывает
    // на CGRAM, а не DDRAM. Пользователь должен вызвать setCursor()
    // перед продолжением вывода текста.
}

// Подсветка
void LiquidCrystal_I2C::noBacklight() {
    _backlightval = LCD_NOBACKLIGHT;
    expanderWrite(_backlightval);
}

void LiquidCrystal_I2C::backlight() {
    _backlightval = LCD_BACKLIGHT;
    expanderWrite(_backlightval);
}

bool LiquidCrystal_I2C::getBacklight() const {
    return _backlightval == LCD_BACKLIGHT;
}

// ============================================================
// Команды среднего уровня
// ============================================================

void LiquidCrystal_I2C::command(uint8_t value) {
    send(value, 0);
}

size_t LiquidCrystal_I2C::write(uint8_t value) {
    send(value, Rs);
    return 1;
}

// ============================================================
// Низкоуровневые команды передачи данных
// ============================================================

// Отправка команды или данных (4-битный режим: 2 передачи)
void LiquidCrystal_I2C::send(uint8_t value, uint8_t mode) {
    uint8_t highnib = (value & 0xf0) | mode;
    uint8_t lownib  = ((value << 4) & 0xf0) | mode;
    write4bits(highnib);
    write4bits(lownib);
}

void LiquidCrystal_I2C::write4bits(uint8_t value) {
    expanderWrite(value);
    pulseEnable(value);
}

void LiquidCrystal_I2C::expanderWrite(uint8_t data) {
    Wire.beginTransmission(_addr);
    Wire.write((int)(data) | _backlightval);
    _lastError = Wire.endTransmission();
    // _lastError: 0=OK, 1=данные слишком длинные,
    //             2=NACK по адресу, 3=NACK по данным, 4=другая ошибка
}

void LiquidCrystal_I2C::pulseEnable(uint8_t data) {
    // En high
    expanderWrite(data | En);
    delayMicroseconds(1);   // импульс Enable > 450 нс

    // En low
    expanderWrite(data & ~En);
    delayMicroseconds(50);  // команда требует > 37 мкс для выполнения
}

// ============================================================
// Совместимость со старым API
// ============================================================

void LiquidCrystal_I2C::load_custom_character(uint8_t char_num, const uint8_t *rows) {
    createChar(char_num, rows);
}

void LiquidCrystal_I2C::setBacklight(uint8_t new_val) {
    if (new_val) {
        backlight();
    } else {
        noBacklight();
    }
}

void LiquidCrystal_I2C::printstr(const char c[]) {
    print(c);
}

// ============================================================
// Диагностика
// ============================================================

bool LiquidCrystal_I2C::isConnected() {
    Wire.beginTransmission(_addr);
    _lastError = Wire.endTransmission();
    return _lastError == 0;
}
