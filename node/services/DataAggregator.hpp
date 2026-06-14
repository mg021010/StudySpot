#ifndef DATA_AGGREGATOR_HPP
#define DATA_AGGREGATOR_HPP

#include <string>
#include <ctime>
#include "../interfaces/SensorData.hpp"
#include "AcousticCategorizer.hpp"
#include "OccupancyFusion.hpp"

struct AggregatedData {
    std::string nodeId;
    std::string roomName;
    time_t timestamp;
    float temperature;
    float humidity;
    float soundRms;
    std::string acousticCategory;
    float bleDeviceCount;
    bool pirMotion;
    std::string occupancyStatus;

    std::string toJson() const {
        char buf[1024];
        snprintf(buf, sizeof(buf),
            "{\"nodeId\":\"%s\",\"roomName\":\"%s\",\"timestamp\":%ld,\"env\":{\"temp\":%.1f,\"hum\":%.1f},"
            "\"acoustic\":{\"rms\":%.3f,\"category\":\"%s\"},"
            "\"occupancy\":{\"bleCount\":%.1f,\"pir\":%s,\"status\":\"%s\"}}",
            nodeId.data(), roomName.data(), timestamp, temperature, humidity,
            soundRms, acousticCategory.data(),
            bleDeviceCount, (pirMotion ? "true" : "false"), occupancyStatus.data());
        return std::string(buf);
    }
};

class DataAggregator {
public:
    static AggregatedData collect(
        const std::string& nodeId,
        const std::string& roomName,
        float temp, float hum, 
        float rms, AcousticCategory acoustic,
        float bleCount, bool pirMotion, OccupancyStatus occupancy
    ) {
        AggregatedData data;
        data.nodeId = nodeId;
        data.roomName = roomName;
        data.timestamp = std::time(nullptr);
        data.temperature = temp;
        data.humidity = hum;
        data.soundRms = rms;
        
        static AcousticCategorizer ac;
        data.acousticCategory = ac.categoryToString(acoustic);
        
        data.bleDeviceCount = bleCount;
        data.pirMotion = pirMotion;
        
        static OccupancyFusion of;
        data.occupancyStatus = of.statusToString(occupancy);
        
        return data;
    }
};

#endif // DATA_AGGREGATOR_HPP
