#include <SPI.h>                           // Библиотека для работы с шиной SPI
#include <nRF24L01.h>                      // Файл настроек для nRF24L01
#include <RF24.h>                          // Библиотека для работы с nRF24L01+
#include <Servo.h>                         // Библиотека для работы с сервоприводами

RF24 radio(9, 10);                         // Создаём объект radio (CE=9, CSN=10)
Servo servo1;                              // Создаём объект для первого сервопривода
Servo servo2;                              // Создаём объект для второго сервопривода

int data[2];                               // Массив для приёма данных
bool toggle = false;                       // Флаг для переключения между сервоприводами
int currentAngle = 0;                      // Текущий угол сервопривода
unsigned long lastMoveTime = 0;            // Время последнего движения
const int moveInterval = 50;               // Интервал между шагами движения (мс)

// Структура для передачи данных на приёмник
struct TelemetryData {
  uint32_t deviceID = 0xABCD1234;          // Уникальный идентификатор устройства
  int angle1 = 0;                          // Угол первого сервопривода (наклон)
  int angle2 = 0;                          // Угол второго сервопривода (поворот)
  int mode = 0;                            // Режим работы (0-ожидание, 1-сканирование)
};

TelemetryData telemetry;                   // Объект для телеметрии

void setup() {
//  Serial.begin(9600);
  delay(1000);
  
  // Подключаем сервоприводы к выводам Arduino
  servo1.attach(3);                        // Первый сервопривод на пине 3
  servo2.attach(4);                        // Второй сервопривод на пине 4
  
  // Инициализация радиомодуля
  radio.begin();                           // Инициируем работу nRF24L01+
  radio.setChannel(5);                     // Канал приёма данных (частота 2,405 ГГц)
  radio.setDataRate(RF24_1MBPS);           // Скорость передачи данных 1 Мбит/сек
  radio.setPALevel(RF24_PA_HIGH);          // Мощность передатчика HIGH (-6dBm)
  
  // Открываем трубу для приёма данных
  uint64_t addressA = 0xABCDABCD01LL;
  uint64_t addressB = 0xABCDABCD02LL;

  radio.openWritingPipe(addressB);   // Пишем устройству B
  radio.openReadingPipe(1, addressA); // Читаем от устройства A  // Идентификатор трубы для передачи
  
  radio.startListening();                  // Включаем режим приёма
  
  // Устанавливаем сервоприводы в начальное положение (0 градусов)
  servo1.write(40);
  servo2.write(65);
  currentAngle = 0;
  
  // Отправляем начальное состояние
  sendTelemetry(0, 0, 0);
  
  Serial.println("Система готова к работе");
  Serial.println("Ожидание сигнала...");
}

void performDiagonalScan2() {
  Serial.println("=== Диагональный скан 2 ===");
  
  for (int i = 0; i <= 80; i += 10) {
    int angle1 = i;
    int angle2 = 103 - i;
    servo1.write(angle1);
    servo2.write(angle2);
    delay(3000);
    // Отправляем углы в градусах (-40..+40)
    sendTelemetry(angle1 - 40, angle2 - 40, 4);
  }
  
  servo1.write(40);
  servo2.write(65);
  delay(1000);
  sendTelemetry(0, 0, 0); // Конец режима
  Serial.println("Все режимы сканирования завершены");
}


// Функция отправки телеметрии на приёмник
void sendTelemetry(int angle1, int angle2, int mode) {
  telemetry.angle1 = angle1;
  telemetry.angle2 = angle2;
  telemetry.mode = mode;
  
  radio.stopListening();                   // Переключаемся в режим передачи
  bool success = radio.write(&telemetry, sizeof(telemetry));
  radio.startListening();                  // Возвращаемся в режим приёма
  
  if (success) {
//    Serial.print("Данные отправлены: A1=");
//    Serial.print(angle1);
//    Serial.print("°, A2=");
//    Serial.print(angle2);
//    Serial.print("°, Режим=");
//    Serial.println(mode);
  } else {
//    Serial.println("Ошибка отправки данных");
  }
}

// Горизонтальный скан
void performHorizontalScan() {
//  Serial.println("=== Горизонтальный скан ===");
  
  // Первый сервопривод от -40° до +40°
  for (int angle = 0; angle <= 80; angle += 10) {
    servo1.write(angle);
    delay(3000);
    // Отправляем текущие углы (угол наклона меняется, поворот = 0)
    sendTelemetry(angle - 40, 0, 1);
  }
  
  // Возвращаем в исходное
  servo1.write(40);
  delay(1000);
  
  // Второй сервопривод от -40° до +40°
  for (int angle = 33; angle <= 100; angle += 10) {
    servo2.write(angle);
    delay(3000);
    // Отправляем текущие углы (наклон = 0, угол поворота меняется)
    sendTelemetry(0, angle - 40, 1);
  }
  
  servo2.write(65);
  delay(1000);
  sendTelemetry(0, 0, 0); // Конец режима
}


// Диагональный скан 1: от (-40,-40) до (+40,+40)
void performDiagonalScan1() {
  Serial.println("=== Диагональный скан 1 ===");
  
  for (int i = 0; i <= 80; i += 10) {
    int angle1 = i;
    int angle2 = i;
    servo1.write(angle1);
    servo2.write(33 + angle2); // Смещение для второго сервопривода
    delay(3000);
    // Отправляем углы в градусах (-40..+40)
    sendTelemetry(angle1 - 40, angle2 - 40, 3);
  }  
//    int angle1 = 0;
//    int angle2 = 0;
//    servo1.write(0);
//    servo2.write(33);
//    sendTelemetry(angle1 - 40, angle2 - 40, 3);
//    angle1+=10;
//    angle2+=10;
//    delay(3000);
//    
//    servo1.write(10);
//    servo2.write(38);
//    sendTelemetry(angle1 - 40, angle2 - 40, 3);
//    angle1+=10;
//    angle2+=10;
//    delay(3000);
//    
//    servo1.write(20);
//    servo2.write(45);
//    sendTelemetry(angle1 - 40, angle2 - 40, 3);
//    angle1+=10;
//    angle2+=10;
//    delay(3000);
//
//
//    servo1.write(30);
//    servo2.write(55);
//    sendTelemetry(angle1 - 40, angle2 - 40, 3);
//    angle1+=10;
//    angle2+=10;
//    delay(3000);
//
//    servo1.write(40);
//    servo2.write(65);
//    sendTelemetry(angle1 - 40, angle2 - 40, 3);
//    angle1+=10;
//    angle2+=10;
//    delay(3000);
//
//    servo1.write(50);
//    servo2.write(77);
//    sendTelemetry(angle1 - 40, angle2 - 40, 3);
//    angle1+=10;
//    angle2+=10;
//    delay(3000);
//
//    servo1.write(60);
//    servo2.write(80);
//    sendTelemetry(angle1 - 40, angle2 - 40, 3);
//    angle1+=10;
//    angle2+=10;
//    delay(3000);
//
//    servo1.write(70);
//    servo2.write(90);
//    sendTelemetry(angle1 - 40, angle2 - 40, 3);
//    angle1+=10;
//    angle2+=10;
//    delay(3000);
//
//    servo1.write(80);
//    servo2.write(100);
//    sendTelemetry(angle1 - 40, angle2 - 40, 3);
//    angle1+=10;
//    angle2+=10;
//    delay(3000);
    
  
  servo1.write(40);
  servo2.write(65);
  delay(1000);
  sendTelemetry(0, 0, 0); // Конец режима
}

// Диагональный скан 2: от (-40,+40) до (+40,-40)
void loop() {
  if (radio.available()) {                 // Если получены данные
    radio.read(&data, sizeof(data));       // Читаем данные
    
    // Проверяем, является ли принятый сигнал командой для движения
    if (data[0] > 0) {
      Serial.print("Получен сигнал: ");
      Serial.println(data[0]);
      Serial.print("Активирован сервопривод: ");
      Serial.println(toggle ? "2 (пин 4)" : "1 (пин 3)");
      
      // Режим 1: Горизонтальный скан (сервоприводы по отдельности)
      performHorizontalScan();
      
      // Режим 2: Вертикальный скан (сервоприводы по отдельности)
//      performVerticalScan();
      // Режим 3: Диагональный скан (оба сервопривода вместе)
      performDiagonalScan1();
      
      // Режим 4: Диагональный скан (в обратную сторону)
      performDiagonalScan2();
      Serial.println("Движение завершено");
      Serial.println("Ожидание следующего сигнала...");
    }
  }
  
  // Небольшая задержка для стабильности работы
  delay(10);
}
