// ESP-32D용
// cds 값을 읽고 led 표시하는 아두이노 코드

// 1. 사용할 핀 번호 정의
#define CDS_SENSOR_PIN 34 // 조도 센서 핀
#define LED1_PIN 25       // 첫 번째 LED 핀
#define LED2_PIN 26       // 두 번째 LED 핀

// --- 시간 제어를 위한 전역 변수 ---
// LED 점멸을 위한 시간 변수
unsigned long previousLedMillis = 0;
const long ledInterval = 500; // LED 점멸 간격 (0.5초)

// CDS 센서 읽기를 위한 시간 변수
unsigned long previousCdsMillis = 0;
const long cdsInterval = 200; // CDS 센서 값 읽기 간격 (0.2초)

// LED 상태를 저장하는 변수
int led1State = LOW;

void setup()
{
    Serial.begin(115200);

    // LED 핀들을 출력 모드로 설정
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
}

void loop()
{
    // 현재 시간을 한 번만 가져와서 여러 작업에 사용
    unsigned long currentMillis = millis();

    // --- 작업 1: LED 번갈아 켜고 끄기 (비동기) ---
    // 현재 시간과 마지막으로 LED 상태를 변경한 시간의 차이가 ledInterval보다 크면
    if (currentMillis - previousLedMillis >= ledInterval) {
        // 마지막 상태 변경 시간을 현재 시간으로 업데이트
        previousLedMillis = currentMillis;

        // LED 상태를 반전시킴 (LOW -> HIGH, HIGH -> LOW)
        if (led1State == LOW) {
            led1State = HIGH;
        } else {
            led1State = LOW;
        }

        // 두 LED의 상태를 서로 반대로 설정
        digitalWrite(LED1_PIN, led1State);
        digitalWrite(LED2_PIN, !led1State); // '!'는 논리 NOT 연산자로, HIGH는 LOW로, LOW는 HIGH로 바꿈
    }

    // --- 작업 2: CDS 센서 값 읽기 (비동기) ---
    // 현재 시간과 마지막으로 센서를 읽은 시간의 차이가 cdsInterval보다 크면
    if (currentMillis - previousCdsMillis >= cdsInterval) {
        // 마지막 센서 읽기 시간을 현재 시간으로 업데이트
        previousCdsMillis = currentMillis;

        // 시리얼 플로터를 위해 숫자 값만 전송합니다.
        Serial.println(analogRead(CDS_SENSOR_PIN));
    }
}