/**
 * @file main.cpp
 * @brief CampusSpot Edge Node Main Entry Point
 * 
 * This program runs on each individual Raspberry Pi (Edge Node).
 * It autonomously collects sensor data, processes it locally using
 * edge computing logic, and transmits only the final results to 
 * the central server via MQTT.
 */

#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cmath>

#include "drivers/DhtSensor.hpp"
#include "drivers/PirSensor.hpp"
#include "drivers/SoundSensor.hpp"
#include "drivers/BleSniffer.hpp"
#include "services/AcousticCategorizer.hpp"
#include "services/OccupancyFusion.hpp"
#include "services/DataAggregator.hpp"
#include "services/MqttClient.hpp"
#include "services/DataLogger.hpp"

int main(int argc, char* argv[]) {
    std::cout << "===========================================" << std::endl;
    std::cout << "   CampusSpot Node: Acoustic & Occupancy   " << std::endl;
    std::cout << "   (Edge Computing & Distributed Mode)     " << std::endl;
    std::cout << "===========================================" << std::endl;

    // --- [CONFIG] Node Specific Identification ---
    std::string NODE_ID = "RPI3-NODE-01";
    std::string ROOM_NAME = "Library-Central-02";

    if (argc >= 3) {
        NODE_ID = argv[1];
        ROOM_NAME = argv[2];
        std::cout << "[CONFIG] Using dynamic node settings from arguments:" << std::endl;
        std::cout << "         Node ID   : " << NODE_ID << std::endl;
        std::cout << "         Room Name : " << ROOM_NAME << std::endl;
    } else {
        std::cout << "[CONFIG] Using default configuration settings:" << std::endl;
        std::cout << "         Node ID   : " << NODE_ID << std::endl;
        std::cout << "         Room Name : " << ROOM_NAME << std::endl;
    }

    // --- [STEP 1] Initialize Hardware Drivers ---
    auto dht = std::make_unique<DhtSensor>(4);    // DHT11 on GPIO 4
    auto pir = std::make_unique<PirSensor>(17);   // PIR on GPIO 17
    auto sound = std::make_unique<SoundSensor>("plughw:1,0"); // I2S Digital Mic
    auto ble = std::make_unique<BleSniffer>();    // Internal BLE Chip

    std::vector<BaseSensor*> sensors = { dht.get(), pir.get(), sound.get(), ble.get() };

    for (auto sensor : sensors) {
        sensor->begin();
    }

    // --- [STEP 2] Initialize Edge Services & Communication ---
    AcousticCategorizer acousticService;
    OccupancyFusion occupancyService;
    MqttClient mqttService(NODE_ID);

    std::cout << "\n[SYSTEM] Connecting to MQTT Broker..." << std::endl;
    mqttService.connect();

    // CSV 로거 초기화 (상위 폴더의 ../logs 디렉토리에 타임스탬프가 붙은 파일 생성)
    DataLogger logger;
    logger.begin();

    std::cout << "\n[STATUS] Node active. Processing & reporting every 1s." << std::endl;

    // --- [STEP 3] Main Execution Loop (Edge Computing) ---
    while (true) {
        // 3.1 Update all physical/virtual sensors
        for (auto sensor : sensors) {
            sensor->update();
        }

        // 3.2 Locally process sound identity
        AcousticCategory acousticCat = acousticService.process(sound->getRms());
        
        // 3.3 Locally fuse motion and device count for hybrid occupancy
        OccupancyStatus occupancyStat = occupancyService.process(pir->isMotionDetected(), ble->getDeviceCount());

        // 3.4 Aggregate all local processed data into a single report
        AggregatedData report = DataAggregator::collect(
            NODE_ID, ROOM_NAME,
            dht->getTemperature(), dht->getHumidity(),
            sound->getRms(), acousticCat,
            ble->getDeviceCount(), pir->isMotionDetected(), occupancyStat
        );

        // --- [STEP 4] Output & Transmission ---
        // 4.1 Display summary for local debugging
        std::cout << "\n--- [" << report.nodeId << " @ " << report.roomName << "] Status Update ---" << std::endl;
        float soundDb = (report.soundRms > 1e-5f) ? (20.0f * std::log10(report.soundRms) + 100.0f) : 0.0f;
        if (soundDb < 0.0f) soundDb = 0.0f;

        std::cout << ">> RAW DATA:  Temp: " << std::fixed << std::setprecision(1) << report.temperature << "C, Hum: " 
                  << report.humidity << "%, Motion: " << (report.pirMotion ? "YES" : "NO") 
                  << ", Sound RMS: " << std::fixed << std::setprecision(4) << report.soundRms 
                  << " (" << std::fixed << std::setprecision(1) << soundDb << " dB)" << std::endl;
        std::cout << ">> IDENTITY:  " << report.acousticCategory << std::endl;
        std::cout << ">> OCCUPANCY: " << report.occupancyStatus << " (" 
                  << std::fixed << std::setprecision(1) << report.bleDeviceCount << " devices)" << std::endl;

        // 4.2 Transmit the lightweight JSON payload to server
        std::string payload = report.toJson();
        mqttService.publish(payload);
        
        std::cout << "[MQTT] Status: " << (mqttService.isConnected() ? "ONLINE" : "OFFLINE (Retrying...)") << std::endl;

        // 4.3 CSV 파일에 데이터 누적 기록
        logger.log(report.temperature, report.humidity, report.pirMotion, 
                   report.soundRms, soundDb, report.acousticCategory, 
                   report.bleDeviceCount, report.occupancyStatus);

        // Interval between reports to balance real-time updates and network load
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
