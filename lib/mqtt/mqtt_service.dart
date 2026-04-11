import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'mqtt_topics.dart';
import 'package:flutter/material.dart';

class MqttService {
  late MqttServerClient client;

  // callback to send data to UI
  Function(String topic, String payload)? onMessage;

  Future<void> connect() async {
    client = MqttServerClient(
      'broker.hivemq.com', //change here if private broker
      'flutter_${DateTime.now().millisecondsSinceEpoch}',
    );

    client.port = 1883;
    client.keepAlivePeriod = 20;
    /// Auto reconnect when disconnected
    client.autoReconnect = true;
    client.resubscribeOnAutoReconnect = true;

    client.logging(on: false);

    client.connectionMessage = MqttConnectMessage()
        .withClientIdentifier(client.clientIdentifier)
        .startClean()
        .withWillQos(MqttQos.atMostOnce);

    try {
      await client.connect();

      /// Subscribe topics
      client.subscribe(MqttTopics.espStatus, MqttQos.atMostOnce);
      client.subscribe(MqttTopics.alcoholStatus, MqttQos.atMostOnce);
      client.subscribe(MqttTopics.frontCollision, MqttQos.atMostOnce);
      client.subscribe(MqttTopics.rearCollision, MqttQos.atMostOnce);
      client.subscribe(MqttTopics.helmetStatus, MqttQos.atMostOnce);
      client.subscribe(MqttTopics.emergency, MqttQos.atMostOnce);

      client.subscribe(MqttTopics.alcoholRaw, MqttQos.atMostOnce);
      client.subscribe(MqttTopics.frontRaw, MqttQos.atMostOnce);
      client.subscribe(MqttTopics.rearRaw, MqttQos.atMostOnce);
      client.subscribe(MqttTopics.helmetRaw, MqttQos.atMostOnce);

      /// ===============================
      /// MQTT MESSAGE LISTENER
      /// ===============================
      client.updates!.listen((events) {
        final msg = events[0].payload as MqttPublishMessage;
        final topic = events[0].topic;

        /// Convert payload bytes to string
        final payload =
            MqttPublishPayload.bytesToStringAsString(msg.payload.message);

        // send back to UI
        onMessage?.call(topic, payload);
      });

    } catch (e) {
      debugPrint("MQTT connection failed: $e");
    }
  }

  // ===== SEND VOLUME TO ESP32 =====
  void setVolume(int volume) {

    final builder = MqttClientPayloadBuilder();
    builder.addString(volume.toString());

    client.publishMessage(
      MqttTopics.volume,
      MqttQos.atLeastOnce,
      builder.payload!,
    );
  }

  // ===== CLEANUP =====
  void dispose() {
    client.disconnect();
    //super.dispose();
  }

  // ===== EMERGENCY =====
  void publishEmergency(String value) {
    final builder = MqttClientPayloadBuilder();
    builder.addString(value);

    client.publishMessage(
      MqttTopics.emergency,
      MqttQos.atLeastOnce,
      builder.payload!,
    );
  } 
}
