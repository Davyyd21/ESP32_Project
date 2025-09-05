#include <Arduino.h>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include <string>
#include <WiFi.h>
#include <HTTPClient.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled!
#endif

#define btcServerName "A48 Serial"
BluetoothSerial SerialBT;
bool connected = false;
const long connection_timeout = 10000;
long startConnection = 0;
String Id;

void deviceConnected(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("Device Connected");
    connected = true;
  }
  if (event == ESP_SPP_CLOSE_EVT) {
    Serial.println("Device disconnected");
    connected = false;
  }
}

void handleGetNetworks(StaticJsonDocument<1024>& pia) {
  Id = pia["teamId"].as<String>();
  int networksFound = WiFi.scanNetworks();
  if (networksFound == 0) {
    Serial.println("No networks found");
  } else {
    Serial.print(networksFound);
    Serial.println(" networks found");
    for (int i = 0; i < networksFound; i++) {
      bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
      const size_t cap = JSON_OBJECT_SIZE(4) + JSON_ARRAY_SIZE(4);
      DynamicJsonDocument pia_retea(cap);
      pia_retea["ssid"] = WiFi.SSID(i);
      pia_retea["strength"] = WiFi.RSSI(i);
      pia_retea["encryption"] = open ? "Open" : "Protected";
      pia_retea["teamId"] = Id;
      String output;
      serializeJson(pia_retea, output);
      Serial.println(output);
      SerialBT.println(output);
    }
  }
}

void handleConnect(StaticJsonDocument<1024>& pia) {
  String nume = pia["ssid"].as<String>();
  String parola = pia["password"].as<String>();
  Serial.println(nume);
  Serial.println(parola);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(nume.c_str(), parola.c_str());
  Serial.println("Connecting");
  startConnection = millis();
  bool open = (WiFi.status() != WL_CONNECTED);
  const size_t cap = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(3);
  DynamicJsonDocument pia_conectare(cap);
  pia_conectare["ssid"] = nume;
  pia_conectare["connected"] = open;
  pia_conectare["teamId"] = Id;
  String output;
  serializeJson(pia_conectare, output);
  Serial.println(output);
  SerialBT.println(output);
}

void handleGetData() {
  HTTPClient http;
  String site = "http://proiectia.bogdanflorea.ro/api/futurama/characters";
  delay(3000);
  http.begin(site);
  delay(3000);
  http.setConnectTimeout(10000);
  http.setTimeout(10000);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    http.end();
    DynamicJsonDocument json_mic(15000);
    DeserializationError error = deserializeJson(json_mic, payload);
    if (error) {
      Serial.println(error.c_str());
    } else {
      JsonArray list = json_mic.as<JsonArray>();
      int index = 1;
      for (JsonVariant value : list) {
        JsonObject listItem = value.as<JsonObject>();
        const size_t cap = JSON_OBJECT_SIZE(4) + JSON_ARRAY_SIZE(20);
        DynamicJsonDocument pia_getData(cap);
        JsonObject object = pia_getData.to<JsonObject>();
        object.set(listItem);
        String output;
        pia_getData["id"] = listItem["Id"].as<String>();
        pia_getData["name"] = listItem["Name"].as<String>();
        pia_getData["image"] = listItem["PicUrl"].as<String>();
        pia_getData["teamId"] = Id;
        serializeJson(pia_getData, output);
        SerialBT.println(output);
        Serial.println(output);
        index++;
      }
    }
  }
}

void handleGetDetails(StaticJsonDocument<1024>& pia) {
  String id_char = pia["id"].as<String>();
  String Site_details = "http://proiectia.bogdanflorea.ro/api/futurama/character?Id=" + id_char;
  Serial.println(Site_details);
  HTTPClient http;
  http.begin(Site_details);
  http.setConnectTimeout(20000);
  http.setTimeout(20000);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
    StaticJsonDocument<1024> getDetails;
    DeserializationError error = deserializeJson(getDetails, payload);
    if (error) {
      Serial.println(error.c_str());
    } else {
      DynamicJsonDocument details(4096);
      details["id"] = id_char;
      details["name"] = getDetails["Name"].as<String>();
      details["image"] = getDetails["PicUrl"].as<String>();
      String descriere = "Species: " + getDetails["Species"].as<String>() + "\n" +
                         "Age: " + getDetails["Age"].as<String>() + "\n" +
                         "Profesion: " + getDetails["Profession"].as<String>() + "\n" +
                         "Status: " + getDetails["Status"].as<String>() + "\n" +
                         "FirstAppearance: " + getDetails["FirstAppearance"].as<String>() + "\n" +
                         "Relatives: " + getDetails["Relatives"].as<String>() + "\n" +
                         "VoicedBy: " + getDetails["VoicedBy"].as<String>();
      Serial.println(descriere);
      details["description"] = descriere;
      details["teamId"] = Id;
      String output;
      serializeJson(details, output);
      Serial.println(output);
      SerialBT.println(output);
    }
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin(btcServerName);
  SerialBT.register_callback(deviceConnected);
}

void loop() {
  if (SerialBT.available()) {
    Serial.println("---Nica Mihai---David Munteanu---");
    String DATA = SerialBT.readString();
    Serial.print("Comm:");
    StaticJsonDocument<1024> pia;
    DeserializationError error = deserializeJson(pia, DATA);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    } else {
      String action = pia["action"].as<String>();
      Serial.println(action);
      Serial.println(Id);
      if (action == "getNetworks") {
        handleGetNetworks(pia);
      } else if (action == "connect") {
        handleConnect(pia);
      } else if (action == "getData") {
        handleGetData();
      } else if (action == "getDetails") {
        handleGetDetails(pia);
      }
    }
  }
}
