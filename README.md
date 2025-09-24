📷 ESP32-CAM Canlı Görüntü Aktarım Sistemi

ESP32-CAM modülü ile gerçek zamanlı video akışı ve hareket algılama sistemi. Firebase üzerinden IP paylaşımı ve Flutter mobil uygulaması ile uzaktan izleme.

---

🎯 Proje Özeti

Bu proje, ESP32-CAM modülünü kullanarak:

📹 Canlı video akışı (MJPEG stream)
📸 Anlık görüntü yakalama
🔔 Hareket algılama bildirimleri
🌐 Dinamik IP yönetimi (Firebase üzerinden)
📱 Mobil uygulama ile uzaktan erişim

---

🏗️ Sistem Mimarisi

┌──────────────┐     Firebase RTDB    ┌──────────────┐
│  ESP32-CAM   │──────────────────────►│ Flutter App  │
│   Kamera     │      IP Adresi        │   (Mobil)    │
│   Sunucu     │◄──────────────────────│   İzleyici   │
└──────────────┘    HTTP Stream        └──────────────┘

---

🛠️ Teknolojiler

Donanım Tarafı

- **ESP32-CAM (AI-Thinker modülü)
- **OV2640 2MP kamera sensörü
- **PSRAM desteği
- **WiFi bağlantısı

Yazılım Tarafı

- **Arduino Core (ESP32)
- **Firebase Realtime Database
- **Flutter/Dart (Mobil uygulama)
- **HTTP/MJPEG protokolleri

---

✨ Özellikler

ESP32-CAM Özellikleri

✅ Otomatik WiFi bağlantısı
✅ Firebase Anonymous Authentication
✅ Dinamik IP adresi yayınlama
✅ HTTP sunucu (3 endpoint)
✅ MJPEG video stream
✅ JPEG anlık görüntü
✅ Otomatik token yenileme

Flutter Uygulama Özellikleri

✅ Gerçek zamanlı IP takibi
✅ Canlı video izleme
✅ Önizleme görüntüsü (2 saniyede bir)
✅ Hareket algılama bildirimleri
✅ InteractiveViewer ile zoom/pan
✅ Otomatik yeniden bağlanma

---

📡 API Endpoints

- **ESP32-CAM üzerindeki HTTP sunucu:

--->EndpointMethodAçıklama/GETDurum kontrolü/jpgGETTek JPEG görüntü/streamGETMJPEG video stream

---

🚀 Kurulum

ESP32-CAM Kurulumu

Gerekli kütüphaneleri yükleyin:

- **ESP32 Board Package
- **ArduinoJson
- **esp_camera


Konfigürasyon (ESP32-Cam.ino):

- **cppconst char* WIFI_SSID = "wifi-adınız";
- **const char* WIFI_PASS = "wifi-şifreniz";
- **const char* FIREBASE_HOST = "proje-adı.firebaseio.com";
- **const char* WEB_API_KEY = "firebase-api-key";

Pin bağlantıları:

- **Kamera modülü dahili bağlantılı
- **GPIO 0: Flash LED kontrolü


Kodu yükleyin ve başlatın

Flutter Uygulama Kurulumu

Bağımlılıkları yükleyin:

- **yamldependencies:
  	flutter:
    	 sdk: flutter
  	firebase_core: ^latest
  	firebase_database: ^latest
  	firebase_auth: ^latest
  	flutter_local_notifications: ^latest

Firebase yapılandırması:

- **bashflutterfire configure

Android izinleri (AndroidManifest.xml):

- **xml<uses-permission android:name="android.permission.INTERNET"/>
- **<uses-permission android:name="android.permission.POST_NOTIFICATIONS"/>

Uygulamayı çalıştırın:

- **bashflutter run

---

📊 Firebase Veritabanı Yapısı

json{
  "devices": {
    "cam-01": {
      "status": {
        "ip": "192.168.1.100",
        "ts": 1234567890
      },
      "events": {
        "eventId": {
          "type": "motion",
          "timestamp": 1234567890
        }
      }
    }
  }
}

---

🎮 Kullanım

ESP32-CAM Başlatma

- **Cihaz WiFi'ye otomatik bağlanır
- **IP adresi Firebase'e yazılır
- **HTTP sunucu başlatılır
- **Kamera hazır duruma gelir

Mobil Uygulama Kullanımı

- **Uygulama açıldığında IP otomatik alınır
- **Önizleme görüntüsü gösterilir
- **"Canlı yayını aç" butonu ile stream başlar
- **Hareket algılandığında bildirim gelir

🔄 Video Stream Akışı

ESP32-CAM                Flutter App
    │                         │
    ├─ Capture Frame          │
    ├─ JPEG Encode           │
    ├─ MJPEG Boundary        │
    └─────HTTP Stream────────►│
                              ├─ Parse MJPEG
                              ├─ Extract Frame
                              └─ Display Image

---

🔐 Güvenlik

- **Firebase Anonymous Authentication
- **HTTPS üzerinden token işlemleri
- **Token otomatik yenileme (60 dakika)
- **SSL/TLS desteği (opsiyonel)

---

⚡ Performans Optimizasyonları

ESP32-CAM

- **Dual buffer kullanımı (PSRAM)
- **VGA çözünürlük (640x480)
- **JPEG kalite: 15 (dengelenmiş)
- **20MHz XCLK frekansı

Flutter App

- **Gapless playback
- **Stream buffer yönetimi
- **Otomatik yeniden bağlanma
- **Memory-efficient frame parsing

---

🐛 Sorun Giderme

Sorun<----->Çözüm

- **Kamera başlatılamadı<----->ESP32-CAM'i resetleyin, PSRAM kontrolü
- **Stream açılmıyor<----->IP adresini kontrol edin, firewall ayarları
- **Düşük FPS<----->WiFi sinyal gücü, JPEG kalitesini düşürün
- **Bildirim gelmiyor<----->Uygulama izinlerini kontrol edin
