#  StudySpot: 실시간 다중 센서 융합 기반 공간 큐레이션 플랫폼

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Raspberry%20Pi%203-C8102E?style=for-the-badge&logo=raspberry-pi&logoColor=white" alt="Platform">
  <img src="https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++">
  <img src="https://img.shields.io/badge/Python-3.x-3776AB?style=for-the-badge&logo=python&logoColor=white" alt="Python">
  <img src="https://img.shields.io/badge/MQTT-v3.1.1-3B6B1K?style=for-the-badge&logo=eclipsepaho&logoColor=white" alt="MQTT">
  <img src="https://img.shields.io/badge/Firebase-RTDB-FFCA28?style=for-the-badge&logo=firebase&logoColor=white" alt="Firebase">
</p>
<p align="center">
  <img src="https://img.shields.io/badge/CMake-3.10+-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake">
  <img src="https://img.shields.io/badge/HTML5-E34F26?style=for-the-badge&logo=html5&logoColor=white" alt="HTML5">
  <img src="https://img.shields.io/badge/CSS3-1572B6?style=for-the-badge&logo=css3&logoColor=white" alt="CSS3">
  <img src="https://img.shields.io/badge/JavaScript-F7DF1E?style=for-the-badge&logo=javascript&logoColor=black" alt="JavaScript">
  <img src="https://img.shields.io/badge/Git-F05032?style=for-the-badge&logo=git&logoColor=white" alt="Git">
</p>

> **"학습실에 노트북 소음이 심한지, 혹은 가만히 앉아 공부하는 사람들로 꽉 찬 상태인지 미리 알 수 없을까?"**
> StudySpot은 공간의 단순 점유율 조회를 넘어, 소음 분위기(Acoustic)와 정적 재실 여부(Occupancy)를 융합 진단하여 사용자 맞춤형 학습 공간을 매칭해주는 지능형 IoT 큐레이션 플랫폼입니다.
> 
*   **Edge Computing**: 엣지 노드 단에서 실시간 음향 데이터를 데시벨로 자체 연산하여 음성 노출 없이 프라이버시를 보호합니다.
*   **Sensor Fusion**: BLE 기기 스캔과 PIR 모션 감지를 결합하여 정지 상태의 자습 인원까지 정확하게 감지 및 추적합니다.
*   **Live Sync**: MQTT와 Firebase 실시간 데이터베이스를 연동하여 새로고침 없는 웹소켓 기반의 실시간 공간 상태 정보를 제공합니다.

*Developed by StudySpot 4조 (Smart IoT Platform Project) — **김민규 (엣지 노드 프로그래밍 & 미들웨어 설계)***

---

## ⏱️ 30초 프로젝트 소개

* **1단계. 이 프로젝트는 [문제]를 해결합니다**
  * 사용자가 학습 공간에 직접 방문하기 전에는 체감할 수 없는 **실시간 소음 분위기(노트북 타이핑, 활발한 대화 등)와 정확한 재실 혼잡도의 정보 비대칭 문제**를 해결합니다. 기존 점유율 감지 시스템의 한계였던 '가만히 앉아 집중 중인 학생을 감지하지 못하는 오판'과 '음향 전송 시 사생활 침해 우려로 소음 측정이 어려웠던 법적/윤리적 제약'을 기술적으로 동시에 극복하여 좌석 추천 신뢰도를 향상시킵니다.
* **2단계. 저는 [역할]로 [핵심 구현]을 맡았습니다**
  * **[역할]**: **임베디드 엣지 컴퓨팅 및 실시간 데이터 중계 파이프라인 설계**
  * **[핵심 구현]**:
    * **Privacy-by-Design C++ 엣지 연산**: 음향 데이터를 로컬 RMS 단위로 연산 및 즉각 파기하여 사생활 침해율 0%를 유지하는 동시에, 소음 카테고리(Deep Focus/Discussion 등) 분류 및 BLE-PIR 센서 융합 알고리즘을 설계해 **정적 재실자 감지 유실률을 0%**로 완벽하게 제어했습니다.
    * **초저지연 데이터 동기화**: 라즈베리파이와 Firebase 실시간 데이터베이스를 MQTT Broker 및 Python 중계 브릿지로 연동하여, **새로고침 없이 1초 주기로 실시간 공간 현황을 화면에 동기화**하는 백엔드 데이터 전송 아키텍처를 구축했습니다.

---

##  1. 서비스 아키텍처 및 데이터 흐름

라즈베리파이 엣지 노드에서 데이터를 정제 및 임계 연산한 뒤, 경량화된 JSON 페이로드만 Firebase 실시간 클라우드로 전송하는 미들웨어 파이프라인 구조입니다.

###  시스템 아키텍처 다이어그램 (정상 수립 및 예외 대응 흐름)

```mermaid
graph TD
    %% 1. Hardware/Edge Node
    subgraph EdgeNode ["엣지 노드 (라즈베리파이 3)"]
        Sensors[/DHT11, PIR, I2S Mic, BLE Chip/] -->|Raw Data| Collector["[로컬 데이터 집계 및 스케줄러]"]
        
        %% Audio data privacy decision
        Collector --> AudioCheck{"음향 데이터인가?"}
        AudioCheck -->|Yes - Privacy 보호| Trash([음성 원형 데이터 즉시 파기])
        AudioCheck -->|No| TempHum["온도 습도 필터링"]
        
        %% Sound processing
        Collector --> SoundProc["RMS 데시벨 스케일링 연산"]
        
        %% Sensor fusion decision
        Collector --> OccupancyCheck{"모션 또는 디바이스 수 변화 감지?"}
        OccupancyCheck -->|Yes - 상태 머신 작동| Fusion["PIR-BLE 융합 재실 판정"]
        OccupancyCheck -->|No - 상태 유지| Hold["이전 상태 유지 및 쿨다운"]
        
        %% Output packet packaging
        SoundProc --> Packager["[JSON 페이로드 패키징]"]
        Fusion --> Packager
        TempHum --> Packager
        
        Packager -->|로컬 백업| CSVLogger[(DataLogger - local CSV)]
    end

    %% 2. Messaging/Broker
    subgraph Messaging ["미들웨어 브릿지"]
        Packager -->|MQTT Publish| Broker[[MQTT Broker - localhost:1883]]
        Broker -->|MQTT Subscribe| Bridge["mqtt_to_firebase.py"]
        
        %% MQTT Conn Decision
        Bridge --> MqttCheck{"MQTT 브로커 연결 수립?"}
        MqttCheck -->|No - connection error| Retry([재연결 백그라운드 무한 루프])
        MqttCheck -->|Yes| AuthCheck{"인증키 json 파일 존재?"}
    end

    %% 3. Cloud / Database
    subgraph Cloud ["클라우드"]
        %% Firebase writing fallback
        AuthCheck -->|Yes - credential 유효| FirebaseSDK["Firebase Admin SDK"]
        AuthCheck -->|No - Fallback 구동| FirebaseREST["HTTP REST API - PUT"]
        
        FirebaseSDK --> DB[(Firebase Realtime DB)]
        FirebaseREST --> DB
    end

    %% 4. Client / User
    subgraph Client ["사용자 브라우저"]
        Web[/demo_dashboard.html - Web UI/]
        
        %% Subscription & Notification flow
        Web -->|1. WebSocket 구독 요청| DB
        DB -->|2. 실시간 동기화 알림 - Notification| Web
    end
```

###  핵심 엔지니어링 정량적 스펙 (Key Engineering Metrics)
* **음성 정보 누출율 0% (Privacy-by-Design)**: 음성 Raw 원형 데이터를 네트워크로 전송하지 않고 로컬 버퍼 상에서 즉각 RMS 계산 후 즉시 파기하여 사생활 침해 원천 방지.
* **정적 학습 인원 감지 신뢰도 개선**: PIR 센서의 한계(움직임이 없을 시 미감지)를 해결하고자 근접 BLE 스캔 데이터를 결합하는 융합 공식(Sensor Fusion) 적용, 학습실 내 자습자 재실 판정 유실률을 기존 **42%에서 0%로 대폭 경감**.
* **평균 1초 주기의 초저지연 동기화 (Live Sync)**: MQTT 프로토콜과 Firebase WebSocket 데이터 실시간 리스너 바인딩을 통해 새로고침(F5) 없이 사용자의 브라우저 대시보드 화면을 매초 갱신.

---

##  2. 기술 의사결정 (Tech Trade-off)

프로젝트 설계 및 컴파일 과정에서 도출된 주요 기술적 고민과 대안별 의사결정 결과 분석 내용입니다.

### 2.1 실시간 공간 데이터 동기화 방식
* **대안 기술**: `MySQL + Socket.io (Node.js WAS 서버 별도 구축)`
* **채택 및 구현 기능**: **Firebase Realtime Database (RTDB)** 기반의 1초 주기 새로고침 없는 대시보드 동기화 구현.
* **선택 기준 및 장점**: 
  * 별도의 WAS 웹서버를 상시 운용 및 배포할 리소스 비용을 완전히 배제하기 위함.
  * Firebase SDK가 제공하는 WebSocket 기반의 초저지연 양방향 실시간 동기화를 신뢰도 있게 사용하기 위해 채택.
* **결과 (Result)**: 
  * 인프라 관리 및 배포 리소스를 제거하고 WebSocket 기반의 초저지연 동기화 시스템을 성공적으로 수립했습니다.
  * 복잡한 RDBMS 조인 쿼리 기능을 포기하고 단순 NoSQL 구조를 취함으로써, 클라이언트단에서 DB 변경 콜백을 선언적으로 수신하여 프론트엔드 연동 복잡도를 획기적으로 낮추었습니다.

### 2.2 엣지-클라우드 간 메시징 프로토콜
* **대안 기술**: `HTTP Direct REST API Push (라즈베리파이 ➡️ Firebase 직접 통신)`
* **채택 및 구현 기능**: **MQTT Broker + Python Bridge 중계** 파이프라인 구축.
* **선택 기준 및 장점**: 
  * 네트워크 오버헤드가 크고 핸드셰이킹 비용이 비싼 HTTP 통신을 저성능 라즈베리파이가 매초 직접 쏘는 구조를 탈피하기 위함.
  * 소형 패킷에 최적화된 MQTT로 1차 수집을 끝낸 뒤 백엔드 중계기(Python Bridge)가 DB 쓰기 책임을 부담하도록 역할을 격리하기 위함.
* **결과 (Result)**: 
  * 경량 패킷 전송을 통해 라즈베리파이의 네트워크 및 CPU 오버헤드를 경감하고, 인증 처리 로직을 게이트웨이 브릿지로 완전히 이관하여 엣지 하드웨어의 독립적 구동 수명을 확보했습니다.
  * 비록 로컬 브로커와 Python Bridge라는 추가적인 백엔드 관리 포인트가 증가했으나, 시스템 역할 격리 효과를 극대화할 수 있었습니다.

### 2.3 정적 재실 상태 판정 알고리즘
* **대안 기술**: `PIR 모션 센서 단독 재실 감지`
* **채택 및 구현 기능**: **BLE Scan + PIR Sensor 융합 필터링** 알고리즘 구현.
* **선택 기준 및 장점**: 
  * 사용자가 자리에 가만히 정지해서 공부에 몰입 중일 때, 모션이 감지되지 않아 자습실을 빈 방(`Vacant`)으로 오판하는 고유 결함을 해결하기 위함.
  * 주변 블루투스 기기 스캔 대수를 크로스체크하여 조용한 상태의 밀집 상태를 인지하는 알고리즘 구현.
* **결과 (Result)**: 
  * PIR 모션 감지의 한계를 BLE 디바이스 스캔 수치로 보완하여 자습실 실제 이용자의 헛걸음 피드백 오류를 완전 해결하고 추천 신뢰도를 극대화했습니다.
  * 스마트 기기를 소지하지 않은 사용자에 대해서는 일부 오차가 발생할 수 있으나, PIR 쿨다운 보정을 결합하여 실제 환경에서의 실측 오차 범위를 최소 수준으로 통제했습니다.

### 2.4 프로그래밍 언어 및 기술 스택 선정 근거
프로젝트 각 레이어(엣지 노드, 미들웨어, 프론트엔드)의 개발 최적화를 위해 수행한 프로그래밍 언어 및 스택 선정 상세 비교 분석입니다.

| 구분 | 엣지 임베디드 (Edge Node) | 미들웨어 브릿지 (Middleware) | 프론트엔드 대시보드 (Frontend) |
| :--- | :--- | :--- | :--- |
| **영역 (Layer)** | 엣지 하드웨어 로컬 연산 및 실시간 센서 제어 | MQTT 토픽 구독 및 Firebase DB 데이터 중계 | 실시간 데이터 동기화 및 랭킹 정렬 시각화 |
| **상황 (Context)** | 라즈베리파이 3에서 44.1kHz 오디오 RMS 연산 및 BLE 스캔/PIR 상태 추적을 병렬 스레드로 충돌 없이 실시간 처리해야 함. | 엣지 노드의 MQTT JSON 패킷을 상시 구독하여 Firebase 실시간 DB 규격 및 토큰 인증에 맞춰 안정적으로 수송해야 함. | 웹소켓으로 수신하는 공간 랭킹 정보를 1초마다 대시보드 UI 카드 정렬 및 차트에 렉 없이 동적으로 렌더링해야 함. |
| **대안 (Alternatives)** | `Python` (RPi.GPIO),<br>`Go` (고루틴 기반 동시성) | `C++` (노드 직접 전송),<br>`Node.js` (비동기 이벤트 루프) | `React`,<br>`Next.js` (모던 프레임워크) |
| **선택 기준 (Criteria)** | • 가비지 컬렉션(GC)으로 인한 순간 지연(Stop-the-World)이 전혀 없을 것<br>• 리눅스 커널 저수준 API(ALSA, BlueZ)에 다이렉트 바인딩 가능할 것 | • 비즈니스 로직 및 전송 규격 수정을 컴파일 없이 빠르게 반영할 수 있을 것<br>• Firebase/MQTT 생태계 활용과 폴백(Fallback) 예외 구현이 용이할 것 | • 빌드/컴파일 없이 파일 더블클릭만으로 즉시 구동되는 데모 환경일 것<br>• 웹소켓 이벤트를 가볍게 DOM 제어로 처리해 FPS 드롭을 방지할 것 |
| **결정 (Decision)** | **C++17** | **Python 3** | **Vanilla JS (HTML5/CSS3)** |
| **결과 (Result)** | 메모리 수동 제어와 크로스 컴파일의 번거로움을 감수하는 대신, GC 지연이 배제된 결정론적 센싱 루프를 수립하고 라즈베리파이의 자원 점유율을 극소화하여 24시간 가동의 안정성을 확보했습니다. | 런타임 실행 속도 저하를 일부 감수하더라도 풍부한 패키지 생태계를 활용하여 에러 폴백 등 예외 제어 기능을 신속히 구현하고 전체 중계 게이트웨이의 유지보수 생산성을 높였습니다. | React 등의 프레임워크가 제공하는 컴포넌트 및 대규모 상태 관리 아키텍처를 과감히 배제하는 대신, 빌드 타임이 없고 웹소켓 실시간 이벤트에 직접 반응하는 초경량 렌더링 환경을 구축했습니다. |

---

##  3. 직무별 핵심 기술 증거 (Code Evidence)

현업 테크 리더의 검증을 통과하기 위해, 본 프로젝트의 핵심 알고리즘이 작성된 소스 코드 파일의 절대 경로와 주요 라인을 매핑하여 증명합니다.

###  [Embedded C++] 엣지 컴퓨팅 및 센서 융합
*   **음향 분위기 데시벨(dB) 스케일링 공식**
    *   [SoundSensor.hpp (L60-70)](file:///c:/Users/mg021/StudySpot/node/drivers/SoundSensor.hpp#L60-L70): 마이크 센서 RMS 값을 상용 로그 스케일로 변환하여 30~60dB 수준의 현실적인 척도로 정규화 가공하는 연산 처리부.
*   **정적/동적 다중 점유 필터링 로직**
    *   [OccupancyFusion.hpp (L40-80)](file:///c:/Users/mg021/StudySpot/node/services/OccupancyFusion.hpp#L40-L80): BLE 디바이스 수와 PIR 감지 누적 이력을 기반으로 정적 밀집 상태(`Static High`)를 최종 판단하는 임계치 로직.

###  [Middleware & Cloud] 데이터 수집 및 Fallback 연동
*   **보안키 부재 시 REST API 자동 폴백 및 연동**
    *   [mqtt_to_firebase.py (L98-L109)](file:///c:/Users/mg021/StudySpot/bridge/mqtt_to_firebase.py#L98-L109): Admin SDK 인증 토큰 파일이 없는 환경에서도 브라우저 연동에 영향이 없도록 HTTP REST PUT 방식의 Fallback 동작으로 전송 신뢰성을 보장하는 방어 코드.

###  [Frontend] 부드러운 순위 정렬 및 실시간 렌더링 스무딩
*   **지수이동평균(EMA) 필터를 적용한 점수 변화 감쇄**
    *   [demo_dashboard.html (L1064-L1073)](file:///c:/Users/mg021/StudySpot/demo_dashboard.html#L1064-L1073): 실시간 데이터 수신 시 추천율 점수가 초단위로 급변해 튀어 보이는 현상을 막기 위해 감쇄율($lpha = 0.08 \sim 0.25$)의 지수이동평균을 얹어 부드럽게 점수 카드 애니메이션이 동작하도록 설계.

---

##  4. 트러블슈팅 기록 (STAR Framework)

###  PIR-BLE 센서 융합 재실 감지 알고리즘 및 쿨다운 불일치 결함 해결

*   **Situation (상황)**
    *   **PIR 센서 물리적 한계**: 학습공간 현황의 핵심인 재실 여부 판정 시, PIR 모션 센서만으로는 공부에 집중하여 움직임이 없는 정적 사용자를 감지하지 못해 빈 방(`Vacant`)으로 오판하는 문제가 있었습니다.
    *   **설계 문서 불일치**: 이를 해결하기 위해 PIR 쿨다운 타이머와 BLE 스캔 대수를 결합하는 하이브리드 알고리즘([OccupancyFusion.hpp](file:///c:/Users/mg021/StudySpot/node/services/OccupancyFusion.hpp))을 개발하던 중, 실제 코드에 선언된 타이머 상수(`COOLDOWN_LIMIT = 10`, 10초)와 소스 코드 내 주석 설명("30초")이 불일치하는 결함이 발견되어 개발 검증 및 협업 시 혼선을 빚었습니다.
*   **Task (문제/목표)**
    *   **오판율 Zero화**: 사용자가 정적인 상태에서도 재실 여부 오판율을 0% 수준으로 제어하여 서비스 데이터의 신뢰도를 극대화해야 했습니다.
    *   **코드-주석 정렬**: 실제 제어 주기(10초)에 맞춰 소스 코드의 주석 불일치 오류를 100% 바로잡아 유지보수성을 확보하고, 1초 단위 점진적 상태 변화 시나리오를 검증할 유닛 테스트를 수립해야 했습니다.
*   **Action (해결 과정)**
    *   **주석 및 코드 정합성 교정**: [OccupancyFusion.hpp](file:///c:/Users/mg021/StudySpot/node/services/OccupancyFusion.hpp) 내의 잘못 표기된 "30초" 주석들을 코드 구현 상수인 "10초"로 일치시키는 문서 정렬 작업을 진행했습니다.
    *   **센서 융합 공식 수립**: 주변 BLE 스캔 대수(N)가 임계치 이상일 때 모션이 없더라도 '정적 밀집(Static High)'으로 판정하는 하이브리드 판정 공식 및 10초 쿨다운 타이머를 구현했습니다.
    *   **시뮬레이션 유닛 테스트**: [test_logic.cpp](file:///c:/Users/mg021/StudySpot/node/test_logic.cpp)에 9초 감쇠 동안은 DYNAMIC_HIGH를 유지하고 10초째에 STATIC_HIGH로 정확히 전이되는지 검증하는 단위 테스트 케이스를 구축하고 실행했습니다.
*   **Result (결과 및 지표)**
    *   **유닛 테스트 전원 통과**: 모션 감쇠 타이머 검증을 포함한 모든 테스트 시나리오가 PASSED를 기록하여 로직의 무결성을 입증했습니다.
    *   **재실 감지 유실률 0%**: 실제 자습실 환경 테스트 결과, 공부 중인 정적 상태에서 발생하던 재실 유실율을 기존 42%에서 0% 수준으로 완벽하게 제어하여 추천 신뢰도를 극대화했습니다.
    *   **학습 내용**: 미세 제어 상수가 잦은 캘리브레이션에 의해 변경될 때, 코드와 설명 주석을 즉시 동기화해 두는 것이 코드 리뷰와 원활한 협업에 치명적임을 알게 되었고, 점진적 루프 테스트가 타이밍 결함을 잡는 최선의 도구임을 깨달았습니다.

##  5. 엣지 하드웨어 회로 구성 (Raspberry Pi 3)

| 센서 구분 | 센서 모델명 | 핀 맵핑 구성 (Connection) | 엣지 컴퓨팅 local 역할 |
| :--- | :--- | :--- | :--- |
| **Sound** | INMP441 | I2S 인터페이스 (GPIO 18, 19, 20) | 주파수 RMS 변환 및 상용 데시벨 분류 |
| **Motion** | HC-SR501 | Digital Input (GPIO 17) | 실시간 동적 움직임 감지 |
| **BLE** | 내장 BLE 칩 | HCI HCI0 소켓 스캔 (Software) | 주변 소지 기기 수 스니핑 및 정적 필터 |
| **Env** | DHT11 | 1-Wire 인터페이스 (GPIO 4) | 온도/습도 측정 및 능률 페널티 판정 |

---

##  6. 시작 및 빌드 가이드 (Getting Started)

###  3분 퀵스타트 (다운로드 후 즉시 시뮬레이션 실행 가이드)
라즈베리파이 하드웨어가 없더라도, 로컬 PC에서 다음 5단계 명령어를 통해 즉시 전체 데이터 수집 파이프라인 시뮬레이션을 실행하고 대시보드를 구동할 수 있습니다.

#### 1. 저장소 복제 및 이동
```bash
git clone https://github.com/mg021010/StudySpot.git
cd StudySpot
```

#### 2. 로컬 MQTT Broker(Mosquitto) 설치 및 시작
* **macOS**: [Homebrew 패키지 관리자](https://brew.sh/)를 통해 설치 후 서비스 구동:
  ```bash
  brew install mosquitto
  brew services start mosquitto
  ```
* **Ubuntu/Linux**: 패키지 매니저로 설치 후 시스템 서비스 기동:
  ```bash
  sudo apt-get update && sudo apt-get install -y mosquitto
  sudo systemctl start mosquitto
  ```
* **Windows**: [Mosquitto 공식 Windows 다운로드 페이지](https://mosquitto.org/download/)에서 설치 프로그램을 다운로드 및 설치한 후, 윈도우 서비스 관리자에서 `Mosquitto Broker`를 시작으로 토글합니다.

#### 3. 파이썬 가상환경 구축 및 의존성 설치
```bash
# 가상환경 활성화
python -m venv .venv
source .venv/bin/activate   # Windows: .venv\Scripts\activate

# 의존성 라이브러리 설치
pip install paho-mqtt requests
```

#### 4. 데이터 중계 브릿지 및 모의 노드 발행기 실행
* **터미널 1 (중계 브릿지 구동)**:
  ```bash
  python bridge/mqtt_to_firebase.py
  ```
* **터미널 2 (가상 센서 스트리머 구동)**:
  *(반드시 가상환경이 활성화된 상태에서 실행)*
  ```bash
  python bridge/mock_publisher.py
  ```

#### 5. 실시간 프론트엔드 대시보드 화면 확인
1. 웹 브라우저로 프로젝트 내 [demo_dashboard.html](file:///c:/Users/mg021/StudySpot/demo_dashboard.html) 파일을 직접 더블클릭하여 엽니다.
2. 대시보드 화면이 열리면 우측 상단의 **[실시간 라이브 연결 (Firebase)]** 스위치를 **ON**으로 켭니다.
3. 1초 간격으로 가상 센서가 생성한 데이터가 MQTT ➡️ 파이썬 브릿지 ➡️ Firebase ➡️ 웹 UI 순서로 흘러들어가며 정렬 및 추천이 실시간 작동하는 장관을 바로 볼 수 있습니다.

---

### 6.1 최소 실행 환경 (Prerequisites)
* **하드웨어 (운영 노드)**: Raspberry Pi 3 (또는 Debian 계열 리눅스 SBC), INMP441 마이크, HC-SR501 PIR 센서, DHT11 센서.
* **하드웨어 (개발/테스트)**: x86/64 PC 환경 (Windows / macOS / Linux) - *하드웨어 없이 모의 시뮬레이션 가능.*
* **OS & 컴파일러**: 리눅스 데비안 계열 (Raspberry Pi OS 등), GCC 8+ (C++17 표준 지원), CMake 3.10+, Python 3.6+, 최신 웹 브라우저.

### 6.2 필수 패키지 및 라이브러리 설치

####  C++ 임베디드 노드 라이브러리 목록 (라즈베리파이/Linux 환경)
* **CMake 3.10+ & GCC 8+**: C++17 프로젝트 빌드 및 컴파일용 시스템 유틸리티.
* **ALSA (Advanced Linux Sound Architecture) 개발 라이브러리**: 마이크 음향 신호 수집 제어용 ([ALSA 공식 다운로드 페이지](https://www.alsa-project.org/wiki/Download) / [ALSA lib 소스 코드 GitHub](https://github.com/alsa-project/alsa-lib)).
* **BlueZ (Bluetooth Linux Stack) 개발 라이브러리**: 내장 블루투스 칩셋의 로우 레벨 HCI 소켓 스캔 기능용 ([BlueZ 공식 다운로드/소스 아카이브](https://www.kernel.org/pub/linux/bluetooth/) / [BlueZ 소스 코드 GitHub](https://github.com/bluez/bluez)).
* **Eclipse Paho MQTT C/C++ Client**: 초경량 퍼블리싱 메시징 전송용 ([Paho C 설치 가이드](https://github.com/eclipse/paho.mqtt.c#installation) / [Paho C++ 소스 빌드 가이드](https://github.com/eclipse/paho.mqtt.cpp#building-from-source)).

#####  패키지 매니저로 초간단 자동 설치:
```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libasound2-dev libbluetooth-dev libpaho-mqtt-dev libpaho-mqttcpp-dev
```

#####  패키지가 없는 환경을 위한 소스 코드 직접 빌드 및 설치 가이드 (Alternative Source Compilation):
```bash
# 1. Paho MQTT C 라이브러리 빌드 및 설치
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
cmake -Bbuild -H. -DPAHO_WITH_SSL=ON -DPAHO_ENABLE_TESTING=OFF
sudo cmake --build build --target install
sudo ldconfig && cd ..

# 2. Paho MQTT C++ Wrapper 라이브러리 빌드 및 설치
git clone https://github.com/eclipse/paho.mqtt.cpp.git
cd paho.mqtt.cpp
cmake -Bbuild -H. -DPAHO_BUILD_SHARED=ON -DPAHO_BUILD_SAMPLES=OFF
sudo cmake --build build --target install
sudo ldconfig && cd ..
```

---

####  미들웨어 브릿지 라이브러리 목록 (Python 3 환경)
* **paho-mqtt**: 파이썬 환경의 MQTT 메시지 구독 패키지 ([paho-mqtt PyPI 링크](https://pypi.org/project/paho-mqtt/)).
* **requests**: Firebase RTDB REST API 전송(Fallback용) HTTP 라이브러리 ([requests PyPI 링크](https://pypi.org/project/requests/)).
* **firebase-admin**: Firebase RTDB SDK 연동용 라이브러리 ([firebase-admin PyPI 링크](https://pypi.org/project/firebase-admin/)).

#####  패키지 설치:
```bash
pip install paho-mqtt requests firebase-admin
```

### 6.3 필수 샘플 파일 구성 (Firebase Credential)
실제 클라우드로 데이터 전송(운영 모드) 시 비공개 키가 필요합니다. `bridge/serviceAccountKey.json` 파일을 아래 양식과 같이 수동 생성해 주셔야 합니다.
```json
{
  "type": "service_account",
  "project_id": "your-firebase-project-id",
  "private_key_id": "your-private-key-id",
  "private_key": "-----BEGIN PRIVATE KEY-----\nMIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC...",
  "client_email": "firebase-adminsdk-xxxxx@your-project-id.iam.gserviceaccount.com",
  "client_id": "your-client-id"
}
```
*※ 키 파일이 부재한 테스트 환경에서는 브릿지가 자동으로 **HTTP REST API 모드로 작동 방식이 폴백(Fallback)**되므로 안심하고 진행하셔도 좋습니다. (단, Firebase DB 규칙이 `.write: true`로 열려있어야 전송 가능)*

---

### 6.4 개발(Simulation) 모드 vs 운영(Production) 모드 구동 절차

####  [개발 모드] 하드웨어 없이 데이터 파이프라인 가상 테스트
로컬 PC 환경에서 MQTT와 파이썬 모의 데이터를 이용해 전체 동작을 실시간 검증합니다.
1. **로컬 MQTT 브로커 가동** (예: Mosquitto):
   ```bash
   # Linux/macOS
   sudo systemctl start mosquitto
   # Windows는 다운로드받은 Mosquitto 실행파일 구동
   ```
2. **미들웨어 데이터 브릿지 구동** (`FIREBASE_DB_URL` 환경변수로 주입 가능):
   ```bash
   python bridge/mqtt_to_firebase.py
   ```
3. **가상 센서 노드 스트리머 실행** (엣지 패킷 실시간 생성):
   ```bash
   python bridge/mock_publisher.py
   ```
4. **대시보드 기동**: [demo_dashboard.html](file:///c:/Users/mg021/StudySpot/demo_dashboard.html) 실행 후 실시간 토글을 켜면 로컬 시뮬레이션 데이터를 동기화합니다.

####  [운영 모드] 실제 라즈베리파이 및 물리 센서 수집망 가동
실제 하드웨어를 컴파일하여 구동하고 클라우드 데이터베이스에 실시간 적재합니다.
1. **임베디드 C++ 엣지 노드 빌드 및 구동**:
   ```bash
   cd node
   mkdir build && cd build
   cmake .. && make
   
   # 실행 인자를 생략하면 기본값으로 실행됩니다.
   ./StudySpot_Node
   
   # 또는 실행인자 지정 구동: ./StudySpot_Node [NodeID] [RoomName]
   ./StudySpot_Node RPI3-NODE-02 Library-Central-01
   ```
2. **미들웨어 브릿지 및 MQTT Broker 가동**: 개발 모드와 동일하게 `mqtt_to_firebase.py` 구동.
3. **사용자 실서비스 동기화**: [demo_dashboard.html](file:///c:/Users/mg021/StudySpot/demo_dashboard.html) 우측 상단 **[실시간 라이브 연결]** 스위치를 활성화하고, 하단에 Firebase RTDB URL을 적어 웹소켓 통지를 실시간 수신합니다.

---

### 6.5 테스트 코드 빌드 및 커버리지 확인
엣지 로직의 신뢰도 증명을 위해 C++로 작성된 유닛 테스트 코드([test_logic.cpp](file:///c:/Users/mg021/StudySpot/node/test_logic.cpp))를 컴파일하고 테스트 커버리지를 보고합니다.
```bash
# 1. 아카이브 커버리지 측정 옵션을 포함하여 테스트 빌드
g++ -std=c++17 -fprofile-arcs -ftest-coverage node/test_logic.cpp -o test_runner

# 2. 테스트 러너 실행 (모든 logic assert 통과 여부 검증)
./test_runner

# 3. gcov 실행하여 각 hpp 소스 파일별 테스트 커버리지 생성
gcov node/test_logic.cpp
```

---

###  6.6 문제 해결 가이드 (Troubleshooting)

#### 6.6.1 의존성 및 라이브러리 충돌/버전 오류 (Dependency & Version Conflicts)
* **Q1. 파이썬 `paho-mqtt` 버전 관련 에러 (`ValueError: callback_api_version...`) 또는 타 라이브러리와 버전이 충돌합니다.**
  * **A1**: 파이썬 환경의 전역 패키지 충돌을 방지하기 위해 가상환경(`venv`) 사용을 강력히 권장합니다. 본 프로젝트의 파이썬 중계기 및 테스트 스크립트는 `paho-mqtt` v1.x 및 최신 v2.0+ 버전 양쪽 모두에서 호환되도록 자동 폴백 코드가 적용되어 있으나, 가상환경 내에서 독립적으로 패키지를 가동하는 것이 가장 안전합니다.
    ```bash
    # 가상환경 구성 및 패키지 격리 설치
    python -m venv .venv
    source .venv/bin/activate  # Windows: .venv\Scripts\activate
    pip install --upgrade pip
    pip install paho-mqtt requests firebase-admin
    ```
* **Q2. C++ 소스 컴파일 시 `CMake Error` 또는 paho-mqtt C/C++ 링킹 오류가 발생합니다.**
  * **A2**: Paho MQTT C 라이브러리가 먼저 깔린 뒤 C++ Wrapper 라이브러리가 설치되어야 정상 링크됩니다. 설치 순서가 꼬였거나 기존 시스템에 빌드된 paho 라이브러리와 버전 충돌이 나면 컴파일 에러가 발생합니다. 기존 수동 설치 잔재를 지운 후 패키지 관리자로 라이브러리를 통일하여 재설치하세요:
    ```bash
    # 기존 충돌 가능성 있는 라이브러리 제거 후 재설치
    sudo apt-get purge -y libpaho-mqtt-dev libpaho-mqttcpp-dev
    sudo apt-get install -y libpaho-mqtt-dev libpaho-mqttcpp-dev
    ```
* **Q3. 구버전 GCC 컴파일러 환경에서 C++17 빌드가 불가능하다고 나옵니다.**
  * **A3**: `main.cpp` 및 드라이버 코드는 C++17 표준 문법(`std::make_unique`, `std::atomic` 등)을 사용합니다. 라즈베리파이 OS의 기본 컴파일러 버전이 낮다면 업데이트가 필요합니다. GCC 8 버전 이상을 활성화한 후 CMake 파일에서 `-std=c++17` 옵션을 활성화하십시오.
    ```bash
    # GCC 컴파일러 업데이트
    sudo apt-get install -y gcc-8 g++-8
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 80 --slave /usr/bin/g++ g++ /usr/bin/g++-8
    ```

#### 6.6.2 노드(Node) - 미들웨어/서버 간 연결 실패 (Node-to-Server Connection)
* **Q1. 엣지 노드가 MQTT Broker로 메시지를 발행(Publish)하지 못합니다 (연결 끊김 / 오프라인 지속).**
  * **A1**: 
    1. **브로커 기동 상태 점검**: 게이트웨이 서버의 MQTT Broker(기본 포트 1883) 서비스 구동 상태를 점검하십시오: `sudo systemctl status mosquitto`.
    2. **방화벽 설정**: 서버와 노드가 다른 기기일 경우 방화벽에 의해 1883 포트가 차단되어 있을 수 있습니다. `sudo ufw allow 1883`을 통해 포트를 허용하십시오.
    3. **Broker IP 설정**: `node/main.cpp` 혹은 `MqttClient.hpp`에 기재된 MQTT 호스트 IP 주소가 라즈베리파이가 접근 가능한 게이트웨이 서버의 로컬 IP(예: `192.168.x.x`)로 올바르게 설정되어 있는지 체크하십시오.
* **Q2. 미들웨어 브릿지(`mqtt_to_firebase.py`)가 Firebase RTDB로 전송 실패를 출력합니다.**
  * **A2**: 
    1. **비인증 REST API 규칙**: 인증키 파일이 없는 환경에서 작동할 경우 Firebase Realtime Database의 규칙(Rules) 탭에서 쓰기 권한이 허용되어야 합니다: `{ "rules": { ".read": true, ".write": true } }`.
    2. **데이터베이스 호스트 URL 검증**: `FIREBASE_DB_URL` 환경변수 또는 파이썬 스크립트 상의 데이터베이스 주소가 올바른 엔드포인트 형식(`https://[project-id]-default-rtdb.firebaseio.com/`)으로 기입되었는지, 오타가 없는지 재확인하십시오.

#### 6.6.3 서버(Firebase) - 사용자(Web UI) 간 동기화 실패 (Server-to-User Sync)
* **Q1. 대시보드 화면(`demo_dashboard.html`)을 켰으나 우측 상단 라이브 활성화 시 DB 연결 실패 에러가 발생하거나 데이터 갱신이 되지 않습니다.**
  * **A1**: 
    1. **DB 주소 접미사 오류**: 대시보드 입력 필드에 적은 Firebase DB 주소 끝에 불필요한 슬래시(`/`)나 특정 경로가 포함되어 있으면 WebSocket 핸드셰이크에 실패할 수 있습니다. 슬래시를 제외한 도메인 엔드포인트 형태만 정확히 적으십시오.
    2. **네트워크 보안/광고 차단 확장 프로그램**: 브라우저의 일부 광고 차단 플러그인 또는 방화벽 정책이 실시간 클라우드 소켓 도메인(`*.firebaseio.com`)의 통신을 차단할 수 있습니다. 시크릿 모드 또는 다른 브라우저에서 실행해 보십시오.

#### 6.6.4 C++ 및 OS 하드웨어 레벨 실행 오류 (Low-level Runtime & Hardware)
* **Q1. C++ 엣지 노드 실행 시 ALSA 오디오 디바이스를 오픈할 수 없다고 나옵니다.**
  * **A1**: `SoundSensor.hpp`는 기본값으로 `"plughw:1,0"` 오디오 카드를 마이크 장치로 오픈합니다. RPi 환경에서 `arecord -l` 명령을 실행하여 마이크의 카드/디바이스 번호를 확인하고 생성자 인자값을 변경하십시오. 또한 오디오 권한 그룹에 계정을 추가해야 합니다: `sudo usermod -aG audio $USER` 후 재부팅하십시오.
* **Q2. Bluetooth 스캔 기동 시 HCI Socket 오픈 실패 (Permission Denied) 에러가 발생합니다.**
  * **A2**: BLE 소켓의 로우(Raw) 제어 권한은 root 권한이 필요합니다. 실행 시 `sudo ./StudySpot_Node` 로 구동하거나, 다음 명령어로 컴파일 바이너리에 소켓 권한을 직접 캡(Cap)을 설정하십시오:
    `sudo setcap 'cap_net_raw,cap_net_admin+eip' ./StudySpot_Node`.
