import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'firebase_options.dart';

const deviceId = 'cam-01';

final flutterLocalNotificationsPlugin = FlutterLocalNotificationsPlugin();
late final FirebaseDatabase rtdb;

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(options: DefaultFirebaseOptions.currentPlatform);
  rtdb = FirebaseDatabase.instanceFor(
    app: Firebase.app(),
    databaseURL: '**********',
  );
  try {
    await FirebaseAuth.instance.signInAnonymously();
  } catch (_) {}

  await _initNotifications();
  runApp(const MyApp());
}

Future<void> _initNotifications() async {
  const android = AndroidInitializationSettings('@mipmap/ic_launcher');
  await flutterLocalNotificationsPlugin.initialize(
    const InitializationSettings(android: android),
  );
  if (Platform.isAndroid) {
    final androidPlugin = flutterLocalNotificationsPlugin
        .resolvePlatformSpecificImplementation<
          AndroidFlutterLocalNotificationsPlugin
        >();
    await androidPlugin?.requestNotificationsPermission();
  }
}

Future<void> showAlertNotification(String body) async {
  const androidDetails = AndroidNotificationDetails(
    'motion_channel',
    'Hareket',
    importance: Importance.max,
    priority: Priority.high,
    showWhen: true,
  );
  await flutterLocalNotificationsPlugin.show(
    1,
    'ESP32-CAM',
    body,
    const NotificationDetails(android: androidDetails),
  );
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});
  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  String? _ip;
  StreamSubscription<DatabaseEvent>? _eventSub;
  StreamSubscription<DatabaseEvent>? _ipSub;

  @override
  void initState() {
    super.initState();
    _ipSub = rtdb.ref('devices/$deviceId/status').onValue.listen((e) {
      String? ip;
      final v = e.snapshot.value;
      if (v is Map) {
        if (v['ip'] != null) {
          ip = v['ip'].toString();
        } else {
          for (final value in v.values) {
            if (value is Map && value['ip'] != null) {
              ip = value['ip'].toString();
              break;
            }
          }
        }
      } else if (v is String) {
        ip = v;
      }
      setState(() => _ip = ip);
    });
    _eventSub = rtdb
        .ref('devices/$deviceId/events')
        .limitToLast(1)
        .onChildAdded
        .listen((e) async {
          await showAlertNotification('Hareket algılandı');
          if (!mounted) return;
          final ip = _ip;
          if (ip != null && ip.isNotEmpty) {
            Navigator.of(context).push(
              MaterialPageRoute(
                builder: (_) => LiveView(url: 'http://$ip/stream'),
              ),
            );
          }
        });
  }

  @override
  void dispose() {
    _eventSub?.cancel();
    _ipSub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final ip = _ip;
    return MaterialApp(
      title: 'ESP32-CAM',
      home: Scaffold(
        appBar: AppBar(title: const Text('ESP32-CAM')),
        body: Center(
          child: (ip == null || ip.isEmpty)
              ? const Text('IP bekleniyor...')
              : Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text('Cihaz IP: $ip'),
                    const SizedBox(height: 12),
                    ElevatedButton(
                      onPressed: () {
                        Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (_) => LiveView(url: 'http://$ip/stream'),
                          ),
                        );
                      },
                      child: const Text('Canlı yayını aç'),
                    ),
                    const SizedBox(height: 24),
                    Expanded(
                      child: Card(
                        child: LivePreviewThumb(url: 'http://$ip/jpg'),
                      ),
                    ),
                  ],
                ),
        ),
      ),
    );
  }
}

class LivePreviewThumb extends StatefulWidget {
  final String url;
  const LivePreviewThumb({super.key, required this.url});
  @override
  State<LivePreviewThumb> createState() => _LivePreviewThumbState();
}

class _LivePreviewThumbState extends State<LivePreviewThumb> {
  Uint8List? _img;
  Timer? _tmr;
  @override
  void initState() {
    super.initState();
    _tmr = Timer.periodic(const Duration(seconds: 2), (_) => _load());
    _load();
  }

  Future<void> _load() async {
    try {
      final uri = Uri.parse(
        '${widget.url}?t=${DateTime.now().millisecondsSinceEpoch}',
      );
      final req = await HttpClient().getUrl(uri);
      final res = await req.close();
      final bytes = await consolidateHttpClientResponseBytes(res);
      if (mounted) setState(() => _img = bytes);
    } catch (_) {}
  }

  @override
  void dispose() {
    _tmr?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (_img == null) return const Center(child: CircularProgressIndicator());
    return Image.memory(_img!, fit: BoxFit.cover, gaplessPlayback: true);
  }
}

class LiveView extends StatefulWidget {
  final String url;
  const LiveView({super.key, required this.url});
  @override
  State<LiveView> createState() => _LiveViewState();
}

class _LiveViewState extends State<LiveView> {
  Uint8List? _frame;
  bool _running = false;

  @override
  void initState() {
    super.initState();
    _start();
  }

  @override
  void dispose() {
    _running = false;
    super.dispose();
  }

  Future<void> _start() async {
    _running = true;
    final client = HttpClient()..connectionTimeout = const Duration(seconds: 5);
    while (_running) {
      try {
        final req = await client.getUrl(Uri.parse(widget.url));
        final res = await req.close();
        final buffer = BytesBuilder(copy: false);
        await for (final chunk in res) {
          if (!_running) break;
          buffer.add(chunk);
          final data = buffer.toBytes();
          final soi = _indexOf(data, const [0xFF, 0xD8]);
          final eoi = _indexOf(data, const [
            0xFF,
            0xD9,
          ], start: (soi >= 0) ? soi + 2 : 0);
          if (soi >= 0 && eoi > soi) {
            final jpg = Uint8List.sublistView(data, soi, eoi + 2);
            if (mounted) setState(() => _frame = jpg);
            final remain = Uint8List.sublistView(data, eoi + 2);
            buffer.clear();
            buffer.add(remain);
          }
        }
      } catch (_) {
        await Future.delayed(const Duration(milliseconds: 600));
      }
    }
    client.close(force: true);
  }

  int _indexOf(Uint8List data, List<int> pat, {int start = 0}) {
    final plen = pat.length;
    for (int i = start; i <= data.length - plen; i++) {
      bool ok = true;
      for (int j = 0; j < plen; j++) {
        if (data[i + j] != pat[j]) {
          ok = false;
          break;
        }
      }
      if (ok) return i;
    }
    return -1;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Canlı Yayın')),
      body: Center(
        child: _frame == null
            ? const CircularProgressIndicator()
            : InteractiveViewer(
                child: Image.memory(
                  _frame!,
                  gaplessPlayback: true,
                  fit: BoxFit.contain,
                ),
              ),
      ),
    );
  }
}
