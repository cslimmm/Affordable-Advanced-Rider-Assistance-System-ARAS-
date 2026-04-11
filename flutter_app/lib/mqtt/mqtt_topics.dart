/// ===============================
/// MQTT TOPIC STRUCTURE
/// ===============================
class MqttTopics {
  /// Company / product prefix (never changes in production)
  static const String product = 'mycompany/vehiclesafety';

  /// Customer ID
  /// In production, each customer should have a unique ID
  static const String customerId = 'demo_customer';

  /// Device ID
  /// One device ID per vehicle / helmet
  static const String deviceId = 'vehicle_001';

  /// Base topic: product/customer/device
  static String get base =>
      '$product/$customerId/$deviceId';

  // ===== STATUS =====
  static String get espStatus => '$base/status/esp';
  static String get alcoholStatus => '$base/status/alcohol';
  static String get frontCollision => '$base/status/collision/front';
  static String get rearCollision => '$base/status/collision/rear';
  static String get helmetStatus => '$base/status/helmet';
  static String get emergency => '$base/status/emergency';

  // ===== RAW SENSOR DATA=====
  static String get alcoholRaw => '$base/raw/alcohol';
  static String get frontRaw => '$base/raw/front';
  static String get rearRaw => '$base/raw/rear';
  static String get helmetRaw => '$base/raw/helmet';

  // ===== CONTROL =====
  static String get volume => '$base/control/volume';
}