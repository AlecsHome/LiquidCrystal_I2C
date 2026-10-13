#include "LiquidCrystal_I2C.h"
#include <inttypes.h>
#include <Arduino.h>
#include <Wire.h>

// ============================================================
// Инициализация после включения питания (HD44780, рис. 24 datasheet,
// «Initializing by Instruction», 4-битный интерфейс):
//   1. Пауза >40 мс после подъёма питания
//   2. Трижды 0x3 (8-битный режим) с паузами 4.1 мс / 100 мкс
//   3. 0x2 — переход в 4-битный режим
//   4. Function set (строки, шрифт), Display on/off, Clear, Entry mode
//
// Сброс Arduino НЕ сбрасывает LCD — нельзя полагаться на
// начальное состояние после reset.
//
// ВАЖНО: последовательность «0x3 x3» рассчитана на неизвестное/8-битное
// состояние контроллера. begin() вызывается один раз за сессию питания.
// ============================================================

LiquidCrystal_I2C::LiquidCrystal_I2C(uint8_t lcd_addr, uint8_t lcd_cols,
                                     uint8_t lcd_rows, uint8_t charsize)
{
    _addr = lcd_addr;
    _cols = lcd_cols;
    _rows = (lcd_rows > 4) ? 4 : lcd_rows;
    _charsize = charsize;
    _backlightval = LCD_BACKLIGHT;
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    _displaycontrol  = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    _displaymode     = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    _lastError = LCD_ERR_NOT_INITIALIZED;
    _initialized = false;
}

bool LiquidCrystal_I2C::begin(bool initWire) {
    _initialized = false;

    if (_rows > 4) _rows = 4;   // begin(cols, rows) мог задать геометрию напрямую

    if (initWire) {
        Wire.begin();
    }

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

    // Сброс экспандера, RS и R/W в 0. Заодно проверка наличия дисплея.
    expanderWrite(_backlightval);
    if (_lastError != 0) {
        return false;   // дисплей не ответил — _initialized остаётся false
    }
    delay(5);

    // === Перевод в 4-битный режим ===
    // Начинаем «вслепую» в неизвестном состоянии и трижды шлём 0x3.

    write4bits(0x03 << 4);
    delayMicroseconds(4500);  // минимум 4.1 мс

    write4bits(0x03 << 4);
    delayMicroseconds(4500);  // минимум 4.1 мс

    write4bits(0x03 << 4);
    delayMicroseconds(150);   // минимум 100 мкс

    // Финально — 4-битный интерфейс
    write4bits(0x02 << 4);
    delayMicroseconds(150);

    // С этого момента command()/write() работают.
    _initialized = true;

    // Кол-во строк, размер шрифта
    command(LCD_FUNCTIONSET | _displayfunction);

    // Включаем дисплей (без курсора и мигания)
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();

    clear();

    // Направление текста по умолчанию (для европейских языков)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    command(LCD_ENTRYMODESET | _displaymode);

    home();

    return _lastError == 0;
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

    // Смещения строк DDRAM зависят от ширины:
    //   20x4: 0x00, 0x40, 0x14, 0x54
    //   16x4: 0x00, 0x40, 0x10, 0x50
    const uint8_t row_offsets[4] = {
        0x00, 0x40, _cols, (uint8_t)(0x40 + _cols)
    };

    if (row >= _rows || row >= 4) row = (_rows - 1 < 3) ? _rows - 1 : 3;
    if (col >= _cols) col = _cols - 1;

    command(LCD_SETDDRAMADDR | (uint8_t)(col + row_offsets[row]));
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
    if (!_initialized) { _lastError = LCD_ERR_NOT_INITIALIZED; return; }

    location &= 0x7;  // только 8 слотов: 0-7
    command(LCD_SETCGRAMADDR | (location << 3));
    for (uint8_t i = 0; i < 8; i++) {
        write(charmap[i]);
    }
    // Возврат в DDRAM (0,0): после записи AC указывает в CGRAM,
    // и последующие print() ушли бы в CGRAM, а не на экран.
    // home() содержит нужную паузу ~1.5 мс.
    home();
}

// Подсветка. До begin() только запоминаем состояние (шина ещё не запущена),
// begin() применит его при первой записи в экспандер.
void LiquidCrystal_I2C::noBacklight() {
    _backlightval = LCD_NOBACKLIGHT;
    if (_initialized) expanderWrite(_backlightval);
}

void LiquidCrystal_I2C::backlight() {
    _backlightval = LCD_BACKLIGHT;
    if (_initialized) expanderWrite(_backlightval);
}

bool LiquidCrystal_I2C::getBacklight() const {
    return _backlightval == LCD_BACKLIGHT;
}

// ============================================================
// Команды среднего уровня
// ============================================================

void LiquidCrystal_I2C::command(uint8_t value) {
    if (!_initialized) { _lastError = LCD_ERR_NOT_INITIALIZED; return; }
    send(value, 0);
}

size_t LiquidCrystal_I2C::write(uint8_t value) {
    if (!_initialized) { _lastError = LCD_ERR_NOT_INITIALIZED; return 0; }
    send(value, LCD_RS);
    return (_lastError == 0) ? 1 : 0;
}

// ============================================================
// Низкоуровневые команды передачи данных
// ============================================================

// Отправка команды или данных (4-битный режим) ОДНОЙ I2C-транзакцией:
// на каждый ниббл — байт с En=1 и байт с En=0. Длительность En=1 определяется
// временем передачи байта по I2C (десятки мкс) и с запасом превышает 450 нс.
void LiquidCrystal_I2C::send(uint8_t value, uint8_t mode) {
    const uint8_t hi = (value & 0xF0) | mode | _backlightval;
    const uint8_t lo = ((value << 4) & 0xF0) | mode | _backlightval;

    Wire.beginTransmission(_addr);
    Wire.write((uint8_t)(hi | LCD_EN));
    Wire.write(hi);
    Wire.write((uint8_t)(lo | LCD_EN));
    Wire.write(lo);
    _lastError = Wire.endTransmission();

#if LCD_EXTRA_DELAY_US > 0
    delayMicroseconds(LCD_EXTRA_DELAY_US);
#endif
}

// Один ниббл (старшие 4 бита value) — только для последовательности
// инициализации в begin(); там паузы выдерживаются вызывающим кодом.
void LiquidCrystal_I2C::write4bits(uint8_t value) {
    value |= _backlightval;
    Wire.beginTransmission(_addr);
    Wire.write((uint8_t)(value | LCD_EN));
    Wire.write(value);
    _lastError = Wire.endTransmission();
}

void LiquidCrystal_I2C::expanderWrite(uint8_t data) {
    Wire.beginTransmission(_addr);
    Wire.write((uint8_t)(data | _backlightval));
    _lastError = Wire.endTransmission();
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

// Шина должна быть уже запущена (Wire.begin() в скетче или begin() библиотеки).
bool LiquidCrystal_I2C::isConnected() {
    Wire.beginTransmission(_addr);
    _lastError = Wire.endTransmission();
    return _lastError == 0;
}