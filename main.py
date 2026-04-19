import threading
import time
import signal
import sys
import cv2
from front_collision import VehicleSafetySystem
from rear_collision import RearSensorSystem
from emergency_contact import SafetySensorSystem
import paho.mqtt.client as mqtt

# Global flag to control thread execution
running = True
        
def on_message(client, userdata, msg):
    payload = msg.payload.decode().strip().upper()
    if msg.topic == "mycompany/vehiclesafety/demo_customer/vehicle_001/status/emergency":
        safety_sys.handle_emergency(payload)
        
def cleanup(sig, frame):
    global running
    print("\nShutdown signal received. Cleaning up...")
    running = False

# Register signal handler for Ctrl+C
signal.signal(signal.SIGINT, cleanup)

def main():
    global running
    
    # 1. Initialize shared MQTT Client
    mqtt_client = mqtt.Client(client_id="", userdata=None, protocol=mqtt.MQTTv311, callback_api_version=2)
    mqtt_client.connect("broker.hivemq.com", 1883, 60)
    mqtt_client.subscribe("mycompany/vehiclesafety/demo_customer/vehicle_001/status/emergency")
    mqtt_client.on_message = lambda c, u, m: safety_sys.handle_emergency(m.payload.decode().strip().upper())
    mqtt_client.loop_start()

    # 2. Instantiate Systems
    camera_sys = VehicleSafetySystem(mqtt_client)
    radar_sys = RearSensorSystem(mqtt_client)
    safety_sys = SafetySensorSystem(mqtt_client)

    # 3. Define Threads
    t_camera = threading.Thread(target=camera_sys.run, daemon=True)
    t_radar = threading.Thread(target=radar_sys.run_cycle_loop, args=(lambda: running,), daemon=True)
    t_safety = threading.Thread(target=safety_sys.run, args=(lambda: running,), daemon=True)

    # 4. Start Threads
    t_camera.start()
    t_radar.start()
    t_safety.start()

    print("-------------------------------------------------------")
    print("System running. Press Ctrl+C to stop.")

    # 5. Main Loop (GUI must stay here)
    try:
        while running:
            if camera_sys.latest_frame is not None:
                with camera_sys.frame_lock:
                    frame = camera_sys.latest_frame.copy()
                cv2.imshow("Object Detection", frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                running = False
            time.sleep(0.01)

    # 6. Cleanup
    finally:
        print("-------------------------------------------------------")
        print("Stopping systems...")
        camera_sys.running = False
        
        #  Wait for threads to finish
        t_camera.join(timeout=2)
        t_radar.join(timeout=2)
        t_safety.join(timeout=2)
    
        # Call the cleanup specifically
        radar_sys.cleanup() 
        
        # Only now stop the MQTT loop
        mqtt_client.loop_stop()
        mqtt_client.disconnect()
    
        cv2.destroyAllWindows()
        print("System safely shut down.")
        sys.exit(0)
    
if __name__ == "__main__":
    main()
