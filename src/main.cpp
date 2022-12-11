#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DFRobot_ENS160.h>
#include <AHTxx.h>

#define I2C_COMMUNICATION

#ifdef  I2C_COMMUNICATION
DFRobot_ENS160_I2C ENS160(&Wire, 0x53);
#else
#endif

LiquidCrystal_I2C lcd(0x3F, 16, 2);
AHTxx aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR);

uint16_t TVOC_new = 0;
uint16_t TVOC_old = 0;
uint16_t CO2_new = 0;
uint16_t CO2_old = 0;
uint16_t AQI_new = 0;
uint16_t AQI_old = 0;
float humidity_new = 0;
float humidity_old = 0;
float temperature_new = 0;
float temperature_old = 0;
const uint32_t program_start_delay = 1000;
const uint32_t program_error_delay = 1000;
const uint32_t program_delay = 1000;
const String space_line = "      ";
const String small_space_line = "  ";

void setupLCD() {
    lcd.init();
    lcd.backlight();
}

void setupENS160() {
    while (ENS160.begin() != NO_ERR) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ENS160 not started");
        delay(program_error_delay);
    }
    ENS160.setPWRMode(ENS160_STANDARD_MODE);
    ENS160.setTempAndHum(20.0, 50.0);
}

void setupATH20() {
    while (!aht20.begin()) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("AHT20 not started");
        delay(program_error_delay);
    }
}

void checkATH20() {
    temperature_new = aht20.readTemperature();
    humidity_new = aht20.readHumidity();
    if (temperature_new == AHTXX_ERROR || humidity_new == AHTXX_ERROR) {
        lcd.clear();
        if (temperature_new == AHTXX_ERROR && humidity_new == AHTXX_ERROR) {
            lcd.setCursor(0, 0);
            lcd.print("AHT20 temp error");
            lcd.setCursor(0, 1);
            lcd.print("AHT20 hum error");
        } else if (humidity_new == AHTXX_ERROR) {
            lcd.setCursor(0, 0);
            lcd.print("AHT20 hum error");
        } else if (temperature_new == AHTXX_ERROR) {
            lcd.setCursor(0, 0);
            lcd.print("AHT20 temp error");
        }
        aht20.softReset();
        delay(program_error_delay);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("reset AHT20");
        delay(program_error_delay);
    } else {
        ENS160.setTempAndHum(temperature_new, humidity_new);
    }
}

void checkENS160() {
    TVOC_new = ENS160.getTVOC();
    CO2_new = ENS160.getECO2();
    AQI_new = ENS160.getAQI();
}

void displayInformation() {
    if (TVOC_new != TVOC_old) {
        lcd.setCursor(0, 0);
        lcd.print(space_line);
        lcd.setCursor(0, 0);
        lcd.print(TVOC_new);
        TVOC_old = TVOC_new;
    }
    if (CO2_new != CO2_old) {
        lcd.setCursor(8, 0);
        lcd.print(space_line);
        lcd.setCursor(8, 0);
        lcd.print(CO2_new);
        CO2_old = CO2_new;
    }
    if (AQI_new != AQI_old) {
        lcd.setCursor(0, 1);
        lcd.print(small_space_line);
        lcd.setCursor(0, 1);
        lcd.print(AQI_new);
        AQI_old = AQI_new;
    }
    if (temperature_new != temperature_old) {
        lcd.setCursor(3, 1);
        lcd.print(temperature_new);
        temperature_old = temperature_new;
    }
    if (humidity_new != humidity_old) {
        lcd.setCursor(11, 1);
        lcd.print(humidity_new);
        humidity_old = humidity_new;
    }
}

void showAuthor() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("    Made by");
    lcd.setCursor(0, 1);
    lcd.print("Roman Kryvolapov");
    lcd.clear();
}

void setup() {
    setupLCD();
    showAuthor();
    setupENS160();
    setupATH20();
    delay(program_start_delay);
}

void loop() {
    checkENS160();
    checkATH20();
    displayInformation();
    delay(program_delay);
}