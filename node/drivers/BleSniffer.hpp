#ifndef BLE_SNIFFER_HPP
#define BLE_SNIFFER_HPP

#include "../interfaces/BaseSensor.hpp"
#include <set>
#include <map>
#include <cstdlib>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <iostream>

#ifdef __linux__
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#endif

class BleSniffer : public BaseSensor {
private:
    int mockCount;
    float realCount;
    std::thread scanThread;
    std::atomic<bool> running;
    
    // 설정 상수 정의 (반경 10미터 타겟 필터링용)
    static constexpr int8_t RSSI_THRESHOLD = -66; // RSSI 임계치 (-66dBm 이상만 수집)
    static constexpr int ROLLING_WINDOW_SECS = 15; // 롤링 윈도우 유지 시간 (15초)
    
    // 세분화된 가중치 임계치 및 비율
    static constexpr int8_t RSSI_CLOSE = -50;     // 초근접 기준 (-50dBm)
    static constexpr int8_t RSSI_MEDIUM = -58;    // 실내 범위 기준 (-58dBm)
    static constexpr float WEIGHT_CLOSE = 1.0f;   // 초근접 기기 가중치 (내 자리)
    static constexpr float WEIGHT_MEDIUM = 0.3f;  // 실내 범위 기기 가중치 (내 방 안)
    static constexpr float WEIGHT_FAR = 0.01f;    // 배경 노이즈 감폭 가중치 (벽 너머/복도)

    struct DeviceInfo {
        std::chrono::steady_clock::time_point last_seen;
        int8_t rssi;
    };

    // 스레드 안전성을 위한 뮤텍스 및 롤링 윈도우용 디바이스 맵
    mutable std::mutex mtx;
    std::map<std::string, DeviceInfo> active_devices;

    // RSSI 구간별 가중치 합산 연산 함수 (스레드 안전성 보장을 위해 호출하는 곳에서 lock 필요)
    float calculateWeightedCount() const {
        float weighted_sum = 0.0f;
        for (const auto& pair : active_devices) {
            int8_t r = pair.second.rssi;
            if (r >= RSSI_CLOSE) {
                weighted_sum += WEIGHT_CLOSE;
            } else if (r >= RSSI_MEDIUM) {
                weighted_sum += WEIGHT_MEDIUM;
            } else {
                weighted_sum += WEIGHT_FAR;
            }
        }
        return weighted_sum;
    }

public:
    BleSniffer() : BaseSensor("BLE_Sniffer", "Internal_Bluetooth"), mockCount(0), realCount(0.0f), running(false) {}

    bool begin() override {
        isInitialized = true;
#ifdef __linux__
        // 메인 루프를 방해하지 않고 연속적으로 스캔하기 위해 백그라운드 스레드 가동
        running = true;
        scanThread = std::thread(&BleSniffer::scanLoop, this);
#endif
        return true;
    }

    void update() override {
        if (!isInitialized) return;
        
#ifndef __linux__
        // Mock: 3~8대의 주변 기기 시뮬레이션
        mockCount = 3 + (rand() % 6);
#endif
    }

    float getDeviceCount() const {
#ifdef __linux__
        std::lock_guard<std::mutex> lock(mtx);
        return realCount;
#else
        return (float)mockCount;
#endif
    }

    ~BleSniffer() {
#ifdef __linux__
        running = false;
        if (scanThread.joinable()) {
            scanThread.join();
        }
#endif
    }

private:
#ifdef __linux__
    void scanLoop() {
        int dev_id = -1;
        int dd = -1;
        
        while (running) {
            // HCI 디바이스 탐색 및 오픈
            dev_id = hci_get_route(NULL);
            if (dev_id < 0) {
                std::cerr << "[BLE] Bluetooth device not found. Retrying in 3s..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(3));
                continue;
            }
            
            dd = hci_open_dev(dev_id);
            if (dd < 0) {
                std::cerr << "[BLE] Failed to open HCI socket. Retrying in 3s..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(3));
                continue;
            }
            
            // [CRITICAL FIX] HCI 소켓 필터 구성
            struct hci_filter nf;
            hci_filter_clear(&nf);
            hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
            hci_filter_set_event(EVT_LE_META_EVENT, &nf);
            if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
                std::cerr << "[BLE] Failed to set HCI filter. Retrying..." << std::endl;
                hci_close_dev(dd);
                std::this_thread::sleep_for(std::chrono::seconds(3));
                continue;
            }
            
            // 0. 이전 실행의 잔재로 스캔이 이미 켜져 있을 수 있으므로 먼저 비활성화
            hci_le_set_scan_enable(dd, 0x00, 0x00, 1000);
            
            // 1. BLE 패시브 스캔 매개변수 설정 (11.25ms 스캔 간격)
            if (hci_le_set_scan_parameters(dd, 0x00, htobs(0x0012), htobs(0x0012), 0x00, 0x00, 1000) < 0) {
                std::cerr << "[BLE] Failed to set LE scan parameters. Retrying..." << std::endl;
                hci_close_dev(dd);
                std::this_thread::sleep_for(std::chrono::seconds(3));
                continue;
            }
            
            // 2. BLE 스캔 활성화
            if (hci_le_set_scan_enable(dd, 0x01, 0x00, 1000) < 0) {
                std::cerr << "[BLE] Failed to enable LE scan. Retrying..." << std::endl;
                hci_close_dev(dd);
                std::this_thread::sleep_for(std::chrono::seconds(3));
                continue;
            }
            
            std::cout << "[BLE] Passive LE Scan successfully started." << std::endl;
            
            // 소켓을 non-blocking 모드로 설정
            int flags = fcntl(dd, F_GETFL, 0);
            fcntl(dd, F_SETFL, flags | O_NONBLOCK);
            
            uint8_t buf[HCI_MAX_EVENT_SIZE];
            auto last_purge = std::chrono::steady_clock::now();
            
            while (running) {
                int len = read(dd, buf, sizeof(buf));
                if (len < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    } else {
                        std::cerr << "[BLE] HCI socket read error: " << errno << std::endl;
                        break;
                    }
                } else if (len >= 12) {
                    // 패킷 타입 바이트(0x04) 접두사 유무에 맞춰 분기 파싱 및 [RSSI 필터링 추가]
                    
                    // 케이스 A: 1바이트 패킷 타입 접두사가 있는 경우
                    if (buf[0] == 0x04 && buf[1] == 0x3e && buf[3] == 0x02 && len >= 13) {
                        uint8_t data_len = buf[13];
                        if (len >= 15 + data_len) {
                            // RSSI 값 추출 (signed 8-bit integer)
                            int8_t rssi = (int8_t)buf[14 + data_len];
                            
                            // [RSSI 임계치 필터링 변경]: RSSI_THRESHOLD 이상만 수집
                            if (rssi >= RSSI_THRESHOLD) {
                                char addr_str[18];
                                sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",
                                        buf[12], buf[11], buf[10], buf[9], buf[8], buf[7]);
                                std::string mac(addr_str);
                                
                                auto now = std::chrono::steady_clock::now();
                                {
                                    std::lock_guard<std::mutex> lock(mtx);
                                    active_devices[mac] = {now, rssi};
                                }
                            }
                        }
                    }
                    // 케이스 B: 패킷 타입 접두사가 없이 직접 이벤트 헤더로 시작하는 경우
                    else if (buf[0] == 0x3e && buf[2] == 0x02) {
                        uint8_t data_len = buf[12];
                        if (len >= 14 + data_len) {
                            // RSSI 값 추출
                            int8_t rssi = (int8_t)buf[13 + data_len];
                            
                            // [RSSI 임계치 필터링 변경]: RSSI_THRESHOLD 이상만 수집
                            if (rssi >= RSSI_THRESHOLD) {
                                char addr_str[18];
                                sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",
                                        buf[11], buf[10], buf[9], buf[8], buf[7], buf[6]);
                                std::string mac(addr_str);
                                
                                auto now = std::chrono::steady_clock::now();
                                {
                                    std::lock_guard<std::mutex> lock(mtx);
                                    active_devices[mac] = {now, rssi};
                                }
                            }
                        }
                    }
                }
                
                // 3초 간격으로 롤링 윈도우 내 오래된 기기 제거 (최근 ROLLING_WINDOW_SECS 초간 신호가 없는 기기 제거)
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - last_purge).count() >= 3) {
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        for (auto it = active_devices.begin(); it != active_devices.end(); ) {
                            if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_seen).count() > ROLLING_WINDOW_SECS) {
                                it = active_devices.erase(it);
                            } else {
                                ++it;
                            }
                        }
                        realCount = calculateWeightedCount();
                    }
                    last_purge = now;
                }
            }
            
            // 스캔 중지 및 소켓 클린업
            fcntl(dd, F_SETFL, flags); // blocking 모드로 복구
            hci_le_set_scan_enable(dd, 0x00, 0x00, 1000);
            hci_close_dev(dd);
            std::cout << "[BLE] Passive LE Scan stopped." << std::endl;
        }
    }
#endif
};

#endif // BLE_SNIFFER_HPP
