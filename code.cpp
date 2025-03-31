/**
 * @file main.cpp
 * @brief Código para la lectura de sensores y control de LED en ESP32.
 * @author Yeisy
 * @date 2025
 * 
 * Este programa utiliza FreeRTOS para manejar múltiples tareas en el ESP32.
 * Se leen datos de temperatura, humedad y luminosidad, y se controla un LED
 * en función de estos valores. Además, se registra un contador mediante interrupciones.
 */

#include <Arduino.h>
#include <DHT.h>      ///< Biblioteca para el sensor DHT
#include <Wire.h>     ///< Biblioteca para comunicación I2C
#include "RTClib.h"   ///< Biblioteca para el RTC
#include "esp_sleep.h" ///< Biblioteca para habilitar el modo de bajo consumo

/// Pin del sensor DHT
#define DHTPIN 13
/// Tipo de sensor DHT (DHT11 o DHT22)
#define DHTTYPE DHT11
/// Pin del sensor de luz (LDR)
#define LDR_PIN 34
/// Pin del LED
#define LED_PIN 4
/// Pin del botón
#define BOTON_PIN 5
/// Pin del sensor infrarrojo
#define SENSOR_IR_PIN 19

// Intervalos de lectura en milisegundos
static const int temp_read_interval = 2500;
static const int hum_read_interval = 3200;
static const int luminosidad_read_interval = 1600;
static const int led_on = 700;
static const int led_off = 500;

/// Objeto para el sensor DHT
DHT dht(DHTPIN, DHTTYPE);
/// Objeto para el RTC
RTC_DS3231 rtc;

// Declaración de colas
QueueHandle_t luminosidadQueue;
QueueHandle_t humedadQueue;
QueueHandle_t temperatureQueue;
QueueHandle_t estadoAlarmaQueue;
QueueHandle_t contadorQueue;

volatile int contador = 0;  ///< Contador global incrementado por interrupciones
volatile unsigned long lastInterruptTime = 0;

/**
 * @brief Entra en modo Deep Sleep.
 */
void enterDeepSleep() {
    Serial.println("Entrando en modo Deep Sleep...");
    digitalWrite(LED_PIN, LOW);
    esp_deep_sleep_start();
}

/**
 * @brief Interrupción para incrementar el contador.
 */
void IRAM_ATTR incrementarContador() {
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > 700) {
        contador++;
        int valor = contador;
        xQueueOverwriteFromISR(contadorQueue, &valor, NULL);
    }
    lastInterruptTime = interruptTime;
}

/**
 * @brief Tarea para leer la temperatura.
 * @param parameter Parámetro no utilizado.
 */
void readTemperature(void *parameter) {
    while (1) {
        float temperature = dht.readTemperature();
        if (!isnan(temperature) && temperatureQueue) {
            xQueueOverwrite(temperatureQueue, &temperature);
        }
        vTaskDelay(temp_read_interval / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Tarea para leer la humedad.
 * @param parameter Parámetro no utilizado.
 */
void readHumidity(void *parameter) {
    while (1) {
        float humidity = dht.readHumidity();
        if (!isnan(humidity) && humedadQueue) {
            xQueueOverwrite(humedadQueue, &humidity);
        }
        vTaskDelay(hum_read_interval / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Calcula el CRC-8 de un conjunto de datos.
 * @param datos Array de datos.
 * @param longitud Tamaño del array.
 * @return Valor del CRC calculado.
 */
uint8_t calcularCRC8(const uint8_t *datos, size_t longitud) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < longitud; i++) {
        crc ^= datos[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
        }
    }
    return crc;
}

/**
 * @brief Configuración inicial del ESP32.
 */
void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BOTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BOTON_PIN), incrementarContador, FALLING);
    attachInterrupt(digitalPinToInterrupt(SENSOR_IR_PIN), incrementarContador, FALLING);

    dht.begin();
    if (!rtc.begin()) {
        Serial.println("No se encontró el RTC. Verifica las conexiones.");
        while (1);
    }
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

/**
 * @brief Bucle principal vacío, ya que el código funciona con FreeRTOS.
 */
void loop() {}

