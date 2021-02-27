

#include "mbedtls/aes.h"
#include "ESPAsyncWebServer.h"

#include "esp_camera.h"
#include <WiFi.h>

#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems

const char* ssid = "";
const char* password = "";

AsyncWebServer server(80);


#define CAMERA_MODEL_AI_THINKER

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


bool initCamera() {

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
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t result = esp_camera_init(&config);

  if (result != ESP_OK) {
    return false;
  }

  return true;
}


void* getCapture() {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;


}

void printHex(uint8_t num) {
  char hexCar[2];
  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
}

void setup() {
  Serial.begin(115200);

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  if (!initCamera()) {
    Serial.printf("Failed to initialize camera...");
    return;
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());

  server.on("/picture", HTTP_GET, [](AsyncWebServerRequest * request) {
    camera_fb_t * fb = NULL;
    size_t _jpg_buf_len = 0;
    size_t length = 0;
    char * key = "abcdefghijklmnop";

    mbedtls_aes_context aes;
    mbedtls_aes_init( &aes );


    fb = esp_camera_fb_get();

    _jpg_buf_len = fb->len;


    length = _jpg_buf_len;
    while (length % 16 != 0) {
      length++;
    }

    mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );

    uint8_t arr1[512];
    uint8_t arr2[512];
    uint8_t arr3[512];


    for (int i = 0; i < length; i += 512) {
      for (int j = 0; j < 512; j++) {
        arr1[j] = fb->buf[i + j];
      }
      uint8_t iv[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      int x =   mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, 512, iv , arr1, arr2);
      for (int k = 0; k < 512; k++) {
        fb->buf[i + k] = arr2[k];
      }
    }
    mbedtls_aes_free( &aes );

    Serial.printf("MJPG: %uB\n", length);

    request->send_P(200, "plain/text", (const uint8_t *)fb->buf, length);

    esp_camera_fb_return(fb);
  });

  server.begin();
}
void loop() {
}
