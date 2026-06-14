#ifndef BASE_SENSOR_HPP
#define BASE_SENSOR_HPP

#include <string>

/**
 * @brief Abstract Base Class for all sensors in the CampusSpot project.
 * 
 * This interface defines the mandatory lifecycle for any sensor integration,
 * ensuring consistency across GPIO, I2C, and I2S interfaces.
 */
class BaseSensor {
protected:
    std::string name;
    std::string interfaceType;
    bool isInitialized;

public:
    /**
     * @param n Name of the sensor (e.g., "DHT22")
     * @param type Interface type (e.g., "I2C", "GPIO")
     */
    BaseSensor(std::string n, std::string type) 
        : name(n), interfaceType(type), isInitialized(false) {}
    
    virtual ~BaseSensor() {}

    /**
     * @brief Perform hardware-specific initialization.
     * @return true if successful, false otherwise.
     */
    virtual bool begin() = 0;

    /**
     * @brief Trigger a data read from the sensor.
     * Specific data structures should be handled by the implementation.
     */
    virtual void update() = 0;

    // Getters
    std::string getName() const { return name; }
    std::string getInterface() const { return interfaceType; }
    bool isActive() const { return isInitialized; }
};

#endif // BASE_SENSOR_HPP
