#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "**********";
const char* WIFI_PASS = "**********";
const char* DEVICE_ID = "cam-01";

const char* FIREBASE_HOST = "**********";
const char* WEB_API_KEY = "**********";

WebServer server(80);

String g_idToken, g_refreshToken;
unsigned long g_tokenExpiresAt = 0;
bool g_cam_ok = false;
volatile bool g_streaming = false;

bool httpsPostJson(const String& url, const String& body, String& resp) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  resp = http.getString();
  Serial.printf("POST %s -> %d\n", url.c_str(), code);
  if (resp.length()) Serial.println(resp);
  http.end();
  return code >= 200 && code < 300;
}

bool httpsPutJson(const String& url, const String& body, String& resp) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/json");
  int code = http.PUT(body);
  resp = http.getString();
  Serial.printf("PUT %s -> %d\n", url.c_str(), code);
  if (resp.length()) Serial.println(resp);
  http.end();
  return code >= 200 && code < 300;
}

bool httpsPostForm(const String& url, const String& form, String& resp) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int code = http.POST(form);
  resp = http.getString();
  Serial.printf("POST %s -> %d\n", url.c_str(), code);
  if (resp.length()) Serial.println(resp);
  http.end();
  return code >= 200 && code < 300;
}

bool firebaseAnonymousSignIn() {
  String url = "https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=" + String(WEB_API_KEY);
  String body = "{\"returnSecureToken\":true}";
  String resp;
  if (!httpsPostJson(url, body, resp)) return false;
  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, resp)) return false;
  g_idToken = doc["idToken"] | "";
  g_refreshToken = doc["refreshToken"] | "";
  long expires = atol((doc["expiresIn"] | "3600"));
  g_tokenExpiresAt = millis() + (unsigned long)((expires - 60) * 1000UL);
  Serial.println("Anon sign-in OK");
  return g_idToken.length() > 0;
}

bool refreshIdToken() {
  if (g_refreshToken.length() == 0) return firebaseAnonymousSignIn();
  String url = "https://securetoken.googleapis.com/v1/token?key=" + String(WEB_API_KEY);
  String form = "grant_type=refresh_token&refresh_token=" + g_refreshToken;
  String resp;
  if (!httpsPostForm(url, form, resp)) return false;
  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, resp)) return false;
  g_idToken = doc["id_token"] | "";
  g_refreshToken = doc["refresh_token"] | "";
  long expires = atol((doc["expires_in"] | "3600"));
  g_tokenExpiresAt = millis() + (unsigned long)((expires - 60) * 1000UL);
  Serial.println("Token refresh OK");
  return g_idToken.length() > 0;
}

bool ensureAuth() {
  unsigned long now = millis();
  if (g_idToken.length() == 0) return firebaseAnonymousSignIn();
  if (now + 5000 >= g_tokenExpiresAt) return refreshIdToken();
  return true;
}

void publishIpToFirebase() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!ensureAuth()) {
    Serial.println("Auth yok, IP gonderemiyorum");
    return;
  }

  String url = String("https://") + FIREBASE_HOST + "/devices/" + DEVICE_ID + "/status.json?auth=" + g_idToken;

  DynamicJsonDocument doc(256);
  doc["ip"] = WiFi.localIP().toString();
  doc["ts"] = (uint32_t)(millis() / 1000);
  String body;
  serializeJson(doc, body);

  String resp;
  if (httpsPutJson(url, body, resp)) {
    Serial.println("IP RTDB'ye yazildi: " + WiFi.localIP().toString());
  } else {
    Serial.println("IP yazma hatasi");
  }
}
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

bool initCamera(framesize_t size, pixformat_t fmt) {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = fmt;
  if (psramFound()) {
    config.frame_size = size;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }
  esp_err_t err = esp_camera_init(&config);
  return (err == ESP_OK);
}

void handleRoot() {
  server.send(200, "text/plain", "ESP32-CAM OK");
}

void handleJpg() {
  if (!g_cam_ok) {
    server.send(500, "text/plain", "camera not ready");
    return;
  }
  sensor_t* s = esp_camera_sensor_get();
  s->set_pixformat(s, PIXFORMAT_JPEG);
  s->set_framesize(s, FRAMESIZE_VGA);
  s->set_quality(s, 15);

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb || fb->format != PIXFORMAT_JPEG) {
    if (fb) esp_camera_fb_return(fb);
    server.send(500, "text/plain", "capture failed");
    return;
  }
  server.setContentLength(fb->len);
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  client.write(fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

const char* BOUNDARY = "frame";
void handleStream() {
  if (!g_cam_ok) {
    server.send(500, "text/plain", "camera not ready");
    return;
  }
  g_streaming = true;
  WiFiClient client = server.client();
  String hdr = "HTTP/1.1 200 OK\r\n"
               "Content-Type: multipart/x-mixed-replace; boundary="
               + String(BOUNDARY) + "\r\n"
                                    "Connection: close\r\n\r\n";
  client.print(hdr);

  sensor_t* s = esp_camera_sensor_get();
  s->set_pixformat(s, PIXFORMAT_JPEG);
  s->set_framesize(s, FRAMESIZE_VGA);
  s->set_quality(s, 15);

  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) break;
    client.printf("--%s\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", BOUNDARY, fb->len);
    client.write(fb->buf, fb->len);
    client.print("\r\n");
    esp_camera_fb_return(fb);
    delay(10);
  }
  g_streaming = false;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(0, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("WiFi connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected, IP: " + WiFi.localIP().toString());
  if (!ensureAuth()) Serial.println("Auth basarisiz");
  publishIpToFirebase();
  if (initCamera(FRAMESIZE_VGA, PIXFORMAT_JPEG)) {
    g_cam_ok = true;
    Serial.println("Camera init OK");
  } else {
    g_cam_ok = false;
    Serial.println("Camera init FAILED (ama sunucu calisiyor, IP RTDB'ye yazildi)");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/jpg", HTTP_GET, handleJpg);
  server.on("/stream", HTTP_GET, handleStream);
  server.begin();
}

void loop() {
  server.handleClient();
  static unsigned long lastIp = 0;
  if (millis() - lastIp > 60000) {
    lastIp = millis();
    publishIpToFirebase();
  }
}
