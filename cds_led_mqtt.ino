// ESP-32D용
// cds 값을 읽고 led를 표시, mqtt로 cds값 전송하는 아두이노 코드
// 아두이노 IDE에서 라이브러리 매니저 사용 (이미 설치되었다고 가정)

#include <WiFi.h>
#include <PubSubClient.h>

// -------------------- 1. 사용자 설정 영역 --------------------
// 와이파이 정보
const char* ssid = "kiptime";        // 사용자의 와이파이 SSID
const char* password = "12345678";    // 사용자의 와이파이 비밀번호

// MQTT 브로커 정보 (라즈베리파이)
const char* mqtt_server = "192.168.0.7"; // 라즈베리파이의 IP 주소
const char* cds_topic = "esp32/cds";     // CDS 센서 값을 보낼 토픽

// 핀 정의 (const int 사용)
const int CDS_SENSOR_PIN = 34;
const int LED1_PIN       = 25;
const int LED2_PIN       = 26;
// -------------------- 설정 영역 끝 --------------------

WiFiClient espClient;
PubSubClient client(espClient);

// -------------------- 시간 제어를 위한 전역 변수 --------------------
unsigned long previousLedMillis  = 0;
const long ledInterval           = 500; // LED 점멸 간격 (0.5초)

unsigned long previousCdsMillis  = 0;
const long cdsInterval           = 200; // CDS 센서 값 읽기/플로터 출력 간격 (0.2초)

unsigned long previousMqttMillis = 0;
const long mqttInterval          = 2000; // MQTT 전송 간격 (2초)

// 상태 저장을 위한 변수
int led1State  = LOW;
int lastCdsValue = 0; // 마지막으로 읽은 CDS 값을 저장할 변수

// -------------------- 와이파이 및 MQTT 연결 함수 --------------------

void setup_wifi()
{
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // 클라이언트 ID 생성
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    // MQTT 연결 시도
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000); // 5초 대기 후 재시도
    }
  }
}

// -------------------- setup() 및 loop() --------------------

void setup()
{
  Serial.begin(115200);

  // 핀 모드 설정
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  // 와이파이 연결
  setup_wifi();
  
  // MQTT 서버 설정
  client.setServer(mqtt_server, 1883);
}

void loop()
{
  // MQTT 연결 관리
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // MQTT 클라이언트의 메시지 처리

  // 현재 시간을 한 번만 가져옴
  unsigned long currentMillis = millis();

  // -------------------- 작업 1: LED 번갈아 켜고 끄기 (비동기) --------------------
  if (currentMillis - previousLedMillis >= ledInterval) {
    previousLedMillis = currentMillis;
    
    led1State = !led1State; // 상태 반전
    
    digitalWrite(LED1_PIN, led1State);
    digitalWrite(LED2_PIN, !led1State); // LED1과 반대로 켜짐
  }

  // -------------------- 작업 2: CDS 센서 값 읽고 시리얼 플로터로 출력 (비동기) --------------------
  if (currentMillis - previousCdsMillis >= cdsInterval) {
    previousCdsMillis = currentMillis;
    
    lastCdsValue = analogRead(CDS_SENSOR_PIN); // 값을 읽어서 변수에 저장
    
    // 시리얼 플로터를 위해 숫자만 출력
    Serial.println(lastCdsValue); 
  }

  // -------------------- 작업 3: MQTT로 CDS 값 전송하기 (비동기) --------------------
  if (currentMillis - previousMqttMillis >= mqttInterval) {
    previousMqttMillis = currentMillis;
    
    // 작업 2에서 가장 최근에 읽은 값을 문자열로 변환하여 전송
    char msg[8];
    snprintf(msg, 8, "%d", lastCdsValue);
    
    client.publish(cds_topic, msg);
    
    // MQTT 전송 확인 메시지를 시리얼 모니터에 출력하면 플로터가 깨지므로 생략합니다.
    // (디버깅이 필요하면 Serial.println()을 잠시 주석 처리하고 아래 라인을 활성화하세요)
    // Serial.printf("MQTT Published: %s\n", msg);
  }
}