#ifndef FDB_LIQUID_CRYSTAL_I2C_H
#define FDB_LIQUID_CRYSTAL_I2C_H

#include <Arduino.h>     // uint8_t, PROGMEM, millis, delay
#include <Print.h>       // базовый класс наследования

// commands
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

#define En 0x04 //  B00000100  // Enable bit
#define Rw 0x02 //  B00000010  // Read/Write bit
#define Rs 0x01 //  B00000001  // Register select bit

/**
 * LiquidCrystal_I2C — драйвер LCD-дисплеев на HD44780 через I2C (PCF8574).
 *
 * После создания экземпляра класса ОБЯЗАТЕЛЬНО вызовите begin().
 * Подсветка включена по умолчанию.
 *
 * ВАЖНО для дисплеев 16x2:
 *   Контроллер HD44780 имеет 80 байт DDRAM, но видимы только 32 (16x2).
 *   При выводе более 16 символов без setCursor() текст уходит в невидимую
 *   область DDRAM (адреса 0x10-0x27 для строки 0). Это аппаратная особенность,
 *   не ошибка библиотеки. Ограничивайте длину строки в своём скетче.
 */
class LiquidCrystal_I2C : public Print {
public:
    /**
     * Конструктор
     *
     * @param lcd_addr   I2C-адрес дисплея (обычно 0x27 или 0x3F)
     * @param lcd_cols   Количество колонок (16 или 20)
     * @param lcd_rows   Количество строк (2 или 4)
     * @param charsize   Размер символа: LCD_5x10DOTS или LCD_5x8DOTS
     * После создания экземпляра ОБЯЗАТЕЛЬНО вызовите begin() —
     * либо без аргументов (геометрия из конструктора), либо begin(cols, rows),
     * либо init() (совместимость с альтернативными версиями библиотеки).
     */
    
    LiquidCrystal_I2C(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows, uint8_t charsize = LCD_5x8DOTS);

    /**
     * Инициализация дисплея. Должна вызываться ПЕРЕД любыми другими операциями.
     */
    void begin();

    /** Совместимость: begin(cols, rows) — задаёт геометрию и инициализирует. */
    void begin(uint8_t cols, uint8_t rows) { _cols = cols; _rows = rows; begin(); }   // <-- сюда

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
    
    /** Код последней I2C-операции: 0=OK, 2=NACK адреса, 3=NACK данных, 4=прочее. */
    uint8_t getLastError() const { return _lastError; }
    
    /**
     * Создать пользовательский символ (всего 8 слотов: 0-7).
     *
     * @param location Номер слота (0-7)
     * @param charmap  Массив из 8 байт, описывающих символ (5x8 пикселей)
     *
     * @warning После createChar() адресный счётчик указывает в CGRAM:
     * последующие записи данных пойдут в CGRAM, а не на экран.
     * Вызовите setCursor()/clear()/home() перед продолжением вывода.
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

    /** Запись одного символа (наследовано от Print). */
    virtual size_t write(uint8_t value) override;

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
     * @return true, если устройство откликнулось.
     */
    bool isConnected();

private:
    void send(uint8_t value, uint8_t mode);
    void write4bits(uint8_t value);
    void expanderWrite(uint8_t data);
    void pulseEnable(uint8_t data);

    uint8_t _addr;
    uint8_t _displayfunction;
    uint8_t _displaycontrol;
    uint8_t _displaymode;
    uint8_t _cols;
    uint8_t _rows;
    uint8_t _charsize;
    uint8_t _backlightval;
    uint8_t _lastError;      // код последней ошибки I2C
    bool    _initialized;    // флаг: begin() был вызван
};

#endif // FDB_LIQUID_CRYSTAL_I2C_H
