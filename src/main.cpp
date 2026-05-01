#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "DS18B20.h"
#include <LiquidCrystal_I2C.h>


#define ONE_WIRE_BUS PA3
#define BUTTON_PIN PB3
#define VALVE PA1

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

LiquidCrystal_I2C lcd(0x27,16,2);

//Переменные датчика
float temp = 0;
//Переменные настройки
float targetTemp = 30;
float g = 0.5;
float startTemp = 0; 
//Переменные таймера
int minute = 5;
int second = 0;
//Флаг тикать или не тикать
bool timerRun = true;
//Флаги клапана
bool valveFlag = false;



//Состояния системы дисплея и всего остального
enum DisplayMode 
{ 
  WAIT,           //главный экран ожидание
  OTBOR,          //режим отбора голов / спирта
  STOP,           //полная / аварийная остановка
  FINISH,         //окончание отбора
  SET_UP,         //настройка гистерезиса
  STABILIZATION,  //режим стабилизации
  OTBOR_WAIT,     //ожидание отбора
  SET_TARGET,     //установка температуры когда начнется стабилизация
  TIMER_STAB,      //таймер стабилизации перед началом отбора
  ALARM
};

// Состояния кнопки
enum ButtonMode {NOPRESS, PRESS, DUR};
// Состояние системы
//enum SystemState {IDLE};

DisplayMode currentmode = WAIT;
//volatile потому что currenButton будут менять разные функции
volatile ButtonMode currentButton = NOPRESS;
//SystemState currentSystem = IDLE;



//Заголовки задач
TaskHandle_t task1;
TaskHandle_t task2;
TaskHandle_t task3;
TaskHandle_t task4;
TaskHandle_t task5;
TaskHandle_t task6;
TaskHandle_t task7;

// Мьютекс для защиты Serial
SemaphoreHandle_t serialMutex;
// Мьютекс для lcd
SemaphoreHandle_t lcdMutex;
// Мьютекс для защиты переменной температуры
SemaphoreHandle_t tempMutex;

// Семафоры
//Семафор таймера
SemaphoreHandle_t TimerSimaphore = NULL;


/*************************************************************************************************************************************************/
//Задача 1
void Task1(void *parametr)
{
  
  pinMode(PC13, OUTPUT);
  
  while(1){
    digitalWrite(PC13, LOW);
    vTaskDelay(1000);
    digitalWrite(PC13, HIGH);
    vTaskDelay(1000);
  }
}



// Задача 2
void Task2(void *parametr)
{
  while(1)
  {
    if (valveFlag == true)
    {
      if (xSemaphoreTake(serialMutex, portMAX_DELAY)==pdTRUE)
        {
          Serial.println("Клапан открыт!");
          xSemaphoreGive(serialMutex);
        }
    }
    if (valveFlag == false)
    {
      if (xSemaphoreTake(serialMutex, portMAX_DELAY)==pdTRUE)
      {
         Serial.println("Клапан закрыт!");
         xSemaphoreGive(serialMutex);
      }
    }
    
  vTaskDelay(5000);
  }
}


/*************************************************************************************************************************************************/
//Задача 3 работа с датчиком температуры
void Task3(void *parametr)
{
  while(1){
    sensor.requestTemperatures();                             //запрашиваем температуру
    unsigned long temptime = millis();                        //
    while (!sensor.isConversionComplete())                    //
    {                                                         //
      if (millis() - temptime > 1000){                        //конвертация температуры 1 сек
        break;                                                //
      }                                                       //
      vTaskDelay(10);                                         //
  }

    if (xSemaphoreTake(serialMutex, portMAX_DELAY)==pdTRUE)
    {
      Serial.println(sensor.getTempC());                        //выводим температуру в сериал
      
      xSemaphoreGive(serialMutex);
    }
    
    if (xSemaphoreTake(tempMutex, portMAX_DELAY)==pdTRUE)
    {
      temp = sensor.getTempC();                                 //пишем температуру в переменную temp
      xSemaphoreGive(tempMutex);
    }
    
    vTaskDelay(1000);
  }
}

/*************************************************************************************************************************************************/
//Задача 4 работа с дисплеем
void Task4(void *parametr)
{
  static unsigned long counter = 0;
  static bool star = false;
  while(1){
    
    switch(currentmode)
    {
      case WAIT:
        if (xSemaphoreTake(lcdMutex, portMAX_DELAY)==pdTRUE)
        {
          lcd.setCursor(0, 0);
          lcd.print("Temp: ");
          lcd.print(temp);
          lcd.print(" C  ");
          lcd.setCursor(0, 1);
          lcd.print("Wait! Press BTN");
          xSemaphoreGive(lcdMutex);
        }
        vTaskDelay(500);
        break;
      case SET_UP:
        if (xSemaphoreTake(lcdMutex, portMAX_DELAY)==pdTRUE)
        {
          lcd.setCursor(0, 0);
          lcd.print("Gysteresis ");
          lcd.setCursor(11, 0);
          lcd.print(g,1); //Выводим с одним знаком после запятой
          lcd.print("  ");
          lcd.setCursor(0, 1);
          lcd.print("Press BTN +0.5");
                    
          xSemaphoreGive(lcdMutex);
        }
        break;
      case STABILIZATION:
        if (xSemaphoreTake(lcdMutex, portMAX_DELAY)==pdTRUE)
        {
          lcd.setCursor(0,0);
          lcd.print("Wait stable...   ");
          lcd.setCursor(0, 1);
          lcd.print(temp, 1);
          lcd.setCursor(5, 1);
          lcd.print("<>");
          lcd.setCursor(8, 1);
          lcd.print(targetTemp, 1);
          if(millis() - counter > 500)
          {
            counter = millis();
            lcd.setCursor(15, 1);
            lcd.print("*");
          }
          else
          {
            lcd.setCursor(15, 1);
            lcd.print(" ");
          }
          xSemaphoreGive(lcdMutex);
        }
        break;
      case SET_TARGET:
        if (xSemaphoreTake(lcdMutex, portMAX_DELAY)==pdTRUE)
        {
         lcd.setCursor(0, 0);
         lcd.print("Target temp");
         lcd.setCursor(12, 0);
         lcd.print(targetTemp);
         lcd.setCursor(0,1);
         lcd.print("Press BTN max 80");
         xSemaphoreGive(lcdMutex);
        }
        break;
      case TIMER_STAB:
        if (xSemaphoreTake(lcdMutex, portMAX_DELAY)==pdTRUE)
        {
          lcd.setCursor(0, 0);
          lcd.print("Stabilization..");
          lcd.setCursor(0, 1);
          if (minute < 10) lcd.print("0");
          lcd.print(minute);
          lcd.print(":");
          lcd.setCursor(3, 1);
          if (second < 10) lcd.print("0");
          lcd.print(second);
          lcd.print(" T:");
          lcd.print(temp, 1);
          lcd.print(" C  ");

          xSemaphoreGive(lcdMutex) ;
        }
        break;
      case OTBOR:
        if (xSemaphoreTake(lcdMutex, portMAX_DELAY)==pdTRUE)
        {
          lcd.setCursor(0, 0);
          lcd.print("Otbor T:");
          lcd.print(startTemp);
          lcd.setCursor(0, 1);
          lcd.print("Column T:");
          lcd.print(temp);
          if (valveFlag == true) 
          {
            lcd.setCursor(15, 0);
            lcd.print("*");
          }
          else if (valveFlag == false)
          {
            lcd.setCursor(15, 0);
            lcd.print(" ");
          }
          if (millis() - counter > 500)
          {
            counter = millis();
            lcd.setCursor(15, 1);
            lcd.print("*");
          }
          else
          {
            lcd.setCursor(15, 1);
            lcd.print(" ");
          }
          xSemaphoreGive(lcdMutex);
        }
        break;
    }
    
    vTaskDelay(50);
  }
}


/*************************************************************************************************************************************************/
//Задача 5 работа кнопки
void Task5(void *parametr)
{
  // Пины
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // Переменные кнопки
  bool lastState = HIGH;        //Последнее состояние кнопки
  bool currentState;            //Текущее состояние
  unsigned long pressStart = 0; //Время нажатия
  unsigned long pressDur = 0;   //Время удержания
  while(1)
  {
    currentState = digitalRead(BUTTON_PIN); //Читаем кнопку
    if(lastState == HIGH && currentState == LOW)
    {
      pressStart = millis();    //Засекаем время
      if (xSemaphoreTake(serialMutex, portMAX_DELAY)==pdTRUE)
      {
        Serial.println("Кнопка нажата");
        xSemaphoreGive(serialMutex);
      }
    }
    if (lastState == LOW && currentState == HIGH)
    {
      pressDur = millis() - pressStart; //Засекаем время сколько держали
      if (pressDur < 2000)              //Если время удержания меньше 2 сек
      {
        if (xSemaphoreTake(serialMutex, portMAX_DELAY)==pdTRUE)
        {
          Serial.println("Короткое нажатие");
          xSemaphoreGive(serialMutex);
        }
        currentButton = PRESS;
      }
    
      else
      {
        if (xSemaphoreTake(serialMutex, portMAX_DELAY)==pdTRUE)
        {
          Serial.println("Удержание");
          xSemaphoreGive(serialMutex);
        }
        currentButton = DUR;
      }
      
      
    }
    lastState = currentState;
    vTaskDelay(20);

  }
   

}



/*************************************************************************************************************************************************/
// Задача 6 основная логика
void Task6(void *parametr)
{
  while(1)
  {
    
    switch (currentmode)
    {
    case WAIT:                              //ждем длинного удержания кнопки в режиме главного экрана WAIT
      if(currentButton == DUR)
      {
        lcd.clear();
        currentmode = SET_UP;
      }
      if(currentButton == PRESS)
      {
        lcd.clear();
        currentmode = STABILIZATION;
        currentButton = NOPRESS;
      }
      break;
    
    case SET_UP:                            //регулируем гистеризис кнопкой в режиме экрана SET_UP
      if(currentButton == PRESS)
      {
        g += 0.5;                           //прибавляем сразу по 0.5
        if(g > 3.0) g = 0.5;                //при значении больше 3 скидываем обратно на 0.5
      }
      else if (currentButton == DUR)        //удерживая кнопку для перехода обратно к экрану WAIT
      {
        currentmode = WAIT;
        lcd.clear();                        //очистили дисплей
      }
      
      break;
    
    case STABILIZATION:
      
      if (currentButton==DUR)
      {
        lcd.clear();
        currentmode = SET_TARGET;
      }
      if (temp >= targetTemp)
      {
        lcd.clear();
        currentmode = TIMER_STAB;
      }
      break;
    
    case SET_TARGET:
      if (currentButton==PRESS)
        {
          targetTemp += 1;
          if(targetTemp > 35) targetTemp = 30;
        }
      else if (currentButton == DUR)
        {
          currentmode = STABILIZATION;
          lcd.clear();
        }
      break;
    
      case TIMER_STAB:
        static bool timerStarted = false; //флаг запуска таймера
        if (!timerStarted)
        {
          //lcd.clear();
          xSemaphoreGive(TimerSimaphore);//даем симафор таймеру стабилизации
          timerStarted = true;
        }
        if (!timerRun)//таймер отработал
        {
        timerStarted = false;
        lcd.clear();
        startTemp = temp; // записываем в переменную температуру для старта отбора
        currentmode = OTBOR;
        }
        break;
      
      case OTBOR:
        if (!(temp > startTemp + g)) //если температура не превысела стартовой температуры + гистерезис
        {
          digitalWrite(VALVE, HIGH);
          valveFlag = true;
          
        }
        
        if (temp > (startTemp + g)) //если температура превысела
        {
          digitalWrite(VALVE, LOW);
          valveFlag = false;
        }
        if (currentButton == DUR)//При длительном удержании выходим в режим ожидания
        {
          lcd.clear();
          digitalWrite(VALVE, LOW);//Закрываем клапан
          valveFlag = false;
          startTemp = 0; //Сбрасываем стартовую температуру
          currentmode = WAIT;
        }
        
        break;
      
    }
    currentButton = NOPRESS;

  vTaskDelay(50);
  }
}

// Задача 7 таймер обратного отсчета
/*************************************************************************************************************************************************/
void Task7(void *parametr)
{
  //возвращает количество тиков с момента старта планировщика
  //Проснулись прямо сейчас, запомнили время
  TickType_t xLastWakeTime = xTaskGetTickCount();
  //Определяем период таймера: ровно 1 секунда.
  //pdMS_TO_TICKS(1000) — макрос, переводящий миллисекунды в тики. 1000 мс = 1 секунда.
  //const — значение не будет меняться никогда
  //TickType_t — специальный тип FreeRTOS для времени (обычно uint32_t)
  const TickType_t xPeriod = pdMS_TO_TICKS(1000);
  while(1)
  {
    // Ждём семафор (задача спит здесь, не жрёт процессор)
    if(xSemaphoreTake(TimerSimaphore,portMAX_DELAY)==pdTRUE)
    {
      //сбрасываем начальные значения
      minute = 0;
      second = 20;
      timerRun = true;
      //перезапускаем точку отчета
      xLastWakeTime = xTaskGetTickCount();
      //крутим пока не выйдет время
      while (timerRun)
      {
        //ждем ровно одну секунду
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
        //уменшаем секунды
        second -- ;
        //если секунды ушли в минус — занимаем у минут
        if (second < 0)
        {
          second = 59;
          minute -- ;
        }
        //если время вышло останавливаем
        if (minute < 0)
        {
          minute = 0;
          second = 0;
          timerRun = false;
        }
      }
    }
  }
}


//Задача 8 диагностика freertos
/*************************************************************************************************************************************************/
// Новая задача для диагностики
void TaskDiagnostic(void *parameter)
{
  // Ждем запуска всех задач
  vTaskDelay(2000);
    
  if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) 
  {
    Serial.println("=== System Diagnostic ===");
    Serial.println("All tasks started!");
    Serial.print("Free heap: ");
    Serial.println(xPortGetFreeHeapSize());
    Serial.print("Task count: ");
    Serial.println(uxTaskGetNumberOfTasks());
    Serial.println("========================");
    xSemaphoreGive(serialMutex);
  }
    
  // Удаляем задачу после диагностики
  vTaskDelete(NULL);
}


/*************************************************************************************************************************************************/
void setup() {
  Serial.begin(115200);
  pinMode(VALVE, OUTPUT);
  digitalWrite(VALVE, LOW); //При старте клапан гарантировано закрыт
  sensor.begin();
  sensor.setResolution(10);
  lcd.backlight();
  lcd.init();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("     VALVE       ");
  lcd.setCursor(0, 1);
  lcd.print("   CONTROLLER    ");
  delay(3000);
  lcd.clear();
  Serial.println("****************************");
  Serial.println("* Black Pill Stm32F401CCU6 *");
  Serial.println("*                          *");
  Serial.println("*     Start FreeRtos OS    *");
  Serial.println("****************************");
  
  serialMutex = xSemaphoreCreateMutex();
  if (serialMutex == NULL) {
    Serial.println("Ошибка создания мьютекса!");
    while (1);
  }

  lcdMutex = xSemaphoreCreateMutex();
  if (lcdMutex == NULL){
    Serial.println("Ошибка создания мьютекса!");
    while(1);
  }

  tempMutex = xSemaphoreCreateMutex();
  if (tempMutex == NULL){
    Serial.println("Ошибка создания мьютекса!");
    while(1);
  }

  //создаем семафор таймера изначально пустой
  TimerSimaphore = xSemaphoreCreateBinary();


  xTaskCreate(Task1, "Blink", 512, NULL, 1, &task1);
  xTaskCreate(Task2, "Serial", 512, NULL, 1, &task2);
  xTaskCreate(Task3, "Temp", 1024, NULL, 3, &task3);
  xTaskCreate(Task4, "Lcd", 1024, NULL, 2, &task4);
  xTaskCreate(Task5, "Button", 1024, NULL, 1, &task5);
  xTaskCreate(Task6, "State", 1024, NULL, 1, &task6);
  xTaskCreate(Task7, "Timer", 512, NULL, 1, &task7);
  xTaskCreate(TaskDiagnostic, "Diagnostic", 256, NULL, 1, NULL);


  vTaskStartScheduler();

  while(1);


}




void loop() {
//тут ничего быть не должно
}