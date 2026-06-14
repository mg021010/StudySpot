#ifndef PIR_SENSOR_HPP
#define PIR_SENSOR_HPP

#include "../interfaces/BaseSensor.hpp"

#ifdef __linux__
#include <pigpio.h>
#endif

#include <cstdlib> // For rand()

class PirSensor : public BaseSensor {
private:
    int gpioPin;
    bool motionDetected;

public:
    PirSensor(int pin) : BaseSensor("HC-SR501", "GPIO"), gpioPin(pin), motionDetected(false) {}

    bool begin() override {
#ifdef __linux__
        gpioCfgClock(5, PI_CLOCK_PWM, 0);
        if (gpioInitialise() < 0) return false;
        gpioSetMode(gpioPin, PI_INPUT);
        gpioSetPullUpDown(gpioPin, PI_PUD_DOWN);
#endif
        isInitialized = true;
        return true;
    }

    void update() override {
        if (!isInitialized) return;
#ifdef __linux__
        motionDetected = (gpioRead(gpioPin) == PI_HIGH);
#else
        // Mock: Random motion every few seconds
        motionDetected = (rand() % 10 == 0); 
#endif
    }

    bool isMotionDetected() const { return motionDetected; }
};

#endif // PIR_SENSOR_HPP
