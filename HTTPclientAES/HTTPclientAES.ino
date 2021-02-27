#include <WiFi.h>
#include <HTTPClient.h>
#include "mbedtls/aes.h"


#include "esp_http_server.h"


const char* ssid = "";
const char* password = "";


#define INPUT_BUFFER_LIMIT (128 + 1)

#define PART_BOUNDARY "123456789000000000000987654321"

const char* serverNameCam = "http://192.168.1.5/picture";


static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

unsigned long previousMillis = 0;
const long interval = 500;

httpd_handle_t stream_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t *req) {
  esp_err_t res = ESP_OK;
  char * part_buf[64];

  char * key = "abcdefghijklmnop";
  mbedtls_aes_context aes;
  mbedtls_aes_init( &aes );


  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }
  while (true) {

    String _jpg_buf = httpGETRequest(serverNameCam);
    size_t _jpg_buf_len = _jpg_buf.length();

    mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );

    uint8_t arr1[512];
    uint8_t arr2[512];

    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }

    for (int i = 0; i < _jpg_buf_len; i += 512) {
      for (int j = 0; j < 512; j++) {

        arr1[j] = _jpg_buf[i + j];
      }
      uint8_t iv[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 512, iv , arr1, arr2);

      if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, (const char *)arr2, 512);
      }
    }


    mbedtls_aes_free( &aes );

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (res != ESP_OK) {
      break;
    }
    Serial.printf("MJPG: %uB\n", _jpg_buf_len);
  }
  return res;
}

static esp_err_t hexcodes_handler(httpd_req_t *req) {
  esp_err_t res = ESP_OK;

  char * key = "abcdefghijklmnop";
  mbedtls_aes_context aes;
  mbedtls_aes_init( &aes );


  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  String _jpg_buf = httpGETRequest(serverNameCam);
  size_t _jpg_buf_len = _jpg_buf.length();

  for (int i = 0; i < _jpg_buf_len; i += 512) {
    char json_response[1024];
    char * p = json_response;
    for (int j = 0; j < 512; j++) {
      p += sprintf(p, "%02X", _jpg_buf[i + j] );
    }
    httpd_resp_send_chunk(req, json_response, strlen(json_response));
  }



  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );

  uint8_t arr1[512];
  uint8_t arr2[512];


  httpd_resp_send_chunk(req, "\n\n", 2);

  char * part_buf[64];
  size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
  httpd_resp_send_chunk(req, (const char *)part_buf, hlen);


  
  for (int i = 0; i < _jpg_buf_len; i += 512) {
    for (int j = 0; j < 512; j++) {
      arr1[j] = _jpg_buf[i + j];
    }

    uint8_t iv[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 512, iv , arr1, arr2);

    res = httpd_resp_send_chunk(req, (const char *)arr2, 512);
  }

  Serial.println();

  res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
  mbedtls_aes_free( &aes );

  return res;
}

void startStreamServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t hexcodes_uri = {
    .uri       = "/hexcodes",
    .method    = HTTP_GET,
    .handler   = hexcodes_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
    httpd_register_uri_handler(stream_httpd, &hexcodes_uri);
  }
}



String httpGETRequest(const char* serverName) {
  HTTPClient http;

  // Your IP address with path or Domain name with URL path
  http.begin(serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "--";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void printHex(uint8_t num) {
  char hexCar[2];
  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  startStreamServer();
}

void loop() {
}
