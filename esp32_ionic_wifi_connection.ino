#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

String ssid = "mokapot-";
String password = "";
WiFiClientSecure espClient;
String s = "";
String mac = "";
String device_mode ="";
uint8_t MAC_array[6];
char MAC_char[18];
WebServer server(80);
WiFiClientSecure client;
#define BYTE_LENGTH 512

void handleRoot() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json; charset=utf-8", "{'status':'ok'}");
}

void handleWifiStatus(){
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("connected network");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json; charset=utf-8", "{\"network_status\":\"connected\"}");
    }
    else{
      Serial.println("not connected network");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json; charset=utf-8", "{\"network_status\":\"not_connected\"}");
    }
}

void handleWifiStatusDone() {
  String statusDone = server.arg("statusDone");
  Serial.println("statusDone");
  Serial.println(statusDone);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json; charset=utf-8", "{\"status_done\":\"done\"}");
  if(statusDone == "done"){
    delay(2000);
    WiFi.softAPdisconnect (true);
  }
}

int writeStringToEEPROM(int addrOffset, const String & strToWrite)
{
    byte len = strToWrite.length();
    EEPROM.write(addrOffset, len);
    for (int i = 0; i < len; i++)
    {
        EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
    }
    return addrOffset + 1 + len;
}

int readStringFromEEPROM(int addrOffset, String * strToRead)
{
    int newStrLen = EEPROM.read(addrOffset);
    char data[newStrLen + 1];
    for (int i = 0; i < newStrLen; i++)
    {
        data[i] = EEPROM.read(addrOffset + 1 + i);
    }
    data[newStrLen] = '\0';
  * strToRead = String(data);
    return addrOffset + 1 + newStrLen;
}

void handleWifiSet() { 
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Headers", "*");
    server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
    server.send(200, "application/json; charset=utf-8", "{\"status\":\"wifi setted\"}");

    if (server.method() == HTTP_POST){
      WiFi.mode(WIFI_AP_STA);
      String body1 = server.arg("plain");
      Serial.println("body1: ");
      Serial.println(body1);
      body1.replace("{","");
      body1.replace("}","");
      body1.replace("ssid","");
      body1.replace("pass","");
      body1.replace(":","");
      int index = body1.indexOf(",");
      int str_index = body1.length();
      String ssid = body1.substring(3,index-1);
      String pass = body1.substring(index+4,str_index-1);
      DynamicJsonDocument doc(1024);
      JsonArray array = doc.to<JsonArray>();
      JsonObject param1 = array.createNestedObject();
      param1["body"] = server.arg("plain");
      String output;
      serializeJson(doc, output);
      
      Serial.println(ssid.c_str());
      Serial.println(pass.c_str());   
  
      WiFi.begin(ssid.c_str(), pass.c_str());
      delay(10000);
  
      if (WiFi.status() == WL_CONNECTED) {
         Serial.println(WiFi.localIP());
         Serial.println(WiFi.macAddress());
         server.on("/", handleRoot);
         server.begin();
         Serial.println("HTTP server started");
         WiFi.macAddress(MAC_array);
          for (int i = 0; i < sizeof(MAC_array); ++i) {
             sprintf(MAC_char, "%s%02x0", MAC_char, MAC_array[i]);
          }
          mac = String(MAC_char);
          mac = mac.substring(0,18);
          Serial.println(mac);
    
          IPAddress ip = WiFi.localIP();
          String ip2Str;
          s = "";
          for (int i = 0; i < 4; i++) {
             s += i ? "." + String(ip[i]) : String(ip[i]);
          }
          Serial.println("ip adresi:");
          Serial.println(s);
          delay(1000); 
  
         int eepromOffset = 0;
         String str1 = ssid;
         String str2 = pass;
         int str1AddrOffset = writeStringToEEPROM(eepromOffset, str1);
         int str2AddrOffset = writeStringToEEPROM(str1AddrOffset, str2);
         EEPROM.commit();
      }
      else if(WiFi.status() != WL_CONNECTED){
        Serial.println("connection failed");
      } 
    }
}

void sendmac(){
    char MAC_char[18] = "";
    WiFi.macAddress(MAC_array);
        for (int i = 0; i < sizeof(MAC_array); ++i) {
            sprintf(MAC_char, "%s%02x0", MAC_char, MAC_array[i]);
        }
    mac = "";
    mac = String(MAC_char);
    Serial.println(mac);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json; charset=utf-8", "{\"status\":\""+mac+"\"}");
}

void handleWifiRoot() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
    String html;
    Serial.println(server.method());
    if (server.method() == HTTP_GET){
      Serial.println("root");
      Serial.println("scan start");

      int numberOfNetworks = WiFi.scanNetworks();
      if(numberOfNetworks > 0){
        String wifiList[numberOfNetworks];
        html = "{ \"list\" : [ ";
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        for (int i = 0; i < numberOfNetworks; i++) {
            wifiList[i] = WiFi.SSID(i);
            html = html + "{ \"ssid\" : \"" + String(wifiList[i]) + "\" }";
            if ((i + 1) < numberOfNetworks) { html = html + ","; }
        }
        html = html + "] }"; 
      }
    }
    server.send(200, "application/json; charset=utf-8", html);
}

void setup(void){
    Serial.begin(115200);
    Serial.println("Starting");
    const int length = 3;
    char result[length+1]; // +1 allows space for the null terminator
    for (int i = 0; i < length; i++)
    {
        result[i] = '0' + random(10);
    }
    result[length] = '\0'; // add the null terminator
    Serial.println(result);
    ssid = ssid + result;
    delay(500);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), password.c_str());
    Serial.println();
    Serial.println("Connected!");
    Serial.print("IP address for network ");
    Serial.print(" : ");
    Serial.println(WiFi.localIP());
    Serial.print("IP address for network ");
    Serial.print(" : ");
    server.on("/network", handleWifiStatus);
    server.on("/sendmac", sendmac);
    server.on("/", handleRoot);
    server.begin();
    Serial.println("HTTP server started");
    
    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
    delay(1000);

    EEPROM.begin(BYTE_LENGTH);
    String newStr1;
    String newStr2;
    int eepromOffset = 0;
    int newStr1AddrOffset = readStringFromEEPROM(eepromOffset, & newStr1);
    int newStr2AddrOffset = readStringFromEEPROM(newStr1AddrOffset, & newStr2);

    if (newStr1 && newStr2) {
      Serial.print("Connecting to ");
      Serial.println(newStr1);
      WiFi.begin(newStr1.c_str(),newStr2.c_str());
      delay(10000);
      if (WiFi.status() == WL_CONNECTED) {
            device_mode = "auto";
            Serial.println(WiFi.localIP());
            Serial.println(WiFi.macAddress());
            server.on("/", handleRoot);
            server.begin();
            Serial.println("HTTP server started");

            WiFi.macAddress(MAC_array);
            for (int i = 0; i < sizeof(MAC_array); ++i) {
               sprintf(MAC_char, "%s%02x0", MAC_char, MAC_array[i]);
            }
            mac = String(MAC_char);
            Serial.println(mac);
            mac = mac.substring(0,18);
      
            IPAddress ip = WiFi.localIP();
            String ip2Str;
            s = "";
            for (int i = 0; i < 4; i++) {
               s += i ? "." + String(ip[i]) : String(ip[i]);
            }
            Serial.println("ip adresi:");
            Serial.println(s);
            delay(1000); 
            Serial.println("request sentttt");
            String line = client.readStringUntil('\n');
            Serial.println(line);
        }
        else{
            Serial.println("internete bağlanamadı");
            WiFi.mode(WIFI_AP);  
        }
    }
    else{
        WiFi.mode(WIFI_STA);
        server.on("/", handleRoot);
        server.on("/sendmac", sendmac);
        server.begin();
    }   
    
    server.on("/wifi", handleWifiRoot);
    server.on("/setwifi", handleWifiSet);
    server.on("/statusdone", handleWifiStatusDone);
}

void loop(void){  
    server.handleClient();
}
