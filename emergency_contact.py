import smbus2
import time
import math
import serial
import pynmea2
import requests
import threading

class SafetySensorSystem:
    def __init__(self, mqtt_client):
        self.client = mqtt_client
        self.bus = smbus2.SMBus(1)
        self.DEVICE_ADDRESS = 0x68
        
        # MPU6050 Wakeup
        try:
            self.bus.write_byte_data(self.DEVICE_ADDRESS, 0x6B, 0x00)
            time.sleep(0.1)
            print("MPU6050 Initialized and Awake.")
        except Exception as e:
            print(f"MPU6050 Initialization Failed: {e}")
        
        # Thresholds
        self.TILT_THRESHOLD = 70.0
        self.GYRO_THRESHOLD = 80.0
        self.MQTT_TOPIC = "mycompany/vehiclesafety/demo_customer/vehicle_001/status/emergency"
        
        # Telegram
        self.BOT_TOKEN = "8683916136:AAFu_Xub_ZORo6koV7d9LcAWLnmxB7Jl6k="
        self.CHAT_ID = "123456"
        
        # GPS
        self.gps = serial.Serial(port='/dev/ttyAMA2', baudrate=9600, timeout=1)
        self.latest_gps = {"latitude": 0.0, "longitude": 0.0}
        
        # State
        self.CURRENT_STATE = "SAFE"
        self.crash_count = 0
        self.is_running = True

    # --- Telegram Alert ---
    def send_telegram_alert(self, lat, lon):
        def _execute_request():
            msg = f"\U0001F6A8 ACCIDENT DETECTED! \nDriver may need help.\n\U0001F4CD Location: https://www.google.com/maps/search/?api=1&query={lat},{lon}"
            url = f"https://api.telegram.org/bot{self.BOT_TOKEN}/sendMessage"
            try:
                requests.post(url, data={"chat_id": self.CHAT_ID, "text": msg}, timeout=5)
            except Exception as e:
                print(f"Telegram alert failed: {e}")
        threading.Thread(target=_execute_request).start()

    # --- MPU6050 Raw Data ---
    def read_raw_data(self, addr):
        high = self.bus.read_byte_data(self.DEVICE_ADDRESS, addr)
        low = self.bus.read_byte_data(self.DEVICE_ADDRESS, addr + 1)
        value = (high << 8) + low
        if value >= 0x8000:
            value -= 65536
        return value

    # --- IMU Readings ---
    def get_imu_data(self):
        # Accelerometer
        ax = self.read_raw_data(0x3B) / 16384.0
        ay = self.read_raw_data(0x3D) / 16384.0
        az = self.read_raw_data(0x3F) / 16384.0
        # Gyroscope
        gx = self.read_raw_data(0x43) / 131.0
        gy = self.read_raw_data(0x45) / 131.0
        gz = self.read_raw_data(0x47) / 131.0

        # Compute tilt
        roll = math.degrees(math.atan2(ay, az))
        pitch = math.degrees(math.atan2(ax, math.sqrt(ay**2 + az**2)))

        return (roll, pitch), (gx, gy, gz)

    # --- GPS Update ---
    def update_gps(self):
        try:
            if self.gps.in_waiting > 0:
                line = self.gps.readline().decode(errors='ignore')
                if line.startswith('$GPRMC'):
                    msg = pynmea2.parse(line)
                    if msg.status == 'A':
                        self.latest_gps = {"latitude": msg.latitude, "longitude": msg.longitude}
        except Exception:
            pass

    # --- Handle MQTT Emergency Payload ---
    def handle_emergency(self, payload):
        print(f"MQTT Received: {payload}")
        if payload == "ON" and self.CURRENT_STATE == "PENDING":
            print("Emergency confirmed! Sending Telegram alert.")
            self.send_telegram_alert(self.latest_gps['latitude'], self.latest_gps['longitude'])
            self.CURRENT_STATE = "SAFE"
            #self.client.publish(self.MQTT_TOPIC, "SAFE")
        elif payload == "OFF":
            print("System reset to SAFE.")
            self.CURRENT_STATE = "SAFE"
            self.client.publish(self.MQTT_TOPIC, "SAFE")

    # --- One Cycle ---
    def run_cycle(self):
        (roll, pitch), (gx, gy, gz) = self.get_imu_data()
        self.update_gps()

        crash_detected = (abs(roll) > self.TILT_THRESHOLD or abs(pitch) > self.TILT_THRESHOLD or
                          abs(gx) > self.GYRO_THRESHOLD or abs(gy) > self.GYRO_THRESHOLD or
                          abs(gz) > self.GYRO_THRESHOLD)
        if crash_detected:
            self.crash_count += 1
            print(f"({self.crash_count}) POSSIBLE ACCIDENT DETECTED")
            print(f"Roll: {roll:.2f}, Pitch: {pitch:.2f}, Gyro: X={gx:.2f}, Y={gy:.2f}, Z={gz:.2f}")
            if self.crash_count >= 5 and self.CURRENT_STATE == "SAFE":
                self.CURRENT_STATE = "PENDING"
                self.client.publish(self.MQTT_TOPIC, "PENDING")
                print("Crash detected - Status: PENDING")
                self.crash_count = 0
        else:
            self.crash_count = 0

    # --- Main Loop ---
    def run(self, is_running_func):
        while self.is_running and is_running_func():
            self.run_cycle()
            time.sleep(0.1)
