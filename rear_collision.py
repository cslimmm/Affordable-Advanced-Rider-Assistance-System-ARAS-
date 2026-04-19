import serial
import time
import board
import neopixel

class RearSensorSystem:
    def __init__(self, mqtt_client):
        # 1. UART Setup
        self.ser = serial.Serial('/dev/ttyAMA0', baudrate=57600, timeout=0.1)
        
        # 2. NeoPixel Setup
        self.pixels = neopixel.NeoPixel(board.D18, 3, brightness=0.5, auto_write=False, pixel_order=neopixel.GRB)
        
        # 3. MQTT Setup
        self.client = mqtt_client
        self.MQTT_TOPIC_DATA = "mycompany/vehiclesafety/demo_customer/vehicle_001/raw/rear"
        self.MQTT_TOPIC = "mycompany/vehiclesafety/demo_customer/vehicle_001/status/collision/rear"
        
        self.led_on = False
        self.last_led_toggle = 0
        self.LED_ON_TIME = 0.3

    def read_distance(self):
        byte = self.ser.read(1) # one byte at a time
        # Wait for header 0xff 0xff 0xff
        if byte == b'\xff':
            b2 = self.ser.read(1)
            b3 = self.ser.read(1)
            if b2 == b'\xff' and b3 == b'\xff':
                # read distance bytes + 126 spectral + 3 tail
                packet = self.ser.read(2 + 126 + 3)
                if len(packet) == 131:
                    pkt = b'\xff\xff\xff' + packet
                    # distance bytes are at index 3 and 4
                    dist_high = pkt[3] # high byte                                                                                                                                           
                    dist_low = pkt[4] # low byte
                    # combine two bytes into 16-bit distance
                    distance_cm = (dist_high << 8) + dist_low
                    # converter cm -> meters
                    distance_m = distance_cm / 100.0
                    #print("Distance: ",distance_m)
                    return distance_m
        return -1
    
    def update_feedback(self, distance_m):
        """Controls LED and MQTT based on distance."""
        current_time = time.monotonic()
            
        if distance_m < 10 and distance_m > 0.5:
            status = "DANGER"
            if distance_m < 5 and distance_m > 0.5:
                color = (255, 0, 0) 
            else:
                color = (255, 165, 0)
            self.pixels.fill(color)
            self.pixels.show()
            self.led_on = True
            self.last_led_toggle = current_time
        else:
            status = "SAFE"
            self.pixels.fill((0, 0, 0))
            self.pixels.show()
            self.led_on = False

        #print(f"Distance: {distance_m} m")
        self.client.publish(self.MQTT_TOPIC, status)
        self.client.publish(self.MQTT_TOPIC_DATA, distance_m)

    def run_cycle(self):
        """Executes one read/update cycle."""
        dist = self.read_distance()
        if dist >= 0:
            self.update_feedback(dist)
        
        # Non-blocking LED timeout
        if self.led_on and (time.monotonic() - self.last_led_toggle) >= self.LED_ON_TIME:
            self.pixels.fill((0, 0, 0))
            self.pixels.show()
            self.led_on = False
    
    def run_cycle_loop(self, is_running_func):
        while is_running_func(): # This loop keeps the thread alive
            self.run_cycle()
            time.sleep(0.01)

    def cleanup(self):
        # 1. Force the status back to SAFE
        print("Cleaning up Rear Sensor")
        self.client.publish(self.MQTT_TOPIC, "SAFE")
        
        # 2. Give MQTT a tiny moment to send the packet before closing the loop
        time.sleep(0.5) 
        
        # 3. Turn off LEDs
        self.pixels.fill((0, 0, 0))
        self.pixels.show()
        
        # 4. Close hardware resources
        self.ser.close()
