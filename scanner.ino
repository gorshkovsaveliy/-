#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10);  // CE, CSN (такие же пины, как на передатчике)

// Структура для приёма телеметрии (должна совпадать с передатчиком)
struct TelemetryData {
  uint32_t deviceID;
  int angle1;   // угол наклона (-40..+40)
  int angle2;   // угол поворота (-40..+40)
  int mode;     // режим работы (0-ожидание, 1-4 сканирование)
};

TelemetryData receivedData;

// Массив для отправки команд на кубсат
int command[2] = {0, 0}; // command[0] - код команды (1 - запуск), command[1] - резерв

// Идентификаторы режимов для удобочитаемого вывода
const char* modeNames[] = {
  "ОЖИДАНИЕ",
  "ГОРИЗОНТАЛЬНЫЙ СКАН",
  "ВЕРТИКАЛЬНЫЙ СКАН", 
  "ДИАГОНАЛЬНЫЙ СКАН 1",
  "ДИАГОНАЛЬНЫЙ СКАН 2"
};

void setup() {
  Serial.begin(9600);
  
  // Инициализация радиомодуля
  radio.begin();
  radio.setChannel(5);            // Тот же канал, что и на передатчике
  radio.setDataRate(RF24_1MBPS);  // Та же скорость
  radio.setPALevel(RF24_PA_HIGH); // Та же мощность
  
  // Открываем трубы для связи
  uint64_t addressA = 0xABCDABCD01LL;
  uint64_t addressB = 0xABCDABCD02LL;

  // УСТРОЙСТВО B
  radio.openWritingPipe(addressA);   // Пишем устройству A
  radio.openReadingPipe(1, addressB); // Читаем от устройства B
  
  radio.startListening();  // Начинаем прослушивание
  
  Serial.println("=== ПРИЁМНИК СИСТЕМЫ НАВЕДЕНИЯ ===");
  Serial.println("Система готова к работе");
  Serial.println("Команды:");
  Serial.println("  's' - запустить сканирование на кубсате");
  Serial.println("  'r' - сбросить команду");
  Serial.println("-----------------------------------");
}

void loop() {
  // 1. Проверяем команды из Serial
  if (Serial.available()) {
    char cmd = Serial.read();
    
    if (cmd == 's' || cmd == 'S') {
      command[0] = 1;  // Команда запуска
      sendCommand();
      Serial.println("Команда отправлена: ЗАПУСК СКАНИРОВАНИЯ");
    } 
    else if (cmd == 'r' || cmd == 'R') {
      command[0] = 0;  // Сброс команды
      sendCommand();
      Serial.println("Команда сброшена");
    }
  }
  
  // 2. Проверяем входящие данные от кубсата
  if (radio.available()) {
    radio.read(&receivedData, sizeof(receivedData));
    displayTelemetry();
  }
  
  // 3. Небольшая задержка для стабильности
  delay(50);
}

// Функция отправки команды на кубсат
void sendCommand() {
  radio.stopListening();  // Переключаемся на передачу
  
  bool success = radio.write(&command, sizeof(command));
  
  radio.startListening();  // Возвращаемся к прослушиванию
  
  if (success) {
    Serial.println("✓ Команда успешно отправлена");
  } else {
    Serial.println("✗ Ошибка отправки команды");
  }
}

// Функция отображения полученной телеметрии
void displayTelemetry() {
  Serial.println("\n=== ПОЛУЧЕНЫ ДАННЫЕ ===");
  
  // Проверяем ID устройства
  Serial.print("ID устройства: 0x");
  Serial.println(receivedData.deviceID, HEX);
  
  if (receivedData.deviceID != 0xABCD1234) {
    Serial.println("⚠ ВНИМАНИЕ: Неизвестное устройство!");
  }
  
  // Выводим углы
  Serial.print("Угол наклона: ");
  Serial.print(receivedData.angle1);
  Serial.print("° (");
  printAngleBar(receivedData.angle1, -40, 40);
  Serial.println(")");
  
  Serial.print("Угол поворота: ");
  Serial.print(receivedData.angle2);
  Serial.print("° (");
  printAngleBar(receivedData.angle2, -40, 40);
  Serial.println(")");
  
  // Выводим режим
  Serial.print("Режим работы: ");
  if (receivedData.mode >= 0 && receivedData.mode <= 4) {
    Serial.println(modeNames[receivedData.mode]);
  } else {
    Serial.print("Неизвестный (");
    Serial.print(receivedData.mode);
    Serial.println(")");
  }
  
  // Визуализация положения лазера
  Serial.println("Положение лазера на мишени:");
  printTarget(receivedData.angle1, receivedData.angle2);
  
  Serial.println("========================\n");
}

// Вспомогательная функция для графического отображения угла
void printAngleBar(int angle, int minVal, int maxVal) {
  int width = 20;
  int pos = map(angle, minVal, maxVal, 0, width);
  
  Serial.print("[");
  for (int i = 0; i < width; i++) {
    if (i == width/2) {
      Serial.print("|");  // Центр
    } else if (i == pos) {
      Serial.print("▲");  // Текущее положение
    } else {
      Serial.print(".");
    }
  }
  Serial.print("]");
}

// Вспомогательная функция для визуализации мишени 1м x 1м
void printTarget(int angleX, int angleY) {
  // Для расстояния 1 метр: смещение = tan(угол) * 1000 мм
  float distance_mm = 1000.0;  // 1 метр в мм
  
  int posX = (int)(tan(radians(angleY)) * distance_mm);
  int posY = (int)(tan(radians(angleX)) * distance_mm);
  
  // Ограничиваем размер мишени (±500 мм)
  posX = constrain(posX, -500, 500);
  posY = constrain(posY, -500, 500);
  
  Serial.print("  X: ");
  Serial.print(posX);
  Serial.print(" мм, Y: ");
  Serial.print(posY);
  Serial.println(" мм");
  
  // Простая ASCII-визуализация
}
