import 'dart:async';
import 'package:flutter/material.dart';
import '../models/alert_record.dart';
import '../models/sensor_log.dart';
import '../mqtt/mqtt_topics.dart';
import '../mqtt/mqtt_service.dart';
import '../widgets/status_card.dart';
import 'alert_history_page.dart';
import 'sensor_detail_page.dart';


/// ===============================
/// MAIN MQTT PAGE
/// ===============================
class MqttPage extends StatefulWidget {
  const MqttPage({super.key});

  @override
  State<MqttPage> createState() => _MqttPageState();
}

class _MqttPageState extends State<MqttPage> {

  late MqttService mqttService; 

  /// Emergency countdown timer
  Timer? emergencyTimer;
  int countdown = 10;

  // ===== STATE VARIABLES =====
  String espStatus = 'Smart Helmet Disconnected';
  String alcoholStatus = 'SAFE';
  String frontCollision = 'SAFE';
  String rearCollision = 'SAFE';
  String helmetStatus = 'UNKNOWN';  

  bool emergencyTriggered = false;
  bool emergencyDialogShown = false;

  double volume = 5;

  // ===== ALERT HISTORY LIST=====
  List<AlertRecord> alertHistory = [];

  /// Sensor log lists
  List<SensorLog> alcoholLogs = [];
  List<SensorLog> frontLogs = [];
  List<SensorLog> rearLogs = [];
  List<SensorLog> helmetLogs = [];

  /// Warning for YOLO
  String warningName(String code) {

    switch(code){

      case "7": return "Construction Ahead";
      case "8": return "Give Way";
      case "9": return "Keep Left";
      case "10": return "Keep Right";
      case "11": return "Beaware of Manhole cover";
      case "12": return "Use the Motorcycle Lane";
      case "13": return "Pass Either Side";
      case "14": return "Pedestrian Crossing Ahead";
      case "15": return "Slippery Road";
      case "16": return "Slow Down";
      case "17": return "Speed Bump Ahead";
      case "18": return "Stop";
      case "19": return "Traffic Light Ahead";

      default: return "SAFE";
    }
  }

  /// ===============================
  /// INIT (setup MQTT connection)
  /// ===============================
  @override
  void initState() {
    super.initState();

    mqttService = MqttService();

    mqttService.onMessage = (topic, payload) {
      handleMqttMessage(topic, payload);
    };

    // Connect to broker
    mqttService.connect();
  }  

  /// ===============================
  /// HANDLE MQTT MESSAGE (main logic)
  /// ===============================
  void handleMqttMessage(String topic, String payload) {
    setState(() {

        // ===== RAW SENSOR LOGS =====
        if (topic == MqttTopics.alcoholRaw) addLog(alcoholLogs, payload);
        if (topic == MqttTopics.frontRaw) addLog(frontLogs, payload);
        if (topic == MqttTopics.rearRaw) addLog(rearLogs, payload);
        if (topic == MqttTopics.helmetRaw) addLog(helmetLogs, payload);

        /// ===============================
        /// DEVICE STATUS
        /// ===============================
        if (topic == MqttTopics.espStatus) {
          espStatus = payload == 'ONLINE'
              ? 'Smart Helmet Connected'
              : 'Smart Helmet Disconnected';
        }

      if (topic == MqttTopics.alcoholStatus) {
        final prev = alcoholStatus;
        alcoholStatus = payload;
        checkAndAddAlert(prev, payload, "Alcohol Detected");
      }

      if (topic == MqttTopics.frontCollision) {
        final prev = frontCollision;
        frontCollision = payload;
        checkAndAddAlert(prev, payload, "Front Collision Warning");
      }

      if (topic == MqttTopics.rearCollision) {
        final prev = rearCollision;
        rearCollision = payload;
        checkAndAddAlert(prev, payload, "Rear Collision Warning");
      }
      
      if (topic == MqttTopics.helmetStatus) helmetStatus = payload;

      // Handle emergency separately
      handleEmergency(topic, payload);
    });
  }

  /// ===============================
  /// HANDLE EMERGENCY LOGIC
  /// ===============================
  void handleEmergency(String topic, String payload) {
    if (topic != MqttTopics.emergency) return;

    // Reset state
    if (payload == 'SAFE') {
      emergencyTriggered = false;
      return;
    }
    
    // Start countdown before triggering alert
    if (payload == 'PENDING' && !emergencyDialogShown) {
      startEmergencyCountdown();
    }

    // Emergency triggered
    if (payload == 'ON') {
      emergencyTriggered = true;
      addAlert("Emergency Triggered"); // ✅ ADD THIS
      showEmergencyDialog();
    }

    // Emergency cancelled
    if (payload == 'OFF') {
      emergencyTriggered = false;
    }
  }

  /// ===============================
  /// ADD ALERT (for history page)
  /// ===============================
  void addAlert(String type) {
      alertHistory.insert(0, AlertRecord(type, DateTime.now()));
  }

  void checkAndAddAlert(String prev, String current, String message) {
    if (current != 'SAFE' && prev == 'SAFE') {
      addAlert(message);
    }
  }
/// ===============================
/// ADD SENSOR LOG (limit size)
/// ===============================
  void addLog(List<SensorLog> list, String payload) {
      list.insert(0, SensorLog(payload, DateTime.now()));

      /// Limit log size to prevent memory overflow
      if (list.length > 200) {
        list.removeLast();
      }
  }

  /// ===============================
  /// EMERGENCY COUNTDOWN
  /// ===============================
  void startEmergencyCountdown() {

    countdown = 10;
    emergencyDialogShown = true;

    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (_) => StatefulBuilder(
        builder: (context, setStateDialog) {

          emergencyTimer?.cancel();

          emergencyTimer = Timer.periodic(
            const Duration(seconds: 1),
            (timer) {

              if (countdown == 0) {

                timer.cancel();

                mqttService.publishEmergency("ON");

                Navigator.pop(context);

                setState(() {
                  emergencyTriggered = true;
                  emergencyDialogShown = false;
                });

                return;
              }

              setStateDialog(() {
                countdown--;
              });

            },
          );

          return AlertDialog(
            title: const Text("🚨 Possible Accident"),
            content: Text(
              "Sending emergency alert in $countdown seconds",
            ),
            actions: [

              /// Cancel emergency trigger
              TextButton(
                onPressed: () {

                  emergencyTimer?.cancel();

                  mqttService.publishEmergency("OFF");

                  Navigator.pop(context);
                  emergencyDialogShown = false;

                },
                child: const Text("CANCEL"),
              ),

            ],
          );
        },
      ),
    );
  }

  /// ===============================
  /// SHOW EMERGENCY ALERT
  /// ===============================
  void showEmergencyDialog() {

    showDialog(
      context: context,
      builder: (_) => AlertDialog(
        title: const Text("🚨 EMERGENCY"),
        content: const Text(
          "Emergency contact will be notified",
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text("OK"),
          ),
        ],
      ),
    );

  }

  void openLogPage(String title, List<SensorLog> logs) {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (_) => SensorDetailPage(title: title, logs: logs),
      ),
    );
  }

  // ===== UI =====
  Widget buildConnectionStatus() {
  return Text(
    espStatus,
    style: TextStyle(
      color: espStatus.contains('Disconnected')
          ? Colors.red
          : Colors.green,
    ),
  );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Vehicle Safety Monitor'),
        actions: [
          IconButton(
            icon: const Icon(Icons.history),
            onPressed: () {
              Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (_) => AlertHistoryPage(alerts: alertHistory),
                ),
              );
            },
          ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: ListView(
          children: [
            
            Center(child: buildConnectionStatus()),

            const SizedBox(height: 20),

            StatusCard(
              title: 'Helmet Status',
              status: helmetStatus,
              safe: Colors.green,
              warning: Colors.red,
              onTap: () {
                Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (_) => SensorDetailPage(
                      title: 'Force Sensor Monitor',
                      logs: helmetLogs,
                    ),
                  ),
                );
              },
            ),

            StatusCard(
              title: 'Alcohol Detection',
              status: alcoholStatus,
              safe: Colors.green,
              warning: Colors.red,
              onTap: () => openLogPage("Alcohol Sensor Monitor", alcoholLogs),
            ),

            StatusCard(
              title: 'Front Collision',
              status: warningName(frontCollision),
              safe: Colors.green,
              warning: Colors.red,
              onTap: () => openLogPage("Front Collision Sensor", frontLogs),
            ),

            StatusCard(
              title: 'Rear Collision',
              status: rearCollision,
              safe: Colors.green,
              warning: Colors.orange,
              onTap: () => openLogPage("Rear Collision Sensor", rearLogs),
            ),

            StatusCard(
              title: 'Emergency Status',
              status: emergencyTriggered ? 'EMERGENCY' : 'SAFE',
              safe: Colors.green,
              warning: Colors.red,
              
              onTap: () {
                Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (_) => SensorDetailPage(
                      title: 'Emergency Events',
                      logs: const [], // Emergency 是事件，没有 raw sensor
                    ),
                  ),
                );
              },
            ),

            const SizedBox(height: 20),

            const Text('Speaker Volume'),
            Slider(
              min: 0,
              max: 30,
              divisions: 30,
              value: volume,
              onChanged: (value) {
                setState(() {
                  volume = value;
                });
                mqttService.setVolume(value.toInt());
              },
            ),
          ],
        ),
      ),
    );
  }
}







