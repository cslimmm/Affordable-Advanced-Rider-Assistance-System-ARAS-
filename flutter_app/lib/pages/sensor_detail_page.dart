// Used for Timer (auto refresh)
import 'dart:async';

// Flutter UI package
import 'package:flutter/material.dart';

import '../models/sensor_log.dart';

/// ==== SENSOR DETAIL PAGE (real time update) ====
class SensorDetailPage extends StatefulWidget {
  final String title; // Page title 
  final List<SensorLog> logs; // List of sensor logs passed into this page

  const SensorDetailPage({
    super.key,
    required this.title,
    required this.logs,
  });

  @override
  State<SensorDetailPage> createState() => _SensorDetailPageState();
}

class _SensorDetailPageState extends State<SensorDetailPage> {
  late Timer _timer; // Timer to refresh UI periodically

  @override
  void initState() {
    super.initState();

    // Runs every 500ms (0.5 second)
    _timer = Timer.periodic(
      const Duration(milliseconds: 500),

      // Every tick → rebuild UI
      (_) => mounted ? setState(() {}) : null,
    );
  }

  @override
  void dispose() {
    // Important: stop timer when page is closed
    _timer.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final logs = widget.logs; // Access logs from widget

    return Scaffold(
      appBar: AppBar(title: Text(widget.title)),
      body: logs.isEmpty
          // If no data → show message
          ? const Center(child: Text('No sensor data yet'))

          // If data exists → show list
          : ListView.builder(
              reverse: true, // newest data at top
              itemCount: logs.length,
              itemBuilder: (_, index) {
                final log = logs[index];
                return Padding(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                  child: Text(
                    '${log.time.toIso8601String()} -> ${log.message}',
                    style: const TextStyle(
                      fontFamily: 'monospace',
                      fontSize: 13,
                    ),
                  ),
                );
              },
            ),
    );
  }
}
