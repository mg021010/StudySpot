# 🏫 StudySpot: 실시간 다중 센서 융합 기반 공간 큐레이션 플랫폼

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Raspberry%20Pi-C8102E?style=for-the-badge&logo=raspberry-pi&logoColor=white" alt="Platform">
  <img src="https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++">
  <img src="https://img.shields.io/badge/Python-3.x-3776AB?style=for-the-badge&logo=python&logoColor=white" alt="Python">
  <img src="https://img.shields.io/badge/Firebase-RTDB-FFCA28?style=for-the-badge&logo=firebase&logoColor=white" alt="Firebase">
</p>

**StudySpot**은 공간의 단순 점유율(인원수) 통보를 넘어, 공간 내부의 **'음향적 분위기(Acoustic Identity)'**와 BLE/PIR 결합형 **'정적 점유 상태(Occupancy Status)'**, 그리고 **'실내 학습 쾌적 온도'**를 종합 분석하여 사용자의 공부 모드에 가장 어울리는 학습 공간을 1초 주기로 연산·추천하는 지능형 IoT 큐레이션 플랫폼입니다.

> **Developed by StudySpot 4조 (Smart IoT Platform Project)**

---

## 📡 시스템 아키텍처 및 데이터 흐름도

StudySpot은 엣지 컴퓨팅(Edge Computing) 모델과 클라우드 데이터 파이프라인을 융합하여 네트워크 트래픽을 최소화하고 화면 갱신 지연을 1초 미만으로 단축했습니다.

```mermaid
graph TD
    subgraph Hardware ["엣지 노드 (라즈베리파이 3)"]
        Sensors["DHT11, PIR, I2S Mic, BLE Chip"] -->|Raw Data| Main["main.cpp (Edge Logic)"]
        Main -->|1차 로컬 연산| Proc["Acoustic & Occupancy 분류"]
    end

    subgraph Messaging ["미들웨어 브릿지"]
        Proc -->|MQTT Publish| Broker["MQTT Broker (localhost:1883)"]
        Broker -->|MQTT Subscribe| Bridge["mqtt_to_firebase.py"]
    end

    subgraph Cloud ["클라우드"]
        Bridge -->|HTTPS PUT (JSON)| Firebase[("Firebase Realtime DB")]
    end

    subgraph Frontend ["사용자 화면"]
        Firebase -->|WebSocket 구독 / 실시간 동기화| Web["demo_dashboard.html (Web UI)"]
    end
```

---

## 🚀 핵심 기능 (Key Features)

### 1. 하이브리드 점유 융합 알고리즘 (Hybrid Occupancy Fusion)
*   **오인식 차단**: 단순히 인체 움직임만 잡는 PIR 센서는 자리에 가만히 정지해서 공부하는 사람을 감지하지 못해 빈 공간으로 오판하는 고질적인 한계가 있습니다.
*   **해결책**: 주변 블루투스(BLE) 디바이스 감지 대수($N$)와 PIR 10초 쿨다운 타이머($C$)를 실시간 교차 대조하여, 움직임이 전혀 없더라도 사람이 상주 중인 **'정적 밀집 집중 상태(Static High)'**를 정확하게 판정합니다.

### 2. 온디바이스 음향 정체성 진단 (Acoustic Identity)
*   **프라이버시 보호 (Privacy by Design)**: 음성 마이크 로우 데이터를 외부 서버로 전송하지 않고 라즈베리파이 내부에서 제곱평균제곱근(RMS) 전압 기반의 상용 로그 데시벨 공식($dB = 20\log_{10}(\text{RMS}) + 100$)을 통해 4단계 분위기(정막, 백색소음, BGM, 토론)로 가공한 뒤 가벼운 분류 텍스트만 서버로 쏩니다.

### 3. 사용자 목적 기반 Study-Fit 매칭 스코어링
*   공간 추천 최종 점수는 기본 신뢰점수(50점)에서 출발해 사용자가 선택한 모드(🤫자습, 💻노트북 작업, 🗣️조별 토론)에 맞는 가중치를 합산 및 감산합니다.
*   학습 집중 능률을 저하시키는 비쾌적 온도 범위(21.0°C 미만 또는 24.5°C 초과) 이탈 시 페널티 감점(-5)을 연산에 자동 반영합니다.
    $$\mathbf{SCORE = 50 + W_{acoustic} + W_{occupancy} - P_{temp}}$$

---

## 📂 프로젝트 폴더 구조 (Directory Structure)

```text
StudySpot/
├── node/                         # C++ 임베디드 엣지 컴퓨팅 소스 코드
│   ├── drivers/                  # 센서 하드웨어 추상화 드라이버 (DHT11, PIR, Mic, BLE)
│   ├── interfaces/               # 센서 기본 인터페이스 규격
│   ├── services/                 # 로컬 연산 및 통신 제어 서비스 (Acoustic, Occupancy, MQTT)
│   └── main.cpp                  # 엣지 노드 메인 실행 루프
├── bridge/                       # MQTT-Firebase 데이터 중계 미들웨어 (Python)
│   ├── mqtt_to_firebase.py       # 실시간 클라우드 전송 브릿지
│   └── mock_publisher.py         # 데모 및 시뮬레이션용 모의 노드 패킷 발행기
├── demo_dashboard.html           # 웹 실시간 모니터링/큐레이션 대시보드 (Frontend)
├── presentation_material.html    # 4조 PPT 발표자 요약 및 대본 웹 슬라이드
├── product_poster.pdf            # 4조 서비스 홍보 포스터 (PDF)
├── README.md                     # 본 개요 문서
└── .gitignore                    # 보안 및 빌드 부산물 제외 설정 파일
```

---

## ⚙️ 실행 및 빌드 가이드 (Getting Started)

### 1. 임베디드 엣지 노드 빌드 (Raspberry Pi 환경)
```bash
# 1. Paho MQTT C++ 라이브러리 및 CMake 설치 확인
sudo apt-get install libpaho-mqtt-dev libpaho-mqttcpp-dev cmake

# 2. 프로젝트 빌드 디렉토리 생성 및 빌드
cd node
mkdir build && cd build
cmake ..
make

# 3. 엣지 노드 실행
./StudySpot_Node
```

### 2. 실시간 미들웨어 브릿지 구동 (Gateway 환경)
```bash
# 1. 의존 패키지 설치
pip install paho-mqtt requests firebase-admin

# 2. 로컬 MQTT 브로커 기동 확인 (예: Mosquitto)
sudo systemctl start mosquitto

# 3. MQTT-Firebase 중계 브릿지 시작
python bridge/mqtt_to_firebase.py
```
*(하드웨어가 없는 테스트 환경에서는 `python bridge/mock_publisher.py`를 기동하여 가상 센서 신호를 발행할 수 있습니다.)*

### 3. 실시간 대시보드 실행
1. 웹 브라우저에서 [demo_dashboard.html](file:///c:/Users/mg021/StudySpot/demo_dashboard.html) 파일을 직접 더블 클릭하여 실행합니다.
2. 실시간 연동을 원할 경우 우측 상단 **[실시간 라이브 연결 (Firebase)]** 스위치를 켜고, 조작기 서랍의 **[DB URL]** 입력창에 본인의 Firebase Realtime DB 주소를 입력하면 자동으로 1초 주기의 동기화가 수립됩니다.

---

## 📡 MQTT 전송 데이터 규격 (JSON Payload)
```json
{
  "nodeId": "RPI3-NODE-01",
  "roomName": "ML415",
  "timestamp": 1718355600,
  "temperature": 23.5,
  "humidity": 48.0,
  "soundRms": 0.024,
  "soundDb": 32.5,
  "acousticCategory": "White Noise",
  "bleDeviceCount": 5.4,
  "pirMotion": 0,
  "occupancyStatus": "Static High"
}
```

---

## 🛡️ License
This project is licensed under the **MIT License**.
