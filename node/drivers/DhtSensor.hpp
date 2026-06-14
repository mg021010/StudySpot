#ifndef DHT_SENSOR_HPP
#define DHT_SENSOR_HPP

#include "../interfaces/BaseSensor.hpp"

#ifdef __linux__
#include <pigpio.h>
#endif

#include <cstdlib>

class DhtSensor : public BaseSensor {
private:
    int gpioPin;
    float temperature;
    float humidity;

public:
    DhtSensor(int pin) : BaseSensor("DHT11", "GPIO"), gpioPin(pin), temperature(0.0f), humidity(0.0f) {}

    bool begin() override {
#ifdef __linux__
        gpioCfgClock(5, PI_CLOCK_PWM, 0);
        if (gpioInitialise() < 0) return false;
#endif
        isInitialized = true;
        return true;
    }

    void update() override {
        if (!isInitialized) return;

#ifdef __linux__
        uint8_t laststate = PI_HIGH;
        uint8_t counter = 0;
        uint8_t j = 0, i;
        int data[5] = {0, 0, 0, 0, 0};

        gpioSetMode(gpioPin, PI_OUTPUT);
        gpioWrite(gpioPin, PI_LOW);
        gpioDelay(18000);
        gpioSetMode(gpioPin, PI_INPUT);

        for (i = 0; i < 85; i++) {
            counter = 0;
            while (gpioRead(gpioPin) == laststate) {
                counter++;
                gpioDelay(1);
                if (counter == 255) break;
            }
            laststate = gpioRead(gpioPin);
            if (counter == 255) break;
            if ((i >= 4) && (i % 2 == 0)) {
                data[j / 8] <<= 1;
                if (counter > 16) data[j / 8] |= 1;
                j++;
            }
        }
        if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
            humidity = (float)data[0] + data[1] * 0.1f;
            temperature = (float)data[2] + data[3] * 0.1f;
        }
#else
        // Mock: Realistic room temperature/humidity
        temperature = 22.0f + (rand() % 50) / 10.0f;
        humidity = 40.0f + (rand() % 100) / 10.0f;
#endif
    }

    float getTemperature() const { return temperature; }
    float getHumidity() const { return humidity; }
};

#endif // DHT_SENSOR_HPP
