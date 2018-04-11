
#include <M5Stack.h>
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define ASSERT(expr) \
{ \
  if (!expr) { \
    M5.Lcd.printf("assertion failed: expected to be true\r\n"); \
    M5.Lcd.printf("  on %s(%d): %s", __FILE__, __LINE__, #expr); \
    while (true); \
  } \
}

#define ASSERT_EQUAL(expr, expected) \
{ \
  auto actual = expr; \
  if (actual != expected) { \
    M5.Lcd.printf("assertion failed: expected \"%s\", got \"%s\"\r\n", String(expected), String(actual)); \
    M5.Lcd.printf("  on %s(%d): %s", __FILE__, __LINE__, #expr); \
    while (true); \
  } \
}

int project_id;
String branch;
String private_token;

// the setup routine runs once when M5Stack starts up
void setup(){
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
  const char* private_token_ = json["gitlab"]["private_token"];
  ASSERT(private_token != NULL);
  private_token = private_token_;
  project_id = json["gitlab"]["project_id"];
  ASSERT(project_id != 0);
  const char* branch_ = json["gitlab"]["branch"];
  ASSERT(branch_ != NULL);
  branch = branch_;
  
  M5.Lcd.print("connecting to ");
  M5.Lcd.print(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("");

  M5.Lcd.print("connected as ");
  M5.Lcd.print(WiFi.localIP());
  M5.Lcd.println("");
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
  M5.Lcd.fillScreen(0);
  M5.Lcd.setCursor(0, 0);

  M5.Lcd.print("requesting https://gitlab.com/projects/:id/pipelines...");
  String url = String("https://gitlab.com/api/v4/projects/") + project_id + "/pipelines?ref=" + branch + "&per_page=1";
  HTTPClient client;
  ASSERT(client.begin(url, cert));
  client.addHeader("PRIVATE-TOKEN", private_token);
  int result = client.GET();
  if (result != HTTP_CODE_OK) {
    M5.Lcd.println(" failed");
    M5.Lcd.println(client.errorToString(result));
    while (true);
  }

  M5.Lcd.println("success");

  String json = client.getString();
  DynamicJsonBuffer jsonBuffer;
  const JsonVariant &stat = jsonBuffer.parseArray(json)[0]["status"];
  ASSERT(stat.success());
  M5.Lcd.println(stat.as<const char*>());
  
  delay(5 * 60 * 1000);
}

