#ifndef OCCUPANCY_FUSION_HPP
#define OCCUPANCY_FUSION_HPP

#include <string>
#include <vector>
#include <numeric>

enum class OccupancyStatus {
    VACANT,            // No devices, no movement
    STATIC_LOW,        // Few devices, no movement (e.g., 1-2 people studying quietly)
    STATIC_HIGH,       // Many devices, no movement (e.g., full quiet library)
    DYNAMIC_LOW,       // Few devices, active movement
    DYNAMIC_HIGH,      // Many devices, active movement (e.g., busy cafe/team project)
    UNKNOWN
};

class OccupancyFusion {
private:
    // 설정 상수 정의 (가중치합 기반 보정 - 실측 데이터 반영)
    static constexpr float BASELINE_WEIGHT = 0.7f;       // 배경 노이즈 보정용 기준 가중치합 (이웃 기기 20개 감폭 상쇄)
    static constexpr float HIGH_DEVICE_THRESHOLD = 3.5f; // 보정 후 혼잡도 기준값 (3인 이상일 때 High)

    // 모션 평활화를 위한 쿨다운 타이머 변수
    int motionCooldown = 0;
    static constexpr int COOLDOWN_LIMIT = 10;            // 움직임이 감지된 후 10초 동안은 Dynamic 상태 유지

public:
    OccupancyFusion() = default;

    /**
     * @brief Fuses PIR and BLE data to determine the hybrid occupancy status.
     * 
     * @param motionDetected Current state of PIR sensor.
     * @param deviceCount Current number of devices from BLE sniffer (weighted sum).
     * @return OccupancyStatus The fused status.
     */
    OccupancyStatus process(bool motionDetected, float deviceCount) {
        // 1. 움직임이 감지되면 쿨다운 타이머를 최대치(10초)로 충전
        if (motionDetected) {
            motionCooldown = COOLDOWN_LIMIT;
        } else if (motionCooldown > 0) {
            // 움직임이 없으면 매초 1초씩 감소
            motionCooldown--;
        }

        // 쿨다운 타이머가 남아있으면 활성(Dynamic) 상태로 판단
        bool isDynamic = (motionCooldown > 0);

        // 2. BLE 배경 노이즈 감산 보정
        float adjustedCount = deviceCount - BASELINE_WEIGHT;
        if (adjustedCount < 0.0f) adjustedCount = 0.0f;

        // 3. 최근 10초간 전혀 움직임이 없었고 기기도 없다면 Vacant
        if (adjustedCount < 0.5f && motionCooldown == 0) {
            return OccupancyStatus::VACANT;
        }

        bool isHighDensity = (adjustedCount >= HIGH_DEVICE_THRESHOLD);

        if (isDynamic) {
            return isHighDensity ? OccupancyStatus::DYNAMIC_HIGH : OccupancyStatus::DYNAMIC_LOW;
        } else {
            // 이 상태에서는 최근 10초간 활발한 동작 밀도가 기준 이하이므로 정적(Static)으로 분류
            return isHighDensity ? OccupancyStatus::STATIC_HIGH : OccupancyStatus::STATIC_LOW;
        }
    }

    std::string statusToString(OccupancyStatus status) const {
        switch (status) {
            case OccupancyStatus::VACANT: return "Vacant";
            case OccupancyStatus::STATIC_LOW: return "Static Low (Quiet Study)";
            case OccupancyStatus::STATIC_HIGH: return "Static High (Deep Focus Room)";
            case OccupancyStatus::DYNAMIC_LOW: return "Dynamic Low";
            case OccupancyStatus::DYNAMIC_HIGH: return "Dynamic High (Active Area)";
            default: return "Unknown";
        }
    }
};

#endif // OCCUPANCY_FUSION_HPP
