# 🎯 Контроллер клапана рект колонны VC-STM32

[![PlatformIO](https://img.shields.io/badge/PlatformIO-STM32F401CCU6-blue)](https://platformio.org)
[![FreeRTOS](https://img.shields.io/badge/FreeRTOS-STM32duino-green)](https://github.com/stm32duino/STM32FreeRTOS)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.0.0-orange)]()

> Автоматический контроллер клапана для дистиллятора на базе STM32F401 (Black Pill) с использованием FreeRTOS.

<p align="center">
  <img src="docs/images/photo1.jpg" alt="Устройство" width="400"/>
</p>

## 📋 Содержание

- [Возможности](#-возможности)
- [Технические характеристики](#-технические-характеристики)
- [Быстрый старт](#-быстрый-старт)
- [Схема подключения](#-схема-подключения)
- [Принцип работы](#-принцип-работы)
- [Установка и прошивка](#-установка-и-прошивка)
- [Использование](#-использование)
- [Документация](#-документация)
- [Лицензия](#-лицензия)

## ✨ Возможности

- 🌡️ Точное измерение температуры (DS18B20, 0.1°C)
- 🔄 Автоматическое управление клапаном отбора
- 📊 LCD дисплей 16x2 с интуитивным интерфейсом
- ⚙️ Настраиваемый гистерезис (0.5°C - 3.0°C)
- ⏱️ Таймер стабилизации перед отбором
- 🎯 Настраиваемая целевая температура
- 💾 Многозадачность на FreeRTOS
- 🔍 Диагностика системы при запуске

## 📊 Технические характеристики

| Параметр               | Значение              |
|------------------------|----------------------|
| Микроконтроллер        | STM32F401CCU6        |
| Датчик температуры     | DS18B20 (1-Wire)     |
| Дисплей                | LCD 16x2 (I2C)       |
| Управление             | 1 кнопка             |
| Выход клапана          | PA1 (3.3В)           |
| ОС реального времени   | FreeRTOS             |
| Фреймворк              | Arduino (STM32duino) |
| Питание                | 5В USB               |

## 🚀 Быстрый старт

### Требования

- [PlatformIO IDE](https://platformio.org/install) (VS Code)
- STM32F401CCU6 (Black Pill)
- Датчик DS18B20
- LCD 16x2 I2C
- Релейный модуль (для управления клапаном)

### Прошивка

```bash
# Клонируем репозиторий
git clone https://github.com/yourusername/valve-controller-stm32.git

# Переходим в папку
cd valve-controller-stm32/firmware

# Устанавливаем зависимости
pio lib install "DS18B20"
pio lib install "LiquidCrystal_I2C"
pio lib install "STM32FreeRTOS"

# Компилируем и загружаем
pio run --target upload
