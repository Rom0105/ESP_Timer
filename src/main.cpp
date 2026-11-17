#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

// Піни I2C для вашої ESP32-S3
#define I2C_SDA 8
#define I2C_SCL 9

// Налаштування дисплея SSD1306 (128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS1307 rtc;

// Масив назв днів тижня (додано квадратні дужки [])
const char* daysOfTheWeek[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Змінні часу та таймера оновлення
DateTime now;
unsigned long previousMillis = 0;
const unsigned long updateInterval = 250; // 4 рази на секунду для плавної анімації

// Прототипи функцій
void initHardware();
void readTime();
void updateDisplay();
void drawCasioFrame();

void setup() {
  Serial.begin(115200);
  initHardware();
}

void loop() {
  unsigned long currentMillis = millis();

  // Оновлення екрану кожні 250 мс (4 рази на секунду)
  if (currentMillis - previousMillis >= updateInterval) {
    previousMillis = currentMillis;
    
    readTime();
    updateDisplay();
  }
}

// 1. Ініціалізація дисплея та годинника
void initHardware() {
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Помилка ініціалізації SSD1306!"));
    while (true) { delay(100); }
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (!rtc.begin()) {
    Serial.println(F("Модуль DS1307 не знайдено!"));
    display.setTextSize(1);
    display.setCursor(10, 28);
    display.println("RTC DS1307 Error!");
    display.display();
    while (true) { delay(100); }
  }

  // Якщо годинник зупинено, встановлюємо час компіляції скетчу
  if (!rtc.isrunning()) {
    Serial.println(F("Встановлення часу компіляції..."));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Якщо час збився на 00:00:00, розкоментуйте рядок нижче на один запуск:
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

// 2. Зчитування поточного часу з DS1307
void readTime() {
  now = rtc.now();
}

// Допоміжна функція малювання рамки у стилі Casio
void drawCasioFrame() {
  // Зовнішній контур із заокругленими кутами
  display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 4, SSD1306_WHITE);
  
  // Верхня лінія під датою
  display.drawLine(4, 15, SCREEN_WIDTH - 5, 15, SSD1306_WHITE);
  
  // Індикатор формату часу "24H" в інверсній рамці
  display.fillRect(103, 3, 21, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(105, 4);
  display.print("24H");
  
  // Повертаємо стандартний білий колір тексту
  display.setTextColor(SSD1306_WHITE);
}

// 3. Оновлення дисплея з анімацією та стилізацією
void updateDisplay() {
  // Буфери для рядків (додано розміри масивів у квадратних дужках)
  char dateBuffer[16];
  char hourBuffer[4];
  char minBuffer[4];
  char secBuffer[4];

  // Форматування дати: Sat 14.02.2026
  snprintf(dateBuffer, sizeof(dateBuffer), "%s %02d.%02d.%04d", 
           daysOfTheWeek[now.dayOfTheWeek()], now.day(), now.month(), now.year());

  // Окреме форматування годин, хвилин та секунд
  snprintf(hourBuffer, sizeof(hourBuffer), "%02d", now.hour());
  snprintf(minBuffer, sizeof(minBuffer), "%02d", now.minute());
  snprintf(secBuffer, sizeof(secBuffer), "%02d", now.second());

  display.clearDisplay();
  drawCasioFrame();

  // --- 1. ВЕРХНІЙ БЛОК: Дата звичайним шрифтом (size = 1) ---
  display.setTextSize(1);
  display.setCursor(6, 4);
  display.print(dateBuffer);

  // --- 2. ЦЕНТРАЛЬНИЙ БЛОК: Години та хвилини ЖИРНИМ/ВЕЛИКИМ (size = 3) ---
  display.setTextSize(3);
  
  // Години
  display.setCursor(10, 23);
  display.print(hourBuffer);

  // Анімована двокрапка (блимає кожні 500 мс)
  bool colonVisible = (millis() / 500) % 2 == 0;
  display.setCursor(46, 23);
  if (colonVisible) {
    display.print(":");
  } else {
    display.print(" ");
  }

  // Хвилини
  display.setCursor(62, 23);
  display.print(minBuffer);

  // --- 3. ПРАВИЙ БЛОК: Секунди звичайним шрифтом (size = 1) ---
  display.setTextSize(1);
  display.setCursor(102, 24);
  display.print("SEC");
  
  display.setCursor(105, 36);
  display.print(secBuffer);

  // --- 4. НИЖНІЙ БЛОК: Анімований прогрес-бар секунд ---
  int barX = 8;
  int barY = 52;
  int barWidth = 112;
  int barHeight = 6;

  // Контур шкали прогрес-бару
  display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

  // Розрахунок ширини заповнення (від 0 до 59 секунд)
  int fillWidth = map(now.second(), 0, 59, 0, barWidth - 4);
  if (fillWidth > 0) {
    display.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, SSD1306_WHITE);
  }

  // Поділки на прогрес-барі кожні 15 секунд
  for (int i = 1; i <= 3; i++) {
    int tickX = barX + (barWidth * i) / 4;
    display.drawPixel(tickX, barY - 2, SSD1306_WHITE);
  }

  display.display();
}
