#ifndef DATA_LOGGER_HPP
#define DATA_LOGGER_HPP

#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <filesystem>

class DataLogger {
private:
    std::ofstream logFile;
    std::string filePath;

public:
    DataLogger() {
        // YYYYMMDD_HHMMSS 포맷의 현재 시간 문자열 취득
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << "../logs/sensor_log_" 
           << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") 
           << ".csv";
        filePath = ss.str();
    }

    bool begin() {
        // 디렉토리가 존재하지 않는 경우 상위 폴더에 자동 생성 (C++17)
        try {
            std::filesystem::create_directories("../logs");
        } catch (const std::exception& e) {
            std::cerr << "[LOGGER] Failed to create logs directory: " << e.what() << std::endl;
            return false;
        }

        // 파일 존재 여부 확인
        std::ifstream checkFile(filePath);
        bool exists = checkFile.good();
        checkFile.close();

        // 이어쓰기(Append) 모드로 파일 열기
        logFile.open(filePath, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "[LOGGER] Failed to open log file: " << filePath << std::endl;
            return false;
        }

        // 파일이 처음 생성되는 경우 CSV 헤더 작성
        if (!exists) {
            logFile << "Timestamp,Temperature,Humidity,Motion,Sound_RMS,Sound_dB,Acoustic_Category,BLE_Count,Occupancy_Status\n";
            logFile.flush();
        }
        std::cout << "[LOGGER] Logging data to " << filePath << std::endl;
        return true;
    }

    void log(float temp, float hum, bool motion, float rms, float db, 
             const std::string& category, float bleCount, const std::string& occupancy) {
        if (!logFile.is_open()) return;

        // 현재 시간 취득 (YYYY-MM-DD HH:MM:SS 포맷)
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        // CSV 행 기록
        logFile << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S") << ","
                << std::fixed << std::setprecision(1) << temp << ","
                << std::fixed << std::setprecision(1) << hum << ","
                << (motion ? 1 : 0) << ","
                << std::fixed << std::setprecision(6) << rms << ","
                << std::fixed << std::setprecision(1) << db << ","
                << "\"" << category << "\","
                << std::fixed << std::setprecision(1) << bleCount << ","
                << "\"" << occupancy << "\"\n";
        
        // 즉시 저장하여 전원 차단 시에도 데이터 유실 최소화
        logFile.flush();
    }

    ~DataLogger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
};

#endif // DATA_LOGGER_HPP
