import 'package:flutter/material.dart';
import '../models/alert_record.dart';


/// ===== ALERT HISTORY PAGE =====
class AlertHistoryPage extends StatelessWidget {
  final List<AlertRecord> alerts;

  const AlertHistoryPage({super.key, required this.alerts});

  @override
  Widget build(BuildContext context) {
  final reversedAlerts = alerts.reversed.toList();

    return Scaffold(
      appBar: AppBar(title: const Text('Alert History')),
      body: alerts.isEmpty
          ? const Center(child: Text('No alerts recorded'))
          : ListView.builder(
              itemCount: alerts.length,
              itemBuilder: (context, index) {
                final alert = reversedAlerts[index];
                return ListTile(
                  leading:
                      const Icon(Icons.warning, color: Colors.red),
                  title: Text(alert.type),
                  subtitle:
                      Text(alert.time.toLocal().toString()),
                );
              },
            ),
    );
  }
}  