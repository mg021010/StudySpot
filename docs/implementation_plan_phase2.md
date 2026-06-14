# CampusSpot: Phase 2 & 3 Implementation Plan

## Overview
Based on the completion of Phase 1 (Sensor Interfaces & Drivers), this plan outlines the next steps for implementing the core logic (Categorization & Fusion) and the communication layer within the C++ Edge Node.

## Step 1: Core Services Implementation (`node/services/`)
We will create service classes that act as the "brain" of the edge node, taking raw sensor data and applying the project's unique logic.

### 1.1 `AcousticCategorizer` Service
*   **Role**: Analyze sound data to determine the "Acoustic Identity" category.
*   **Logic**: Process audio characteristics (RMS volume, basic frequency analysis if applicable) to classify the current environment into one of 4 states:
    *   `Deep Focus`
    *   `White Noise`
    *   `BGM`
    *   `Active Discussion`
*   *Note*: This replaces the deprecated Study-Fit Score.

### 1.2 `OccupancyFusion` Service
*   **Role**: Implement the Hybrid Occupancy logic.
*   **Logic**: Combine data from `PirSensor` (movement events) and `BleSniffer` (device count).
    *   E.g., High BLE count + Low PIR events = "Static High Occupancy" (Deep study state).
    *   E.g., High BLE count + High PIR events = "Dynamic High Occupancy" (Active area).

### 1.3 `DataAggregator` Service
*   **Role**: Periodically poll all sensors and fusion services to construct a complete snapshot of the current state.

## Step 2: Communication Layer (`node/network/` or `node/services/`)
*   **Library**: Introduce a lightweight C++ MQTT client (e.g., Eclipse Paho MQTT C++).
*   **`MqttPublisher` Service**: 
    *   Format the aggregated snapshot into a JSON payload.
    *   Publish to a designated MQTT broker topic (e.g., `campusspot/node/1/status`).
*   **Data Format Example**:
    ```json
    {
      "node_id": "rpi3-lib-01",
      "timestamp": 1678886400,
      "acoustic_category": "Deep Focus",
      "occupancy_status": "Static High",
      "raw_metrics": {
        "device_count": 45,
        "pir_events": 2,
        "temperature": 22.5,
        "humidity": 45.0
      }
    }
    ```

## Step 3: Main Application Integration (`node/main.cpp`)
*   Initialize all drivers (`Mock` or `Hardware` based on configuration).
*   Inject drivers into the Services.
*   Setup the main event loop to run at defined intervals (e.g., every 10 seconds).
*   Ensure clean shutdown handling (Ctrl+C).

## Step 4: CMake Configuration Updates
*   Add new service files to the build.
*   Integrate MQTT and JSON libraries (e.g., `nlohmann/json`) into `CMakeLists.txt` (using FetchContent or system packages).

---
**Review Request:** Does this sequence align with your vision? Should we begin with Step 1.1 (`AcousticCategorizer`)?