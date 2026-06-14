#ifndef ACOUSTIC_CATEGORIZER_HPP
#define ACOUSTIC_CATEGORIZER_HPP

#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

enum class AcousticCategory {
    DEEP_FOCUS,      // Silence or very low noise
    WHITE_NOISE,     // Steady, moderate noise (fans, air conditioning)
    BGM,             // Rhythmic or musical background
    ACTIVE_DISCUSSION, // High variability, voice-heavy
    UNKNOWN
};

class AcousticCategorizer {
private:
    std::vector<float> history;
    const size_t HISTORY_SIZE = 10; // Number of samples to analyze

public:
    AcousticCategorizer() = default;

    /**
     * @brief Add a new RMS sample and get the current category.
     * 
     * @param currentRms The latest RMS value from SoundSensor.
     * @return AcousticCategory The detected category.
     */
    AcousticCategory process(float currentRms) {
        history.push_back(currentRms);
        if (history.size() > HISTORY_SIZE) {
            history.erase(history.begin());
        }

        if (history.size() < 5) return AcousticCategory::UNKNOWN;

        return classify();
    }

    std::string categoryToString(AcousticCategory category) const {
        switch (category) {
            case AcousticCategory::DEEP_FOCUS: return "Deep Focus";
            case AcousticCategory::WHITE_NOISE: return "White Noise";
            case AcousticCategory::BGM: return "BGM";
            case AcousticCategory::ACTIVE_DISCUSSION: return "Active Discussion";
            default: return "Unknown";
        }
    }

private:
    AcousticCategory classify() {
        // Calculate mean
        float sum = std::accumulate(history.begin(), history.end(), 0.0f);
        float mean = sum / history.size();

        // Calculate variance (for variability analysis)
        float sq_sum = std::inner_product(history.begin(), history.end(), history.begin(), 0.0f);
        float variance = (sq_sum / history.size()) - (mean * mean);

        // [Tuned Thresholds based on real-world test data]
        if (mean < 0.0008f) { // Approximately 38dB or below (very quiet)
            return AcousticCategory::DEEP_FOCUS;
        } 
        else if (mean >= 0.0040f || variance > 5e-6f) { // Approximately 52dB or above, or high variance
            return AcousticCategory::ACTIVE_DISCUSSION;
        } 
        else if (variance < 5e-7f) { // Steady noise between 38dB and 52dB
            return AcousticCategory::WHITE_NOISE;
        } 
        else { // Rhythmic or background noise between 38dB and 52dB
            return AcousticCategory::BGM;
        }
    }
};

#endif // ACOUSTIC_CATEGORIZER_HPP
