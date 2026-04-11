import cv2
import time
import numpy as np
from picamera2 import Picamera2
from ultralytics import YOLO
import paho.mqtt.client as mqtt
import board
import neopixel
import threading

class VehicleSafetySystem:
    def __init__(self, mqtt_client):
        # FPS Counter variables
        self.prev_time = 0
        self.fps = 0
        self.alpha = 0.1  # smoothing factor
        self.dist = 999.0
        
        # Setup MQTT
        self.client = mqtt_client
        self.MQTT_TOPIC_DATA = "mycompany/vehiclesafety/demo_customer/vehicle_001/raw/front"
        self.MQTT_TOPIC = "mycompany/vehiclesafety/demo_customer/vehicle_001/status/collision/front"
        
        # Load AI Model
        self.model = YOLO('/home/cspi/Documents/best_ncnn_model_train10_640', task='detect')
        self.img_size = 640
        
        # LED Setup
        self.pixels = neopixel.NeoPixel(board.D18, 10, brightness=0.5, auto_write=False)
        self.FRONT_LEDS = [3, 4, 5]
        self.blink_state = True
        self.last_blink_time = time.time()
        
        # Alert Logic
        self.warning_map = {
            'Construction Ahead': 7, 'Give Way': 8, 'Keep Left': 9, 'Keep Right': 10,
            'Manhole Cover': 11, 'Motorcycle Lane': 12, 'Pass Either Side': 13,
            'Pedestrian Crossing': 14, 'Slippery road': 15, 'Slow Down': 16,
            'Bumps': 17, 'Stop': 18, 'Traffic Light Ahead': 19   
        }
        self.last_alert_times = {} # To store the last time each sign type was reported
        self.COOLDOWN = 5 # Only alert for the same sign every 5 seconds
        self.name = None
        self.last_sign = " "
        
        # Auto publish "SAFE"
        self.last_status = "SAFE"
        self.last_seen_time = 0
        self.HOLD_DURATION = 5.0

        # 4. Setup Cameras
        self.cam_l = Picamera2(0)
        self.cam_r = Picamera2(1)
        config = {"main": {"size": (640, 480)}, "sensor": {"output_size": (1640, 1232)}, "buffer_count": 1}
        for pc in [self.cam_l, self.cam_r]:
            pc.configure(pc.create_preview_configuration(**config))
            pc.start()
            
        self.latest_frame = None
        self.frame_lock = threading.Lock()
        self.running = False

        # 5. Parameters for distance measurment.
        self.BASELINE = 10.0 # The distance between your lenses
        self.FOCAL_PIXELS = 600 # Calibration constant for IMX219 @ 640x480

    def update_leds(self, dist_cm):
        dist_m = dist_cm / 100.0
        target_color = (0, 0, 0)
        if dist_m < 3.0:
            current_time = time.time()
            if current_time - self.last_blink_time > 0.2:
                self.blink_state = not self.blink_state
                self.last_blink_time = current_time
            target_color = (255, 0, 0) if self.blink_state else (0, 0, 0)
        elif dist_m < 5.0:
            target_color = (255, 100, 0)
        else:
            # Off
            target_color = (0, 0, 0)
            
        for i in self.FRONT_LEDS:
            self.pixels[i] = target_color
        self.pixels.show()
        
    def get_frames(self):
        return (cv2.cvtColor(self.cam_l.capture_array(), cv2.COLOR_BGRA2RGB),
                cv2.cvtColor(self.cam_r.capture_array(), cv2.COLOR_BGRA2RGB))

    def process_ai(self, frame):
        return self.model.predict(frame, imgsz=self.img_size, conf=0.4, verbose=False, rect=True)

    def handle_alerts(self, results):
        sign_detected_in_frame = False

        for box in results[0].boxes:
            # Identify the detected class name.
            self.name = self.model.names[int(box.cls[0])]
            
            if self.name in self.warning_map:
                sign_detected_in_frame = True

                # Determine the current status for this sign
                sign_id = str(self.warning_map.get(self.name, 0))
                    
                # Publish only if status changed OR cooldown passed                    
                if self.name != self.last_sign:
                    self.client.publish(self.MQTT_TOPIC, sign_id)
                    self.client.publish(self.MQTT_TOPIC_DATA, self.name)
                    self.last_status = "DANGER"
                    print(f"Status changed to: {self.name}")
                    self.last_seen_time = time.time()
                    self.last_sign = self.name
                
                else:
                    if (time.time() - self.last_seen_time) > self.HOLD_DURATION:
                        self.client.publish(self.MQTT_TOPIC, sign_id)
                        self.client.publish(self.MQTT_TOPIC_DATA, self.name)
                        self.last_status = "DANGER"
                        print(f"Status changed to: {self.name}")
                        self.last_seen_time = time.time()
                    
        if self.last_status == "DANGER" and time.time() - self.last_seen_time > self.HOLD_DURATION: 
            self.client.publish(self.MQTT_TOPIC, "SAFE")
            self.last_status = "SAFE"
            print("Status changed to: SAFE") 

    def draw_text_with_background(self, img, dist, fps, position):
        # 1. Get the size of the text to calculate the box dimensions
        if dist < 900:
            line1 = f"Dist: {dist:.1f}cm"
        else:
            line1 = "Dist: N/A"
        line2 = f"FPS : {fps:.1f}"
         
        font = cv2.FONT_HERSHEY_SIMPLEX
        font_scale = 0.7
        thickness = 2
        line_spacing = 25
        
        # 2. Get size of the longest line to size the box
        (w1, h1), _ = cv2.getTextSize(line1, font, font_scale, thickness)
        (w2, h2), _ = cv2.getTextSize(line2, font, font_scale, thickness)
         
        box_w = max(w1, w2) + 10
        box_h = h1 + h2 + line_spacing
         
        # 3. Draw one single white box for both
        x, y = position
        cv2.rectangle(img, (x, y - h1 - 5), (x + box_w, y + box_h - h1), (255, 255, 255), cv2.FILLED)
         
        # 4. Draw Black Text lines
        cv2.putText(img, line1, (x + 5, y), font, font_scale, (0, 0, 0), thickness)
        cv2.putText(img, line2, (x + 5, y + line_spacing), font, font_scale, (0, 0, 0), thickness)
    
    def calculate_distance(self, res_l, f_r, annotated_l):
        # Check if at least one object is detected in both cameras
        if len(res_l[0].boxes) > 0:
            box_l = res_l[0].boxes[0]
            self.name = self.model.names[int(box_l.cls[0])]
            
            if self.name in ['Car', 'Person']:
                # Get X centers of the first detection on each side
                # .xywh[0] gets the first box, [0] inside that is the X-center
                res_r = self.process_ai(f_r)
                if len(res_r[0].boxes) > 0:
                    x_l = box_l.xywh[0][0].item()
                    x_r = res_r[0].boxes.xywh[0][0].item()
                    
                    # Calculate Disparity (horizontal shift)
                    disparity = abs(x_l - x_r)
                    if disparity > 0: 
                        # Calculate Distance in CM
                        self.dist = (self.BASELINE * self.FOCAL_PIXELS) / disparity  # Baseline * Focal / Disparity 
                        self.update_leds(self.dist)
        else:
            # No object detected, turn off LEDs
            self.update_leds(999)
            self.dist = 999.0

    def run(self):
        self.running = True
        try:
            while self.running:
                loop_start = time.time()
                
                f_l, f_r = self.get_frames()
                res_l = self.process_ai(f_l)
                self.handle_alerts(res_l)
                annotated_l = res_l[0].plot()
                
                self.calculate_distance(res_l, f_r, annotated_l)
                
                # --- FPS CALCULATION ---
                # Calculate time difference
                time_diff = time.time() - loop_start
                if time_diff > 0:
                    instant_fps = 1 / time_diff
                    self.fps = (1 - self.alpha) * self.fps + self.alpha * instant_fps
                else:
                    self.fps = 0
                
                self.draw_text_with_background(annotated_l, self.dist, self.fps, (15, 40))
                
                # Stitch views together side-by-side
                combined = np.hstack((annotated_l, f_r))

                with self.frame_lock:
                    self.latest_frame = combined
                    
        except Exception as e:
            print(f"Camera Error: {e}")
        finally:
            # IMPORTANT: Call the actual cleanup here
            self.close()
            
    def close(self): 
        self.running = False
        
        # 1. Clear the AI model from memory first
        # This forces the internal NCNN/YOLO timers to stop 
        # while we are still inside the Camera Thread.
        if hasattr(self, 'model'):
            del self.model 
            
        # 2. Stop and Close Picamera2
        if self.cam_l: self.cam_l.stop(); self.cam_l.close()
        if self.cam_r: self.cam_r.stop(); self.cam_r.close()

        # 3. Cleanup OpenCV Windows
        cv2.destroyAllWindows()
        print('Cleaning up cameras.')
        
        # 4. Turn off LEDs
        self.pixels.fill((0, 0, 0))
        self.pixels.show()