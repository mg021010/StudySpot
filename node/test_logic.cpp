#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "services/AcousticCategorizer.hpp"
#include "services/OccupancyFusion.hpp"

void testAcousticCategorizer() {
    std::cout << "[TEST] Running AcousticCategorizer tests..." << std::endl;
    AcousticCategorizer categorizer;

    // Test Case 1: Deep Focus (Very low and steady noise, RMS = 0.0003f < 0.0008f)
    AcousticCategory cat1;
    for(int i=0; i<10; ++i) cat1 = categorizer.process(0.0003f); 
    assert(cat1 == AcousticCategory::DEEP_FOCUS);
    std::cout << "  - Deep Focus: PASSED" << std::endl;

    // Test Case 2: White Noise (Steady moderate noise, RMS = 0.0020f, steady)
    AcousticCategory cat2;
    for(int i=0; i<10; ++i) cat2 = categorizer.process(0.0020f); 
    assert(cat2 == AcousticCategory::WHITE_NOISE);
    std::cout << "  - White Noise: PASSED" << std::endl;

    // Test Case 3: Active Discussion (High variability/loud, RMS > 0.0040f)
    AcousticCategory cat3;
    std::vector<float> discussionSamples = {0.006f, 0.002f, 0.007f, 0.003f, 0.008f, 0.002f, 0.006f, 0.004f, 0.007f, 0.009f};
    for(float s : discussionSamples) cat3 = categorizer.process(s);
    assert(cat3 == AcousticCategory::ACTIVE_DISCUSSION);
    std::cout << "  - Active Discussion: PASSED" << std::endl;
}

void testOccupancyFusion() {
    std::cout << "[TEST] Running OccupancyFusion tests..." << std::endl;

    // Test Case 1: Vacant (deviceCount 0.5f - baseline 0.7f = -0.2f -> 0.0f < 0.5f threshold, no motion)
    {
        OccupancyFusion fusion;
        assert(fusion.process(false, 0.5f) == OccupancyStatus::VACANT);
    }
    
    // Test Case 2: Static High (deviceCount 4.5f - baseline 0.7f = 3.8f >= 3.5f threshold, no motion)
    {
        OccupancyFusion fusion;
        assert(fusion.process(false, 4.5f) == OccupancyStatus::STATIC_HIGH);
    }
    
    // Test Case 3: Dynamic High & Cooldown transition (cooldown timer test - 10 seconds)
    {
        OccupancyFusion fusion;
        // 1. 첫 움직임 감지 -> 즉시 Dynamic High 진입
        assert(fusion.process(true, 4.5f) == OccupancyStatus::DYNAMIC_HIGH);
        
        // 2. 움직임이 사라진 후 9초 동안은 쿨다운 타이머가 작동하여 Dynamic High 유지
        for (int i = 0; i < 9; ++i) {
            assert(fusion.process(false, 4.5f) == OccupancyStatus::DYNAMIC_HIGH);
        }
        
        // 3. 10초째 움직임이 없으면 비로소 Static High로 상태 전환
        assert(fusion.process(false, 4.5f) == OccupancyStatus::STATIC_HIGH);
    }
    
    // Test Case 4: Static Low (deviceCount 2.5f - baseline 0.7f = 1.8f, which is between 0.5f and 3.5f, no motion)
    {
        OccupancyFusion fusion;
        assert(fusion.process(false, 2.5f) == OccupancyStatus::STATIC_LOW);
    }

    std::cout << "  - All Occupancy scenarios: PASSED" << std::endl;
}

int main() {
    try {
        testAcousticCategorizer();
        testOccupancyFusion();
        std::cout << "\n[SUCCESS] All logic verification tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n[FAILURE] Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
