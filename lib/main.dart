// Import Flutter's material design package (UI components like buttons, app bars, etc.)
import 'package:flutter/material.dart';

// Import the custom page where MQTT logic/UI is implemented
import 'pages/mqtt_page.dart';

void main() {
  // runApp starts the app and loads the root widget (MyApp)
  runApp(const MyApp());
}

/// ===============================
/// MAIN APP
/// ===============================
class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    // MaterialApp is the root widget that sets up app design, navigation, theme, etc.
    return const MaterialApp(
      debugShowCheckedModeBanner: false,
      home: MqttPage(), // This is the first screen shown when app starts
    );
  }
}