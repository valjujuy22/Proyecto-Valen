#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "DHT.h"

#include <Arduino.h>
#include <WiFi.h>
#include "PubSubClient.h"

#define placa "ESP32"
#define Voltage_Resolution 3.3
#define pin 34
#define ADC_Bit_Resolution 12 

const char* ssid = "valen";
const char* password = "joacojoaco";
const char* mqtt_server = "broker.emqx.io"; 
const char* topic_tarea = "vaguilar/node1/sensores";

const int analogPin = 34; 
int value = 0;


WiFiClient espClient;
PubSubClient client(espClient);

SemaphoreHandle_t mqttMutex;

void conectarWiFi();
void reconnect();
void tempSensor(void * parameter);
void task_wifi(void * parameter);


#define DHTPIN GPIO_NUM_26
#define DHTTYPE DHT22

// Instanciamos el objeto DHT
DHT dht(DHTPIN, DHTTYPE);

extern "C" void app_main(void) // "extern C" es obligatorio para que ESP-IDF encuentre el punto de entrada en C++
{
    initArduino();
    Serial.begin(115200);

    conectarWiFi();
    client.setServer(mqtt_server, 1883);
    mqttMutex = xSemaphoreCreateMutex();

    if (mqttMutex != NULL) {
        // Núcleo 0: Tareas de comunicación y Temperatura
        xTaskCreatePinnedToCore(tempSensor, "Temp_Sensor", 3072, NULL, 1, NULL, 0);
        xTaskCreatePinnedToCore(task_wifi, "Task_WIFI", 3072, NULL, 2, NULL, 0); // Mayor prioridad para la red
    }    

    
    vTaskDelete(NULL); 
    // Inicializar el sensor de temperatura
    dht.begin();

}

void task_wifi(void * parameter) {
    
    for(;;){
        if (!client.connected()) {
            reconnect();
        }
        client.loop();
        vTaskDelay(pdMS_TO_TICKS(2500)); 
    }
}

void tempSensor(void * parameter) {
    
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        //esp_log_level_set("gpio", ESP_LOG_WARN);

        // Leer la temperatura en Celsius
        String temp = String(dht.readTemperature());
        // Leer la humedad relativa
        String hum = String(dht.readHumidity());

    

        if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            if (client.connected()) {
                client.publish(topic_tarea, hum.c_str());
                client.publish(topic_tarea, temp.c_str());
                Serial.println("La Humedad es: " + hum);
                Serial.println("La Temperatura es: " + temp);
            }

            xSemaphoreGive(mqttMutex);
        }
    }
}


void conectarWiFi() {
    Serial.print("Conectando a WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    Serial.println("\nWiFi Conectado.");
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Intentando conexión MQTT...");
        String clientId = "ESP32Client-" + String(random(0, 0xffff), HEX);
        if (client.connect(clientId.c_str())) {
            Serial.println("Conectado al Broker.");
        } else {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}