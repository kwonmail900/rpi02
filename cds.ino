// ESP-32D용 cds 값을 읽는 아두이노 코드
/*
1. 아두이노 IDE 환경 설정
아두이노 IDE를 엽니다.

상단 메뉴에서 파일(File) > **환경설정(Preferences)**을 클릭합니다.

환경설정 창에서 추가적인 보드 매니저 URLs (Additional Board Manager URLs) 입력란을 찾습니다.

다음 URL을 복사하여 입력란에 붙여넣고 **확인(OK)**을 클릭합니다.

https://espressif.github.io/arduino-esp32/package_esp32_index.json 이미 다른 URL이 있다면 쉼표(,)로 구분하여 추가하면 됩니다.

2. 보드 패키지 설치 (Boards Manager)
상단 메뉴에서 툴(Tool) > 보드(Board) > **보드 매니저(Boards Manager...)**를 클릭하거나, 왼쪽 툴바의 보드 매니저 아이콘을 클릭합니다.

보드 매니저 검색창에 **esp32**를 입력합니다.

검색 결과로 나오는 **esp32 by Espressif Systems**를 찾아 설치(Install) 버튼을 클릭합니다.

3. ESP32 보드 선택
설치가 완료되면, 다시 상단 메뉴 툴(Tool) > 보드(Board) > ESP32 Arduino로 이동합니다.

여기서 여러분의 ESP32 보드 모델에 가장 적합한 보드를 선택합니다. 일반적으로는 **ESP32 Dev Module**을 선택하면 대부분의 ESP32 보드에서 작동합니다.

마지막으로, 툴(Tool) > **포트(Port)**에서 ESP32 보드가 연결된 COM 포트를 선택하면 준비가 완료됩니다. 포트가 보이지 않는 경우, 보드에 사용된 USB-UART 칩(예: CP2102 또는 CH340)의 드라이버를 별도로 설치해야 할 수 있습니다.
*/
 #define CDS_SENSOR_PIN 34


void setup()
{
  Serial.begin(115200);
}


void loop()
{
  Serial.println(analogRead(CDS_SENSOR_PIN)); // 34
  delay(100);
}
