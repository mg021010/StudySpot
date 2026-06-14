#ifndef SOUND_SENSOR_HPP
#define SOUND_SENSOR_HPP

#include "../interfaces/BaseSensor.hpp"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <thread>
#include <atomic>

#ifdef __linux__
#include <alsa/asoundlib.h>
#endif

class SoundSensor : public BaseSensor {
private:
#ifdef __linux__
    snd_pcm_t* handle;
#endif
    std::string deviceName;
    std::atomic<float> rmsValue;
    std::thread captureThread;
    std::atomic<bool> running;

public:
    SoundSensor(std::string device = "default") 
        : BaseSensor("INMP441", "I2S"), deviceName(device), rmsValue(0.0f), running(false) {
    }

    bool begin() override {
#ifdef __linux__
        if (snd_pcm_open(&handle, deviceName.c_str(), SND_PCM_STREAM_CAPTURE, 0) < 0) return false;
        snd_pcm_hw_params_t *params;
        snd_pcm_hw_params_alloca(&params);
        snd_pcm_hw_params_any(handle, params);
        snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S32_LE);
        snd_pcm_hw_params_set_channels(handle, params, 2);
        unsigned int rate = 44100;
        snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0);
        snd_pcm_hw_params(handle, params);
#endif
        isInitialized = true;

#ifdef __linux__
        // 백그라운드에서 끊임없이 녹음 데이터를 Drain할 쓰레드 구동
        running = true;
        captureThread = std::thread(&SoundSensor::captureLoop, this);
#endif
        return true;
    }

    void update() override {
        // 백그라운드 쓰레드에서 실시간으로 계산하므로 여기선 빈 함수로 둡니다.
    }

    float getRms() const { return rmsValue.load(); }

    ~SoundSensor() {
#ifdef __linux__
        running = false;
        if (captureThread.joinable()) {
            captureThread.join();
        }
#ifdef __linux__
        if (handle) snd_pcm_close(handle);
#endif
#endif
    }

private:
#ifdef __linux__
    void captureLoop() {
        int32_t buffer[2048];
        while (running) {
            // ALSA 버퍼가 가득 차기 전에 지속적으로 읽어옵니다. (블로킹 모드이므로 여기서 약 23ms 대기함)
            int rc = snd_pcm_readi(handle, buffer, 1024);
            if (rc == -EPIPE) {
                snd_pcm_prepare(handle);
            } else if (rc < 0) {
                snd_pcm_recover(handle, rc, 0);
            } else if (rc > 0) {
                long long sumSquares = 0;
                int count = 0;
                // 우측 채널 플로팅 노이즈는 버리고 좌측 채널 데이터만 계산
                for (int i = 0; i < rc * 2; i += 2) {
                    sumSquares += (static_cast<long long>(buffer[i]) * buffer[i]);
                    count++;
                }
                rmsValue.store(std::sqrt(static_cast<float>(sumSquares) / count) / 2147483648.0f);
            }
            // CPU 점유율 과부하 방지용 미세 대기
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
#endif
};

#endif // SOUND_SENSOR_HPP
