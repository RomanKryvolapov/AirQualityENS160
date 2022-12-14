#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DFRobot_ENS160.h>
#include <AHTxx.h>

const uint8_t LCD_I2C_ADDRESS = 0x3F;
const uint8_t LCD_COLUMNS = 16;
const uint8_t LCD_LINES = 2;
const uint8_t ENS160_I2C_ADDRESS = 0x53;
const uint8_t AHT20_I2C_ADDRESS = 0x38;

const uint8_t PROGRAM_UPDATE_SENSORS_DELAY = 100;
const uint8_t PROGRAM_UPDATE_DISPLAY_DELAY = 100;
const float DEFAULT_AMBIENT_TEMPERATURE = 20.0;
const float DEFAULT_AMBIENT_HUMIDITY = 50.0;
const String SPACE_LINE = "      ";
const String SMALL_SPACE_LINE = "  ";

LiquidCrystal_I2C LCD(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_LINES);
AHTxx AHT20(AHT20_I2C_ADDRESS, AHT2x_SENSOR);
DFRobot_ENS160_I2C ENS160(&Wire, ENS160_I2C_ADDRESS);

TaskHandle_t TaskGetSensorData;
TaskHandle_t TaskUpdateDisplay;

volatile uint16_t TVOC_index_new = 0;
volatile uint16_t TVOC_index_old = 0;

volatile uint16_t CO2_index_new = 0;
volatile uint16_t CO2_index_old = 0;

volatile uint8_t air_quality_index_new = 0;
volatile uint8_t air_quality_index_old = 0;

volatile uint8_t humidity_new = 0;
volatile uint8_t humidity_old = 0;

volatile uint8_t temperature_new = 0;
volatile uint8_t temperature_old = 0;

void setupLCD() {
    LCD.init();
    LCD.backlight();
}

void printIntro() {
    LCD.clear();
    LCD.setCursor(4, 0);
    LCD.print("Made by");
    LCD.setCursor(0, 1);
    LCD.print("Roman Kryvolapov");
}

void printHelpOne() {
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("TVOC ppb CO2 ppm");
    LCD.setCursor(0, 1);
    LCD.print("AirQual Temp Hum");
}

void printHelpTwo() {
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("0-65k    400-65k");
    LCD.setCursor(0, 1);
    LCD.print("1-5");
}

void setupLCDConst() {
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("T");
    LCD.setCursor(8, 0);
    LCD.print("C");
    LCD.setCursor(0, 1);
    LCD.print("AQ");
}

void displayInformation() {
    if (TVOC_index_new != TVOC_index_old) {
        LCD.setCursor(2, 0);
        LCD.print(SPACE_LINE);
        LCD.setCursor(2, 0);
        LCD.print(TVOC_index_new);
        TVOC_index_old = TVOC_index_new;
    }
    if (CO2_index_new != CO2_index_old) {
        LCD.setCursor(10, 0);
        LCD.print(SPACE_LINE);
        LCD.setCursor(10, 0);
        LCD.print(CO2_index_new);
        CO2_index_old = CO2_index_new;
    }
    if (air_quality_index_new != air_quality_index_old) {
        LCD.setCursor(3, 1);
        LCD.print(SMALL_SPACE_LINE);
        LCD.setCursor(3, 1);
        LCD.print(air_quality_index_new);
        air_quality_index_old = air_quality_index_new;
    }
    if (temperature_new != temperature_old) {
        LCD.setCursor(7, 1);
        LCD.print(SPACE_LINE);
        LCD.setCursor(7, 1);
        LCD.print(String(temperature_new) + String("C"));
        temperature_old = temperature_new;
    }
    if (humidity_new != humidity_old) {
        LCD.setCursor(13, 1);
        LCD.print(SPACE_LINE);
        LCD.setCursor(13, 1);
        LCD.print(String(humidity_new) + String("%"));
        humidity_old = humidity_new;
    }
}

void setupENS160() {
    while (ENS160.begin() != NO_ERR) {
        LCD.clear();
        LCD.setCursor(0, 0);
        LCD.print("ENS160 not started");
        delay(3000);
    }
    ENS160.setPWRMode(ENS160_STANDARD_MODE);
    ENS160.setTempAndHum(DEFAULT_AMBIENT_TEMPERATURE, DEFAULT_AMBIENT_HUMIDITY);
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("ENS160 started");
}

void setupATH20() {
    while (!AHT20.begin()) {
        LCD.clear();
        LCD.setCursor(0, 0);
        LCD.print("AHT20 not started");
        delay(3000);
    }
    LCD.setCursor(1, 1);
    LCD.print("AHT20  started");
}

void checkATH20() {
    temperature_new = AHT20.readTemperature();
    humidity_new = AHT20.readHumidity();
    if (temperature_new == AHTXX_ERROR || humidity_new == AHTXX_ERROR) {
        LCD.clear();
        if (temperature_new == AHTXX_ERROR && humidity_new == AHTXX_ERROR) {
            LCD.setCursor(0, 0);
            LCD.print("AHT20 temp error");
            LCD.setCursor(0, 1);
            LCD.print("AHT20 hum error");
        } else if (humidity_new == AHTXX_ERROR) {
            LCD.setCursor(0, 0);
            LCD.print("AHT20 hum error");
        } else if (temperature_new == AHTXX_ERROR) {
            LCD.setCursor(0, 0);
            LCD.print("AHT20 temp error");
        }
        AHT20.softReset();
        delay(3000);
        LCD.clear();
        LCD.setCursor(0, 0);
        LCD.print("reset AHT20");
        delay(3000);
    }
}

void checkENS160() {
    TVOC_index_new = ENS160.getTVOC();
    CO2_index_new = ENS160.getECO2();
    air_quality_index_new = ENS160.getAQI();
}

void taskGetSensorData(void *pvParameters) {
    while (true) {
        checkENS160();
        checkATH20();
        delay(PROGRAM_UPDATE_SENSORS_DELAY);
    }
}

void taskUpdateDisplay(void *pvParameters) {
    while (true) {
        displayInformation();
        delay(PROGRAM_UPDATE_DISPLAY_DELAY);
    }
}

void setupTasksForESP32Cores() {
    xTaskCreatePinnedToCore(
            taskGetSensorData,
            "taskGetSensorData",
            50000,
            NULL,
            1,
            &TaskGetSensorData,
            0);
    xTaskCreatePinnedToCore(
            taskUpdateDisplay,
            "taskUpdateDisplay",
            50000,
            NULL,
            1,
            &TaskUpdateDisplay,
            1);
}

void setup() {
    delay(100);
    setupLCD();
    printIntro();
    delay(1000);
    setupENS160();
    delay(500);
    setupATH20();
    delay(500);
    printHelpOne();
    delay(3000);
    printHelpTwo();
    delay(3000);
    setupLCDConst();
    setupTasksForESP32Cores();
}

void loop() {
}