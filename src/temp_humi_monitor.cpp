#include "temp_humi_monitor.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#if DHT_TYPE == DHT_TYPE_DHT20
    DHT20 dht20;
#elif DHT_TYPE == DHT_TYPE_DHT11
    SimpleDHT11 dht11(DHT_PIN);
#endif

//LiquidCrystal_I2C lcd(33,16,2); // Đã được thay thế bằng OLED

// Khai báo cho màn hình OLED 0.96 inch I2C
#define SCREEN_WIDTH 128 // Chiều rộng OLED
#define SCREEN_HEIGHT 64 // Chiều cao OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


void temp_humi_monitor(void *pvParameters){
    #if DHT_TYPE == DHT_TYPE_DHT20
        Wire.begin(11, 12);
        dht20.begin();
    #else // Giả sử DHT11 cũng dùng I2C pins này cho OLED
        Wire.begin(11, 12);
    #endif
    
    Serial.begin(115200);

    // Khởi tạo màn hình OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Vòng lặp vô hạn nếu không khởi tạo được
    }
    display.clearDisplay();
    display.display();

    float prev_temperature = 0.0;
    float prev_humidity = 0.0;

    while (1){
        /* code */
        int err = 0; // SimpleDHTErrSuccess
        // Reading temperature and humidity from DHT20
        #if DHT_TYPE == DHT_TYPE_DHT20
            dht20.read();
            float temperature = dht20.getTemperature();
            float humidity = dht20.getHumidity();
            // Check if any reads failed and exit early
            if (isnan(temperature) || isnan(humidity)) {
                Serial.println("Failed to read from DHT sensor!");
                temperature = humidity =  -1;
                err = -1; // Đặt lỗi để xử lý chung
                //return;
            }
        #elif DHT_TYPE == DHT_TYPE_DHT11
            float temperature = 0;
            float humidity = 0;
            err = dht11.read2(&temperature, &humidity, NULL);
        #endif

        if (err != 0) { // SimpleDHTErrSuccess là 0
            Serial.print("Read DHT failed, err=");
            Serial.println(err);
            temperature = prev_temperature;
            humidity = prev_humidity;
            //return;
        }

        //Update global variables for temperature and humidity
        glob_temperature = temperature;
        glob_humidity = humidity;
        prev_temperature = temperature;
        prev_humidity = humidity;
        
        // Print the results to Serial
        
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");

        // Hiển thị lên màn hình OLED
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        
        display.setCursor(0, 10);
        display.print("Temperature: ");
        display.print(temperature);
        display.print(" C");

        display.setCursor(0, 30);
        display.print("Humidity:    ");
        display.print(humidity);
        display.print(" %");

        display.display();
        
        vTaskDelay(5000);
    }
    
}