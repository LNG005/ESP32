//======================================== Including the libraries.
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include "esp_camera.h"
#include "wifiConfig.h"//moithem
//======================================== 

//======================================== CAMERA_MODEL_AI_THINKER GPIO.
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
//======================================== 
#define FLASH_LED_PIN 4
#define IRpin 13
//======================================== 
const char* ssid = "LNG";
const char* password = "55555444";
//======================================== 

//======================================== 
String myDeploymentID = "AKfycbxhX3pK3kLND4DmCzXZpiJzQbKbwvLWAzQ1IYfJHcN9fBwc7ThbC3KSE5IdvdDKm1vfKg";
String myMainFolderName = "ESP32-CAM";
//======================================== 

//======================================== 
unsigned long previousMillis = 0; 
const int Interval = 20000; //Gửi ảnh sau 20s
//======================================== 
bool LED_Flash_ON = true; //Đặt false để tắt đèn 

// Initialize WiFiClientSecure.
WiFiClientSecure client;

//________________________________________________________________________________ Test_Con()
// kiểm tra kết nối tới Google Script.
void Test_Con() {
  const char* host = "script.google.com";
  while(1) {
    Serial.println("-----------");
    Serial.println("Kiểm tra kết nối...");
    Serial.println("kết nối tới " + String(host));
  
    client.setInsecure();
  
    if (client.connect(host, 443)) {
      Serial.println("Kết nối thành công.");
      Serial.println("-----------");
      client.stop();
      break;
    } else {
      Serial.println("Kết nối tới " + String(host) + " Thất bại.");
      Serial.println("Đợi mốt chút để kết nối lại.");
      Serial.println("-----------");
      client.stop();
    }
    delay(1000);
  }
}
//________________________________________________________________________________ 

//________________________________________________________________________________ SendCapturedPhotos()
// Chụp và gửi ảnh tới Google Drive.
void SendCapturedPhotos() {
  const char* host = "script.google.com";
  Serial.println();
  Serial.println("-----------");
  Serial.println("kết nối tới " + String(host));
  
  client.setInsecure();

  //---------------------------------------- LED nháy 1 lần để báo hiệu bắt đầu.
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(100);
  digitalWrite(FLASH_LED_PIN, LOW);
  delay(100);
  //---------------------------------------- 

  //---------------------------------------- Kết nối và chụp ảnh.
  if (client.connect(host, 443)) {
    Serial.println("kết nối thành công.");
    
    if (LED_Flash_ON == true) {
      digitalWrite(FLASH_LED_PIN, HIGH);
      delay(100);
    }

    //.............................. Chụp ảnh.
    Serial.println();
    Serial.println("Chụp ảnh...");
    
    for (int i = 0; i <= 3; i++) {
      camera_fb_t * fb = NULL;
      fb = esp_camera_fb_get();
       if(!fb) {
          Serial.println("Camera chụp ảnh thất bại.");
          Serial.println("Đang khởi động lại ESP32 CAM.");
          delay(1000);
          ESP.restart();
          return;
        } 
      esp_camera_fb_return(fb);
      delay(200);
    }
  
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if(!fb) {
      Serial.println("Camera chụp ảnh thất bại.");
      Serial.println("Đang khởi động lại ESP32 CAM.");
      delay(1000);
      ESP.restart();
      return;
    } 
  
    if (LED_Flash_ON == true) digitalWrite(FLASH_LED_PIN, LOW);
    
    Serial.println("Chụp ảnh thành công.");
    //.............................. 

    //.............................. Gửi ảnh tới Google Drive.
    Serial.println();
    Serial.println("Gửi ảnh tới Google Drive.");
    Serial.println("Size: " + String(fb->len) + "byte");
    
    String url = "/macros/s/" + myDeploymentID + "/exec?folder=" + myMainFolderName;

    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Transfer-Encoding: chunked");
    client.println();

    int fbLen = fb->len;
    char *input = (char *)fb->buf;
    int chunkSize = 3 * 1000; //--> Để bội của 3.
    int chunkBase64Size = base64_enc_len(chunkSize);
    char output[chunkBase64Size + 1];

    Serial.println();
    int chunk = 0;
    for (int i = 0; i < fbLen; i += chunkSize) {
      int l = base64_encode(output, input, min(fbLen - i, chunkSize));
      client.print(l, HEX);
      client.print("\r\n");
      client.print(output);
      client.print("\r\n");
      delay(100);
      input += chunkSize;
      Serial.print(".");
      chunk++;
      if (chunk % 50 == 0) {
        Serial.println();
      }
    }
    client.print("0\r\n");
    client.print("\r\n");

    esp_camera_fb_return(fb);
    //.............................. 

    //.............................. Chờ phản hồi trả về.
    Serial.println("Chờ đợi phản hồi.");
    long int StartTime = millis();
    while (!client.available()) {
      Serial.print(".");
      delay(100);
      if ((StartTime + 10 * 1000) < millis()) {
        Serial.println();
        Serial.println("Không có phản hồi.");
        break;
      }
    }
    Serial.println();
    while (client.available()) {
      Serial.print(char(client.read()));
    }
    //.............................. 

    //.............................. LED nháy một lần để báo đã gửi ảnh thành công Google Drive.
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(500);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(500);
    //.............................. 
  }
  else {
    Serial.println("Kết nối tới " + String(host) + " thất bại.");
    
    //.............................. LED nháy 2 lần để báo kết nối không thành công.
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(500);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(500);
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(500);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(500);
    //.............................. 
  }
  //---------------------------------------- 

  Serial.println("-----------");

  client.stop();
}
//________________________________________________________________________________ 

//________________________________________________________________________________ VOID SETUP()
void setup() {
  pinMode(IRpin,INPUT); //IRsensor

  pinMode(13, OUTPUT); //TRIG
  pinMode(15, INPUT);  //echo
  pinMode(14, OUTPUT); //loa

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  wifiConfig.begin(); //moithem
  Serial.println();
  delay(1000);

  pinMode(FLASH_LED_PIN, OUTPUT);
  
  // Đặt esp 32 vào station mode.
  Serial.println();
  Serial.println("Đặt esp 32 vào chế độ chờ.");
  WiFi.mode(WIFI_STA);

  //---------------------------------------- Kết nối esp32 với wifi nhà và trả về thông tin.
  Serial.println();
  Serial.print("Kết nối tới : ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  // Đặt thời gian chờ kết nối = 20 s.
  // Nếu trong 20s không kết nối được thì restart esp.
  int connecting_process_timed_out = 20; //--> 20 = 20 giây.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(250);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(250);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      Serial.println();
      Serial.print("Không thể kết nối tới ");
      Serial.println(ssid);
      Serial.println("Khởi động lại ESP32 CAM.");
      delay(1000);
      ESP.restart();
    }
  }

  digitalWrite(FLASH_LED_PIN, LOW);
  
  Serial.println();
  Serial.print("Kết nối thành công tới ");
  Serial.println(ssid);
  //Serial.print("ESP32-CAM IP: ");
  //Serial.println(WiFi.localIP());
  //---------------------------------------- 

  //---------------------------------------- Setting ESP32 CAM.
  Serial.println();
  Serial.println("Thiết lập ESP32 CAM...");
  
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
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Khởi tạo khung hình.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 số càng thấp chất lượng càng cao.
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 8;  //0-63 số càng thấp chất lượng càng cao.
    config.fb_count = 1;
  }
  
  // Khởi tạo cam
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Khởi tạo camera không thành công, lỗi 0x%x", err);
    Serial.println();
    Serial.println("Khởi động lại ESP32 CAM.");
    delay(1000);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();

  // Độ phân giải:
  // -UXGA   = 1600 x 1200 pixels
  // -SXGA   = 1280 x 1024 pixels
  // -XGA    = 1024 x 768  pixels
  // -SVGA   = 800 x 600   pixels
  // -VGA    = 640 x 480   pixels
  // -CIF    = 352 x 288   pixels
  // -QVGA   = 320 x 240   pixels
  // -HQVGA  = 240 x 160   pixels
  // -QQVGA  = 160 x 120   pixels
  s->set_framesize(s, FRAMESIZE_SXGA);  //--> UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA

  Serial.println("Cài đặt camera thành công.");
  Serial.println();

  delay(1000);

  Test_Con();

  Serial.println();
  Serial.println("ESP32-CAM captures HEHEHE Luong day!");
  Serial.println();
  delay(1000);
}
//________________________________________________________________________________ 

//________________________________________________________________________________ 
/*void loop() {
  //---------------------------------------- Check lại biến Interval để set thời gian chụp
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= Interval) {
    previousMillis = currentMillis;

    SendCapturedPhotos();
  }
  //---------------------------------------- 
}*/

//________________________________________________________________________________ 
//---------------------------------------- IR
/*void loop() { 
  int state = digitalRead(IRpin);
  if (state ==LOW){
  SendCapturedPhotos();
  delay(790);
  }
}*/
//________________________________________________________________________________ 

//________________________________________________________________________________ 
//---------------------------------------- SieuAM.
long ultrasound_distance_simple() {
   long duration, distance;
   digitalWrite(13,LOW);
   delayMicroseconds(2);
   digitalWrite(13, HIGH);
   delayMicroseconds(10);
   digitalWrite(13, LOW);
   duration = pulseIn(15, HIGH);
   distance = duration/58;
   return distance;
}

void loop() {
    wifiConfig.run();
    if (ultrasound_distance_simple() > 8) {
      Serial.print("Khoảng cách ");
      Serial.print(ultrasound_distance_simple());
      Serial.println(" cm");
    }
    if (ultrasound_distance_simple() <= 8) {
      Serial.print("Có vật cản ");
      Serial.print(ultrasound_distance_simple());
      Serial.println(" cm");
      tone(14,440,1000);
      SendCapturedPhotos();
      delay(790);
    }
  delay(500);
}
//________________________________________________________________________________ 
