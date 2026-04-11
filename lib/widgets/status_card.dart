import 'package:flutter/material.dart';

/// ===============================
/// STATUS CARD UI
/// ===============================
class StatusCard extends StatelessWidget {
  final String title;     // e.g. "Helmet Status"
  final String status;    // e.g. "SAFE" or "DANGER"
  final Color safe;       // color when safe
  final Color warning;    // color when warning
  final VoidCallback onTap; // what happens when user taps

  const StatusCard({
    super.key,
    required this.title,
    required this.status,
    required this.safe,
    required this.warning,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    bool isWarning;

    if (title == 'Helmet Status') {
      isWarning = status != 'HELMET DETECTED';
    } else {
      isWarning = status != 'SAFE';
    }

    return InkWell(
      onTap: onTap,
      child: Container(
        margin: const EdgeInsets.only(bottom: 12),
        padding: const EdgeInsets.all(14),
        decoration: BoxDecoration(
          color: isWarning ? warning : safe,
          borderRadius: BorderRadius.circular(8),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              title,
              style: const TextStyle(
                color: Colors.white,
                fontSize: 20,
                fontWeight: FontWeight.w600,
              ),
            ),
            const SizedBox(height: 6),
            Text(
              status,
              style: const TextStyle(
                color: Colors.white,
                fontSize: 14,
                fontWeight: FontWeight.bold,
                letterSpacing: 1.2,
              ),
            ),
          ],
        ),
      ),
    );
  }
}