/*
 * LiquidCrystal_I2C — драйвер HD44780 через PCF8574 (форк).
 *
 * MIT License
 *
 * Copyright (c) 2026 alecs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FDB_LIQUID_CRYSTAL_I2C_H
#define FDB_LIQUID_CRYSTAL_I2C_H

#define LIQUIDCRYSTAL_I2C_VERSION "1.1.0-alecs"

#include <Arduino.h>     // uint8_t, millis, delay
#include <Print.h>       // базовый класс наследования

// ===== Команды HD44780 =====
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

// Биты PCF8574 -> HD44780
#define LCD_EN 0x04      // Enable
//#define LCD_RW 0x02    // Read/Write (всегда 0, только запись)
#define LCD_RS 0x01      // Register select

// Собственный код ошибки (не пересекается с кодами Wire 1..5):
// метод вызван до успешного begin().
#define LCD_ERR_NOT_INITIALIZED 0xFE

// Дополнительная пауза (мкс) после каждой команды/символа.
// При 100 кГц не нужна. При Wire.setClock(400000) и выше задайте 40
// (например, -DLCD_EXTRA_DELAY_US=40), т.к. HD44780 нужно >37 мкс на команду.
#ifndef LCD_EXTRA_DELAY_US
#define LCD_EXTRA_DELAY_US 0
#endif

/**
 * LiquidCrystal_I2C — драйвер LCD-дисплеев на HD44780 через I2C (PCF8574).
 *
 * После создания экземпляра ОБЯЗАТЕЛЬНО вызовите begin() (или init()).
 * Подсветка включена по умолчанию.
 *
 * Каждый символ/команда передаётся ОДНОЙ I2C-транзакцией (импульсы Enable
 * формируются внутри неё), поэтому вывод быстрее классической реализации.
 *
 * ВАЖНО для дисплеев 16x2:
 *   Контроллер HD44780 имеет 80 байт DDRAM, но видимы только 32 (16x2).
 *   При выводе более 16 символов без setCursor() текст уходит в невидимую
 *   область DDRAM (адреса 0x10-0x27 для строки 0). Это аппаратная особенность,
 *   не ошибка библиотеки. Ограничивайте длину строки в своём скетче.
 *
 * Коды getLastError():
 *   0 = OK, 1 = данные слишком длинные, 2 = NACK по адресу,
 *   3 = NACK по данным, 4 = прочая ошибка, 5 = таймаут (ядра с поддержкой),
 *   0xFE (LCD_ERR_NOT_INITIALIZED) = begin() не вызывался или завершился ошибкой.
 */
class LiquidCrystal_I2C : public Print {
public:
    /**
     * Конструктор
     *
     * @param lcd_addr   I2C-адрес дисплея (обычно 0x27 или 0x3F)
     * @param lcd_cols   Количество колонок (16 или 20)
     * @param lcd_rows   Количество строк (1, 2 или 4; значения >4 ограничиваются до 4)
     * @param charsize   Размер символа: LCD_5x10DOTS или LCD_5x8DOTS
     *                   (5x10 действует только для однострочных дисплеев)
     */
    LiquidCrystal_I2C(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows,
                      uint8_t charsize = LCD_5x8DOTS);

    /**
     * Инициализация дисплея. Должна вызываться ПЕРЕД любыми другими операциями.
     *
     * @warning Вызывается ОДИН раз за сессию питания. Повторный вызов на уже
     *          инициализированном контроллере (он в 4-битном режиме) не
     *          поддерживается: серия «0x3 ×3» будет принята как пары нибблов,
     *          синхронизация интерфейса нарушится. Для смены геометрии —
     *          перезапуск питания дисплея.
     *
     * @param initWire true  — вызвать Wire.begin() (по умолчанию);
     *                 false — шина уже настроена скетчем (Wire.begin(SDA,SCL),
     *                         Wire.setClock(...)), её параметры не трогаются.
     *                         Шина ДОЛЖНА быть уже запущена, иначе на ряде ядер
     *                         (AVR) вызов может не вернуть управление.
     * @return true, если дисплей ответил на шине и инициализирован.
     *         false — дисплей не ответил на самой первой записи (методы
     *         после этого ничего не отправляют) либо шина отказала в процессе
     *         инициализации (методы продолжают работать вхолостую, не зависая,
     *         и снова заработают после восстановления шины; состояние
     *         дисплея в этом случае не гарантировано). Причину смотрите в
     *         getLastError().
     */
    bool begin(bool initWire = true);

    /** Совместимость: begin(cols, rows) — задаёт геометрию и инициализирует. */
    bool begin(uint8_t cols, uint8_t rows, bool initWire = true) {
        _cols = cols;
        _rows = rows;
        return begin(initWire);
    }

    /** Совместимость: init() — идиома из альтернативных версий библиотеки. */
    void init() { begin(); }

    /** Очистка дисплея и возврат курсора в позицию (0,0). */
    void clear();

    /** Возврат курсора в позицию (0,0) без очистки дисплея. */
    void home();

    /** Выключить отображение символов (содержимое DDRAM сохраняется). */
    void noDisplay();

    /** Включить отображение символов. */
    void display();

    /** Выключить мигание курсора. */
    void noBlink();

    /** Включить мигание курсора. */
    void blink();

    /** Скрыть курсор. */
    void noCursor();

    /** Показать курсор (подчёркивание). */
    void cursor();

    /** Прокрутить содержимое дисплея влево. */
    void scrollDisplayLeft();

    /** Прокрутить содержимое дисплея вправо. */
    void scrollDisplayRight();

    /** Направление текста слева направо (по умолчанию). */
    void leftToRight();

    /** Направление текста справа налево. */
    void rightToLeft();

    /** Выключить подсветку. */
    void noBacklight();

    /** Включить подсветку. */
    void backlight();

    /** Получить текущее состояние подсветки. */
    bool getBacklight() const;

    /** Включить автопрокрутку. */
    void autoscroll();

    /** Выключить автопрокрутку. */
    void noAutoscroll();

    /** Код последней I2C-операции (см. список в шапке класса). */
    uint8_t getLastError() const { return _lastError; }

    /**
     * Создать пользовательский символ (всего 8 слотов: 0-7).
     *
     * @param location Номер слота (0-7)
     * @param charmap  Массив из 8 байт (5x8 пикселей)
     *
     * @note После создания автоматически выполняется home(): курсор в (0,0),
     *       цель записи — DDRAM. Можно сразу печатать.
     */
    void createChar(uint8_t location, const uint8_t charmap[]);

    /**
     * Установить позицию курсора.
     *
     * @param col Колонка (0..cols-1)
     * @param row Строка (0..rows-1)
     *
     * @note Если col/row выходят за пределы, они обрезаются до максимума.
     *       Если begin() не вызывался — вызов игнорируется.
     */
    void setCursor(uint8_t col, uint8_t row);

    // Оставляем доступными write(const char*) и write(const uint8_t*, size_t)
    using Print::write;

    /** Запись одного символа (наследовано от Print). Возвращает 0 при ошибке I2C. */
    size_t write(uint8_t value) override;

    /** Отправить команду напрямую. */
    void command(uint8_t value);

    // ===== Совместимость со старым API =====
    inline void blink_on()  { blink(); }
    inline void blink_off() { noBlink(); }
    inline void cursor_on() { cursor(); }
    inline void cursor_off(){ noCursor(); }

    __attribute__((deprecated("Use backlight() / noBacklight() instead")))
    void setBacklight(uint8_t new_val);

    __attribute__((deprecated("Use createChar() instead")))
    void load_custom_character(uint8_t char_num, const uint8_t *rows);

    void printstr(const char c[]);

    /**
     * Проверка связи с дисплеем по I2C.
     * Wire.begin() здесь НЕ вызывается: шина должна быть уже запущена
     * (Wire.begin() в скетче или begin() библиотеки); иначе поведение не
     * определено (на AVR вызов может не вернуть управление).
     * @return true, если устройство откликнулось.
     */
    bool isConnected();

private:
    void send(uint8_t value, uint8_t mode);
    void write4bits(uint8_t value);
    void expanderWrite(uint8_t data);

    uint8_t _addr;
    uint8_t _displayfunction;
    uint8_t _displaycontrol;
    uint8_t _displaymode;
    uint8_t _cols;
    uint8_t _rows;
    uint8_t _charsize;       // нужен для begin(cols, rows), где _rows меняется
    uint8_t _backlightval;
    uint8_t _lastError;      // код последней ошибки I2C
    bool    _initialized;    // флаг: begin() завершился успешно
};

#endif // FDB_LIQUID_CRYSTAL_I2C_H