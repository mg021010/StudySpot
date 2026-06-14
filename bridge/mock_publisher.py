import json
import time
import random
import os
import sys

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("[ERROR] 'paho-mqtt' module not found. Run: pip install paho-mqtt")
    sys.exit(1)

MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_TOPIC_PREFIX = "campus-spot/nodes"

print(f"[MOCK] Initializing Mock Node Publisher...")
print(f"[MOCK] Broker: {MQTT_BROKER}:{MQTT_PORT}")

# State tracking for nodes
nodes = {
    "RPI3-NODE-01": {
        "roomName": "학생회실 회의 공간",
        "temp": 22.4,
        "hum": 48.0,
        "base_ble": 10.0,
        "activity": "discuss" # "discuss" -> noisy, high motion
    },
    "RPI3-NODE-02": {
        "roomName": "ML415 자습 공간",
        "temp": 22.8,
        "hum": 49.5,
        "base_ble": 5.8,
        "activity": "study" # "study" -> quiet, low motion
    }
}

def generate_report(node_id):
    cfg = nodes[node_id]
    
    # Random walk for env values
    cfg["temp"] += random.uniform(-0.02, 0.02)
    cfg["temp"] = max(20.0, min(26.0, cfg["temp"]))
    cfg["hum"] += random.uniform(-0.05, 0.05)
    cfg["hum"] = max(40.0, min(60.0, cfg["hum"]))
    
    # Sound & Occupancy based on activity style
    pir = 0
    sound_db = 25.0
    sound_rms = 0.0001
    ble_count = cfg["base_ble"] + random.uniform(-0.5, 0.5)
    
    if cfg["activity"] == "discuss":
        # Noisy discussion room
        pir = 1 if random.random() < 0.3 else 0
        sound_db = random.uniform(42.0, 58.0)
        sound_rms = 10 ** ((sound_db - 100) / 20)
        
        # Acoustic categorizer
        acoustic_cat = "Active Discussion" if sound_db > 45.0 else "BGM"
        
        # Occupancy fusion
        occupancy_status = "Dynamic High (Active Area)" if ble_count >= 3.5 else "Dynamic Low"
        
    else:
        # Quiet study room
        pir = 1 if random.random() < 0.02 else 0
        sound_db = 26.0 + random.uniform(-1.0, 3.0)
        if random.random() < 0.02:
            sound_db = random.uniform(32.0, 36.0) # page turn
        sound_rms = 10 ** ((sound_db - 100) / 20)
        
        # Acoustic categorizer
        acoustic_cat = "Deep Focus" if sound_db < 30.0 else "White Noise"
        
        # Occupancy fusion
        occupancy_status = "Static High (Deep Focus Room)" if ble_count >= 3.5 else "Static Low (Quiet Study)"

    report = {
        "nodeId": node_id,
        "roomName": cfg["roomName"],
        "timestamp": int(time.time()),
        "env": {
            "temp": round(cfg["temp"], 1),
            "hum": round(cfg["hum"], 1)
        },
        "acoustic": {
            "rms": round(sound_rms, 6),
            "db": round(sound_db, 1),
            "category": acoustic_cat
        },
        "occupancy": {
            "bleCount": round(ble_count, 1),
            "pir": pir,
            "status": occupancy_status
        }
    }
    return report

def main():
    client = mqtt.Client()
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        print("[MOCK] Connected to broker successfully. Publishing mock sensor ticks...")
    except Exception as e:
        print(f"[MOCK] Connection failed: {e}")
        print("[MOCK] Running in Offline console logging mode. Start your MQTT broker to test actual publishing.")
        client = None

    try:
        while True:
            for node_id in nodes.keys():
                report = generate_report(node_id)
                topic = f"{MQTT_TOPIC_PREFIX}/{node_id}"
                payload = json.dumps(report, ensure_ascii=False)
                
                if client:
                    client.publish(topic, payload)
                    print(f"[MOCK] Published to {topic}: {payload}")
                else:
                    print(f"[MOCK OFFLINE] Would publish to {topic}: {payload}")
                    
                time.sleep(0.5) # Sleep 0.5s between rooms (giving 1s interval per room)
                
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("\n[MOCK] Stopping mock node publisher...")
        if client:
            client.loop_stop()
            client.disconnect()

if __name__ == "__main__":
    main()
