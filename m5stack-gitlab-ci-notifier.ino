#include <M5Stack.h>
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define ASSERT(expr) \
{ \
  auto actual = expr; \
  if (!actual) { \
    M5.Lcd.printf("assertion failed: expected trueish, got \"%s\"\r\n", String(actual).c_str()); \
    M5.Lcd.printf("  on %s(%d): %s", __FILE__, __LINE__, #expr); \
    break; \
  } \
}

#define ASSERT_EQUAL(expr, expected) \
{ \
  auto actual = expr; \
  if (actual != expected) { \
    M5.Lcd.printf("assertion failed: expected \"%s\", got \"%s\"\r\n", #expected, String(actual).c_str()); \
    M5.Lcd.printf("  on %s(%d): %s", __FILE__, __LINE__, #expr); \
    break; \
  } \
}

int project_id;
String branch;
String private_token;

// the setup routine runs once when M5Stack starts up
void setup() {
  do {
    // Initialize the M5Stack object
    M5.begin();

    File file = SD.open("/config.json", "r");
    ASSERT(file);
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(file);
    ASSERT(json.success());
    file.close();

    const char* ssid = json["wifi"]["ssid"];
    ASSERT(ssid != NULL);
    const char* pass = json["wifi"]["password"];
    ASSERT(pass != NULL);
    private_token = json["gitlab"]["private_token"].as<String>();
    ASSERT(private_token);
    ASSERT(json["gitlab"]["project_id"].is<int>());
    project_id = json["gitlab"]["project_id"];
    branch = json["gitlab"]["branch"].as<String>();
    ASSERT(branch);
    
    M5.Lcd.print("connecting to ");
    M5.Lcd.print(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      M5.Lcd.print(".");
    }
    M5.Lcd.println("");

    M5.Lcd.print("connected as ");
    M5.Lcd.println(WiFi.localIP());

    return;
  } while (false);

  while (true) {
    delay(5000);
  }
}

const char* cert = "-----BEGIN CERTIFICATE-----\n" \
  "MIIF2DCCA8CgAwIBAgIQTKr5yttjb+Af907YWwOGnTANBgkqhkiG9w0BAQwFADCB\n" \
  "hTELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G\n" \
  "A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxKzApBgNV\n" \
  "BAMTIkNPTU9ETyBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAwMTE5\n" \
  "MDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBhTELMAkGA1UEBhMCR0IxGzAZBgNVBAgT\n" \
  "EkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEaMBgGA1UEChMR\n" \
  "Q09NT0RPIENBIExpbWl0ZWQxKzApBgNVBAMTIkNPTU9ETyBSU0EgQ2VydGlmaWNh\n" \
  "dGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQCR\n" \
  "6FSS0gpWsawNJN3Fz0RndJkrN6N9I3AAcbxT38T6KhKPS38QVr2fcHK3YX/JSw8X\n" \
  "pz3jsARh7v8Rl8f0hj4K+j5c+ZPmNHrZFGvnnLOFoIJ6dq9xkNfs/Q36nGz637CC\n" \
  "9BR++b7Epi9Pf5l/tfxnQ3K9DADWietrLNPtj5gcFKt+5eNu/Nio5JIk2kNrYrhV\n" \
  "/erBvGy2i/MOjZrkm2xpmfh4SDBF1a3hDTxFYPwyllEnvGfDyi62a+pGx8cgoLEf\n" \
  "Zd5ICLqkTqnyg0Y3hOvozIFIQ2dOciqbXL1MGyiKXCJ7tKuY2e7gUYPDCUZObT6Z\n" \
  "+pUX2nwzV0E8jVHtC7ZcryxjGt9XyD+86V3Em69FmeKjWiS0uqlWPc9vqv9JWL7w\n" \
  "qP/0uK3pN/u6uPQLOvnoQ0IeidiEyxPx2bvhiWC4jChWrBQdnArncevPDt09qZah\n" \
  "SL0896+1DSJMwBGB7FY79tOi4lu3sgQiUpWAk2nojkxl8ZEDLXB0AuqLZxUpaVIC\n" \
  "u9ffUGpVRr+goyhhf3DQw6KqLCGqR84onAZFdr+CGCe01a60y1Dma/RMhnEw6abf\n" \
  "Fobg2P9A3fvQQoh/ozM6LlweQRGBY84YcWsr7KaKtzFcOmpH4MN5WdYgGq/yapiq\n" \
  "crxXStJLnbsQ/LBMQeXtHT1eKJ2czL+zUdqnR+WEUwIDAQABo0IwQDAdBgNVHQ4E\n" \
  "FgQUu69+Aj36pvE8hI6t7jiY7NkyMtQwDgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB\n" \
  "/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAArx1UaEt65Ru2yyTUEUAJNMnMvl\n" \
  "wFTPoCWOAvn9sKIN9SCYPBMtrFaisNZ+EZLpLrqeLppysb0ZRGxhNaKatBYSaVqM\n" \
  "4dc+pBroLwP0rmEdEBsqpIt6xf4FpuHA1sj+nq6PK7o9mfjYcwlYRm6mnPTXJ9OV\n" \
  "2jeDchzTc+CiR5kDOF3VSXkAKRzH7JsgHAckaVd4sjn8OoSgtZx8jb8uk2Intzna\n" \
  "FxiuvTwJaP+EmzzV1gsD41eeFPfR60/IvYcjt7ZJQ3mFXLrrkguhxuhoqEwWsRqZ\n" \
  "CuhTLJK7oQkYdQxlqHvLI7cawiiFwxv/0Cti76R7CZGYZ4wUAc1oBmpjIXUDgIiK\n" \
  "boHGhfKppC3n9KUkEEeDys30jXlYsQab5xoq2Z0B15R97QNKyvDb6KkBPvVWmcke\n" \
  "jkk9u+UJueBPSZI9FoJAzMxZxuY67RIuaTxslbH9qh17f4a+Hg4yRvv7E491f0yL\n" \
  "S0Zj/gA0QHDBw7mh3aZw4gSzQbzpgJHqZJx64SIDqZxubw5lT2yHh17zbqD5daWb\n" \
  "QOhTsiedSrnAdyGN/4fy3ryM7xfft0kL0fJuMAsaDk527RH89elWsn2/x20Kk4yl\n" \
  "0MC2Hb46TpSi125sC8KKfPog88Tk5c0NqMuRkrF8hey1FGlmDoLnzc7ILaZRfyHB\n" \
  "NVOFBkpdn627G190\n" \
  "-----END CERTIFICATE-----\n";


// the loop routine runs over and over again forever
void loop() {
  do {
    M5.Lcd.setTextColor(BLACK, WHITE);
    M5.Lcd.fillScreen(WHITE);
    M5.Lcd.setCursor(0, 0);
    
    IconLoading_begin();
    
    M5.Lcd.print("GET https://gitlab.com/projects/:id/pipelines...");
    String url = String("https://gitlab.com/api/v4/projects/") + project_id + "/pipelines?ref=" + branch + "&per_page=1";
    HTTPClient client;
    ASSERT(client.begin(url, cert));
    client.addHeader("PRIVATE-TOKEN", private_token);

    int status = client.GET();
    M5.Lcd.printf(" %d %s\n", status, client.errorToString(status).c_str());
    ASSERT_EQUAL(status, HTTP_CODE_OK);

    String json = client.getString();
    DynamicJsonBuffer buffer;
    const char *stat = buffer.parseArray(json)[0]["status"];
    ASSERT(stat != NULL);
    M5.Lcd.println(stat);
    
    IconLoading_end();
    if (strcmp(stat, "success") == 0) {
      IconOK_run();
    } else {
      IconNG_run();
    }

    delay(5 * 60 * 1000);
    return;
  } while (false);

  IconLoading_end();
  IconQuestion_run();
  delay(5 * 60 * 1000);
}

void IconLoading_update() {
  int tick = int(millis() / 40UL);
  M5.Lcd.fillCircle(160 + 0,  112 - 52, 8, dotColor(tick, 7));
  M5.Lcd.fillCircle(160 + 37, 112 - 37, 8, dotColor(tick, 6));
  M5.Lcd.fillCircle(160 + 52, 112 + 0,  8, dotColor(tick, 5));
  M5.Lcd.fillCircle(160 + 37, 112 + 37, 8, dotColor(tick, 4));
  M5.Lcd.fillCircle(160 + 0,  112 + 52, 8, dotColor(tick, 3));
  M5.Lcd.fillCircle(160 - 37, 112 + 37, 8, dotColor(tick, 2));
  M5.Lcd.fillCircle(160 - 52, 112 + 0,  8, dotColor(tick, 1));
  M5.Lcd.fillCircle(160 - 37, 112 - 37, 8, dotColor(tick, 0));
}

static uint16_t dotColor(int tick, int offset) {
  uint16_t v = (tick + offset * 4) % 32;
  return (v << 11) + (v << 6) + (v << 0);
}

TaskHandle_t task;
void IconLoading_begin() {
  xTaskCreatePinnedToCore(IconLoading_updateTask,
                          "IconLoading_updateTask",
                          1024,
                          nullptr,
                          2,
                          &task,
                          1);
}

void IconLoading_end() {
  vTaskDelete(task);
}

static void IconLoading_updateTask(void *parameters) {
  while (true) {
    IconLoading_update();
    delay(30);
  }
}

void IconOK_run() {
  M5.Lcd.fillCircle(160, 112, 62, GREEN);
  M5.Lcd.fillCircle(160, 112, 60, WHITE);
  for (int step = 0; step < 6; step++) {
    int x0 = 130, y0 = 105, dx = 4, dy = 5, r = 6, rx = 4, ry = 4;
    M5.Lcd.fillCircle(x0, y0, r, GREEN);
    M5.Lcd.fillTriangle(x0 + rx, y0 - ry, x0 - rx, y0 + ry, x0 + (dx * step) + rx, y0 + (dy * step) - ry, GREEN);
    M5.Lcd.fillTriangle(x0 - rx, y0 + ry, x0 + (dx * step) - rx, y0 + (dy * step) + ry, x0 + (dx * step) + rx, y0 + (dy * step) - ry, GREEN);
    M5.Lcd.fillCircle(x0 + (dx * step), y0 + (dy * step), r, GREEN);
    delay(30);
  }
  for (int step = 0; step < 8; step++) {
    int x0 = 150, y0 = 130, dx = 6, dy = -4, r = 6, rx = 4, ry = 4;
    M5.Lcd.fillCircle(x0, y0, r, GREEN);
    M5.Lcd.fillTriangle(x0 + rx, y0 - ry, x0 - rx, y0 + ry, x0 + (dx * step) + rx, y0 + (dy * step) - ry, GREEN);
    M5.Lcd.fillTriangle(x0 - rx, y0 + ry, x0 + (dx * step) - rx, y0 + (dy * step) + ry, x0 + (dx * step) + rx, y0 + (dy * step) - ry, GREEN);
    M5.Lcd.fillCircle(x0 + (dx * step), y0 + (dy * step), r, GREEN);
    delay(30);
  }
}

void IconNG_run() {
  M5.Lcd.fillCircle(160, 112, 62, RED);
  M5.Lcd.fillCircle(160, 112, 60, WHITE);
  for (int step = 0; step < 6; step++) {
    int x0 = 160, y0 = 72, dy = 12, r = 6;
    M5.Lcd.fillRoundRect(x0 - r, y0 - r, r * 2, (dy * step) + r, r, RED);
    delay(30);
  }
  M5.Lcd.fillCircle(160, 152, 8, RED);
}

void IconQuestion_run() {
  M5.Lcd.fillCircle(160, 112, 62, DARKGREY);
  M5.Lcd.fillCircle(160, 112, 60, WHITE);
  M5.Lcd.drawChar(135, 80, '?', DARKGREY, WHITE, 10);
}
