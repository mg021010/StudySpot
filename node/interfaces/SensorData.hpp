#ifndef SENSOR_DATA_HPP
#define SENSOR_DATA_HPP

#include <string>
#include <vector>

/**
 * @brief Unified data structure for sensor readings.
 */
struct SensorReading {
    std::string sensorName;
    float value;
    std::string unit;
    long long timestamp; // Epoch time
};

/**
 * @brief Container for a batch of readings from all sensors.
 */
struct EnvironmentPacket {
    std::string nodeId;
    std::vector<SensorReading> readings;
    bool motionDetected;
    int occupancyCount;
};

#endif // SENSOR_DATA_HPP
