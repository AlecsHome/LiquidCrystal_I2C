# Installation #
Create a new folder called "LiquidCrystal_I2C" under the folder named "libraries" in your Arduino sketchbook folder.
Create the folder "libraries" in case it does not exist yet. Place all the files in the "LiquidCrystal_I2C" folder.

# Usage #
To use the library in your own sketch, select it from *Sketch > Import Library*.

-------------------------------------------------------------------------------------------------------------------
This library is based on work done by DFROBOT (www.dfrobot.com).
  Что исправлено
Полностью переработанная версия библиотеки с исправлением критических ошибок, оптимизацией использования памяти и полной русской документацией. 
⚠️ Важно для дисплеев 16x2: Проблема «вывода за пределы экрана» — это аппаратная особенность контроллера HD44780 (у него 80 байт DDRAM, но видимы только 32). В скетче ограничивайте длину строки 16 символами или вызывайте setCursor(0, row) между строками.
