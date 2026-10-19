idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
        freertos 
        driver 
        esp_driver_gpio 
        arduino 
        DHT 
        Adafruit_Sensor 
        PubSubClient
)
