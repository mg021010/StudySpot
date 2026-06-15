import json
import os
import sys
import time

# Robust dependency check with helpful error messages
try:
    import requests
except ImportError:
    print("[ERROR] 'requests' module not found. Run: pip install requests")
    sys.exit(1)

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("[ERROR] 'paho-mqtt' module not found. Run: pip install paho-mqtt")
    sys.exit(1)

# Firebase admin is optional since we have a REST API fallback
USING_SDK = False
try:
    import firebase_admin
    from firebase_admin import credentials
    from firebase_admin import db as firebase_db
    USING_SDK = True
except ImportError:
    print("[WARNING] 'firebase-admin' module not found. Will default to Firebase REST API mode.")
    print("If you want to use the Admin SDK, install it using: pip install firebase-admin")

# --- Configuration ---
# You can change these variables or set them via Environment Variables
MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_TOPIC = "campus-spot/nodes/#"

# The RTDB URL is required. Paste your Realtime Database URL here:
# Example: "https://my-project-default-rtdb.firebaseio.com/"
FIREBASE_DB_URL = os.getenv(
    "FIREBASE_DB_URL", 
    "https://study-spot-demo-default-rtdb.firebaseio.com/" # Placeholder/Test URL
)

# Service Account Key file for SDK authentication. Put your json file in bridge/serviceAccountKey.json
SERVICE_ACCOUNT_PATH = os.getenv("SERVICE_ACCOUNT_PATH", "bridge/serviceAccountKey.json")

# Mapping dictionary to translate physical Room Names or Node IDs into DB keys ('council' or 'lab')
ROOM_MAPPING = {
    "RPI3-NODE-01": "council",
    "RPI3-NODE-02": "lab",
    "Library-Central-02": "council",
    "Library-Central-01": "lab",
    "ML415": "lab",
    "학생회실": "council"
}

# --- Initialize Firebase ---
SDK_ACTIVE = False
if USING_SDK and os.path.exists(SERVICE_ACCOUNT_PATH):
    try:
        print(f"[FIREBASE] Initializing Firebase Admin SDK with credentials from: {SERVICE_ACCOUNT_PATH}")
        cred = credentials.Certificate(SERVICE_ACCOUNT_PATH)
        firebase_admin.initialize_app(cred, {
            'databaseURL': FIREBASE_DB_URL
        })
        SDK_ACTIVE = True
        print("[FIREBASE] Admin SDK successfully initialized.")
    except Exception as e:
        print(f"[FIREBASE] SDK Initialization failed: {e}. Falling back to REST API mode.")
else:
    if not USING_SDK:
        print("[FIREBASE] Using REST API fallback because firebase-admin SDK is not installed.")
    else:
        print(f"[FIREBASE] Service Account file not found at '{SERVICE_ACCOUNT_PATH}'. Using REST API mode.")
        print("[FIREBASE] Please make sure your Firebase Realtime Database Rules allow public writes during testing:")
        print('           { "rules": { ".read": true, ".write": true } }')

print(f"[CONFIG] MQTT Broker: {MQTT_BROKER}:{MQTT_PORT}")
print(f"[CONFIG] Subscribing to topic: {MQTT_TOPIC}")
print(f"[CONFIG] Database URL: {FIREBASE_DB_URL}")

# --- Helper to write to Firebase RTDB ---
def write_to_firebase(room_key, data):
    """
    Writes status data to Firebase RTDB under /spaces/{room_key}
    """
    clean_db_url = FIREBASE_DB_URL.rstrip('/')
    
    if SDK_ACTIVE:
        try:
            ref = firebase_db.reference(f'spaces/{room_key}')
            ref.set(data)
            print(f"[FIREBASE SDK] [SUCCESS] Updated '{room_key}' with payload.")
            return True
        except Exception as e:
            print(f"[FIREBASE SDK] [ERROR] Failed to write using SDK: {e}")
            # Try falling back to REST
    
    # REST API Fallback
    try:
        url = f"{clean_db_url}/spaces/{room_key}.json"
        response = requests.put(url, json=data, timeout=5)
        if response.status_code == 200:
            print(f"[FIREBASE REST] [SUCCESS] Updated '{room_key}' via REST API.")
            return True
        else:
            print(f"[FIREBASE REST] [ERROR] Failed to update. Status: {response.status_code}, Response: {response.text}")
    except Exception as e:
        print(f"[FIREBASE REST] [CONNECTION ERROR] Failed to connect to Firebase RTDB: {e}")
    return False

# --- MQTT Callbacks ---
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[MQTT] Connected to Broker successfully.")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"[MQTT] Connection failed with code: {rc}")

def on_message(client, userdata, msg):
    try:
        payload_str = msg.payload.decode('utf-8')
        print(f"\n[MQTT] Received message on topic {msg.topic}")
        print(f"[MQTT] Payload: {payload_str}")
        
        # Parse JSON payload
        data = json.loads(payload_str)
        
        # Identify room key
        node_id = data.get("nodeId", "")
        room_name = data.get("roomName", "")
        
        # Determine database path room key
        room_key = None
        if node_id in ROOM_MAPPING:
            room_key = ROOM_MAPPING[node_id]
        elif room_name in ROOM_MAPPING:
            room_key = ROOM_MAPPING[room_name]
        else:
            # Check substrings if exact match not found
            for key, val in ROOM_MAPPING.items():
                if key in room_name or key in node_id:
                    room_key = val
                    break
        
        if not room_key:
            # Default fallback if no mapping found: use room_name cleaned or node_id
            room_key = room_name.lower().replace(" ", "_") if room_name else node_id.lower()
            if not room_key:
                room_key = "unknown_node"
        
        # Format the database payload
        # This mirrors the structure of demo_dashboard updates
        db_payload = {
            "nodeId": node_id,
            "roomName": room_name,
            "timestamp": data.get("timestamp", int(time.time())),
            "temperature": float(data.get("env", {}).get("temp", data.get("temperature", 0))),
            "humidity": float(data.get("env", {}).get("hum", data.get("humidity", 0))),
            "soundDb": float(data.get("acoustic", {}).get("db", data.get("soundDb", 0))),
            "soundRms": float(data.get("acoustic", {}).get("rms", data.get("soundRms", 0))),
            "acousticCategory": data.get("acoustic", {}).get("category", data.get("acousticCategory", "Unknown")),
            "bleDeviceCount": float(data.get("occupancy", {}).get("bleCount", data.get("bleDeviceCount", 0))),
            "pirMotion": int(data.get("occupancy", {}).get("pir", data.get("pirMotion", 0))),
            "occupancyStatus": data.get("occupancy", {}).get("status", data.get("occupancyStatus", "Unknown"))
        }
        
        # If timestamp is integer (Unix epoch), format it to matching string "YYYY-MM-DD HH:MM:S" or similar
        if isinstance(db_payload["timestamp"], (int, float)):
            struct_time = time.localtime(db_payload["timestamp"])
            db_payload["timestamp"] = time.strftime("%Y-%m-%d %H:%M:%S", struct_time)
            
        print(f"[BRIDGE] Routed packet to space: '{room_key}'")
        write_to_firebase(room_key, db_payload)
        
    except json.JSONDecodeError:
        print("[MQTT] Error: Failed to parse payload as JSON.")
    except Exception as e:
        print(f"[BRIDGE] Error processing message: {e}")

# --- Main loop ---
def main():
    # Compatibility with paho-mqtt v2.0+ which requires CallbackAPIVersion
    try:
        from paho.mqtt.enums import CallbackAPIVersion
        client = mqtt.Client(CallbackAPIVersion.VERSION1)
    except (ImportError, ValueError, AttributeError):
        client = mqtt.Client()
        
    client.on_connect = on_connect
    client.on_message = on_message
    
    print("[MQTT] Connecting to broker...")
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
    except Exception as e:
        print(f"[MQTT] Connection to broker {MQTT_BROKER}:{MQTT_PORT} failed: {e}")
        print("[MQTT] Please make sure your MQTT broker is running (e.g. Mosquitto).")
        print("[MQTT] Script will still attempt to reconnect in background...")
    
    # Non-blocking network loop
    client.loop_forever()

if __name__ == "__main__":
    main()
